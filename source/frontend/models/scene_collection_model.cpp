#include "scene_collection_model.h"
#include "public/rra_blas.h"
#include "public/intersect.h"

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

        uint32_t              triangle_count;
        std::vector<uint32_t> traverse_nodes = {root_node};
        std::vector<uint32_t> swap_nodes;

        // Memoized traversal of the tree.
        while (!traverse_nodes.empty())
        {
            swap_nodes.clear();

            for (size_t i = 0; i < traverse_nodes.size(); i++)
            {
                if (ShouldSkipBLASNodeInTraversal(bvh_index, traverse_nodes[i]))
                {
                    continue;
                }

                BoundingVolumeExtents extent = {};

                if (RraBlasGetBoundingVolumeExtents(bvh_index, traverse_nodes[i], &extent) != kRraOk)
                {
                    continue;
                }

                if (renderer::IntersectAABB(
                        origin, direction, glm::vec3(extent.min_x, extent.min_y, extent.min_z), glm::vec3(extent.max_x, extent.max_y, extent.max_z)))
                {
                    // Get the child nodes. If this is not a box node, the child count is 0.
                    uint32_t child_node_count;
                    if (RraBlasGetChildNodeCount(bvh_index, traverse_nodes[i], &child_node_count) != kRraOk)
                    {
                        continue;
                    }

                    std::vector<uint32_t> child_nodes(child_node_count);
                    if (RraBlasGetChildNodes(bvh_index, traverse_nodes[i], child_nodes.data()) != kRraOk)
                    {
                        continue;
                    }

                    swap_nodes.insert(swap_nodes.end(), child_nodes.begin(), child_nodes.end());
                }

                // Get the triangle nodes. If this is not a triangle the triangle count is 0.
                if (RraBlasGetNodeTriangleCount(bvh_index, traverse_nodes[i], &triangle_count) != kRraOk)
                {
                    continue;
                }

                std::vector<TriangleVertices> triangles(triangle_count);
                if (RraBlasGetNodeTriangles(bvh_index, traverse_nodes[i], triangles.data()) != kRraOk)
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
                        if (hit_distance > 0.0 && (scene_model_closest_hit.distance < 0.0f || hit_distance < scene_model_closest_hit.distance) &&
                            HitInBoundingVolume(origin, direction, hit_distance, extent))
                        {
                            scene_model_closest_hit.distance       = hit_distance;
                            scene_model_closest_hit.blas_index     = bvh_index;
                            scene_model_closest_hit.instance_node  = instance_node;
                            scene_model_closest_hit.triangle_node  = traverse_nodes[i];
                            scene_model_closest_hit.triangle_index = UINT32_MAX;
                        }
                    }
                }
            }

            // Swap the old list with the new.
            traverse_nodes = swap_nodes;
        }
    }
}  // namespace rra
