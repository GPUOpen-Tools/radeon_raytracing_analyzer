//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of the BLAS scene model.
//=============================================================================

#ifndef RRA_MODELS_BLAS_BLAS_SCENE_MODEL_H_
#define RRA_MODELS_BLAS_BLAS_SCENE_MODEL_H_

#include <map>

#include "models/scene.h"
#include "models/scene_collection_model.h"

namespace rra
{
    /// @brief Model for all BLAS scenes in the trace file.
    class BlasSceneCollectionModel : public SceneCollectionModel
    {
    public:
        /// @brief Constructor.
        BlasSceneCollectionModel() = default;

        /// @brief Destructor.
        virtual ~BlasSceneCollectionModel();

        /// @brief Populate the scene with BLAS mesh geometry.
        ///
        /// @param [in] renderer The renderer used to render the scene.
        /// @param [in] bvh_index The index of the BVH to load.
        virtual void PopulateScene(renderer::RendererInterface* renderer, uint32_t bvh_index) override;

        /// @brief Retrieve a scene instance associated with the given BVH index.
        ///
        /// @param [in] bvh_index The index of the BVH to load.
        ///
        /// @returns The scene instance associated with the given BLAS.
        virtual Scene* GetSceneByIndex(uint64_t bvh_index) const override;

        /// @brief Retrieve the scene bounds for the BLAS at the provided index.
        ///
        /// @param [in] bvh_index The index of the BVH used to query the scene bounds.
        /// @param [out] volume The volume of the scene.
        ///
        /// @returns True if the volume is retrieved properly.
        virtual bool GetSceneBounds(uint64_t bvh_index, BoundingVolumeExtents& volume) const override;

        /// @brief Retreive the current selection bounds from the scene with the given index.
        ///
        /// @param [in] bvh_index The index of the BVH used to query the scene bounds.
        /// @param [out] volume The volume of the selection.
        ///
        /// @returns True if the volume is retrieved properly.
        virtual bool GetSceneSelectionBounds(uint64_t bvh_index, BoundingVolumeExtents& volume) const override;

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
                                                    SceneCollectionModelClosestHit& scene_model_closest_hit) const override;

        /// @brief Reset any values in the model to their default state.
        virtual void ResetModelValues() override;

        /// @brief Check if fused instances are enabled.
        ///
        /// @param [in] bvh_index The index to check for fused instances.
        ///
        /// @returns True if fused instances are enabled.
        virtual bool GetFusedInstancesEnabled(uint64_t bvh_index) const override;

    private:
        /// @brief Create a new scene populated from a single BLAS.
        ///
        /// @param [in] renderer The renderer interface used to draw the scene.
        /// @param [in] blas_index The index of the BLAS used to populate the scene.
        ///
        /// @returns The new Scene instance.
        Scene* CreateRenderSceneForBLAS(renderer::RendererInterface* renderer, uint32_t blas_index);

    protected:
        /// @brief During ray cast traversal, get whether this node should be skipped.
        ///
        /// @param blas_index The index of the BLAS containing the node.
        /// @param node_id The node to query.
        ///
        /// @return true if node should be skipped, false otherwise.
        virtual bool ShouldSkipBLASNodeInTraversal(uint64_t blas_index, uint32_t node_id) const override;

        std::map<uint64_t, Scene*> blas_scenes_;  ///< A map of all loaded BLAS scenes.
    };
}  // namespace rra

#endif  // RRA_MODELS_BLAS_BLAS_SCENE_MODEL_H_

