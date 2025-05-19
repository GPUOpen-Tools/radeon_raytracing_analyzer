//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the TLAS scene model.
//=============================================================================

#include "models/ray/ray_inspector_scene_collection_model.h"

#include "qt_common/utils/qt_util.h"

#include "glm/glm/gtx/intersect.hpp"

#include "public/intersect.h"
#include "public/renderer_interface.h"
#include "public/rra_assert.h"
#include "public/rra_blas.h"
#include "public/rra_error.h"
#include "public/rra_tlas.h"

namespace rra
{
    RayInspectorSceneCollectionModel::~RayInspectorSceneCollectionModel()
    {
        for (auto scene_iter = tlas_scenes_.begin(); scene_iter != tlas_scenes_.end(); ++scene_iter)
        {
            delete scene_iter->second;
        }
    }

    void RayInspectorSceneCollectionModel::PopulateScene(renderer::RendererInterface* renderer, uint32_t bvh_index)
    {
        if (tlas_scenes_[bvh_index] != nullptr)
        {
            return;
        }
        tlas_scenes_[bvh_index] = CreateRenderSceneForTLAS(renderer, bvh_index);
    }

    Scene* RayInspectorSceneCollectionModel::GetSceneByIndex(uint64_t bvh_index) const
    {
        Scene* result = nullptr;

        auto tlas_iter = tlas_scenes_.find(bvh_index);
        if (tlas_iter != tlas_scenes_.end())
        {
            result = tlas_iter->second;
        }

        return result;
    }

    bool RayInspectorSceneCollectionModel::GetSceneBounds(uint64_t bvh_index, BoundingVolumeExtents& volume) const
    {
        auto scene = GetSceneByIndex(bvh_index);
        if (scene)
        {
            return scene->GetSceneBoundingVolume(volume);
        }
        return false;
    }

    bool RayInspectorSceneCollectionModel::GetSceneSelectionBounds(uint64_t bvh_index, BoundingVolumeExtents& volume) const
    {
        auto scene = GetSceneByIndex(bvh_index);
        if (scene)
        {
            return scene->GetBoundingVolumeForSelection(volume);
        }
        return false;
    }

    Scene* RayInspectorSceneCollectionModel::CreateRenderSceneForTLAS(renderer::RendererInterface* renderer, uint64_t tlas_index)
    {
        Q_UNUSED(renderer);

        // Create a scene.
        Scene* tlas_scene = new Scene{};

        // Construct a tree by using the tlas_index.
        auto tlas_root_node = SceneNode::ConstructFromTlas(tlas_index);

        // Initialize the scene with the given node.
        tlas_scene->Initialize(tlas_root_node, tlas_index, true);

        return tlas_scene;
    }

    bool RayInspectorSceneCollectionModel::ShouldSkipBLASNodeInTraversal(uint64_t blas_index, uint32_t node_id) const
    {
        RRA_UNUSED(blas_index);
        RRA_UNUSED(node_id);
        return false;
    }

    RraErrorCode RayInspectorSceneCollectionModel::CastClosestHitRayOnBvh(uint64_t                        bvh_index,
                                                                          const glm::vec3&                origin,
                                                                          const glm::vec3&                direction,
                                                                          SceneCollectionModelClosestHit& scene_model_closest_hit) const
    {
        scene_model_closest_hit.distance = -1.0f;
        std::vector<uint32_t> hit_instances;

        auto scene = GetSceneByIndex(bvh_index);
        if (scene)
        {
            auto scene_nodes = scene->CastRayCollectNodes(origin, direction);
            for (auto node : scene_nodes)
            {
                renderer::Instance* instance = node->GetInstance();
                if (instance)
                {
                    hit_instances.push_back(instance->instance_node);
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
            RraErrorCode error_code = RraTlasGetBlasIndexFromInstanceNode(bvh_index, hit_instances[i], &blas_index);
            RRA_ASSERT(error_code == kRraOk);

            // Transform the ray into the blas space.
            glm::vec3 transformed_origin    = glm::transpose(transform) * glm::vec4(origin, 1.0f);
            glm::vec3 transformed_direction = glm::mat3(glm::transpose(transform)) * direction;

            // Trace
            CastClosestHitRayOnBlas(blas_index, hit_instances[i], transformed_origin, transformed_direction, scene_model_closest_hit);
            scene_model_closest_hit.triangle_node = UINT32_MAX;
        }

        return kRraOk;
    }

    void RayInspectorSceneCollectionModel::ResetModelValues()
    {
        for (auto scene_iter = tlas_scenes_.begin(); scene_iter != tlas_scenes_.end(); ++scene_iter)
        {
            delete scene_iter->second;
        }
        tlas_scenes_.clear();
    }

    bool RayInspectorSceneCollectionModel::GetFusedInstancesEnabled(uint64_t bvh_index) const
    {
        bool is_enabled = false;
        RraErrorCode error_code = RraTlasGetFusedInstancesEnabled(bvh_index, &is_enabled);
        RRA_ASSERT(error_code == kRraOk);
        return is_enabled;
    }

}  // namespace rra

