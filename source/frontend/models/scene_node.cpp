//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the SceneNode class.
//=============================================================================

#include <deque>
#include <algorithm>
#include <array>

#include "public/rra_blas.h"
#include "public/rra_tlas.h"
#include "public/rra_bvh.h"
#include "public/rra_rtip_info.h"
#include "scene_node.h"
#include "scene.h"
#include "public/shared.h"
#include "util/stack_vector.h"

#include "public/intersect.h"

// We can't use std::max or glm::max since the windows macro ends up overriding the max keyword.
// So we underfine max for this file only.
#undef max
// Same for min
#undef min

namespace rra
{
    const float        kVolumeEpsilon = 0.0001f;
    constexpr uint32_t kObbDisabled{0x7f};

    SceneNode::SceneNode()
    {
    }

    SceneNode::~SceneNode()
    {
        for (auto child_node : child_nodes_)
        {
            child_node->~SceneNode();
        }
    }

    void SceneNode::AppendInstancesTo(renderer::InstanceMap& instances_map) const
    {
        std::deque<const SceneNode*> traversal_stack;
        traversal_stack.push_back(this);

        while (!traversal_stack.empty())
        {
            const SceneNode* node = traversal_stack.front();
            traversal_stack.pop_front();
            if (node->instance_.has_value())
            {
                instances_map[node->instance_.value().blas_index].push_back(node->instance_.value());
            }

            for (const auto& child_node : node->child_nodes_)
            {
                traversal_stack.push_back(child_node);
            }
        }
    }

    std::array<glm::vec4, 6> GetNormalizedPlanesFromMatrix(glm::mat4 m)
    {
        std::array<glm::vec4, 6> planes = {
            glm::vec4(m[0][3] + m[0][0],
                      m[1][3] + m[1][0],
                      m[2][3] + m[2][0],
                      m[3][3] + m[3][0]),  // Left
            glm::vec4(m[0][3] - m[0][0],
                      m[1][3] - m[1][0],
                      m[2][3] - m[2][0],
                      m[3][3] - m[3][0]),  // Right
            glm::vec4(m[0][3] + m[0][1],
                      m[1][3] + m[1][1],
                      m[2][3] + m[2][1],
                      m[3][3] + m[3][1]),  // Bottom
            glm::vec4(m[0][3] - m[0][1],
                      m[1][3] - m[1][1],
                      m[2][3] - m[2][1],
                      m[3][3] - m[3][1]),  // Top
            glm::vec4(m[0][2],
                      m[1][2],
                      m[2][2],
                      m[3][2]),  // Far
            glm::vec4(m[0][3] - m[0][2],
                      m[1][3] - m[1][2],
                      m[2][3] - m[2][2],
                      m[3][3] - m[3][2]),  // Close
        };

        // Normalize by the plane normal.
        for (size_t i = 0; i < planes.size(); i++)
        {
            float magnitude = 1.0f / glm::sqrt((planes[i].x * planes[i].x) + (planes[i].y * planes[i].y) + (planes[i].z * planes[i].z));
            planes[i]       = planes[i] * magnitude;
        }

        return planes;
    }

    bool PlaneBackFaceTest(glm::vec4 plane, BoundingVolumeExtents e)
    {
        glm::vec3 min;
        glm::vec3 max;

        max.x = plane.x >= 0.0f ? e.min_x : e.max_x;
        max.y = plane.y >= 0.0f ? e.min_y : e.max_y;
        max.z = plane.z >= 0.0f ? e.min_z : e.max_z;

        min.x = plane.x >= 0.0f ? e.max_x : e.min_x;
        min.y = plane.y >= 0.0f ? e.max_y : e.min_y;
        min.z = plane.z >= 0.0f ? e.max_z : e.min_z;

        float distance = glm::dot(glm::vec3(plane), max);
        if (distance + plane.w > 0.0f)
        {
            return false;
        }

        distance = glm::dot(glm::vec3(plane), min);
        if (distance + plane.w < 0.0f)
        {
            return true;
        }

        return false;
    }

    bool BoundingVolumeExtentFovCull(BoundingVolumeExtents extents, glm::vec3 camera_position, float fov, float fov_ratio)
    {
        auto min = glm::vec3(extents.min_x, extents.min_y, extents.min_z);
        auto max = glm::vec3(extents.max_x, extents.max_y, extents.max_z);

        auto volume_position = min + (max - min) / 2.0f;
        auto distance        = glm::distance(camera_position, volume_position);

        auto diff = max - min;

        auto volume_radius = glm::max(diff.x, glm::max(diff.y, diff.z)) / 2.0f;

        auto volume_fov = glm::atan(volume_radius / distance);

        return volume_fov < (glm::radians(fov) * fov_ratio);
    }

    bool BoundingVolumeExtentsInsidePlanes(BoundingVolumeExtents extents, const std::array<glm::vec4, 6>& planes)
    {
        for (size_t i = 0; i < planes.size(); i++)
        {
            if (PlaneBackFaceTest(planes[i], extents))
            {
                return false;
            }
        }

        return true;
    }

    void SceneNode::AppendFrustumCulledInstanceMap(renderer::InstanceMap& instance_map,
                                                   std::vector<bool>&     rebraid_duplicates,
                                                   const Scene*           scene,
                                                   renderer::FrustumInfo& frustum_info) const
    {
        // Skip if marked as not visible.
        if (!(visible_ && enabled_ && !filtered_))
        {
            return;
        }

        // Extract the planes from the view_projection.
        auto culling_planes = GetNormalizedPlanesFromMatrix(frustum_info.camera_view_projection);

        // Cull for the child nodes.
        for (auto child_node : child_nodes_)
        {
            if (!BoundingVolumeExtentFovCull(
                    child_node->bounding_volume_, frustum_info.camera_position, frustum_info.camera_fov, frustum_info.fov_threshold_ratio) &&
                BoundingVolumeExtentsInsidePlanes(child_node->bounding_volume_, culling_planes))
            {
                child_node->AppendFrustumCulledInstanceMap(instance_map, rebraid_duplicates, scene, frustum_info);
            }
        }

        // Check for instances.
        if (instance_.has_value())
        {
            // If we've added one of this instances rebraid siblings already, don't add this one.
            // Just checking if this SceneNode is equal to the first rebraid sibling is not enough, since then the BLAS will be culled
            // if only the first rebraid sibling is out of the frustum. So we must check if any rebraid siblings are in the frustum,
            // but use rebraid_duplicates to avoid rendering duplicates.
            if (!rebraid_duplicates[instance_.value().instance_index])
            {
                rebraid_duplicates[instance_.value().instance_index] = true;

                if (!BoundingVolumeExtentFovCull(bounding_volume_, frustum_info.camera_position, frustum_info.camera_fov, frustum_info.fov_threshold_ratio) &&
                    BoundingVolumeExtentsInsidePlanes(bounding_volume_, culling_planes))
                {
                    AppendMergedInstanceToInstanceMap(instance_.value(), instance_map, scene);
                }
            }
        }
    }

    void SceneNode::AppendInstanceMap(renderer::InstanceMap& instance_map, const Scene* scene) const
    {
        // Skip if marked as not visible.
        if (!(visible_ && enabled_ && !filtered_))
        {
            return;
        }

        for (auto child_node : child_nodes_)
        {
            child_node->AppendInstanceMap(instance_map, scene);
        }

        if (instance_.has_value())
        {
            // Since we don't have to worry about culling here, it is enough to just check that this SceneNode
            // is equal to the first rebraid sibling to avoid duplicates.
            auto sibling_nodes = scene->GetRebraidedInstances(instance_.value().instance_index);
            if (!sibling_nodes.empty() && sibling_nodes[0] == this)
            {
                AppendMergedInstanceToInstanceMap(instance_.value(), instance_map, scene);
            }
        }
    }

    void SceneNode::AppendTrianglesTo(VertexList& vertex_list) const
    {
        // Skip if marked as not visible.
        if (!visible_ || filtered_)
        {
            return;
        }

        vertex_list.insert(vertex_list.end(), vertices_, vertices_ + vertex_count_);

        for (auto child_node : child_nodes_)
        {
            child_node->AppendTrianglesTo(vertex_list);
        }
    }

    SceneNode* SceneNode::ConstructFromBlasNode(uint64_t blas_index, uint32_t root_id, renderer::RraVertex* vertex_buffer, std::byte* child_buffer)
    {
        uint32_t current_child_buffer_offset{0};

        SceneNode* root_node = new (child_buffer + current_child_buffer_offset) SceneNode();
        current_child_buffer_offset += sizeof(SceneNode);
        root_node->node_id_   = root_id;
        root_node->depth_     = 0;
        root_node->bvh_index_ = blas_index;

        std::vector<SceneNode*> traversal_stack;
        traversal_stack.reserve(64);  // It is rare for the traversal stack to get deeper than ~28 so this should be sufficient memory to reserve.
        traversal_stack.push_back(root_node);

        uint32_t vertex_buffer_idx{0};

        while (!traversal_stack.empty())
        {
            SceneNode* node{traversal_stack.back()};
            traversal_stack.pop_back();

            RraBlasGetBoundingVolumeExtents(blas_index, node->node_id_, &node->bounding_volume_);

            if (RraBlasIsTriangleNode(blas_index, node->node_id_))
            {
                // Get the triangle nodes. If this is not a triangle the triangle count is 0.
                uint32_t triangle_count;
                RraBlasGetNodeTriangleCount(blas_index, node->node_id_, &triangle_count);

                // Make vertices_ a subset of the BLAS's vertex buffer.
                node->vertices_ = vertex_buffer + vertex_buffer_idx;
                vertex_buffer_idx += triangle_count * 3;

                // Continue with processing the node if it's a triangle node with 1 or more triangles within.
                if (triangle_count > 0)
                {
                    // Populate a vector of triangle vertex data.
                    std::array<TriangleVertices, 8> triangles{};
                    RraBlasGetNodeTriangles(blas_index, node->node_id_, triangles.data());

                    // Retrieve the geometry index associated with the current triangle node.
                    RraBlasGetGeometryIndex(blas_index, node->node_id_, &node->geometry_index_);

                    uint32_t geometry_flags = 0;
                    RraBlasGetGeometryFlags(blas_index, node->geometry_index_, &geometry_flags);

                    // Extract the opacity flag.
                    bool is_opaque = (geometry_flags & GeometryFlags::kOpaque) == GeometryFlags::kOpaque;

                    RraBlasGetPrimitiveIndex(blas_index, node->node_id_, 0, &node->primitive_index_);

                    // Step over each triangle and extract data used to populate the vertex buffer.
                    for (size_t triangle_index = 0; triangle_index < triangle_count; triangle_index++)
                    {
                        const TriangleVertices& triangle = triangles[triangle_index];

                        // Extract the vertex positions.
                        glm::vec3 p0 = glm::vec3(triangle.a.x, triangle.a.y, triangle.a.z);
                        glm::vec3 p1 = glm::vec3(triangle.b.x, triangle.b.y, triangle.b.z);
                        glm::vec3 p2 = glm::vec3(triangle.c.x, triangle.c.y, triangle.c.z);

                        // Compute the triangle normal.
                        glm::vec3 a      = p1 - p0;
                        glm::vec3 b      = p2 - p0;
                        glm::vec3 normal = glm::cross(a, b);
                        normal           = glm::normalize(normal);

                        // We can infer the z-component from x and y, but the sign is lost. So we encode the sign of z by adding
                        // kNormalSignIndicatorOffset to x if z is negative. This is then decoded in the shader.
                        glm::vec2 compact_normal = glm::vec2(normal.x, normal.y);
                        compact_normal.x         = (normal.z < 0.0f) ? compact_normal.x : compact_normal.x + kNormalSignIndicatorOffset;

                        float triangle_sah = 0.0f;
                        RraBlasGetTriangleSurfaceAreaHeuristic(blas_index, node->node_id_, &triangle_sah);

                        float average_epo = 0.0f;
                        float max_epo     = 0.0f;
                        RRA_UNUSED(average_epo);
                        RRA_UNUSED(max_epo);

                        // We pack geometry index, depth, split, and opaque into one uint32_t.
                        uint32_t geometry_index_depth_split_opaque{};
                        geometry_index_depth_split_opaque |= node->geometry_index_ << 16;  // Bits 31-16 are geometry index.
                        geometry_index_depth_split_opaque |= node->depth_ << 2;            // Bits 15-2 are depth.
                        geometry_index_depth_split_opaque |= 0 << 1;               // Bit 1 is split. We write to this in PopulateSplitVertexAttribute().
                        geometry_index_depth_split_opaque |= (uint32_t)is_opaque;  // Bit 0 is opaque.

                        // Triangle SAH is negative initially to indicate deselected triangles.
                        renderer::RraVertex v0 = {p0, -triangle_sah, compact_normal, geometry_index_depth_split_opaque, node->node_id_};
                        renderer::RraVertex v1 = {p1, -triangle_sah, compact_normal, geometry_index_depth_split_opaque, node->node_id_};
                        renderer::RraVertex v2 = {p2, -triangle_sah, compact_normal, geometry_index_depth_split_opaque, node->node_id_};

                        // Add 3 new triangle vertices to the output array.
                        node->vertices_[node->vertex_count_ + 0] = v0;
                        node->vertices_[node->vertex_count_ + 1] = v1;
                        node->vertices_[node->vertex_count_ + 2] = v2;

                        node->vertex_count_ += 3;
                    }
                }
            }

            if (!RraBvhIsBoxNode(node->node_id_))
            {
                node->obb_index_ = kObbDisabled;
                node->rotation_  = glm::mat3(1.0f);
                continue;
            }

            uint32_t child_node_count;
            RraBlasGetChildNodeCount(blas_index, node->node_id_, &child_node_count);

            std::array<uint32_t, 8> child_nodes{};
            RraBlasGetChildNodes(blas_index, node->node_id_, child_nodes.data());

            for (uint32_t i{0}; i < child_node_count; ++i)
            {
                if (child_nodes[i] == node->node_id_)
                {
                    // Self refencing node will cause a stack overflow. Skip to prevent a crash.
                    continue;
                }

                // Use placement new operator to allocate child in child_nodes_buffer_.
                SceneNode* new_node = new (child_buffer + current_child_buffer_offset) SceneNode();
                current_child_buffer_offset += sizeof(SceneNode);
                new_node->node_id_   = child_nodes[i];
                new_node->bvh_index_ = blas_index;
                new_node->depth_     = node->depth_ + 1;
                new_node->parent_    = node;
                node->child_nodes_.PushBack(new_node);
                traversal_stack.push_back(new_node);
            }

            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() >= rta::RayTracingIpLevel::RtIp3_0)
            {
                RraBlasGetNodeObbIndex(blas_index, node->node_id_, &node->obb_index_);
                RraBlasGetNodeBoundingVolumeOrientation(blas_index, node->node_id_, &node->rotation_[0][0]);
            }
        }

        return root_node;
    }

    void SceneNode::AppendMergedInstanceToInstanceMap(renderer::Instance instance, renderer::InstanceMap& instance_map, const Scene* scene) const
    {
        auto sibling_nodes = scene->GetRebraidedInstances(instance.instance_index);

        bool selected = false;
        for (auto sibling_node : sibling_nodes)
        {
            selected |= sibling_node->selected_;
        }
        instance.selected = selected;
        instance_map[instance.blas_index].push_back(instance);
    }

    SceneNode* SceneNode::ConstructFromBlas(uint32_t blas_index, renderer::RraVertex* vertex_buffer, std::byte* child_buffer)
    {
        uint32_t root_node_index = UINT32_MAX;
        RraBvhGetRootNodePtr(&root_node_index);
        auto node = ConstructFromBlasNode(blas_index, root_node_index, vertex_buffer, child_buffer);

        uint32_t geometry_count{};
        RraBlasGetGeometryCount(blas_index, &geometry_count);
        std::vector<uint32_t> geometry_offsets{};
        geometry_offsets.resize(geometry_count);
        uint32_t current_geo_offset{0};

        for (uint32_t geo_idx{0}; geo_idx < geometry_count; ++geo_idx)
        {
            geometry_offsets[geo_idx] = current_geo_offset;

            uint32_t primitive_count{};
            RraBlasGetGeometryPrimitiveCount(blas_index, geo_idx, &primitive_count);
            current_geo_offset += primitive_count;
        }
        PopulateSplitVertexAttribute(node, geometry_offsets, current_geo_offset);

        return node;
    }

    uint64_t GetGeometryPrimitiveIndexKey(uint32_t geometry_index, uint32_t primitive_index)
    {
        return (static_cast<uint64_t>(geometry_index) << 32) | static_cast<uint64_t>(primitive_index);
    }

    SceneNode* SceneNode::ConstructFromTlasBoxNode(uint64_t tlas_index, uint32_t node_id, uint32_t depth)
    {
        SceneNode* node  = new SceneNode();
        node->node_id_   = node_id;
        node->depth_     = depth;
        node->bvh_index_ = tlas_index;

        RraTlasGetBoundingVolumeExtents(tlas_index, node_id, &node->bounding_volume_);

        if (RraBvhIsInstanceNode(node_id))
        {
            renderer::Instance instance = {};
            instance.selected           = false;
            instance.instance_node      = node_id;
            instance.depth              = depth;

            instance.transform = glm::mat4(0.0f);  // Reset the transform to prevent misalignment.

            RraTlasGetInstanceNodeTransform(tlas_index, node_id, reinterpret_cast<float*>(&instance.transform));
            instance.transform[3][3] = 1.0f;

            // Navi IP 1.1 encoding specifies that the transform is inverse, so we inverse it again to get the correct transform.
            instance.transform = glm::inverse(instance.transform);

            RraTlasGetBoundingVolumeExtents(tlas_index, node_id, &instance.bounding_volume);

            RraTlasGetBlasIndexFromInstanceNode(tlas_index, node_id, &instance.blas_index);
            RraBlasGetMaxTreeDepth(instance.blas_index, &instance.max_depth);
            RraBlasGetAvgTreeDepth(instance.blas_index, &instance.average_depth);

            uint32_t root_node = UINT32_MAX;
            RraBvhGetRootNodePtr(&root_node);

            RraBlasGetAverageSurfaceAreaHeuristic(instance.blas_index, root_node, true, &instance.average_triangle_sah);
            RraBlasGetMinimumSurfaceAreaHeuristic(instance.blas_index, root_node, true, &instance.min_triangle_sah);

            RraTlasGetUniqueInstanceIndexFromInstanceNode(tlas_index, node_id, &instance.instance_unique_index);

            RraTlasGetInstanceIndexFromInstanceNode(tlas_index, node_id, &instance.instance_index);

            RraBlasGetBuildFlags(instance.blas_index, reinterpret_cast<VkBuildAccelerationStructureFlagBitsKHR*>(&instance.build_flags));

            RraTlasGetInstanceNodeMask(tlas_index, node_id, &instance.mask);

            RraTlasGetInstanceFlags(tlas_index, node_id, &instance.flags);

            node->instance_ = instance;

            node->obb_index_ = kObbDisabled;
            node->rotation_  = glm::mat3(1.0f);
            return node;
        }

        RraTlasGetNodeObbIndex(tlas_index, node_id, &node->obb_index_);
        RraTlasGetNodeBoundingVolumeOrientation(tlas_index, node_id, &node->rotation_[0][0]);

        uint32_t child_node_count{};
        RraTlasGetChildNodeCount(tlas_index, node_id, &child_node_count);

        std::vector<uint32_t> child_nodes(child_node_count);
        RraTlasGetChildNodes(tlas_index, node_id, child_nodes.data());

        for (auto child_node : child_nodes)
        {
            auto child_node_ptr     = SceneNode::ConstructFromTlasBoxNode(tlas_index, child_node, depth + 1);
            child_node_ptr->parent_ = node;
            node->child_nodes_.PushBack(child_node_ptr);
        }

        return node;
    }

    SceneNode* SceneNode::ConstructFromTlas(uint64_t tlas_index)
    {
        uint32_t root_node_index = UINT32_MAX;
        RraBvhGetRootNodePtr(&root_node_index);
        return ConstructFromTlasBoxNode(tlas_index, root_node_index, 0);
    }

    void SceneNode::ResetSelection(std::unordered_set<uint32_t>& selected_node_ids)
    {
        selected_ = false;
        selected_node_ids.erase(node_id_);

        for (auto child_node : child_nodes_)
        {
            child_node->ResetSelection(selected_node_ids);
        }

        if (instance_.has_value())
        {
            instance_.value().selected = false;
        }

        for (uint32_t i{0}; i < vertex_count_; ++i)
        {
            // Unselect.
            vertices_[i].triangle_sah_and_selected = -std::abs(vertices_[i].triangle_sah_and_selected);
        }
    }

    void SceneNode::ResetSelectionNonRecursive()
    {
        selected_ = false;

        if (instance_.has_value())
        {
            instance_.value().selected = false;
        }

        for (uint32_t i{0}; i < vertex_count_; ++i)
        {
            // Unselect.
            vertices_[i].triangle_sah_and_selected = -std::abs(vertices_[i].triangle_sah_and_selected);
        }
    }

    void SceneNode::ApplyNodeSelection(std::unordered_set<uint32_t>& selected_node_ids)
    {
        if (!visible_ || filtered_)
        {
            return;
        }

        selected_ = true;
        selected_node_ids.insert(node_id_);

        for (auto child_node : child_nodes_)
        {
            child_node->ApplyNodeSelection(selected_node_ids);
        }

        if (instance_.has_value())
        {
            instance_.value().selected = true;
        }

        for (uint32_t i{0}; i < vertex_count_; ++i)
        {
            // Select.
            vertices_[i].triangle_sah_and_selected = std::abs(vertices_[i].triangle_sah_and_selected);
        }
    }

    BoundingVolumeExtents SceneNode::GetBoundingVolume() const
    {
        return bounding_volume_;
    }

    void SceneNode::CollectNodes(std::map<uint32_t, SceneNode*>& nodes)
    {
        nodes[node_id_] = this;
        for (auto child_node : child_nodes_)
        {
            child_node->CollectNodes(nodes);
        }
    }

    void SceneNode::Enable(Scene* scene)
    {
        enabled_ = true;
        if (!visible_ || filtered_)
        {
            return;
        }
        for (auto child_node : child_nodes_)
        {
            child_node->Enable(scene);
        }

        // Enable rebraided siblings.
        auto instance = GetInstance();
        if (instance && scene)
        {
            for (auto sibling : scene->GetRebraidedInstances(instance->instance_index))
            {
                sibling->Enable(nullptr);
            }
        }
        // Enable split triangle siblings.
        if (!GetTriangles().Empty() && scene)
        {
            for (auto sibling : scene->GetSplitTriangles(geometry_index_, primitive_index_))
            {
                sibling->Enable(nullptr);
            }
        }
    }

    void SceneNode::Disable(Scene* scene)
    {
        if (!enabled_)
        {
            return;
        }

        enabled_ = false;
        for (auto child_node : child_nodes_)
        {
            child_node->Disable(scene);
        }

        // Disable rebraided siblings.
        auto instance = GetInstance();
        if (instance && scene)
        {
            for (auto sibling : scene->GetRebraidedInstances(instance->instance_index))
            {
                sibling->Disable(nullptr);
            }
        }
        // Disable split triangle siblings.
        if (!GetTriangles().Empty() && scene)
        {
            for (auto sibling : scene->GetSplitTriangles(geometry_index_, primitive_index_))
            {
                sibling->Disable(nullptr);
            }
        }
    }

    void SceneNode::SetVisible(bool visible, Scene* scene)
    {
        visible_ = visible;
        if (visible_)
        {
            for (auto child_node : child_nodes_)
            {
                child_node->Enable(scene);
            }
        }
        else
        {
            for (auto child_node : child_nodes_)
            {
                child_node->Disable(scene);
            }
        }
    }

    void SceneNode::ShowParentChain()
    {
        if (visible_)
        {
            return;
        }

        SetVisible(true, nullptr);
        if (parent_)
        {
            parent_->ShowParentChain();
        }
    }

    void SceneNode::SetAllChildrenAsVisible(std::unordered_set<uint32_t>& selected_node_ids)
    {
        if (!visible_)
        {
            visible_ = true;
            selected_node_ids.insert(node_id_);
            ApplyNodeSelection(selected_node_ids);
        }

        enabled_ = true;
        for (auto child_node : child_nodes_)
        {
            child_node->SetAllChildrenAsVisible(selected_node_ids);
        }
    }

    bool SceneNode::IsVisible()
    {
        return visible_ && !filtered_;
    }

    bool SceneNode::IsEnabled()
    {
        return enabled_;
    }

    bool SceneNode::IsSelected()
    {
        return selected_;
    }

    /// @brief Reduces the volume by min max on opposite corners.
    BoundingVolumeExtents ReduceVolumeExtents(const BoundingVolumeExtents& global_volume, const BoundingVolumeExtents& local_volume)
    {
        BoundingVolumeExtents reduced;
        reduced.max_x = glm::max(global_volume.max_x, local_volume.max_x);
        reduced.max_y = glm::max(global_volume.max_y, local_volume.max_y);
        reduced.max_z = glm::max(global_volume.max_z, local_volume.max_z);
        reduced.min_x = glm::min(global_volume.min_x, local_volume.min_x);
        reduced.min_y = glm::min(global_volume.min_y, local_volume.min_y);
        reduced.min_z = glm::min(global_volume.min_z, local_volume.min_z);
        return reduced;
    }

    /// @brief Reduces the volume after applying rotation.
    BoundingVolumeExtents ReduceVolumeExtentsOBB(const BoundingVolumeExtents& global_volume,
                                                 const BoundingVolumeExtents& local_volume,
                                                 const glm::mat3&             rotation)
    {
        BoundingVolumeExtents reduced;
        glm::vec3             rotated_max{local_volume.max_x, local_volume.max_y, local_volume.max_z};
        glm::vec3             rotated_min{local_volume.min_x, local_volume.min_y, local_volume.min_z};
        rotated_max = rotation * rotated_max;
        rotated_min = rotation * rotated_min;

        reduced.max_x = glm::max(global_volume.max_x, rotated_max.x);
        reduced.max_y = glm::max(global_volume.max_y, rotated_max.y);
        reduced.max_z = glm::max(global_volume.max_z, rotated_max.z);
        reduced.min_x = glm::min(global_volume.min_x, rotated_min.x);
        reduced.min_y = glm::min(global_volume.min_y, rotated_min.y);
        reduced.min_z = glm::min(global_volume.min_z, rotated_min.z);
        return reduced;
    }

    void SceneNode::GetBoundingVolumeForSelection(BoundingVolumeExtents& volume) const
    {
        if (!visible_ || filtered_)
        {
            return;
        }

        for (auto child_node : child_nodes_)
        {
            child_node->GetBoundingVolumeForSelection(volume);
        }

        if (selected_)
        {
            bool is_obb{parent_ && parent_->rotation_ != glm::mat3(1.0f)};
            volume = is_obb ? ReduceVolumeExtentsOBB(volume, bounding_volume_, parent_->rotation_) : ReduceVolumeExtents(volume, bounding_volume_);
        }
    }

    void SceneNode::CastRayCollectNodes(glm::vec3 ray_origin, glm::vec3 ray_direction, std::vector<SceneNode*>& intersected_nodes)
    {
        if (!visible_ || filtered_)
        {
            return;
        }

        float closest = 0.0f;

        if (renderer::IntersectAABB(ray_origin,
                                    ray_direction,
                                    glm::vec3(bounding_volume_.min_x, bounding_volume_.min_y, bounding_volume_.min_z),
                                    glm::vec3(bounding_volume_.max_x, bounding_volume_.max_y, bounding_volume_.max_z),
                                    closest))
        {
            intersected_nodes.push_back(this);
            for (auto child : child_nodes_)
            {
                child->CastRayCollectNodes(ray_origin, ray_direction, intersected_nodes);
            }
        }
    }

    renderer::Instance* SceneNode::GetInstance()
    {
        if (!instance_.has_value())
        {
            return nullptr;
        }

        return &instance_.value();
    }

    StackVector<SceneTriangle, 8> SceneNode::GetTriangles() const
    {
        RRA_ASSERT(vertex_count_ % 3 == 0);
        StackVector<SceneTriangle, 8> triangles;
        for (size_t i = 0; i < vertex_count_; i += 3)
        {
            SceneTriangle triangle;
            triangle.a = vertices_[i];
            triangle.b = vertices_[i + 1];
            triangle.c = vertices_[i + 2];
            triangles.PushBack(triangle);
        }
        return triangles;
    }

    uint32_t SceneNode::GetPrimitiveIndex() const
    {
        return primitive_index_;
    }

    uint32_t SceneNode::GetGeometryIndex() const
    {
        return geometry_index_;
    }

    uint32_t SceneNode::GetId() const
    {
        return node_id_;
    }

    void SceneNode::AppendBoundingVolumesTo(renderer::BoundingVolumeList& volume_list, uint32_t lower_bound, uint32_t upper_bound) const
    {
        if (depth_ > upper_bound)
        {
            return;
        }

        if (visible_ && !filtered_)
        {
            for (auto child : child_nodes_)
            {
                child->AppendBoundingVolumesTo(volume_list, lower_bound, upper_bound);
            }

            if (depth_ >= lower_bound)
            {
                renderer::BoundingVolumeInstance bvi;
                bvi.min = {bounding_volume_.min_x, bounding_volume_.min_y, bounding_volume_.min_z, depth_};
                bvi.max = {bounding_volume_.max_x, bounding_volume_.max_y, bounding_volume_.max_z};

                bvi.metadata = glm::vec4(-1.0f, depth_, 0.0f, 1.0f);

                if (RraBvhIsBox16Node(node_id_))
                {
                    bvi.metadata.x = 1.0f;
                }
                else if (RraBvhIsBox32Node(node_id_))
                {
                    bvi.metadata.x = 2.0f;
                }
                else if (RraBvhIsInstanceNode(node_id_))
                {
                    bvi.metadata.x = 3.0f;
                }
                else if (RraBvhIsProceduralNode(node_id_))
                {
                    bvi.metadata.x = 4.0f;
                }
                else if (RraBlasIsTriangleNode(bvh_index_, node_id_))
                {
                    bvi.metadata.x = 5.0f;
                }

                if (selected_)
                {
                    bvi.metadata.x = 0.0f;
                }

                bvi.rotation = parent_ ? parent_->rotation_ : glm::mat3(1.0f);

                volume_list.push_back(bvi);
            }
        }
    }

    uint32_t SceneNode::GetDepth() const
    {
        return depth_;
    }

    std::vector<SceneNode*> SceneNode::GetPath() const
    {
        std::vector<SceneNode*> path;
        auto                    temp = parent_;
        while (temp)
        {
            path.push_back(temp);
            temp = temp->parent_;
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    SceneNode* SceneNode::GetParent() const
    {
        return parent_;
    }

    void SceneNode::AddToTraversalTree(bool populate_vertex_buffer, renderer::TraversalTree& traversal_tree)
    {
        std::vector<std::pair<SceneNode*, uint32_t> > traversal_stack{};  // Pairs of scene node and its index into traversal_tree.volumes.
        traversal_stack.reserve(64);  // It is rare for the traversal stack to get deeper than ~28 so this should be sufficient memory to reserve.
        traversal_stack.push_back({this, 0});

        renderer::TraversalVolume root_volume{};
        root_volume.parent          = 0;
        root_volume.index_at_parent = -1;
        traversal_tree.volumes.push_back(root_volume);

        uint32_t vertices_size{0};

        while (!traversal_stack.empty())
        {
            auto pair = traversal_stack.back();
            traversal_stack.pop_back();
            SceneNode* node{pair.first};
            uint32_t   current_index{pair.second};

            renderer::TraversalVolume& traversal_volume = traversal_tree.volumes[current_index];
            traversal_volume.min       = glm::vec4(node->bounding_volume_.min_x, node->bounding_volume_.min_y, node->bounding_volume_.min_z, 1.0f);
            traversal_volume.max       = glm::vec4(node->bounding_volume_.max_x, node->bounding_volume_.max_y, node->bounding_volume_.max_z, 1.0f);
            traversal_volume.obb_index = node->obb_index_;

            if (RraBvhIsInstanceNode(node->node_id_))
            {
                traversal_volume.volume_type = renderer::TraversalVolumeType::kInstance;
                traversal_volume.leaf_start  = static_cast<uint32_t>(traversal_tree.instances.size());

                if (node->instance_.has_value())
                {
                    renderer::TraversalInstance ci;
                    ci.transform         = node->instance_.value().transform;
                    ci.inverse_transform = glm::inverse(node->instance_.value().transform);
                    ci.selected          = IsSelected() ? 1 : 0;
                    ci.blas_index        = static_cast<uint32_t>(node->instance_.value().blas_index);
                    ci.geometry_index    = 0;
                    ci.flags             = node->instance_.value().flags;

                    traversal_tree.instances.push_back(ci);
                }

                traversal_volume.leaf_end = static_cast<uint32_t>(traversal_tree.instances.size());
            }
            else if (RraBlasIsTriangleNode(bvh_index_, node->node_id_))
            {
                traversal_volume.volume_type = renderer::TraversalVolumeType::kTriangle;
                if (populate_vertex_buffer)
                {
                    traversal_volume.leaf_start = (uint32_t)traversal_tree.vertices.size();
                    traversal_tree.vertices.insert(traversal_tree.vertices.end(), node->vertices_, node->vertices_ + node->vertex_count_);
                    traversal_volume.leaf_end = (uint32_t)traversal_tree.vertices.size();
                }
                else
                {
                    traversal_volume.leaf_start = vertices_size;
                    vertices_size += node->vertex_count_;
                    traversal_volume.leaf_end = vertices_size;
                }
            }
            else if (RraBvhIsBoxNode(node->node_id_))
            {
                traversal_volume.volume_type = renderer::TraversalVolumeType::kBox;

                // Separate for loops needed to preserve alignment.

                uint32_t child_index = 0;
                for (auto child : node->child_nodes_)
                {
                    uint32_t child_addr = static_cast<uint32_t>(traversal_tree.volumes.size());
                    traversal_stack.push_back({child, child_addr});
                    traversal_tree.volumes.emplace_back();

                    if (child->IsEnabled() && child->IsVisible())
                    {
                        traversal_volume.child_mask = traversal_volume.child_mask | (0x1 << child_index);
                    }

                    auto child_bounds = child->GetBoundingVolume();

                    traversal_volume.child_nodes[child_index]          = child_addr;
                    traversal_volume.child_nodes_min[child_index]      = {child_bounds.min_x, child_bounds.min_y, child_bounds.min_z, 0.0f};
                    traversal_volume.child_nodes_max[child_index]      = {child_bounds.max_x, child_bounds.max_y, child_bounds.max_z, 0.0f};
                    traversal_tree.volumes[child_addr].parent          = current_index;
                    traversal_tree.volumes[child_addr].index_at_parent = child_index;

                    child_index++;
                }
            }
        }
    }

    void SceneNode::PopulateSplitVertexAttribute(SceneNode* root, const std::vector<uint32_t>& geometry_offsets, uint32_t primitive_count)
    {
        std::vector<uint8_t> primitive_counts{};
        primitive_counts.resize(primitive_count);

        root->PopulateSplitVertexAttribute(geometry_offsets, primitive_counts);
    }

    void SceneNode::PopulateSplitVertexAttribute(const std::vector<uint32_t>& geometry_offsets, std::vector<uint8_t>& primitive_counts)
    {
        if (vertex_count_)
        {
            uint32_t geo_offset{geometry_offsets[geometry_index_]};
            if (++primitive_counts[(size_t)geo_offset + primitive_index_] > 1)
            {
                for (size_t i = 0; i < vertex_count_; i += 3)
                {
                    vertices_[i].geometry_index_depth_split_opaque |= 1 << 1;
                }
            }
        }

        for (SceneNode* child : child_nodes_)
        {
            child->PopulateSplitVertexAttribute(geometry_offsets, primitive_counts);
        }
    }

    void SceneNode::SetFiltered(bool filtered)
    {
        filtered_ = filtered;
    }

}  // namespace rra
