//=============================================================================
// Copyright (c) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the TLAS scene model.
//=============================================================================

#include "models/tlas/tlas_scene_collection_model.h"

#include "qt_common/utils/qt_util.h"

#include "public/rra_assert.h"
#include "public/rra_error.h"

#include "public/rra_tlas.h"
#include "public/rra_blas.h"
#include "public/renderer_interface.h"
#include "public/intersect_min_max.h"

#include "glm/glm/gtx/intersect.hpp"

namespace rra
{
    TlasSceneCollectionModel::~TlasSceneCollectionModel()
    {
        for (auto scene_iter = tlas_scenes_.begin(); scene_iter != tlas_scenes_.end(); ++scene_iter)
        {
            delete scene_iter->second;
        }
    }

    void TlasSceneCollectionModel::PopulateScene(renderer::RendererInterface* renderer, uint32_t bvh_index)
    {
        tlas_scenes_[bvh_index] = CreateRenderSceneForTLAS(renderer, bvh_index);
    }

    Scene* TlasSceneCollectionModel::GetSceneByIndex(uint64_t bvh_index) const
    {
        Scene* result = nullptr;

        auto tlas_iter = tlas_scenes_.find(bvh_index);
        if (tlas_iter != tlas_scenes_.end())
        {
            result = tlas_iter->second;
        }

        return result;
    }

    bool TlasSceneCollectionModel::GetSceneBounds(uint64_t bvh_index, BoundingVolumeExtents& volume) const
    {
        auto scene = GetSceneByIndex(bvh_index);
        if (scene)
        {
            return scene->GetSceneBoundingVolume(volume);
        }
        return false;
    }

    bool TlasSceneCollectionModel::GetSceneSelectionBounds(uint64_t bvh_index, BoundingVolumeExtents& volume) const
    {
        auto scene = GetSceneByIndex(bvh_index);
        if (scene)
        {
            return scene->GetBoundingVolumeForSelection(volume);
        }
        return false;
    }

    Scene* TlasSceneCollectionModel::CreateRenderSceneForTLAS(renderer::RendererInterface* renderer, uint64_t tlas_index)
    {
        Q_UNUSED(renderer);

        // Create a scene.
        Scene* tlas_scene = new Scene{};

        // Construct a tree by using the blas_index
        auto tlas_root_node = SceneNode::ConstructFromTlas(tlas_index);

        // Initialize the scene with the given node.
        tlas_scene->Initialize(tlas_root_node);

        return tlas_scene;
    }

    RraErrorCode CastClosestHitRayOnBlas(uint64_t                        bvh_index,
                                         uint32_t                        instance_node,
                                         const glm::vec3&                origin,
                                         const glm::vec3&                direction,
                                         SceneCollectionModelClosestHit& scene_model_closest_hit)
    {
        uint32_t root_node = UINT32_MAX;
        RRA_BUBBLE_ON_ERROR(RraBvhGetRootNodePtr(&root_node));

        uint32_t              triangle_count;
        std::vector<uint32_t> traverse_nodes = {root_node};
        std::vector<uint32_t> swap_nodes;

        // Memoized traversal of the tree.
        while (!traverse_nodes.empty())
        {
            swap_nodes.clear();

            for (size_t i = 0; i < traverse_nodes.size(); i++)
            {
                BoundingVolumeExtents extent = {};

                RRA_BUBBLE_ON_ERROR(RraBlasGetBoundingVolumeExtents(bvh_index, traverse_nodes[i], &extent));
                if (renderer::IntersectMinMax(
                        origin, direction, glm::vec3(extent.min_x, extent.min_y, extent.min_z), glm::vec3(extent.max_x, extent.max_y, extent.max_z)))
                {
                    // Get the child nodes. If this is not a box node, the child count is 0.
                    uint32_t child_node_count;
                    RRA_BUBBLE_ON_ERROR(RraBlasGetChildNodeCount(bvh_index, traverse_nodes[i], &child_node_count));
                    std::vector<uint32_t> child_nodes(child_node_count);
                    RRA_BUBBLE_ON_ERROR(RraBlasGetChildNodes(bvh_index, traverse_nodes[i], child_nodes.data()));
                    swap_nodes.insert(swap_nodes.end(), child_nodes.begin(), child_nodes.end());
                }

                // Get the triangle nodes. If this is not a triangle the triangle count is 0.
                RRA_BUBBLE_ON_ERROR(RraBlasGetNodeTriangleCount(bvh_index, traverse_nodes[i], &triangle_count));
                std::vector<TriangleVertices> triangles(triangle_count);
                RRA_BUBBLE_ON_ERROR(RraBlasGetNodeTriangles(bvh_index, traverse_nodes[i], triangles.data()));

                // Go over each triangle and test for intersection.
                for (size_t k = 0; k < triangles.size(); k++)
                {
                    TriangleVertices triangle_vertices = triangles[k];
                    glm::vec3        hit_output;

                    glm::vec3 a = {triangle_vertices.a.x, triangle_vertices.a.y, triangle_vertices.a.z};
                    glm::vec3 b = {triangle_vertices.b.x, triangle_vertices.b.y, triangle_vertices.b.z};
                    glm::vec3 c = {triangle_vertices.c.x, triangle_vertices.c.y, triangle_vertices.c.z};

                    if (glm::intersectLineTriangle(origin, direction, a, b, c, hit_output))
                    {
                        float distance = hit_output.x;  // GLM does not document this...
                        if (distance > 0.0 && (scene_model_closest_hit.distance < 0.0f || distance < scene_model_closest_hit.distance))
                        {
                            scene_model_closest_hit.distance       = distance;
                            scene_model_closest_hit.blas_index     = bvh_index;
                            scene_model_closest_hit.instance_node  = instance_node;
                            scene_model_closest_hit.triangle_node  = UINT32_MAX;
                            scene_model_closest_hit.triangle_index = UINT32_MAX;
                        }
                    }
                }
            }

            // Swap the old list with the new.
            traverse_nodes = swap_nodes;
        }
        return kRraOk;
    }

    RraErrorCode TlasSceneCollectionModel::CastClosestHitRayOnBvh(uint64_t                        bvh_index,
                                                                  const glm::vec3&                origin,
                                                                  const glm::vec3&                direction,
                                                                  SceneCollectionModelClosestHit& scene_model_closest_hit) const
    {
        scene_model_closest_hit.distance = -1.0f;
        std::vector<uint32_t> hit_instances;

        auto scene = GetSceneByIndex(bvh_index);
        if (scene)
        {
            auto scene_nodes = scene->CastRay(origin, direction);
            for (auto node : scene_nodes)
            {
                for (const auto& instance : node->GetInstances())
                {
                    hit_instances.push_back(instance.instance_node);
                }
            }
        }

        for (size_t i = 0; i < hit_instances.size(); i++)
        {
            // Gather the transform data for this instance.
            glm::mat4 transform;
            RRA_BUBBLE_ON_ERROR(RraTlasGetInstanceNodeTransform(bvh_index, hit_instances[i], reinterpret_cast<float*>(&transform)));

            // Adjust the transform from 3x4 to 4x4.
            transform[3][3] = 1.0f;

            // Get the blas index for this transform;
            uint64_t blas_index;
            RraTlasGetBlasIndexFromInstanceNode(bvh_index, hit_instances[i], &blas_index);

            // Transform the ray into the blas space.
            glm::vec3 transformed_origin    = glm::transpose(transform) * glm::vec4(origin, 1.0f);
            glm::vec3 transformed_direction = glm::mat3(glm::transpose(transform)) * direction;

            // Trace
            RRA_BUBBLE_ON_ERROR(CastClosestHitRayOnBlas(blas_index, hit_instances[i], transformed_origin, transformed_direction, scene_model_closest_hit));
        }

        return kRraOk;
    }

    void TlasSceneCollectionModel::ResetModelValues()
    {
        for (auto scene_iter = tlas_scenes_.begin(); scene_iter != tlas_scenes_.end(); ++scene_iter)
        {
            delete scene_iter->second;
        }
        tlas_scenes_.clear();
    }

}  // namespace rra
