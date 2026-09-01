//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the TLAS scene model.
//=============================================================================

#include "models/tlas/tlas_scene_collection_model.h"

#include "qt_common/utils/qt_util.h"

#include "glm/glm/gtx/intersect.hpp"

#include "public/intersect.h"
#include "public/renderer_interface.h"
#include "public/rra_assert.h"
#include "public/rra_blas.h"
#include "public/rra_error.h"
#include "public/rra_rtip_info.h"
#include "public/rra_tlas.h"

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

        // Construct a tree by using the tlas_index.
        auto tlas_root_node = SceneNode::ConstructFromTlas(tlas_index);

        // Initialize the scene with the given node.
        tlas_scene->Initialize(tlas_root_node, tlas_index, true);

        return tlas_scene;
    }

    bool TlasSceneCollectionModel::ShouldSkipBLASNodeInTraversal(uint64_t blas_index, uint64_t node_child_id) const
    {
        RRA_UNUSED(blas_index);
        RRA_UNUSED(node_child_id);
        return false;
    }

    RraErrorCode TlasSceneCollectionModel::CastClosestHitRayOnBvh(uint64_t                        bvh_index,
                                                                  const glm::vec3&                origin,
                                                                  const glm::vec3&                direction,
                                                                  std::vector<rra::SceneNode*>*   blas_root_nodes,
                                                                  SceneCollectionModelClosestHit& scene_model_closest_hit) const
    {
        scene_model_closest_hit.distance = -1.0f;
        std::vector<uint64_t>            hit_instances;
        std::vector<renderer::Instance*> hit_instance_data;
        std::vector<rra::SceneNode*>     hit_instance_nodes;

        auto scene = GetSceneByIndex(bvh_index);
        if (scene)
        {
            auto scene_nodes = scene->CastRayCollectNodes(origin, direction);
            for (auto node : scene_nodes)
            {
                // Under RTIP3.1 node packing, a packed-ref slot carries its own per-slot bounding box but no
                // geometry/instance of its own; the shared subtree lives on the primary sibling. A ray can hit a
                // packed-ref box without hitting the primary's (distinct) box, so redirect the hit to the primary.
                // This makes a 3D click on a packed-ref select the same instance the "referenced subtree" tree row
                // resolves to, instead of falling through to the object behind it.
                if (SceneNode* primary = node->GetPackedPrimary())
                {
                    node = primary;
                }

                renderer::Instance* instance = node->GetInstance();
                if (instance)
                {
                    const rta::RayTracingIpLevel rtip = (rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel();
                    bool                         use_composite_key = (rtip == rta::RayTracingIpLevel::RtIp3_1 && RraTlasHasNodePacking(bvh_index));
                    if (use_composite_key)
                    {
                        hit_instances.push_back(((uint64_t)node->GetGlobalChildIndex() << 32) | instance->instance_node);
                    }
                    else
                    {
                        hit_instances.push_back(instance->instance_node);
                    }
                    hit_instance_data.push_back(instance);
                    hit_instance_nodes.push_back(node);
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
                uint64_t     blas_index;
                RraErrorCode error_code = RraTlasGetBlasIndexFromInstanceNode(bvh_index, hit_instances[i], &blas_index);
                RRA_ASSERT(error_code == kRraOk);

                // Check per-BLAS geometry filter before descending into the BLAS.
                if (scene->ShouldFilterBlasInstance(geometry_filter_state_, hit_instance_data[i], blas_index))
                {
                    continue;
                }

                // A Cluster BLAS (CBLAS) has no triangles of its own; its geometry is the CLASes it references,
                // pre-flattened into world->CLAS-object sub-instances. Trace each CLAS so a CBLAS instance is
                // pickable. The hit still reports the TLAS instance node, so selection lands on the TLAS instance.
                const std::vector<renderer::Instance>& sub_instances = hit_instance_nodes[i]->GetClusterSubInstances();
                if (!sub_instances.empty())
                {
                    // Remember the winning distance before tracing this CBLAS's CLASes. If one of them wins,
                    // CastClosestHitRayOnBlas will have overwritten blas_index with the inner CLAS index; but a
                    // pick on a CBLAS instance should drill to the CBLAS, so restore the CBLAS index afterward.
                    const float distance_before = scene_model_closest_hit.distance;
                    for (const renderer::Instance& sub_instance : sub_instances)
                    {
                        const glm::mat4 world_to_object = glm::transpose(glm::inverse(sub_instance.transform));
                        glm::vec3       clas_origin     = world_to_object * glm::vec4(origin, 1.0f);
                        glm::vec3       clas_direction  = glm::mat3(world_to_object) * direction;
                        CastClosestHitRayOnBlas(
                            (*blas_root_nodes)[sub_instance.blas_index], hit_instances[i], clas_origin, clas_direction, scene_model_closest_hit);
                    }
                    if (scene_model_closest_hit.distance != distance_before)
                    {
                        scene_model_closest_hit.blas_index = blas_index;
                    }
                    continue;
                }

                // Transform the ray into the blas space.
                glm::vec3 transformed_origin    = glm::transpose(transform) * glm::vec4(origin, 1.0f);
                glm::vec3 transformed_direction = glm::mat3(glm::transpose(transform)) * direction;

                // Trace
                CastClosestHitRayOnBlas((*blas_root_nodes)[blas_index], hit_instances[i], transformed_origin, transformed_direction, scene_model_closest_hit);
            }
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

    bool TlasSceneCollectionModel::GetFusedInstancesEnabled(uint64_t bvh_index) const
    {
        bool         is_enabled = false;
        RraErrorCode error_code = RraTlasGetFusedInstancesEnabled(bvh_index, &is_enabled);
        RRA_ASSERT(error_code == kRraOk);
        return is_enabled;
    }

}  // namespace rra


