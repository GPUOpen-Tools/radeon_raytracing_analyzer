//=============================================================================
// Copyright (c) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the BLAS scene model.
//=============================================================================

#include "models/blas/blas_scene_collection_model.h"

#include "qt_common/utils/qt_util.h"

#include "public/rra_assert.h"
#include "public/rra_error.h"

#include "public/rra_blas.h"
#include "public/renderer_interface.h"

#include "glm/glm/gtx/intersect.hpp"

namespace rra
{
    BlasSceneCollectionModel::~BlasSceneCollectionModel()
    {
        // Delete each BLAS scene.
        for (auto scene_iter = blas_scenes_.begin(); scene_iter != blas_scenes_.end(); ++scene_iter)
        {
            delete scene_iter->second;
        }
    }

    void BlasSceneCollectionModel::PopulateScene(renderer::RendererInterface* renderer, uint32_t bvh_index)
    {
        // Step through all BLAS instances in the file and populate with BLAS instance info.
        uint64_t blas_count = 0;
        if (RraBvhGetTotalBlasCount(&blas_count) == kRraOk)
        {
            uint64_t blas_address = 0;
            if (RraBlasGetBaseAddress(bvh_index, &blas_address) == kRraOk)
            {
                // Create a new BLAS scene instance.
                Scene* blas_scene = CreateRenderSceneForBLAS(renderer, bvh_index);

                // Add the scene to the map using the TLAS's address.
                blas_scenes_[blas_address] = blas_scene;
            }
        }
    }

    Scene* BlasSceneCollectionModel::CreateRenderSceneForBLAS(renderer::RendererInterface* renderer, uint32_t blas_index)
    {
        RRA_UNUSED(renderer);

        // Create a scene.
        Scene* blas_scene = new Scene();

        // Construct a tree by using the blas_index.
        auto blas_node = SceneNode::ConstructFromBlas(blas_index);

        // Initialize the scene with the given node.
        blas_scene->Initialize(blas_node);

        return blas_scene;
    }

    Scene* BlasSceneCollectionModel::GetSceneByIndex(uint64_t bvh_index) const
    {
        Scene* result = nullptr;

        uint64_t blas_address = 0;
        if (RraBlasGetBaseAddress(bvh_index, &blas_address) == kRraOk)
        {
            auto blas_iter = blas_scenes_.find(blas_address);
            if (blas_iter != blas_scenes_.end())
            {
                result = blas_iter->second;
            }
        }

        return result;
    }

    bool BlasSceneCollectionModel::GetSceneBounds(uint64_t bvh_index, BoundingVolumeExtents& volume) const
    {
        auto scene = GetSceneByIndex(bvh_index);
        if (scene)
        {
            return scene->GetSceneBoundingVolume(volume);
        }
        return false;
    }

    bool BlasSceneCollectionModel::GetSceneSelectionBounds(uint64_t bvh_index, BoundingVolumeExtents& volume) const
    {
        auto scene = GetSceneByIndex(bvh_index);
        if (scene)
        {
            return scene->GetBoundingVolumeForSelection(volume);
        }
        return false;
    }

    RraErrorCode BlasSceneCollectionModel::CastClosestHitRayOnBvh(uint64_t                        bvh_index,
                                                                  const glm::vec3&                origin,
                                                                  const glm::vec3&                direction,
                                                                  SceneCollectionModelClosestHit& scene_model_closest_hit) const
    {
        scene_model_closest_hit.distance = -1.0f;

        auto scene = GetSceneByIndex(bvh_index);
        if (scene)
        {
            auto scene_nodes = scene->CastRay(origin, direction);
            for (auto node : scene_nodes)
            {
                auto triangles = node->GetTriangles();
                for (size_t i = 0; i < triangles.size(); i++)
                {
                    const auto& triangle = triangles[i];
                    glm::vec3   hit_output;

                    if (glm::intersectLineTriangle(
                            origin, direction, glm::vec3(triangle.a.position), glm::vec3(triangle.b.position), glm::vec3(triangle.c.position), hit_output))
                    {
                        float distance = hit_output.x;  // GLM does not document this...
                        if (distance > 0.0 && (scene_model_closest_hit.distance < 0.0f || distance < scene_model_closest_hit.distance))
                        {
                            scene_model_closest_hit.distance       = distance;
                            scene_model_closest_hit.blas_index     = bvh_index;
                            scene_model_closest_hit.instance_node  = 0;
                            scene_model_closest_hit.triangle_node  = node->GetId();
                            scene_model_closest_hit.triangle_index = static_cast<uint32_t>(i);
                        }
                    }
                }
            }
        }

        return kRraOk;
    }

    void BlasSceneCollectionModel::ResetModelValues()
    {
        for (auto scene_iter = blas_scenes_.begin(); scene_iter != blas_scenes_.end(); ++scene_iter)
        {
            delete scene_iter->second;
        }
        blas_scenes_.clear();
    }
}  // namespace rra
