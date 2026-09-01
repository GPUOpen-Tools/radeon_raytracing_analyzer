//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the BLAS scene model.
//=============================================================================

#include "models/blas/blas_scene_collection_model.h"

#include "glm/glm/gtx/intersect.hpp"

#include "qt_common/utils/qt_util.h"

#include "public/renderer_interface.h"
#include "public/rra_assert.h"
#include "public/rra_blas.h"
#include "public/rra_error.h"

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

        // Allocate buffers needed by BLAS nodes.
        renderer::RraVertex* vertex_buffer{blas_scene->AllocateVertexBuffer(blas_index)};
        std::byte*           child_buffer{blas_scene->AllocateChildBuffer(blas_index)};

        // Construct a tree by using the blas_index.
        auto blas_node = SceneNode::ConstructFromBlas(blas_index, vertex_buffer, child_buffer);

        // Initialize the scene with the given node.
        blas_scene->Initialize(blas_node, blas_index, false);

        return blas_scene;
    }

    bool BlasSceneCollectionModel::ShouldSkipBLASNodeInTraversal(uint64_t blas_index, uint64_t node_child_id) const
    {
        Scene* scene = GetSceneByIndex(blas_index);
        if (scene)
        {
            SceneNode* node = scene->GetNodeById(node_child_id);
            if (node)
            {
                return !(node->IsEnabled() && node->IsVisible());
            }
        }
        return true;
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
                                                                  std::vector<rra::SceneNode*>*   blas_root_nodes,
                                                                  SceneCollectionModelClosestHit& scene_model_closest_hit) const
    {
        auto scene = GetSceneByIndex(bvh_index);
        if (!scene)
        {
            return kRraErrorIndexOutOfRange;
        }

        // A Cluster BLAS (CBLAS) renders its CLAS references as instances (nested instancing), so it has no
        // triangles of its own. Pick like the TLAS pane: find the hit cluster-ref instances, transform the ray
        // into each referenced CLAS, and trace triangles there. The hit's instance_node is the cluster-ref leaf,
        // which is what the tree/drill navigation selects on.
        if (RraBlasIsClusterBlas(bvh_index) && blas_root_nodes)
        {
            scene_model_closest_hit.distance = -1.0f;

            auto scene_nodes = scene->CastRayCollectNodes(origin, direction);
            for (auto node : scene_nodes)
            {
                renderer::Instance* instance = node->GetInstance();
                if (!instance)
                {
                    continue;
                }

                const uint32_t cluster_ref_node = instance->instance_node;

                // The cluster-ref leaf stores a world-to-object transform in the HW instance-node layout.
                glm::mat4 transform;
                if (RraBlasGetClusterRefNodeTransform(bvh_index, cluster_ref_node, reinterpret_cast<float*>(&transform)) != kRraOk)
                {
                    continue;
                }
                transform[3][3] = 1.0f;

                // Transform the ray into the referenced CLAS's space.
                glm::vec3 transformed_origin    = glm::transpose(transform) * glm::vec4(origin, 1.0f);
                glm::vec3 transformed_direction = glm::mat3(glm::transpose(transform)) * direction;

                CastClosestHitRayOnBlas(
                    (*blas_root_nodes)[instance->blas_index], cluster_ref_node, transformed_origin, transformed_direction, scene_model_closest_hit);
            }
            return kRraOk;
        }

        CastClosestHitRayOnBlas(scene->GetRootNode(), UINT32_MAX, origin, direction, scene_model_closest_hit);
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

    bool BlasSceneCollectionModel::GetFusedInstancesEnabled(uint64_t bvh_index) const
    {
        RRA_UNUSED(bvh_index);
        return false;
    }

}  // namespace rra

