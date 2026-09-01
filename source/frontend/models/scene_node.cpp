//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the SceneNode class.
//=============================================================================

#include "models/scene_node.h"

#include <algorithm>
#include <array>
#include <deque>
#include <limits>
#include <unordered_map>
#include <unordered_set>

#include <iomanip>
#include <sstream>
#include "public/intersect.h"
#include "public/rra_blas.h"
#include "public/rra_bvh.h"
#include "public/rra_rtip_info.h"
#include "public/rra_tlas.h"
#include "public/shared.h"

#include "models/scene.h"
#include "util/stack_vector.h"

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
            const SceneNode* node = traversal_stack.back();
            traversal_stack.pop_back();
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

    std::string uintToHexString(uint64_t value)
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(8) << value;
        return "0x" + ss.str();
    }

    uint32_t SceneNode::CountBlasEmittedTriangles(uint64_t blas_index)
    {
        uint32_t root_id = UINT32_MAX;
        if (RraBvhGetRootNodePtr(&root_id) != kRraOk)
        {
            return 0;
        }
        if (RraBlasIsEmpty(blas_index))
        {
            return 0;
        }

        struct WalkNode
        {
            uint32_t node_id;
            uint32_t child_index;
            bool     is_packed_ref;
        };

        std::deque<WalkNode> traversal_stack;
        traversal_stack.push_back({root_id, 0, false});

        uint32_t total_triangles   = 0;
        uint32_t global_child_index = UINT32_MAX;

        while (!traversal_stack.empty())
        {
            WalkNode current{traversal_stack.back()};
            traversal_stack.pop_back();
            ++global_child_index;

            // Node packing (RTIP3.1): a packed-ref slot shares the primary sibling's subtree. It still consumes a
            // global_child_index (to keep this walk aligned with ConstructFromBlasNode and the backend parent map) but
            // emits no geometry and does not descend, so the shared subtree is counted exactly once.
            if (current.is_packed_ref)
            {
                continue;
            }

            // Cluster-ref leaves render via ConstructClusterRefInstance, not the vertex buffer, so they emit nothing.
            if (RraBlasIsClusterRefNode(blas_index, current.node_id))
            {
                continue;
            }

            if (RraBlasIsTriangleNode(blas_index, current.node_id))
            {
                uint32_t triangle_count = 0;
                RraBlasGetNodeTriangleCount(blas_index, current.node_id, current.child_index, global_child_index, &triangle_count);
                total_triangles += triangle_count;
            }

            if (!RraBlasHasChildren(blas_index, current.node_id))
            {
                continue;
            }

            uint32_t child_node_count = 0;
            RraBlasGetChildNodeCount(blas_index, current.node_id, &child_node_count);

            std::array<uint32_t, MAX_CHILD_NODES> child_nodes{};
            RraBlasGetChildNodes(blas_index, current.node_id, child_nodes.data());

            std::array<uint32_t, MAX_CHILD_NODES> child_indices{};
            RraBlasGetChildIndices(blas_index, current.node_id, child_indices.data());

            std::array<uint32_t, MAX_CHILD_NODES> box_primaries{};
            uint32_t                              box_primaries_count{0};

            for (uint32_t i{0}; i < child_node_count; ++i)
            {
                if (child_nodes[i] == current.node_id)
                {
                    continue;  // Self-reference: skip (matches ConstructFromBlasNode; neither pushes it).
                }

                bool is_packed_ref{false};
                if (RraBlasIsBoxNode(blas_index, child_nodes[i]))
                {
                    bool seen{false};
                    for (uint32_t p{0}; p < box_primaries_count; ++p)
                    {
                        if (box_primaries[p] == child_nodes[i])
                        {
                            seen = true;
                            break;
                        }
                    }
                    if (seen)
                    {
                        is_packed_ref = true;
                    }
                    else
                    {
                        box_primaries[box_primaries_count++] = child_nodes[i];
                    }
                }

                traversal_stack.push_back({child_nodes[i], child_indices[i], is_packed_ref});
            }
        }

        return total_triangles;
    }

    SceneNode* SceneNode::ConstructFromBlasNode(uint64_t blas_index, uint32_t root_id, renderer::RraVertex* vertex_buffer, std::byte* child_buffer)
    {
        uint32_t current_child_buffer_offset{0};

        SceneNode* root_node = new (child_buffer + current_child_buffer_offset) SceneNode();
        current_child_buffer_offset += sizeof(SceneNode);
        root_node->node_id_            = root_id;
        root_node->depth_              = 0;
        root_node->bvh_index_          = blas_index;
        root_node->is_tlas_            = false;
        root_node->global_child_index_ = 0;

        if (RraBlasIsEmpty(blas_index))
        {
            return root_node;
        }

        std::deque<SceneNode*> traversal_stack;
        traversal_stack.push_back(root_node);

        uint32_t vertex_buffer_idx{0};
        uint32_t global_child_index{UINT32_MAX};

        RraErrorCode error_code = kRraOk;
        uint64_t     total_node_count{};
        error_code = RraBlasGetTotalNodeCount(blas_index, &total_node_count);
        RRA_ASSERT(error_code == kRraOk);
        const size_t node_buffer_capacity_bytes = (total_node_count + 1) * sizeof(SceneNode);

        // Size against the triangles the traversal actually emits, not the header's unique-triangle count, which can
        // under-count (and cause a vertex-buffer overflow) when triangle nodes share prim-ranges.
        uint32_t vertex_buffer_capacity = CountBlasEmittedTriangles(blas_index) * 3;

        while (!traversal_stack.empty())
        {
            SceneNode* node{traversal_stack.back()};
            traversal_stack.pop_back();
            ++global_child_index;
            node->global_child_index_ = global_child_index;

            error_code = RraBlasGetBoundingVolumeExtents(blas_index, node->node_id_, node->child_index_, node->global_child_index_, &node->bounding_volume_);
            RRA_ASSERT(error_code == kRraOk);

            // Node packing (RTIP3.1): a packed-ref slot draws its own bounding box (resolved above via its child_index)
            // but shares the primary sibling's subtree, so it must not descend children or emit geometry here.
            if (node->IsPackedRef())
            {
                node->obb_index_ = kObbDisabled;
                node->rotation_  = glm::mat3(1.0f);
                continue;
            }

            // A Cluster BLAS (CBLAS) has hardware instance-node leaves that reference CLASes (nested instancing).
            // Build a renderer::Instance for each such leaf so the BLAS pane renders the referenced CLAS geometry
            // (flattened by the leaf transform), reusing the TLAS instance-render path.
            if (RraBlasIsClusterRefNode(blas_index, node->node_id_))
            {
                ConstructClusterRefInstance(blas_index, node);
                continue;
            }

            bool is_triangle_node = RraBlasIsTriangleNode(blas_index, node->node_id_);
            bool is_box_node      = RraBlasIsBoxNode(blas_index, node->node_id_);
            bool has_children     = RraBlasHasChildren(blas_index, node->node_id_);

            if (is_triangle_node)
            {
                // Get the triangle nodes. If this is not a triangle the triangle count is 0.
                uint32_t triangle_count;
                error_code = RraBlasGetNodeTriangleCount(blas_index, node->node_id_, node->child_index_, node->global_child_index_, &triangle_count);
                RRA_ASSERT(error_code == kRraOk);

                RRA_ASSERT(vertex_buffer_idx + (triangle_count * 3) <= vertex_buffer_capacity);

                // Make vertices_ a subset of the BLAS's vertex buffer.
                node->vertices_ = vertex_buffer + vertex_buffer_idx;
                vertex_buffer_idx += triangle_count * 3;

                // Continue with processing the node if it's a triangle node with 1 or more triangles within.
                if (triangle_count > 0)
                {
                    // Populate a vector of triangle vertex data.
                    std::array<TriangleVertices, MAX_TRIANGLES> triangles{};
                    error_code = RraBlasGetNodeTriangles(blas_index, node->node_id_, node->child_index_, node->global_child_index_, triangles.data());
                    RRA_ASSERT(error_code == kRraOk);

                    // Retrieve the geometry index associated with the current triangle node.
                    error_code = RraBlasGetGeometryIndex(blas_index, node->node_id_, node->child_index_, node->global_child_index_, &node->geometry_index_);
                    RRA_ASSERT(error_code == kRraOk);

                    uint32_t geometry_flags = 0;
                    error_code              = RraBlasGetGeometryFlags(blas_index, node->geometry_index_, &geometry_flags);
                    RRA_ASSERT(error_code == kRraOk);

                    // Extract the opacity flag.
                    bool is_opaque = (geometry_flags & GeometryFlags::kOpaque) == GeometryFlags::kOpaque;

                    error_code =
                        RraBlasGetPrimitiveIndex(blas_index, node->node_id_, node->child_index_, node->global_child_index_, 0, &node->primitive_index_);
                    RRA_ASSERT(error_code == kRraOk);

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
                        error_code         = RraBlasGetTriangleSurfaceAreaHeuristic(blas_index, node->node_id_, node->global_child_index_, &triangle_sah);
                        RRA_ASSERT(error_code == kRraOk);

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

            if (!is_box_node)
            {
                node->obb_index_ = kObbDisabled;
                node->rotation_  = glm::mat3(1.0f);
            }

            if (!has_children)
            {
                continue;
            }

            uint32_t child_node_count;
            error_code = RraBlasGetChildNodeCount(blas_index, node->node_id_, &child_node_count);
            RRA_ASSERT(error_code == kRraOk);

            std::array<uint32_t, MAX_CHILD_NODES> child_nodes{};
            error_code = RraBlasGetChildNodes(blas_index, node->node_id_, child_nodes.data());
            RRA_ASSERT(error_code == kRraOk);

            std::array<uint32_t, MAX_CHILD_NODES> child_indices{};
            error_code = RraBlasGetChildIndices(blas_index, node->node_id_, child_indices.data());
            RRA_ASSERT(error_code == kRraOk);

            // Node packing (RTIP3.1): up to 4 sibling slots of this box node may point to the same child node, each
            // with its own bounding box. Sharing is always within a single parent, so dedup locally over this parent's
            // child list: create one SceneNode per slot (so every slot's distinct box and tree row survives), but only
            // the first (primary) slot for a given node id expands the shared subtree. Later slots become packed-refs.
            std::array<std::pair<uint32_t, SceneNode*>, MAX_CHILD_NODES> primaries_by_node_id{};
            uint32_t                                                     primaries_count{0};

            for (uint32_t i{0}; i < child_node_count; ++i)
            {
                if (child_nodes[i] == node->node_id_)
                {
                    // Self refencing node will cause a stack overflow. Skip to prevent a crash.
                    continue;
                }

                // Use placement new operator to allocate child in child_nodes_buffer_.
                // Global child index is set at beginning of traversal loop.
                RRA_ASSERT(current_child_buffer_offset + sizeof(SceneNode) <= node_buffer_capacity_bytes);

                SceneNode* new_node = new (child_buffer + current_child_buffer_offset) SceneNode();
                current_child_buffer_offset += sizeof(SceneNode);
                new_node->node_id_     = child_nodes[i];
                new_node->bvh_index_   = blas_index;
                new_node->is_tlas_     = false;
                new_node->depth_       = node->depth_ + 1;
                new_node->parent_      = node;
                new_node->child_index_ = child_indices[i];
                node->child_nodes_.PushBack(new_node);

                SceneNode* primary{nullptr};
                if (RraBlasIsBoxNode(blas_index, child_nodes[i]))
                {
                    for (uint32_t p{0}; p < primaries_count; ++p)
                    {
                        if (primaries_by_node_id[p].first == child_nodes[i])
                        {
                            primary = primaries_by_node_id[p].second;
                            break;
                        }
                    }

                    if (primary == nullptr)
                    {
                        primaries_by_node_id[primaries_count++] = {child_nodes[i], new_node};
                    }
                }

                if (primary != nullptr)
                {
                    // Packed-ref duplicate: keep the SceneNode (for its own box / tree row) but flag it so that when it
                    // is popped it only resolves its own bounding box and global index; it does not re-expand the shared
                    // subtree or re-emit geometry, which the primary sibling handles exactly once.
                    new_node->packed_primary_ = primary;
                }

                // Push every slot (primary and packed-ref) so each gets its per-slot bounding volume and a unique
                // global_child_index_ assigned in the pop step; packed-refs early-out there before descending.
                traversal_stack.push_back(new_node);
            }

            if (is_box_node && (rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() >= rta::RayTracingIpLevel::RtIp3_0)
            {
                error_code = RraBlasGetNodeObbIndex(blas_index, node->node_id_, &node->obb_index_);
                RRA_ASSERT(error_code == kRraOk);
                error_code = RraBlasGetNodeBoundingVolumeOrientation(blas_index, node->node_id_, &node->rotation_[0][0]);
                RRA_ASSERT(error_code == kRraOk);
            }
        }

        return root_node;
    }

    void SceneNode::ConstructClusterRefInstance(uint64_t blas_index, SceneNode* node)
    {
        renderer::Instance instance = {};
        instance.selected           = false;
        instance.instance_node      = node->node_id_;
        instance.depth              = node->depth_;

        // Resolve the CLAS referenced by this cluster-ref leaf. If it can't be resolved, leave the node
        // without an instance (it will simply not render).
        RraErrorCode error_code = RraBlasGetClasIndexFromClusterRefNode(blas_index, node->node_id_, &instance.blas_index);
        if (error_code != kRraOk)
        {
            node->obb_index_ = kObbDisabled;
            node->rotation_  = glm::mat3(1.0f);
            return;
        }

        // The cluster-ref leaf stores a world-to-object transform in the same HW instance-node layout as a TLAS
        // instance, so apply the same inverse to get the object-to-world transform used for rendering.
        instance.transform = glm::mat4(0.0f);
        error_code         = RraBlasGetClusterRefNodeTransform(blas_index, node->node_id_, reinterpret_cast<float*>(&instance.transform));
        RRA_ASSERT(error_code == kRraOk);
        instance.transform[3][3] = 1.0f;
        instance.transform       = glm::inverse(instance.transform);

        instance.bounding_volume = node->bounding_volume_;

        // The CLAS id (0..N) doubles as the instance index. It is unique per cluster-ref leaf within the CBLAS,
        // which is what the scene's rebraid/instance-node maps require.
        uint32_t clas_id = 0;
        error_code       = RraBlasGetClusterRefNodeId(blas_index, node->node_id_, &clas_id);
        RRA_ASSERT(error_code == kRraOk);
        instance.instance_index        = clas_id;
        instance.instance_unique_index = clas_id;

        // Query per-instance stats from the referenced CLAS (a normal triangle BLAS).
        uint32_t root_node = UINT32_MAX;
        error_code         = RraBvhGetRootNodePtr(&root_node);
        RRA_ASSERT(error_code == kRraOk);

        RraBlasGetMaxTreeDepth(instance.blas_index, &instance.max_depth);
        RraBlasGetAvgTreeDepth(instance.blas_index, &instance.average_depth);
        RraBlasGetTriangleNodeCount(instance.blas_index, &instance.triangle_count);
        RraBlasGetAverageSurfaceAreaHeuristic(instance.blas_index, root_node, 0, true, &instance.average_triangle_sah);
        RraBlasGetMinimumSurfaceAreaHeuristic(instance.blas_index, root_node, 0, true, &instance.min_triangle_sah);
        RraBlasGetBuildFlags(instance.blas_index, reinterpret_cast<VkBuildAccelerationStructureFlagBitsKHR*>(&instance.build_flags));

        // A degenerate leaf AABB means the CBLAS did not provide usable extents; reconstruct the world-space AABB
        // from the CLAS root bounds transformed by the leaf transform (same approach as the TLAS instance path).
        const BoundingVolumeExtents& iv = instance.bounding_volume;
        if (iv.min_x == iv.max_x && iv.min_y == iv.max_y && iv.min_z == iv.max_z)
        {
            BoundingVolumeExtents blas_extents{};
            if (RraBlasGetBoundingVolumeExtents(instance.blas_index, root_node, 0, 0, &blas_extents) == kRraOk)
            {
                const glm::vec3 corners[8] = {
                    {blas_extents.min_x, blas_extents.min_y, blas_extents.min_z},
                    {blas_extents.max_x, blas_extents.min_y, blas_extents.min_z},
                    {blas_extents.min_x, blas_extents.max_y, blas_extents.min_z},
                    {blas_extents.min_x, blas_extents.min_y, blas_extents.max_z},
                    {blas_extents.max_x, blas_extents.max_y, blas_extents.min_z},
                    {blas_extents.max_x, blas_extents.min_y, blas_extents.max_z},
                    {blas_extents.min_x, blas_extents.max_y, blas_extents.max_z},
                    {blas_extents.max_x, blas_extents.max_y, blas_extents.max_z},
                };

                const glm::mat4 world_transform = glm::transpose(instance.transform);

                glm::vec3 world_min(std::numeric_limits<float>::max());
                glm::vec3 world_max(std::numeric_limits<float>::lowest());
                for (const glm::vec3& corner : corners)
                {
                    glm::vec3 world_corner = glm::vec3(world_transform * glm::vec4(corner, 1.0f));
                    world_min              = glm::min(world_min, world_corner);
                    world_max              = glm::max(world_max, world_corner);
                }

                instance.bounding_volume.min_x = world_min.x;
                instance.bounding_volume.min_y = world_min.y;
                instance.bounding_volume.min_z = world_min.z;
                instance.bounding_volume.max_x = world_max.x;
                instance.bounding_volume.max_y = world_max.y;
                instance.bounding_volume.max_z = world_max.z;
                node->bounding_volume_         = instance.bounding_volume;
            }
        }

        node->instance_  = instance;
        node->obb_index_ = kObbDisabled;
        node->rotation_  = glm::mat3(1.0f);
    }

    void SceneNode::BuildClusterSubInstances(SceneNode* node, const renderer::Instance& cblas_instance)
    {
        const uint64_t cblas_index = cblas_instance.blas_index;
        if (!RraBlasIsClusterBlas(cblas_index))
        {
            return;
        }

        uint32_t root_node = UINT32_MAX;
        if (RraBvhGetRootNodePtr(&root_node) != kRraOk)
        {
            return;
        }

        // Traverse the CBLAS collecting its cluster-reference leaves (each references one CLAS).
        std::deque<uint32_t> traversal_stack;
        traversal_stack.push_back(root_node);
        while (!traversal_stack.empty())
        {
            const uint32_t current = traversal_stack.back();
            traversal_stack.pop_back();

            if (RraBlasIsClusterRefNode(cblas_index, current))
            {
                uint64_t clas_index = 0;
                if (RraBlasGetClasIndexFromClusterRefNode(cblas_index, current, &clas_index) != kRraOk)
                {
                    continue;
                }

                // The cluster-ref leaf transform maps CLAS-object -> CBLAS-object in the same (inverted, row-major)
                // convention as a TLAS instance transform. Compose it with the CBLAS instance transform to place the
                // CLAS in world space. In storage (transposed) convention this composition is cref * cblas.
                glm::mat4 cref_transform = glm::mat4(0.0f);
                if (RraBlasGetClusterRefNodeTransform(cblas_index, current, reinterpret_cast<float*>(&cref_transform)) != kRraOk)
                {
                    continue;
                }
                cref_transform[3][3] = 1.0f;
                cref_transform       = glm::inverse(cref_transform);

                // Inherit the CBLAS instance's shared metadata (instance_index for selection, flags, mask, depth),
                // and point the sub-instance at the CLAS geometry with the composed transform.
                renderer::Instance sub_instance = cblas_instance;
                sub_instance.blas_index         = clas_index;
                sub_instance.transform          = cref_transform * cblas_instance.transform;

                RraBlasGetTriangleNodeCount(clas_index, &sub_instance.triangle_count);

                node->cluster_sub_instances_.push_back(sub_instance);
                continue;
            }

            if (RraBlasHasChildren(cblas_index, current))
            {
                uint32_t child_count = 0;
                if (RraBlasGetChildNodeCount(cblas_index, current, &child_count) != kRraOk)
                {
                    continue;
                }
                std::vector<uint32_t> children(child_count);
                if (RraBlasGetChildNodes(cblas_index, current, children.data()) != kRraOk)
                {
                    continue;
                }
                for (uint32_t i = 0; i < child_count; ++i)
                {
                    if (children[i] != current)
                    {
                        traversal_stack.push_back(children[i]);
                    }
                }
            }
        }
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

        // A CBLAS instance has no geometry of its own; emit its precomputed flattened CLAS sub-instances instead so
        // the nested cluster geometry is drawn (each keyed by its own CLAS blas_index). The selection state is
        // inherited so selecting the CBLAS instance highlights all of its CLAS geometry.
        if (!cluster_sub_instances_.empty())
        {
            for (renderer::Instance sub_instance : cluster_sub_instances_)
            {
                sub_instance.selected = selected;
                instance_map[sub_instance.blas_index].push_back(sub_instance);
            }
            return;
        }

        instance_map[instance.blas_index].push_back(instance);
    }

    uint64_t SceneNode::GetChildIdHash() const
    {
        const rta::RayTracingIpLevel rtip = (rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel();
        // Packing can occur in both BLAS and TLAS QuantizedBVH8 box nodes; query the matching structure.
        const bool has_packing = is_tlas_ ? RraTlasHasNodePacking(bvh_index_) : RraBlasHasNodePacking(bvh_index_);
        if (rtip == rta::RayTracingIpLevel::RtIp3_1 && has_packing)
        {
            return ((uint64_t)global_child_index_ << 32) | node_id_;
        }
        return node_id_;
    }

    SceneNode* SceneNode::ConstructFromBlas(uint32_t blas_index, renderer::RraVertex* vertex_buffer, std::byte* child_buffer)
    {
        uint32_t     root_node_index = UINT32_MAX;
        RraErrorCode error_code      = RraBvhGetRootNodePtr(&root_node_index);
        RRA_ASSERT(error_code == kRraOk);

        auto node = ConstructFromBlasNode(blas_index, root_node_index, vertex_buffer, child_buffer);

        uint32_t geometry_count{};
        error_code = RraBlasGetGeometryCount(blas_index, &geometry_count);
        RRA_ASSERT(error_code == kRraOk);
        std::vector<uint32_t> geometry_offsets{};
        geometry_offsets.resize(geometry_count);
        uint32_t current_geo_offset{0};

        for (uint32_t geo_idx{0}; geo_idx < geometry_count; ++geo_idx)
        {
            geometry_offsets[geo_idx] = current_geo_offset;

            uint32_t primitive_count{};
            error_code = RraBlasGetGeometryPrimitiveCount(blas_index, geo_idx, &primitive_count);
            RRA_ASSERT(error_code == kRraOk);
            current_geo_offset += primitive_count;
        }
        PopulateSplitVertexAttribute(node, geometry_offsets, current_geo_offset);

        return node;
    }

    uint64_t GetGeometryPrimitiveIndexKey(uint32_t geometry_index, uint32_t primitive_index)
    {
        return (static_cast<uint64_t>(geometry_index) << 32) | static_cast<uint64_t>(primitive_index);
    }

    SceneNode* SceneNode::ConstructFromTlasBoxNode(uint64_t                                         tlas_index,
                                                   uint32_t                                         node_id,
                                                   uint32_t                                         child_index,
                                                   uint32_t                                         depth,
                                                   std::unordered_map<uint64_t, BlasIntrinsicInfo>* blas_cache,
                                                   bool                                             is_packed_ref)
    {
        SceneNode* node    = new SceneNode();
        node->node_id_     = node_id;
        node->depth_       = depth;
        node->bvh_index_   = tlas_index;
        node->is_tlas_     = true;
        node->child_index_ = child_index;

        RraErrorCode error_code = RraTlasGetBoundingVolumeExtents(tlas_index, node_id, child_index, &node->bounding_volume_);
        RRA_ASSERT(error_code == kRraOk);

        // Node packing (RTIP3.1): a packed-ref slot draws its own per-slot bounding box (resolved above via its
        // child_index) but shares the primary sibling's subtree, so it must not build an instance or recurse. The
        // caller wires up packed_primary_ afterwards. Mirrors the BLAS packed-ref handling.
        if (is_packed_ref)
        {
            node->obb_index_ = kObbDisabled;
            node->rotation_  = glm::mat3(1.0f);
            return node;
        }

        bool is_instance_node = RraTlasIsInstanceNode(tlas_index, node_id);
        if (is_instance_node)
        {
            renderer::Instance instance = {};
            instance.selected           = false;
            instance.instance_node      = node_id;
            instance.depth              = depth;

            instance.transform = glm::mat4(0.0f);  // Reset the transform to prevent misalignment.

            error_code = RraTlasGetInstanceNodeTransform(tlas_index, node_id, reinterpret_cast<float*>(&instance.transform));
            RRA_ASSERT(error_code == kRraOk);
            instance.transform[3][3] = 1.0f;

            // Navi IP 1.1 encoding specifies that the transform is inverse, so we inverse it again to get the correct transform.
            instance.transform = glm::inverse(instance.transform);

            error_code = RraTlasGetBoundingVolumeExtents(tlas_index, node_id, child_index, &instance.bounding_volume);
            RRA_ASSERT(error_code == kRraOk);

            error_code = RraTlasGetBlasIndexFromInstanceNode(tlas_index, node_id, &instance.blas_index);
            if (error_code != kRraOk)
            {
                return node;
            }

            uint32_t root_node = UINT32_MAX;
            error_code         = RraBvhGetRootNodePtr(&root_node);
            RRA_ASSERT(error_code == kRraOk);

            // The following values depend only on the referenced BLAS, not on this instance. Traversing the BLAS to
            // recompute them for every instance is O(instances * BLAS-nodes); cache per BLAS so instances that share a
            // BLAS reuse the result.
            auto compute_blas_info = [root_node](uint64_t blas_index) {
                BlasIntrinsicInfo info{};
                RraErrorCode      ec = RraBlasGetMaxTreeDepth(blas_index, &info.max_depth);
                RRA_ASSERT(ec == kRraOk);
                ec = RraBlasGetAvgTreeDepth(blas_index, &info.average_depth);
                RRA_ASSERT(ec == kRraOk);
                ec = RraBlasGetTriangleNodeCount(blas_index, &info.triangle_count);
                RRA_ASSERT(ec == kRraOk);
                ec = RraBlasGetAverageSurfaceAreaHeuristic(blas_index, root_node, 0, true, &info.average_triangle_sah);
                RRA_ASSERT(ec == kRraOk);
                ec = RraBlasGetMinimumSurfaceAreaHeuristic(blas_index, root_node, 0, true, &info.min_triangle_sah);
                RRA_ASSERT(ec == kRraOk);
                RRA_UNUSED(ec);
                return info;
            };

            BlasIntrinsicInfo blas_info{};
            if (blas_cache != nullptr)
            {
                auto it = blas_cache->find(instance.blas_index);
                if (it == blas_cache->end())
                {
                    it = blas_cache->emplace(instance.blas_index, compute_blas_info(instance.blas_index)).first;
                }
                blas_info = it->second;
            }
            else
            {
                blas_info = compute_blas_info(instance.blas_index);
            }

            instance.max_depth            = blas_info.max_depth;
            instance.average_depth        = blas_info.average_depth;
            instance.triangle_count       = blas_info.triangle_count;
            instance.average_triangle_sah = blas_info.average_triangle_sah;
            instance.min_triangle_sah     = blas_info.min_triangle_sah;

            error_code = RraTlasGetUniqueInstanceIndexFromInstanceNode(tlas_index, node_id, &instance.instance_unique_index);
            RRA_ASSERT(error_code == kRraOk);

            error_code = RraTlasGetInstanceIndexFromInstanceNode(tlas_index, node_id, &instance.instance_index);
            RRA_ASSERT(error_code == kRraOk);

            error_code = RraBlasGetBuildFlags(instance.blas_index, reinterpret_cast<VkBuildAccelerationStructureFlagBitsKHR*>(&instance.build_flags));
            RRA_ASSERT(error_code == kRraOk);

            error_code = RraTlasGetInstanceNodeMask(tlas_index, node_id, &instance.mask);
            RRA_ASSERT(error_code == kRraOk);

            error_code = RraTlasGetInstanceFlags(tlas_index, node_id, &instance.flags);
            RRA_ASSERT(error_code == kRraOk);

            // Partition index is only meaningful for PTLAS traces; leave it at 0 otherwise.
            if (RraTlasIsPartitioned(tlas_index))
            {
                uint32_t partition_index = 0;
                bool     partition_active = false;
                if (RraTlasGetInstancePartitionIndex(tlas_index, instance.instance_index, &partition_index, &partition_active) == kRraOk)
                {
                    instance.partition_index = partition_index;
                }
            }

            // PTLAS dumps do not populate the parent pointers that the TLAS uses to derive an instance node's
            // bounding volume, so RraTlasGetBoundingVolumeExtents returns a degenerate (zero) AABB for them. When
            // that happens, reconstruct the instance's world-space AABB from the BLAS root bounds transformed by the
            // instance transform (the same authoritative data the renderer uses), so broad-phase picking works.
            const BoundingVolumeExtents& iv = instance.bounding_volume;
            if (iv.min_x == iv.max_x && iv.min_y == iv.max_y && iv.min_z == iv.max_z)
            {
                BoundingVolumeExtents blas_extents{};
                if (RraBlasGetBoundingVolumeExtents(instance.blas_index, root_node, 0, 0, &blas_extents) == kRraOk)
                {
                    const glm::vec3 corners[8] = {
                        {blas_extents.min_x, blas_extents.min_y, blas_extents.min_z},
                        {blas_extents.max_x, blas_extents.min_y, blas_extents.min_z},
                        {blas_extents.min_x, blas_extents.max_y, blas_extents.min_z},
                        {blas_extents.min_x, blas_extents.min_y, blas_extents.max_z},
                        {blas_extents.max_x, blas_extents.max_y, blas_extents.min_z},
                        {blas_extents.max_x, blas_extents.min_y, blas_extents.max_z},
                        {blas_extents.min_x, blas_extents.max_y, blas_extents.max_z},
                        {blas_extents.max_x, blas_extents.max_y, blas_extents.max_z},
                    };

                    // The backend returns the instance transform in row-major 3x4 layout, so instance.transform
                    // ends up as the transpose of the forward (local->world) transform (its translation lives in the
                    // 4th row, not the 4th column). Transpose it back before applying it to the BLAS corners.
                    const glm::mat4 world_transform = glm::transpose(instance.transform);

                    glm::vec3 world_min(std::numeric_limits<float>::max());
                    glm::vec3 world_max(std::numeric_limits<float>::lowest());
                    for (const glm::vec3& corner : corners)
                    {
                        glm::vec3 world_corner = glm::vec3(world_transform * glm::vec4(corner, 1.0f));
                        world_min              = glm::min(world_min, world_corner);
                        world_max              = glm::max(world_max, world_corner);
                    }

                    instance.bounding_volume.min_x = world_min.x;
                    instance.bounding_volume.min_y = world_min.y;
                    instance.bounding_volume.min_z = world_min.z;
                    instance.bounding_volume.max_x = world_max.x;
                    instance.bounding_volume.max_y = world_max.y;
                    instance.bounding_volume.max_z = world_max.z;
                    node->bounding_volume_         = instance.bounding_volume;
                }
            }

            node->instance_ = instance;

            // If this instance references a Cluster BLAS (CBLAS), precompute its flattened CLAS instances so the
            // TLAS view renders the nested geometry instead of just the CBLAS bounding box.
            BuildClusterSubInstances(node, instance);

            node->obb_index_ = kObbDisabled;
            node->rotation_  = glm::mat3(1.0f);
            return node;
        }

        error_code = RraTlasGetNodeObbIndex(tlas_index, node_id, &node->obb_index_);
        RRA_ASSERT(error_code == kRraOk);
        error_code = RraTlasGetNodeBoundingVolumeOrientation(tlas_index, node_id, &node->rotation_[0][0]);
        RRA_ASSERT(error_code == kRraOk);

        uint32_t child_node_count{};
        error_code = RraTlasGetChildNodeCount(tlas_index, node_id, &child_node_count);
        RRA_ASSERT(error_code == kRraOk);

        std::array<uint32_t, MAX_CHILD_NODES> child_nodes{};
        error_code = RraTlasGetChildNodes(tlas_index, node_id, child_nodes.data());
        RRA_ASSERT(error_code == kRraOk);

        std::array<uint32_t, MAX_CHILD_NODES> child_node_indices{};
        error_code = RraTlasGetChildIndices(tlas_index, node_id, child_node_indices.data());
        RRA_ASSERT(error_code == kRraOk);

        // Node packing (RTIP3.1): up to 4 sibling slots of this box node may point to the same child node, each with its
        // own bounding box. Sharing is always within a single parent, so dedup locally over this parent's child list:
        // build one SceneNode per slot (so every slot's distinct box and tree row survives), but only the first (primary)
        // slot for a given node id expands the shared subtree. Later slots become shallow packed-refs pointing at it.
        std::array<std::pair<uint32_t, SceneNode*>, MAX_CHILD_NODES> primaries_by_node_id{};
        uint32_t                                                     primaries_count{0};

        for (uint32_t i = 0; i < child_node_count; ++i)
        {
            // Only box (internal) nodes are packed; a shared instance-node/leaf pointer is never a packed-ref.
            SceneNode* primary{nullptr};
            if (RraTlasIsBoxNode(tlas_index, child_nodes[i]))
            {
                for (uint32_t p{0}; p < primaries_count; ++p)
                {
                    if (primaries_by_node_id[p].first == child_nodes[i])
                    {
                        primary = primaries_by_node_id[p].second;
                        break;
                    }
                }
            }

            const bool child_is_packed_ref = (primary != nullptr);
            auto       child_node_ptr =
                SceneNode::ConstructFromTlasBoxNode(tlas_index, child_nodes[i], child_node_indices[i], depth + 1, blas_cache, child_is_packed_ref);
            child_node_ptr->parent_ = node;

            if (child_is_packed_ref)
            {
                child_node_ptr->packed_primary_ = primary;
            }
            else if (RraTlasIsBoxNode(tlas_index, child_nodes[i]))
            {
                primaries_by_node_id[primaries_count++] = {child_nodes[i], child_node_ptr};
            }

            node->child_nodes_.PushBack(child_node_ptr);
        }

        return node;
    }

    SceneNode* SceneNode::ConstructFromTlas(uint64_t tlas_index)
    {
        uint32_t     root_node_index = UINT32_MAX;
        RraErrorCode error_code      = RraBvhGetRootNodePtr(&root_node_index);
        RRA_ASSERT(error_code == kRraOk);
        std::unordered_map<uint64_t, BlasIntrinsicInfo> blas_cache;
        SceneNode*                                      root = ConstructFromTlasBoxNode(tlas_index, root_node_index, 0, 0, &blas_cache);

        // Assign global_child_index_ in the exact order the tree view's TraverseTree numbers nodes, so composite keys
        // ((global_child_index << 32) | node_id) agree between the scene and the tree browser for RTIP3.1 TLAS node
        // packing. TraverseTree uses a LIFO stack that pushes child slots 0..N and pops back(); mirror that here.
        const rta::RayTracingIpLevel rtip = (rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel();
        if (rtip == rta::RayTracingIpLevel::RtIp3_1 && RraTlasHasNodePacking(tlas_index))
        {
            std::deque<SceneNode*> index_stack;
            index_stack.push_back(root);
            uint32_t global_child_index{UINT32_MAX};
            while (!index_stack.empty())
            {
                SceneNode* current = index_stack.back();
                index_stack.pop_back();
                current->global_child_index_ = ++global_child_index;
                for (SceneNode* child : current->child_nodes_)
                {
                    index_stack.push_back(child);
                }
            }
        }

        return root;
    }

    void SceneNode::ResetSelection(std::unordered_set<uint64_t>& selected_node_ids)
    {
        selected_ = false;
        selected_node_ids.erase(GetChildIdHash());

        // A packed-ref has no subtree of its own; its geometry lives under the primary sibling that shares its node id.
        // Recurse into the primary's children so the shared subtree's highlight is cleared (mirrors ApplyNodeSelection).
        auto& reset_children = (IsPackedRef() && packed_primary_ != nullptr) ? packed_primary_->child_nodes_ : child_nodes_;
        for (auto child_node : reset_children)
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

    void SceneNode::ApplyNodeSelection(std::unordered_set<uint64_t>& selected_node_ids)
    {
        if (!(visible_ && enabled_) || filtered_)
        {
            return;
        }

        selected_ = true;
        selected_node_ids.insert(GetChildIdHash());

        // A packed-ref has no subtree of its own; its geometry lives under the primary sibling that shares its node id.
        // Recurse into the primary's children so selecting a packed box highlights the shared subtree (gray triangles)
        // without duplicating geometry (no over-draw).
        auto& select_children = (IsPackedRef() && packed_primary_ != nullptr) ? packed_primary_->child_nodes_ : child_nodes_;
        for (auto child_node : select_children)
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

    void SceneNode::CollectNodes(std::map<uint64_t, SceneNode*>& nodes)
    {
        nodes[GetChildIdHash()] = this;
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

    void SceneNode::SetAllChildrenAsVisible(std::unordered_set<uint64_t>& selected_node_ids)
    {
        if (!visible_)
        {
            visible_ = true;
            selected_node_ids.insert(GetChildIdHash());
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

            // A packed-ref (RTIP3.1 node packing) has its own per-slot box but no subtree of its own; the shared
            // geometry lives under the primary sibling. A ray can hit the packed-ref's box without hitting the
            // primary's (distinct) box, so descend into the primary's children to collect the shared subtree's
            // instance descendants for hit-testing (mirrors ApplyNodeSelection/ResetSelection).
            auto& collect_children = (IsPackedRef() && packed_primary_ != nullptr) ? packed_primary_->child_nodes_ : child_nodes_;
            for (auto child : collect_children)
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

    const std::vector<renderer::Instance>& SceneNode::GetClusterSubInstances() const
    {
        return cluster_sub_instances_;
    }

    StackVector<SceneTriangle, MAX_CHILD_NODES> SceneNode::GetTriangles() const
    {
        RRA_ASSERT(vertex_count_ % 3 == 0);
        RRA_ASSERT(vertex_count_ == 0 || vertices_ != nullptr);
        StackVector<SceneTriangle, MAX_CHILD_NODES> triangles;
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

    uint64_t SceneNode::GetId() const
    {
        if (!is_tlas_)
        {
            return GetChildIdHash();
        }
        const rta::RayTracingIpLevel rtip = (rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel();
        if (rtip == rta::RayTracingIpLevel::RtIp3_1 && RraTlasHasNodePacking(bvh_index_))
        {
            return ((uint64_t)global_child_index_ << 32) | node_id_;
        }
        return node_id_;
    }

    void SceneNode::AppendBoundingVolumesTo(renderer::BoundingVolumeList& volume_list,
                                            uint32_t                      lower_bound,
                                            uint32_t                      upper_bound,
                                            bool                          show_internal_bounds,
                                            bool                          show_leaf_bounds) const
    {
        if (depth_ > upper_bound)
        {
            return;
        }

        if (visible_ && !filtered_)
        {
            for (auto child : child_nodes_)
            {
                child->AppendBoundingVolumesTo(volume_list, lower_bound, upper_bound, show_internal_bounds, show_leaf_bounds);
            }

            if (depth_ >= lower_bound)
            {
                renderer::BoundingVolumeInstance bvi;
                bvi.min = {bounding_volume_.min_x, bounding_volume_.min_y, bounding_volume_.min_z, depth_};
                bvi.max = {bounding_volume_.max_x, bounding_volume_.max_y, bounding_volume_.max_z};

                bvi.metadata = glm::vec4(-1.0f, depth_, 0.0f, 1.0f);

                bool is_box_16_node{};
                bool is_box_32_node{};
                bool is_instance_node{};
                bool is_procedural_node{};
                bool is_triangle_node{};
                if (is_tlas_)
                {
                    is_box_16_node   = RraTlasIsBox16Node(bvh_index_, node_id_);
                    is_box_32_node   = RraTlasIsBox32Node(bvh_index_, node_id_);
                    is_instance_node = RraTlasIsInstanceNode(bvh_index_, node_id_);
                }
                else
                {
                    is_box_16_node     = RraBlasIsBox16Node(bvh_index_, node_id_);
                    is_box_32_node     = RraBlasIsBox32Node(bvh_index_, node_id_);
                    is_procedural_node = RraBlasIsProceduralNode(bvh_index_, node_id_);
                    is_triangle_node   = RraBlasIsTriangleNode(bvh_index_, node_id_);
                }

                bool is_internal = false;
                bool is_leaf     = false;
                if (is_box_16_node)
                {
                    bvi.metadata.x = 1.0f;
                    is_internal    = true;
                }
                else if (is_box_32_node)
                {
                    bvi.metadata.x = 2.0f;
                    is_internal    = true;
                }
                else if (is_instance_node)
                {
                    bvi.metadata.x = 3.0f;
                    is_leaf        = true;
                }
                else if (is_procedural_node)
                {
                    bvi.metadata.x = 4.0f;
                    is_leaf        = true;
                }
                else if (is_triangle_node)
                {
                    bvi.metadata.x = 5.0f;
                    is_leaf        = true;
                }

                if (selected_)
                {
                    bvi.metadata.x = 0.0f;
                }

                // A node's extents are decoded in its parent's OBB frame, so draw with the
                // parent's rotation. The root has no parent and its extents are computed in its
                // own OBB frame (union of its children), so draw the root with its own rotation.
                bvi.rotation = parent_ ? parent_->rotation_ : rotation_;

                if ((is_internal && show_internal_bounds) || (is_leaf && show_leaf_bounds))
                {
                    volume_list.push_back(bvi);
                }
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

    void SceneNode::AddToTraversalTree(bool populate_vertex_buffer, bool is_tlas, renderer::TraversalTree& traversal_tree)
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

            bool is_leaf_node{};
            bool is_box_node{};
            if (is_tlas)
            {
                is_leaf_node = RraTlasIsInstanceNode(node->bvh_index_, node->node_id_);
                is_box_node  = RraTlasIsBoxNode(node->bvh_index_, node->node_id_);
            }
            else
            {
                is_leaf_node = RraBlasIsTriangleNode(node->bvh_index_, node->node_id_);
                is_box_node  = RraBlasIsBoxNode(node->bvh_index_, node->node_id_);
            }

            renderer::TraversalVolume& traversal_volume = traversal_tree.volumes[current_index];
            traversal_volume.min       = glm::vec4(node->bounding_volume_.min_x, node->bounding_volume_.min_y, node->bounding_volume_.min_z, 1.0f);
            traversal_volume.max       = glm::vec4(node->bounding_volume_.max_x, node->bounding_volume_.max_y, node->bounding_volume_.max_z, 1.0f);
            traversal_volume.obb_index = node->obb_index_;

            // RTIP3.1 node packing: the shared subtree hangs off the primary sibling alone, so the traversal descends it
            // exactly once (via the primary). But each packed-ref sibling carries its own distinct per-slot box, and a ray
            // can hit a packed-ref's box while missing the primary's. With the subtree living only under the primary, that
            // ray would leave the subtree undescended and its triangles absent from the heatmap (the "holes"). The driver
            // resolves and de-duplicates the group's box hits before a single subtree descent, so mirror that here by
            // growing the primary's traversal box to the union of the group's per-slot boxes: the subtree then descends
            // whenever any slot in the group is hit, still exactly once. The packed-ref volumes keep their own boxes
            // unchanged, so every slot is still box-tested (N tests) and drawn as its own distinct box.
            //
            // A stackless descent redirect (the kBoxPackedRef idea) was rejected: this shader backtracks via a single
            // parent pointer per volume, so a shared child descended through a packed-ref would rise back into the primary
            // and the group would be re-descended on the next sibling scan, double-counting. Unioning keeps the subtree
            // uniquely under the primary and needs no shader change.
            if (node->parent_ != nullptr && !node->IsPackedRef())
            {
                for (SceneNode* sibling : node->parent_->child_nodes_)
                {
                    if (sibling->packed_primary_ == node)
                    {
                        traversal_volume.min = glm::min(
                            traversal_volume.min,
                            glm::vec3(sibling->bounding_volume_.min_x, sibling->bounding_volume_.min_y, sibling->bounding_volume_.min_z));
                        traversal_volume.max = glm::max(
                            traversal_volume.max,
                            glm::vec3(sibling->bounding_volume_.max_x, sibling->bounding_volume_.max_y, sibling->bounding_volume_.max_z));
                    }
                }
            }

            bool is_instance_node = is_tlas && is_leaf_node;
            bool is_triangle_node = !is_tlas && is_leaf_node;

            if (is_instance_node)
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
            else if (is_triangle_node)
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
            else if (is_box_node)
            {
                traversal_volume.volume_type = renderer::TraversalVolumeType::kBox;

                // RTIP3.1 node packing: a packed-ref sibling shares its child pointer with an earlier slot, so
                // ConstructFromBlasNode gives it an empty child_nodes_ and it lands here as a dead-end box. Its parent still
                // sets the child_mask bit below, so the ray box-tests every packed slot's own AABB (N box tests), but the
                // shared subtree is descended exactly once — via the primary sibling, which alone carries the children.
                // Do NOT populate children here from the shared node: that would re-descend the subtree once per slot (the
                // original over-count bug). A ray that hits a packed slot but misses the primary's AABB is handled above by
                // unioning the group's per-slot boxes into the primary's traversal box, so the shared subtree still descends
                // (once) rather than leaving heatmap holes — matching the driver's resolve-and-dedup-before-descent behavior.
                RRA_ASSERT(node->child_nodes_.Size() <= MAX_CHILD_NODES);

                // First pass: reserve child slots in volumes and record their addresses.
                // emplace_back may reallocate, so all mutations to volumes must complete
                // before re-taking a reference into the vector for the current node.
                std::vector<uint32_t> child_addrs(node->child_nodes_.Size());
                uint32_t              child_index = 0;
                for (auto child : node->child_nodes_)
                {
                    uint32_t child_addr      = static_cast<uint32_t>(traversal_tree.volumes.size());
                    child_addrs[child_index] = child_addr;
                    traversal_stack.push_back({child, child_addr});
                    traversal_tree.volumes.emplace_back();
                    traversal_tree.volumes[child_addr].parent          = current_index;
                    traversal_tree.volumes[child_addr].index_at_parent = child_index;
                    child_index++;
                }

                // Second pass: write child data into the current volume. Re-fetch the
                // reference by index because emplace_back above may have reallocated.
                renderer::TraversalVolume& current_volume = traversal_tree.volumes[current_index];
                child_index                               = 0;
                for (auto child : node->child_nodes_)
                {
                    RRA_ASSERT(child_index < std::size(current_volume.child_nodes));
                    uint32_t child_addr = child_addrs[child_index];

                    if (child->IsEnabled() && child->IsVisible())
                    {
                        current_volume.child_mask = current_volume.child_mask | (0x1 << child_index);
                    }

                    auto child_bounds = child->GetBoundingVolume();

                    current_volume.child_nodes[child_index]     = child_addr;
                    current_volume.child_nodes_min[child_index] = {child_bounds.min_x, child_bounds.min_y, child_bounds.min_z, 0.0f};
                    current_volume.child_nodes_max[child_index] = {child_bounds.max_x, child_bounds.max_y, child_bounds.max_z, 0.0f};

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
            if (geometry_index_ >= geometry_offsets.size())
            {
                return;
            }
            uint32_t     geo_offset{geometry_offsets[geometry_index_]};
            const size_t index = (size_t)geo_offset + primitive_index_;
            if (index >= primitive_counts.size())
            {
                primitive_counts.resize(index + 1);
            }
            if (++primitive_counts[index] > 1)
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

    uint32_t SceneNode::GetChildIndex() const
    {
        return child_index_;
    }

    uint32_t SceneNode::GetGlobalChildIndex() const
    {
        return global_child_index_;
    }

    SceneNode* SceneNode::GetPackedPrimary() const
    {
        return packed_primary_;
    }

    size_t SceneNode::GetChildCount() const
    {
        return child_nodes_.Size();
    }

    SceneNode* SceneNode::GetChild(uint32_t child_index)
    {
        RRA_ASSERT(child_index < child_nodes_.Size());
        return child_nodes_[child_index];
    }

    glm::mat3 SceneNode::GetRotation()
    {
        return rotation_;
    }

    uint64_t SceneNode::GetBvhIndex()
    {
        return bvh_index_;
    }

}  // namespace rra


