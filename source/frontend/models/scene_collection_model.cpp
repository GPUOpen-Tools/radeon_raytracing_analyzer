//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the BVH scene model.
//=============================================================================

#include "models/scene_collection_model.h"

#include <cmath>

#include "public/intersect.h"
#include "public/rra_blas.h"
#include "public/rra_rtip_info.h"
#include "scene.h"

#include "util/stack_vector.h"

namespace rra
{
    void SceneCollectionModel::SetGeometryFilterState(const GeometryFilterState& state)
    {
        geometry_filter_state_ = state;
    }

    bool HitInBoundingVolume(const glm::vec3& origin, const glm::vec3& direction, float distance, const BoundingVolumeExtents& extent)
    {
        float bbox_diagonal = glm::length(glm::vec3{extent.max_x - extent.min_x, extent.max_y - extent.min_y, extent.max_z - extent.min_z});

        // Perfectly axis aligned triangles have 0 volume bounding boxes, so add a bit of padding.
        float     epsilon{0.01f * bbox_diagonal};
        glm::vec3 hit_pos = origin + direction * distance;
        return (extent.min_x - epsilon <= hit_pos.x && hit_pos.x <= extent.max_x + epsilon) &&
               (extent.min_y - epsilon <= hit_pos.y && hit_pos.y <= extent.max_y + epsilon) &&
               (extent.min_z - epsilon <= hit_pos.z && hit_pos.z <= extent.max_z + epsilon);
    }

    bool HitInBoundingVolumeOBB(const glm::vec3&             origin,
                                const glm::vec3&             direction,
                                float                        distance,
                                const BoundingVolumeExtents& extent,
                                const glm::mat3&             rotation)
    {
        float bbox_diagonal = glm::length(glm::vec3{extent.max_x - extent.min_x, extent.max_y - extent.min_y, extent.max_z - extent.min_z});

        // Perfectly axis aligned triangles have 0 volume bounding boxes, so add a bit of padding.
        float     epsilon{0.00001f * bbox_diagonal};
        glm::vec3 hit_pos = glm::transpose(rotation) * (origin + direction * distance);
        return (extent.min_x - epsilon <= hit_pos.x && hit_pos.x <= extent.max_x + epsilon) &&
               (extent.min_y - epsilon <= hit_pos.y && hit_pos.y <= extent.max_y + epsilon) &&
               (extent.min_z - epsilon <= hit_pos.z && hit_pos.z <= extent.max_z + epsilon);
    }

    void SceneCollectionModel::CastClosestHitRayOnBlas(SceneNode*                      blas_root,
                                                       uint64_t                        instance_node,
                                                       const glm::vec3&                origin,
                                                       const glm::vec3&                direction,
                                                       SceneCollectionModelClosestHit& scene_model_closest_hit) const
    {
        // Two stack allocated buffers (since heap allocation was a bottleneck in this function).
        StackVector<SceneNode*, 1024> buffer0{};
        StackVector<SceneNode*, 1024> buffer1{};

        // We use the buffers indirectly through a pointer so we can swap them without having to copy.
        StackVector<SceneNode*, 1024>* traverse_nodes_ptr = &buffer0;
        StackVector<SceneNode*, 1024>* swap_nodes_ptr     = &buffer1;
        traverse_nodes_ptr->PushBack(blas_root);

        // Memoized traversal of the tree.
        while (!traverse_nodes_ptr->Empty())
        {
            swap_nodes_ptr->Clear();

            for (size_t i = 0; i < traverse_nodes_ptr->Size(); i++)
            {
                SceneNode* blas_node = (*traverse_nodes_ptr)[i];

                if (!(blas_node->IsEnabled() && blas_node->IsVisible()))
                {
                    continue;
                }

                BoundingVolumeExtents extent = {blas_node->GetBoundingVolume()};

                // Get the triangle nodes. If this is not a triangle the triangle count is 0.
                StackVector<SceneTriangle, MAX_CHILD_NODES> triangles{blas_node->GetTriangles()};

                float closest = 0.0f;
                bool  intersected{};
                if (RraRtipInfoGetOBBSupported())
                {
                    // A node's extents are decoded in its parent's OBB frame; the root has no parent
                    // and its extents are in its own OBB frame (the union of its children). Use the
                    // matching frame so the test agrees with the rendered box. Previously the root
                    // fell through to an axis-aligned test, which pruned the whole BLAS for many ray
                    // directions when the root had a non-identity OBB.
                    glm::mat3 obb_rotation = (blas_node == blas_root) ? blas_node->GetRotation() : blas_node->GetParent()->GetRotation();
                    intersected            = renderer::IntersectOBB(origin,
                                                         direction,
                                                         glm::vec3(extent.min_x, extent.min_y, extent.min_z),
                                                         glm::vec3(extent.max_x, extent.max_y, extent.max_z),
                                                         obb_rotation,
                                                         closest);
                }
                else
                {
                    intersected = renderer::IntersectAABB(
                        origin, direction, glm::vec3(extent.min_x, extent.min_y, extent.min_z), glm::vec3(extent.max_x, extent.max_y, extent.max_z), closest);
                }

                if (intersected)
                {
                    // Descend into child nodes. If this is not a box node, the child count is 0.
                    // RTIP3.1 node packing: a packed-ref slot has its own per-slot box but no children of its own; the
                    // shared subtree lives under the primary sibling. A ray can hit the packed-ref's box without hitting
                    // the primary's (distinct) box, so descend into the primary's children here -- otherwise the shared
                    // triangles are never tested and become unselectable (mirrors CastRayCollectNodes/ApplyNodeSelection).
                    SceneNode* child_owner = blas_node->GetPackedPrimary();
                    if (child_owner == nullptr)
                    {
                        child_owner = blas_node;
                    }

                    uint32_t child_node_count = (uint32_t)child_owner->GetChildCount();
                    for (uint32_t child_idx = 0; child_idx < child_node_count; ++child_idx)
                    {
                        swap_nodes_ptr->PushBack(child_owner->GetChild(child_idx));
                    }
                }

                // Go over each triangle and test for intersection.
                for (size_t k = 0; k < triangles.Size(); k++)
                {
                    TriangleVertices triangle_vertices = SceneTriangleToTriVertices(triangles[k]);
                    float            hit_distance;

                    glm::vec3 a = {triangle_vertices.a.x, triangle_vertices.a.y, triangle_vertices.a.z};
                    glm::vec3 b = {triangle_vertices.b.x, triangle_vertices.b.y, triangle_vertices.b.z};
                    glm::vec3 c = {triangle_vertices.c.x, triangle_vertices.c.y, triangle_vertices.c.z};

                    if (renderer::IntersectTriangle(origin, direction, a, b, c, &hit_distance))
                    {
                        bool hit_in_bounds{};
                        if (RraRtipInfoGetOBBSupported())
                        {
                            hit_in_bounds = HitInBoundingVolumeOBB(origin, direction, hit_distance, extent, blas_node->GetParent()->GetRotation());
                        }
                        else
                        {
                            hit_in_bounds = HitInBoundingVolume(origin, direction, hit_distance, extent);
                        }
                        if (hit_distance > 0.0 && (scene_model_closest_hit.distance < 0.0f || hit_distance < scene_model_closest_hit.distance) && hit_in_bounds)
                        {
                            // Check geometry filter for per-triangle coloring modes.
                            if (ShouldFilterTriangle(geometry_filter_state_, triangles[k], blas_node))
                            {
                                continue;
                            }

                            scene_model_closest_hit.distance            = hit_distance;
                            scene_model_closest_hit.blas_index          = blas_node->GetBvhIndex();
                            scene_model_closest_hit.instance_node       = instance_node;
                            scene_model_closest_hit.triangle_child_node = blas_node->GetId();
                            scene_model_closest_hit.triangle_index      = UINT32_MAX;
                            scene_model_closest_hit.triangle_node       = blas_node;
                        }
                    }
                }
            }

            // Swap the old list with the new.
            std::swap(traverse_nodes_ptr, swap_nodes_ptr);
        }
    }
}  // namespace rra

