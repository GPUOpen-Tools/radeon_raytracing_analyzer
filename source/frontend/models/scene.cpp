//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the Scene class.
//=============================================================================

#include "models/scene.h"

// We can't use std::max or glm::max since the windows macro ends up overriding the max keyword.
// So we underfine max for this file only.
#undef max

#include <algorithm>
#include <sstream>
#include <string>

#include "public/intersect.h"
#include "public/rra_blas.h"
#include "public/rra_tlas.h"

#include "util/stack_vector.h"

namespace rra
{
    static SceneNodeColors global_scene_node_colors_;
    uint64_t               Scene::scene_iteration_ = 0;
    bool                   Scene::multi_select_    = false;

    void SetSceneNodeColors(SceneNodeColors new_colors)
    {
        global_scene_node_colors_ = new_colors;
    }

    const SceneNodeColors& GetSceneNodeColors()
    {
        return global_scene_node_colors_;
    }

    Scene::~Scene()
    {
        delete root_node_;
    }

    void Scene::Initialize(SceneNode* root_node, uint64_t bvh_index, bool is_tlas)
    {
        delete root_node_;
        root_node_ = root_node;
        bvh_index_ = bvh_index;
        is_tlas_   = is_tlas;

        nodes_.clear();
        root_node->CollectNodes(nodes_);

        // Now that the scene mesh and instance maps have been initialized, build the scene info.
        PopulateSceneInfo();

        IncrementSceneIteration();
    }

    renderer::InstanceMap Scene::GetInstances() const
    {
        renderer::InstanceMap instance_map;

        root_node_->AppendInstancesTo(instance_map);

        return instance_map;
    }

    renderer::InstanceMap Scene::GetFrustumCulledInstanceMap(renderer::FrustumInfo& frustum_info) const
    {
        renderer::InstanceMap instance_map;

        std::vector<bool> rebraid_duplicates(rebraid_siblings_.size());  // Only needed during appending frustum culled instances.
        root_node_->AppendFrustumCulledInstanceMap(instance_map, rebraid_duplicates, this, frustum_info);

        float min_distance = std::numeric_limits<float>::infinity();

        for (auto& instance_type : instance_map)
        {
            for (auto& instance : instance_type.second)
            {
                glm::vec3 min      = {instance.bounding_volume.min_x, instance.bounding_volume.min_y, instance.bounding_volume.min_z};
                glm::vec3 max      = {instance.bounding_volume.max_x, instance.bounding_volume.max_y, instance.bounding_volume.max_z};
                glm::vec3 center   = min + (max - min) / 2.0f;
                float     distance = glm::distance(frustum_info.camera_position, center);
                if (distance < min_distance)
                {
                    frustum_info.closest_point_to_camera = center;
                    min_distance                         = distance;
                }
            }
        }

        for (auto& custom_triangle : custom_triangles_)
        {
            float distance = glm::distance(frustum_info.camera_position, glm::vec3(custom_triangle.position));
            if (distance < min_distance)
            {
                frustum_info.closest_point_to_camera = custom_triangle.position;
                min_distance                         = distance;
            }
        }

        return instance_map;
    }

    renderer::InstanceMap Scene::GetInstanceMap()
    {
        renderer::InstanceMap instance_map;

        // If this is a BLAS scene there are no instances, so return empty map.
        if (!custom_triangles_.empty())
        {
            return instance_map;
        }

        root_node_->AppendInstanceMap(instance_map, this);

        return instance_map;
    }

    const renderer::BoundingVolumeList* Scene::GetBoundingVolumeList() const
    {
        return &bounding_volume_list_;
    }

    const VertexList* Scene::GetCustomTriangles() const
    {
        return &custom_triangles_;
    }

    const SceneStatistics& Scene::GetSceneStatistics() const
    {
        return scene_stats_;
    }

    const std::vector<renderer::SelectedVolumeInstance>& Scene::GetSelectedVolumeInstances() const
    {
        return selected_volume_instances_;
    }

    const std::vector<SceneNode*>& Scene::GetRebraidedInstances(uint32_t instance_index) const
    {
        return rebraid_siblings_[instance_index];
    }

    bool Scene::IsInstanceRebraided(uint32_t instance_index) const
    {
        return GetRebraidedInstances(instance_index).size() > 1;
    }

    const std::vector<SceneNode*>& Scene::GetSplitTriangles(uint32_t geometry_index, uint32_t primitive_index) const
    {
        auto itr = split_triangle_siblings_.find(GetGeometryPrimitiveIndexKey(geometry_index, primitive_index));
        return itr->second;
    }

    bool Scene::IsTriangleSplit(uint32_t goemetry_index, uint32_t primitive_index) const
    {
        return GetSplitTriangles(goemetry_index, primitive_index).size() > 1;
    }

    std::unordered_set<uint32_t>& Scene::GetSelectedNodeIDs()
    {
        return selected_node_ids_;
    }

    void Scene::FilterNodesByInstanceMask(uint32_t filter)
    {
        for (auto& pair : nodes_)
        {
            SceneNode*          node     = pair.second;
            renderer::Instance* instance = node->GetInstance();

            if (instance)
            {
                node->SetFiltered((instance->mask & filter) == 0);
            }
        }

        IncrementSceneIteration();
    }

    SceneNode* Scene::GetNodeByInstanceIndex(uint32_t instance_index)
    {
        if (instance_index >= instance_nodes_.size())
        {
            return nullptr;
        }
        return instance_nodes_[instance_index];
    }

    renderer::RraVertex* Scene::AllocateVertexBuffer(uint32_t blas_index)
    {
        uint32_t total_tri_count{};
        RraErrorCode error_code = RraBlasGetUniqueTriangleCount(blas_index, &total_tri_count);
        RRA_ASSERT(error_code);
        vertices_.resize((size_t)total_tri_count * 3);
        return vertices_.data();
    }

    std::byte* Scene::AllocateChildBuffer(uint32_t blas_index)
    {
        uint64_t total_node_count{};
        RraErrorCode error_code = RraBlasGetTotalNodeCount(blas_index, &total_node_count);
        RRA_ASSERT(error_code);
        child_nodes_buffer_.resize(total_node_count * sizeof(SceneNode));
        return child_nodes_buffer_.data();
    }

    void Scene::PopulateSceneInfo()
    {
        scene_stats_.max_instance_count = ComputeMaxInstanceCount();
        scene_stats_.max_triangle_count = ComputeMaxTriangleCount();
        scene_stats_.max_tree_depth     = ComputeMaxTreeDepth();
        PopulateRebraidMap();
        PopulateSplitTrianglesMap();
        PopulateInstanceNodes();

        // We set rebraided member later than other instance members since this must be done
        // after the rebraid map has been created.
        for (auto& it : nodes_)
        {
            if (it.second && it.second->GetInstance())
            {
                renderer::Instance* instance = it.second->GetInstance();
                if (instance)
                {
                    instance->rebraided = IsInstanceRebraided(instance->instance_index);
                }
            }
        }

        renderer::InstanceMap instance_map;
        root_node_->AppendInstancesTo(instance_map);

        // Set the instance counts.
        blas_instance_counts_.clear();
        for (auto& iter : instance_map)
        {
            blas_instance_counts_[iter.first] = static_cast<uint32_t>(iter.second.size());
        }

        scene_stats_.max_node_depth = 0;
        for (const auto& node : nodes_)
        {
            if (node.second && node.second->GetDepth() > scene_stats_.max_node_depth)
            {
                scene_stats_.max_node_depth = node.second->GetDepth();
            }
        }
    }

    void Scene::RebuildCustomTriangles()
    {
        custom_triangles_.clear();
        custom_triangle_map_.clear();
        custom_triangles_.reserve((size_t)scene_stats_.max_triangle_count * 3);

        for (const auto& id_node : nodes_)
        {
            auto node = id_node.second;
            // The last condition prevents overdrawing for split triangles.
            if (node && !node->GetTriangles().Empty() && node->IsVisible() && node->IsEnabled() &&
                GetSplitTriangles(node->GetGeometryIndex(), node->GetPrimitiveIndex())[0] == node)
            {
                custom_triangle_map_[node->GetId()] = (uint32_t)custom_triangles_.size();
                for (const auto& triangle : node->GetTriangles())
                {
                    custom_triangles_.push_back(triangle.a);
                    custom_triangles_.push_back(triangle.b);
                    custom_triangles_.push_back(triangle.c);
                }
            }
        }
    }

    void Scene::UpdateBoundingVolumes()
    {
        bounding_volume_list_.clear();
        root_node_->AppendBoundingVolumesTo(bounding_volume_list_, depth_range_lower_bound_, depth_range_upper_bound_);
    }

    uint32_t Scene::ComputeMaxTriangleCount() const
    {
        uint32_t current_max = 0;

        renderer::InstanceMap instance_map;
        root_node_->AppendInstancesTo(instance_map);
        for (auto& iter : instance_map)
        {
            uint32_t triangle_count = 0;
            RraErrorCode error_code = RraBlasGetTriangleNodeCount(iter.first, &triangle_count);
            RRA_ASSERT(error_code);
            current_max = std::max(triangle_count, current_max);
        }

        return current_max;
    }

    uint32_t Scene::ComputeMaxInstanceCount() const
    {
        uint32_t current_max_instance_count = 0;

        renderer::InstanceMap instance_map;
        root_node_->AppendInstancesTo(instance_map);
        for (auto& iter : instance_map)
        {
            current_max_instance_count = std::max(static_cast<uint32_t>(iter.second.size()), current_max_instance_count);
        }

        return current_max_instance_count;
    }

    uint32_t Scene::ComputeMaxTreeDepth() const
    {
        uint32_t current_max_tree_depth = 0;

        renderer::InstanceMap instance_map;
        root_node_->AppendInstancesTo(instance_map);
        for (auto& iter : instance_map)
        {
            uint32_t depth = 0;
            RraErrorCode error_code = RraBlasGetMaxTreeDepth(iter.first, &depth);
            RRA_ASSERT(error_code);
            current_max_tree_depth = std::max(depth, current_max_tree_depth);
        }

        return current_max_tree_depth;
    }

    void Scene::PopulateSelectedVolumeInstances()
    {
        selected_volume_instances_.clear();

        BoundingVolumeExtents selection_extents = {};

        if (GetBoundingVolumeForSelection(selection_extents))
        {
            renderer::SelectedVolumeInstance selected_volume = {};
            selected_volume.min                              = {selection_extents.min_x, selection_extents.min_y, selection_extents.min_z};
            selected_volume.max                              = {selection_extents.max_x, selection_extents.max_y, selection_extents.max_z};
            selected_volume_instances_.push_back(selected_volume);
        }

        std::vector<renderer::SelectedVolumeInstance> substrate_instances;

        uint32_t root_node;
        RraErrorCode error_code = RraBvhGetRootNodePtr(&root_node);
        RRA_ASSERT(error_code);

        for (uint32_t node_id : selected_node_ids_)
        {
            renderer::Instance* instance = GetNodeById(node_id)->GetInstance();
            if (instance)
            {
                renderer::SelectedVolumeInstance selected_volume = {};
                error_code = RraBlasGetBoundingVolumeExtents(instance->blas_index, root_node, &selection_extents);
                RRA_ASSERT(error_code);

                selected_volume.min          = {selection_extents.min_x, selection_extents.min_y, selection_extents.min_z};
                selected_volume.max          = {selection_extents.max_x, selection_extents.max_y, selection_extents.max_z};
                selected_volume.is_transform = true;
                selected_volume.transform    = instance->transform;

                substrate_instances.push_back(selected_volume);
            }
        }

        selected_volume_instances_.insert(selected_volume_instances_.end(), substrate_instances.begin(), substrate_instances.end());
    }

    void Scene::PopulateRebraidMap()
    {
        // Get maximum index. This will not be equal to nodes_.size() - 1 when rebraiding is enabled.
        uint32_t max_index{};
        for (auto& node : nodes_)
        {
            renderer::Instance* instance = node.second->GetInstance();
            if (!instance)
            {
                continue;
            }

            if (instance->instance_index > max_index)
            {
                max_index = instance->instance_index;
            }
        }

        rebraid_siblings_.resize((size_t)max_index + 1);
        for (auto& node : nodes_)
        {
            renderer::Instance* instance = node.second->GetInstance();
            if (!instance)
            {
                continue;
            }

            rebraid_siblings_[instance->instance_index].push_back(node.second);
        }

        // Sort.
        for (auto& siblings : rebraid_siblings_)
        {
            if (siblings.size() > 1)
            {
                std::sort(siblings.begin(), siblings.end(), [](SceneNode* a, SceneNode* b) { return a->GetId() < b->GetId(); });
            }
        }
    }

    void Scene::PopulateSplitTrianglesMap()
    {
        for (auto& node : nodes_)
        {
            if (node.second->GetTriangles().Empty())
            {
                continue;
            }

            uint64_t key = GetGeometryPrimitiveIndexKey(node.second->GetGeometryIndex(), node.second->GetPrimitiveIndex());
            split_triangle_siblings_[key].push_back(node.second);
        }

        // Sort.
        for (auto& siblings : split_triangle_siblings_)
        {
            if (siblings.second.size() > 1)
            {
                std::sort(siblings.second.begin(), siblings.second.end(), [](SceneNode* a, SceneNode* b) { return a->GetId() < b->GetId(); });
            }
        }
    }

    void Scene::PopulateInstanceNodes()
    {
        instance_nodes_.clear();

        for (auto& it : nodes_)
        {
            auto instance = it.second->GetInstance();
            if (it.second && instance)
            {
                if (instance->instance_index >= instance_nodes_.size())
                {
                    instance_nodes_.resize(instance->instance_index + 1);
                }
                instance_nodes_[instance->instance_index] = it.second;
            }
        }
    }

    void Scene::SetMultiSelect(bool multi_select)
    {
        multi_select_ = multi_select;
    }

    uint32_t Scene::GetArbitrarySelectedNodeId() const
    {
        if (!selected_node_ids_.empty())
        {
            return *selected_node_ids_.begin();
        }
        return 0;
    }

    void Scene::UpdateCustomTriangleSelection(const std::unordered_set<uint32_t>& old_selection)
    {
        // Deselect old triangles.
        for (uint32_t node_id : old_selection)
        {
            auto node = GetNodeById(node_id);
            if (node)
            {
                // We only draw the first of the split triangles, so deselect that one.
                node = node->GetTriangles().Size() == 0 ? node : GetSplitTriangles(node->GetGeometryIndex(), node->GetPrimitiveIndex())[0];

                if (!node->IsEnabled())
                {
                    continue;
                }

                uint32_t num_triangles{(uint32_t)node->GetTriangles().Size()};
                uint32_t custom_tri_idx = custom_triangle_map_[node->GetId()];

                for (uint32_t i = 0; i < num_triangles; ++i)
                {
                    auto& a = custom_triangles_[custom_tri_idx + (size_t)i * 3 + 0].triangle_sah_and_selected;
                    auto& b = custom_triangles_[custom_tri_idx + (size_t)i * 3 + 1].triangle_sah_and_selected;
                    auto& c = custom_triangles_[custom_tri_idx + (size_t)i * 3 + 2].triangle_sah_and_selected;
                    a       = -std::abs(a);
                    b       = -std::abs(b);
                    c       = -std::abs(c);
                }
            }
        }

        // Select new triangles.
        for (uint32_t node_id : selected_node_ids_)
        {
            auto node = GetNodeById(node_id);
            if (node)
            {
                // We only draw the first of the split triangles, so select that one.
                node = node->GetTriangles().Size() == 0 ? node : GetSplitTriangles(node->GetGeometryIndex(), node->GetPrimitiveIndex())[0];

                if (!node->IsEnabled())
                {
                    continue;
                }

                uint32_t num_triangles{(uint32_t)node->GetTriangles().Size()};

                if (!num_triangles)
                {
                    continue;
                }

                uint32_t custom_tri_idx = custom_triangle_map_[node->GetId()];
                for (uint32_t i = 0; i < num_triangles; ++i)
                {
                    auto& a = custom_triangles_[custom_tri_idx + (size_t)i * 3 + 0].triangle_sah_and_selected;
                    auto& b = custom_triangles_[custom_tri_idx + (size_t)i * 3 + 1].triangle_sah_and_selected;
                    auto& c = custom_triangles_[custom_tri_idx + (size_t)i * 3 + 2].triangle_sah_and_selected;
                    a       = std::abs(a);
                    b       = std::abs(b);
                    c       = std::abs(c);
                }
            }
        }
    }

    void Scene::SetSceneSelection(uint32_t node_id)
    {
        auto old_selection{selected_node_ids_};

        if (!multi_select_)
        {
            for (uint32_t id : selected_node_ids_)
            {
                GetNodeById(id)->ResetSelectionNonRecursive();
            }
            selected_node_ids_.clear();
        }

        auto node = GetNodeById(node_id);
        if (node)
        {
            if (node->IsSelected() && multi_select_)
            {
                node->ResetSelection(selected_node_ids_);

                // Tree view and 3D view get out of synch when most_recent_selected_node_id_
                // references an unselected node during multiselect. So don't let that happen.
                if (!GetNodeById(most_recent_selected_node_id_)->IsSelected())
                {
                    most_recent_selected_node_id_ = GetArbitrarySelectedNodeId();
                }
            }
            else
            {
                most_recent_selected_node_id_ = node->GetId();
                node->ApplyNodeSelection(selected_node_ids_);
            }
        }

        IncrementSceneIteration(false);
        UpdateCustomTriangleSelection(old_selection);
    }

    void Scene::ResetSceneSelection()
    {
        auto old_selection{selected_node_ids_};
        if (root_node_)
        {
            root_node_->ResetSelection(selected_node_ids_);
        }
        selected_node_ids_.clear();
        IncrementSceneIteration(false);
        UpdateCustomTriangleSelection(old_selection);
    }

    bool Scene::HasSelection() const
    {
        return !selected_node_ids_.empty();
    }

    uint32_t Scene::GetMostRecentSelectedNodeId() const
    {
        return most_recent_selected_node_id_;
    }

    bool Scene::GetSceneBoundingVolume(BoundingVolumeExtents& volume) const
    {
        if (root_node_)
        {
            volume = root_node_->GetBoundingVolume();
            return true;
        }
        return false;
    }

    bool Scene::GetBoundingVolumeForSelection(BoundingVolumeExtents& volume) const
    {
        BoundingVolumeExtents extents;
        extents.min_x = std::numeric_limits<float>::infinity();
        extents.min_y = std::numeric_limits<float>::infinity();
        extents.min_z = std::numeric_limits<float>::infinity();
        extents.max_x = -std::numeric_limits<float>::infinity();
        extents.max_y = -std::numeric_limits<float>::infinity();
        extents.max_z = -std::numeric_limits<float>::infinity();

        if (root_node_)
        {
            root_node_->GetBoundingVolumeForSelection(extents);
        }

        if (extents.min_x != std::numeric_limits<float>::infinity())
        {
            volume = extents;
            return true;
        }

        return false;
    }

    uint64_t Scene::GetSceneIteration() const
    {
        return scene_iteration_;
    }

    void Scene::IncrementSceneIteration(bool rebuild_custom_triangles)
    {
        scene_iteration_++;

        // Expensive, so we prefer updating the selection rather than rebuilding.
        if (rebuild_custom_triangles)
        {
            RebuildCustomTriangles();
        }

        UpdateBoundingVolumes();

        PopulateSelectedVolumeInstances();
    }

    uint32_t Scene::GetTotalInstanceCountForBlas(uint64_t blas_index) const
    {
        auto result = blas_instance_counts_.find(blas_index);
        if (result != blas_instance_counts_.end())
        {
            return result->second;
        }
        return 0;
    }

    void Scene::EnableNode(uint32_t node_id)
    {
        auto node = GetNodeById(node_id);
        if (node)
        {
            node->Enable(this);
            IncrementSceneIteration();
        }
    }

    void Scene::DisableNode(uint32_t node_id)
    {
        auto node = GetNodeById(node_id);
        if (node)
        {
            node->Disable(this);
            IncrementSceneIteration();
        }
    }

    bool Scene::IsNodeEnabled(uint32_t node_id)
    {
        auto node = GetNodeById(node_id);
        if (node)
        {
            return node->IsEnabled();
        }
        return false;
    }

    SceneNode* Scene::GetNodeById(uint32_t node_id)
    {
        auto iter = nodes_.find(node_id);
        if (iter != nodes_.end())
        {
            return iter->second;
        }
        return nullptr;
    }

    void Scene::SetDepthRange(uint32_t lower_bound, uint32_t upper_bound)
    {
        depth_range_lower_bound_ = lower_bound;
        depth_range_upper_bound_ = upper_bound;
        IncrementSceneIteration();
    }

    uint32_t Scene::GetDepthRangeLowerBound() const
    {
        return depth_range_lower_bound_;
    }

    uint32_t Scene::GetDepthRangeUpperBound() const
    {
        return depth_range_upper_bound_;
    }

    std::vector<SceneNode*> Scene::CastRayCollectNodes(glm::vec3 ray_origin, glm::vec3 ray_direction) const
    {
        std::vector<SceneNode*> intersected_nodes;

        if (root_node_)
        {
            root_node_->CastRayCollectNodes(ray_origin, ray_direction, intersected_nodes);
        }

        return intersected_nodes;
    }

    RraErrorCode CastClosestHitRayOnBlas(uint64_t         bvh_index,
                                         SceneNode*       node,
                                         const glm::vec3& origin,
                                         const glm::vec3& direction,
                                         SceneClosestHit& scene_closest_hit)
    {
        uint32_t root_node = UINT32_MAX;
        RRA_BUBBLE_ON_ERROR(RraBvhGetRootNodePtr(&root_node));

        uint32_t triangle_count;

        // Two stack allocated buffers (since heap allocation was a bottleneck in this function).
        StackVector<uint32_t, 1024> buffer0{};
        StackVector<uint32_t, 1024> buffer1{};

        // We use the buffers indirectly through a pointer so we can swap them without having to copy.
        StackVector<uint32_t, 1024>* traverse_nodes_ptr = &buffer0;
        StackVector<uint32_t, 1024>* swap_nodes_ptr     = &buffer1;
        traverse_nodes_ptr->PushBack(root_node);

        // Memoized traversal of the tree.
        while (!traverse_nodes_ptr->Empty())
        {
            swap_nodes_ptr->Clear();

            for (size_t i = 0; i < traverse_nodes_ptr->Size(); i++)
            {
                BoundingVolumeExtents extent = {};

                float closest = std::numeric_limits<float>::infinity();

                RRA_BUBBLE_ON_ERROR(RraBlasGetBoundingVolumeExtents(bvh_index, (*traverse_nodes_ptr)[i], &extent));
                if (renderer::IntersectAABB(
                        origin, direction, glm::vec3(extent.min_x, extent.min_y, extent.min_z), glm::vec3(extent.max_x, extent.max_y, extent.max_z), closest))
                {
                    if (closest >= 0.0 && (scene_closest_hit.distance <= 0.0f || closest <= scene_closest_hit.distance))
                    {
                        // Get the child nodes. If this is not a box node, the child count is 0.
                        uint32_t child_node_count;
                        RRA_BUBBLE_ON_ERROR(RraBlasGetChildNodeCount(bvh_index, (*traverse_nodes_ptr)[i], &child_node_count));
                        StackVector<uint32_t, 8> child_nodes{};
                        child_nodes.Resize(child_node_count);
                        RRA_BUBBLE_ON_ERROR(RraBlasGetChildNodes(bvh_index, (*traverse_nodes_ptr)[i], child_nodes.Data()));

                        for (uint32_t child : child_nodes)
                        {
                            swap_nodes_ptr->PushBack(child);
                        }
                    }
                }

                // Get the triangle nodes. If this is not a triangle the triangle count is 0.
                RRA_BUBBLE_ON_ERROR(RraBlasGetNodeTriangleCount(bvh_index, (*traverse_nodes_ptr)[i], &triangle_count));
                StackVector<TriangleVertices, 8> triangles{};
                triangles.Resize(triangle_count);
                RRA_BUBBLE_ON_ERROR(RraBlasGetNodeTriangles(bvh_index, (*traverse_nodes_ptr)[i], triangles.Data()));

                // Go over each triangle and test for intersection.
                for (size_t k = 0; k < triangles.Size(); k++)
                {
                    TriangleVertices triangle_vertices = triangles[k];
                    float            hit_distance;

                    glm::vec3 a = {triangle_vertices.a.x, triangle_vertices.a.y, triangle_vertices.a.z};
                    glm::vec3 b = {triangle_vertices.b.x, triangle_vertices.b.y, triangle_vertices.b.z};
                    glm::vec3 c = {triangle_vertices.c.x, triangle_vertices.c.y, triangle_vertices.c.z};

                    if (renderer::IntersectTriangle(origin, direction, a, b, c, &hit_distance))
                    {
                        if (hit_distance > 0.0 && (scene_closest_hit.distance < 0.0f || hit_distance < scene_closest_hit.distance))
                        {
                            scene_closest_hit.distance = hit_distance;
                            scene_closest_hit.node     = node;
                        }
                    }
                }
            }

            // Swap the old list with the new.
            std::swap(traverse_nodes_ptr, swap_nodes_ptr);
        }
        return kRraOk;
    }

    void CastClosestHitRayOnTriangle(SceneTriangle    triangle,
                                     SceneNode*       node,
                                     const glm::vec3& origin,
                                     const glm::vec3& direction,
                                     SceneClosestHit& scene_closest_hit)
    {
        float hit_distance;

        if (renderer::IntersectTriangle(
                origin, direction, glm::vec3(triangle.a.position), glm::vec3(triangle.b.position), glm::vec3(triangle.c.position), &hit_distance))
        {
            if (hit_distance > 0.0 && (scene_closest_hit.distance < 0.0f || hit_distance < scene_closest_hit.distance))
            {
                scene_closest_hit.distance = hit_distance;
                scene_closest_hit.node     = node;
            }
        }
    }

    SceneClosestHit Scene::CastRayGetClosestHit(glm::vec3 ray_origin, glm::vec3 ray_direction) const
    {
        auto            cast_results      = CastRayCollectNodes(ray_origin, ray_direction);
        SceneClosestHit scene_closest_hit = {};

        for (auto node : cast_results)
        {
            renderer::Instance* instance = node->GetInstance();
            if (instance)
            {
                glm::vec3 transformed_origin    = glm::transpose(glm::inverse(instance->transform)) * glm::vec4(ray_origin, 1.0f);
                glm::vec3 transformed_direction = glm::mat3(glm::transpose(glm::inverse(instance->transform))) * ray_direction;
                CastClosestHitRayOnBlas(instance->blas_index, node, transformed_origin, transformed_direction, scene_closest_hit);
            }

            for (auto& triangle : node->GetTriangles())
            {
                CastClosestHitRayOnTriangle(triangle, node, ray_origin, ray_direction, scene_closest_hit);
            }
        }

        return scene_closest_hit;
    }

    SceneContextMenuOptions Scene::GetSceneContextOptions(SceneContextMenuRequest request)
    {
        std::map<std::string, std::function<void()>> options;

        if (request.location == SceneContextMenuLocation::kSceneContextMenuLocationViewer)
        {
            options["Show everything"] = [&]() { ShowAllNodes(); };

            options["Show selection only"] = [&]() { ShowSelectionOnly(); };

            options["Deselect all"] = [&]() { ResetSceneSelection(); };

            options["Select all visible"] = [&]() {
                root_node_->ApplyNodeSelection(selected_node_ids_);
                most_recent_selected_node_id_ = root_node_->GetId();
                IncrementSceneIteration();
            };

            options["Hide selected"] = [&]() { HideSelectedNodes(); };

            auto cast_results = CastRayCollectNodes(request.origin, request.direction);

            if (!cast_results.empty())
            {
                SceneClosestHit scene_closest_hit = {};

                for (auto node : cast_results)
                {
                    renderer::Instance* instance = node->GetInstance();
                    if (instance)
                    {
                        glm::vec3 transformed_origin    = glm::transpose(glm::inverse(instance->transform)) * glm::vec4(request.origin, 1.0f);
                        glm::vec3 transformed_direction = glm::mat3(glm::transpose(glm::inverse(instance->transform))) * request.direction;
                        CastClosestHitRayOnBlas(instance->blas_index, node, transformed_origin, transformed_direction, scene_closest_hit);
                    }

                    for (auto& triangle : node->GetTriangles())
                    {
                        CastClosestHitRayOnTriangle(triangle, node, request.origin, request.direction, scene_closest_hit);
                    }
                }

                if (scene_closest_hit.node)
                {
                    const char* node_name{};
                    if (is_tlas_)
                    {
                        RraErrorCode error_code = RraTlasGetNodeName(scene_closest_hit.node->GetId(), &node_name);
                        RRA_ASSERT(error_code == kRraOk);
                    }
                    else
                    {
                        RraErrorCode error_code = RraBlasGetNodeName(bvh_index_, scene_closest_hit.node->GetId(), &node_name);
                        RRA_ASSERT(error_code == kRraOk);
                    }
                    std::string node_display_name = std::to_string(scene_closest_hit.node->GetId());

                    uint64_t node_address;
                    auto     error_code = RraBvhGetNodeOffset(scene_closest_hit.node->GetId(), &node_address);
                    if (error_code == kRraOk)
                    {
                        std::ostringstream ss;
                        ss << "0x" << std::hex << node_address << std::dec;
                        node_display_name = ss.str();
                    }

                    if (scene_closest_hit.node->IsSelected())
                    {
                        options["Remove " + std::string(node_name) + " under mouse from selection (" + node_display_name + ")"] = [&, scene_closest_hit]() {
                            scene_closest_hit.node->ResetSelection(selected_node_ids_);
                            IncrementSceneIteration(false);
                        };
                    }
                    else
                    {
                        options["Add " + std::string(node_name) + " under mouse to selection (" + node_display_name + ")"] = [&, scene_closest_hit]() {
                            scene_closest_hit.node->ApplyNodeSelection(selected_node_ids_);
                            IncrementSceneIteration(false);
                        };
                    }
                }
            }
        }

        if (request.location == SceneContextMenuLocation::kSceneContextMenuLocationTreeView)
        {
            options["Show all under selection"] = [&]() {
                auto most_recent_node = nodes_[most_recent_selected_node_id_];
                if (most_recent_node != nullptr)
                {
                    for (auto path_node : most_recent_node->GetPath())
                    {
                        path_node->SetVisible(true, this);
                    }

                    most_recent_node->SetVisible(true, this);
                    most_recent_node->ApplyNodeSelection(selected_node_ids_);

                    for (uint32_t id : selected_node_ids_)
                    {
                        auto node = GetNodeById(id);
                        if (node)
                        {
                            auto parent = node->GetParent();
                            if (parent && !parent->IsSelected())
                            {
                                // Is a top level selected node.
                                node->SetAllChildrenAsVisible(selected_node_ids_);
                            }
                        }
                    }
                    IncrementSceneIteration(false);
                }
            };
        }

        return options;
    }

    void Scene::HideSelectedNodes()
    {
        for (uint32_t id : selected_node_ids_)
        {
            auto node = GetNodeById(id);
            if (node)
            {
                auto instance = node->GetInstance();

                // If instance is rebraided, hide all rebraided siblings.
                if (instance)
                {
                    for (auto sibling : GetRebraidedInstances(instance->instance_index))
                    {
                        sibling->SetVisible(false, this);
                    }
                }
                // If triangle node is split, hide all split sibling nodes.
                else if (!node->GetTriangles().Empty())
                {
                    for (auto sibling : GetSplitTriangles(node->GetGeometryIndex(), node->GetPrimitiveIndex()))
                    {
                        sibling->SetVisible(false, this);
                    }
                }
                else
                {
                    node->SetVisible(false, this);
                }
            }
        }
        selected_node_ids_.clear();
        IncrementSceneIteration();
    }

    void Scene::ShowAllNodes()
    {
        if (root_node_)
        {
            root_node_->SetAllChildrenAsVisible(selected_node_ids_);
            IncrementSceneIteration();
        }
    }

    void Scene::ShowSelectionOnly()
    {
        // Initially hide all nodes.
        for (auto& node : nodes_)
        {
            node.second->SetVisible(false, this);
        }

        for (uint32_t id : selected_node_ids_)
        {
            auto node = GetNodeById(id);
            if (node)
            {
                auto instance = node->GetInstance();

                // Show all rebraiding siblings if we're showing at least one.
                if (instance)
                {
                    for (auto sibling : GetRebraidedInstances(instance->instance_index))
                    {
                        sibling->ShowParentChain();
                    }
                }
                // Show all split triangle siblings if we're showing at least one.
                else if (!node->GetTriangles().Empty())
                {
                    for (auto sibling : GetSplitTriangles(node->GetGeometryIndex(), node->GetPrimitiveIndex()))
                    {
                        sibling->ShowParentChain();
                    }
                }
                else
                {
                    node->ShowParentChain();
                }
            }
        }
        IncrementSceneIteration();
    }

    renderer::TraversalTree Scene::GenerateTraversalTree()
    {
        renderer::TraversalTree traversal_tree;

        if (root_node_ && root_node_->IsVisible())
        {
            traversal_tree.volumes.reserve(nodes_.size());
            root_node_->AddToTraversalTree(true, traversal_tree);
        }

        return traversal_tree;
    }

    const std::map<uint64_t, uint32_t>* Scene::GetBlasInstanceCounts() const
    {
        return &blas_instance_counts_;
    }

}  // namespace rra

