//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of the BVH scene model.
//=============================================================================

#ifndef RRA_MODELS_SCENE_MODEL_H_
#define RRA_MODELS_SCENE_MODEL_H_

#include "public/renderer_interface.h"
#include "scene.h"

namespace rra
{
    /// @brief Info on a raycast's closest intersection.
    struct SceneCollectionModelClosestHit
    {
        float    distance       = -1.0f;
        uint64_t blas_index     = ULLONG_MAX;
        uint32_t instance_node  = UINT32_MAX;
        uint32_t triangle_node  = UINT32_MAX;
        uint32_t triangle_index = UINT32_MAX;
    };

    /// @brief The SceneCollectionModel base class declaration.
    ///
    /// The scene collection model is used to query the loaded TLAS & BLAS data sets to generate rendering data.
    /// It contains data and accepts queries about ALL scenes in the trace, in oppose to instances of Scene
    /// which are specific TLAS and BLAS scenes in the trace.
    class SceneCollectionModel
    {
    public:
        /// @brief Constructor.
        SceneCollectionModel() = default;

        /// @brief Destructor.
        virtual ~SceneCollectionModel() = default;

        /// @brief Populate a scene for the given BVH.
        ///
        /// @param [in] renderer The renderer used to render the scene.
        /// @param [in] bvh_index The bvh scene index.
        virtual void PopulateScene(renderer::RendererInterface* renderer, uint32_t bvh_index) = 0;

        /// @brief Retrieve a scene pointer using a BVH index.
        ///
        /// @param [in] bvh_index The BVH scene index.
        ///
        /// @returns The scene associated with the given BVH index.
        virtual Scene* GetSceneByIndex(uint64_t bvh_index) const = 0;

        /// @brief Retrieve the scene bounds for the BVH at the provided index.
        ///
        /// @param [in] bvh_index The index of the BVH used to query the scene bounds.
        /// @param [out] volume The volume of the scene.
        ///
        /// @returns True if the volume is retrieved properly.
        virtual bool GetSceneBounds(uint64_t bvh_index, BoundingVolumeExtents& volume) const = 0;

        /// @brief Retrieve the current selection bounds from the scene with the given index.
        ///
        /// @param [in] bvh_index The index of the BVH used to query the scene bounds.
        /// @param [out] volume The volume of the selection.
        ///
        /// @returns True if the volume is retrieved properly.
        virtual bool GetSceneSelectionBounds(uint64_t bvh_index, BoundingVolumeExtents& volume) const = 0;

        /// @brief Cast a ray into the scene for a closest hit.
        ///
        /// @param [in] bvh_index The index of the BVH used to cast the ray into.
        /// @param [in] origin The origin of the ray.
        /// @param [in] direction The direction of the ray.
        /// @param [out] scene_model_closest_hit The closest hit.
        ///
        /// @returns Will return 'kRraOk' when successful, or an error code in case of failure.
        virtual RraErrorCode CastClosestHitRayOnBvh(uint64_t                        bvh_index,
                                                    const glm::vec3&                origin,
                                                    const glm::vec3&                direction,
                                                    SceneCollectionModelClosestHit& scene_model_closest_hit) const = 0;

        /// @brief Reset any values in the model to their default state.
        virtual void ResetModelValues() = 0;

        /// @brief Check if fused instances are enabled.
        ///
        /// @param [in] bvh_index The index to check for fused instances.
        ///
        /// @returns True if fused instances are enabled.
        virtual bool GetFusedInstancesEnabled(uint64_t bvh_index) const = 0;

    protected:
        /// @brief Cast a ray into a BLAS to find the closest hit.
        ///
        /// @param bvh_index     The index of the BLAS.
        /// @param instance_node The nod
        /// @param origin
        /// @param direction
        /// @param scene_model_closest_hit
        void CastClosestHitRayOnBlas(uint64_t                        bvh_index,
                                     uint32_t                        instance_node,
                                     const glm::vec3&                origin,
                                     const glm::vec3&                direction,
                                     SceneCollectionModelClosestHit& scene_model_closest_hit) const;

        /// @brief During ray cast traversal, get whether this node should be skipped.
        ///
        /// @param blas_index The index of the BLAS containing the node.
        /// @param node_id The node to query.
        ///
        /// @return true if node should be skipped, false otherwise.
        virtual bool ShouldSkipBLASNodeInTraversal(uint64_t blas_index, uint32_t node_id) const = 0;
    };
}  // namespace rra

#endif  // RRA_MODELS_SCENE_MODEL_H_
