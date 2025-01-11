//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the BVH scene model.
//=============================================================================

#include "scene_collection_model.h"
#include "public/rra_blas.h"
#include "public/intersect.h"
#include "public/rra_rtip_info.h"
#include "util/stack_vector.h"

namespace rra
{
    bool HitInBoundingVolume(const glm::vec3& origin, const glm::vec3& direction, float distance, const BoundingVolumeExtents& extent)
    {
        float bbox_diagonal = glm::length(glm::vec3{extent.max_x - extent.min_x, extent.max_y - extent.min_y, extent.max_z - extent.min_z});

        // Perfectly axis aligned triangles have 0 volume bounding boxes, so add a bit of padding.
        float     epsilon{0.00001f * bbox_diagonal};
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

    void SceneCollectionModel::CastClosestHitRayOnBlas(uint64_t                        bvh_index,
                                                       uint32_t                        instance_node,
                                                       const glm::vec3&                origin,
                                                       const glm::vec3&                direction,
                                                       SceneCollectionModelClosestHit& scene_model_closest_hit) const
    {
        uint32_t root_node = UINT32_MAX;
        if (RraBvhGetRootNodePtr(&root_node) != kRraOk)
        {
            return;
        }

        uint32_t triangle_count;

        // Two stack allocated buffers (since heap allocation was a bottleneck in this function).
        StackVector<uint32_t, 1024> buffer0{};
        StackVector<uint32_t, 1024> buffer1{};

        // We use the buffers indirectly through a pointer so we can swap them without having to copy.
        StackVector<uint32_t, 1024>* traverse_nodes_ptr = &buffer0;
        StackVector<uint32_t, 1024>* swap_nodes_ptr     = &buffer1;
        traverse_nodes_ptr->PushBack(root_node);

        StackVector<uint32_t, 8> child_nodes{};

        // Memoized traversal of the tree.
        while (!traverse_nodes_ptr->Empty())
        {
            swap_nodes_ptr->Clear();

            for (size_t i = 0; i < traverse_nodes_ptr->Size(); i++)
            {
                if (ShouldSkipBLASNodeInTraversal(bvh_index, (*traverse_nodes_ptr)[i]))
                {
                    continue;
                }

                BoundingVolumeExtents extent = {};

                if (RraBlasGetBoundingVolumeExtents(bvh_index, (*traverse_nodes_ptr)[i], &extent) != kRraOk)
                {
                    continue;
                }

                // Get the triangle nodes. If this is not a triangle the triangle count is 0.
                if (RraBlasGetNodeTriangleCount(bvh_index, (*traverse_nodes_ptr)[i], &triangle_count) != kRraOk)
                {
                    continue;
                }

                // Get the triangle nodes. If this is not a triangle the triangle count is 0.
                if (RraBlasGetNodeTriangleCount(bvh_index, (*traverse_nodes_ptr)[i], &triangle_count) != kRraOk)
                {
                    continue;
                }

                float     closest = 0.0f;
                bool      intersected{};
                glm::mat3 rotation(1.0f);
                if (RraRtipInfoGetOBBSupported() && (*traverse_nodes_ptr)[i] != root_node)
                {
                    uint32_t parent_node{};
                    RraBlasGetNodeParent(bvh_index, (*traverse_nodes_ptr)[i], &parent_node);

                    RraBlasGetNodeBoundingVolumeOrientation(bvh_index, parent_node, &rotation[0][0]);
                    intersected = renderer::IntersectOBB(origin,
                                                         direction,
                                                         glm::vec3(extent.min_x, extent.min_y, extent.min_z),
                                                         glm::vec3(extent.max_x, extent.max_y, extent.max_z),
                                                         rotation,
                                                         closest);
                }
                else
                {
                    intersected = renderer::IntersectAABB(
                        origin, direction, glm::vec3(extent.min_x, extent.min_y, extent.min_z), glm::vec3(extent.max_x, extent.max_y, extent.max_z), closest);
                }

                if (intersected)
                {
                    // Get the child nodes. If this is not a box node, the child count is 0.
                    uint32_t child_node_count;
                    if (RraBlasGetChildNodeCount(bvh_index, (*traverse_nodes_ptr)[i], &child_node_count) != kRraOk)
                    {
                        continue;
                    }

                    child_nodes.Resize(child_node_count);
                    if (RraBlasGetChildNodes(bvh_index, (*traverse_nodes_ptr)[i], child_nodes.Data()) != kRraOk)
                    {
                        continue;
                    }

                    for (uint32_t child : child_nodes)
                    {
                        swap_nodes_ptr->PushBack(child);
                    }
                }

                std::vector<TriangleVertices> triangles(triangle_count);
                if (RraBlasGetNodeTriangles(bvh_index, (*traverse_nodes_ptr)[i], triangles.data()) != kRraOk)
                {
                    continue;
                }

                // Go over each triangle and test for intersection.
                for (size_t k = 0; k < triangles.size(); k++)
                {
                    TriangleVertices triangle_vertices = triangles[k];
                    float            hit_distance;

                    glm::vec3 a = {triangle_vertices.a.x, triangle_vertices.a.y, triangle_vertices.a.z};
                    glm::vec3 b = {triangle_vertices.b.x, triangle_vertices.b.y, triangle_vertices.b.z};
                    glm::vec3 c = {triangle_vertices.c.x, triangle_vertices.c.y, triangle_vertices.c.z};

                    if (renderer::IntersectTriangle(origin, direction, a, b, c, &hit_distance))
                    {
                        bool hit_in_bounds{};
                        if (RraRtipInfoGetOBBSupported())
                        {
                            hit_in_bounds = HitInBoundingVolumeOBB(origin, direction, hit_distance, extent, rotation);
                        }
                        else
                        {
                            hit_in_bounds = HitInBoundingVolume(origin, direction, hit_distance, extent);
                        }
                        if (hit_distance > 0.0 && (scene_model_closest_hit.distance < 0.0f || hit_distance < scene_model_closest_hit.distance) && hit_in_bounds)
                        {
                            scene_model_closest_hit.distance       = hit_distance;
                            scene_model_closest_hit.blas_index     = bvh_index;
                            scene_model_closest_hit.instance_node  = instance_node;
                            scene_model_closest_hit.triangle_node  = (*traverse_nodes_ptr)[i];
                            scene_model_closest_hit.triangle_index = UINT32_MAX;
                        }
                    }
                }
            }

            // Swap the old list with the new.
            std::swap(traverse_nodes_ptr, swap_nodes_ptr);
        }
    }
}  // namespace rra
