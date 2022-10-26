//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the Render State Adapter interface. This type can
///         be used by frontend to query and alter renderer display state.
//=============================================================================

#ifndef RRA_RENDERER_RENDER_STATE_ADAPTER_H_
#define RRA_RENDERER_RENDER_STATE_ADAPTER_H_

#include "public/renderer_adapter.h"
#include "public/renderer_interface.h"

#include <functional>

namespace rra
{
    namespace renderer
    {
        static const char* kBVHColoringModeName_VolumeType        = "Color BVH by volume type";
        static const char* kBVHColoringModeDescription_VolumeType = "Constant coloring for each volume type.";
        static const char* kBVHColoringModeName_TreeDepth         = "Color BVH by tree depth";
        static const char* kBVHColoringModeDescription_TreeDepth  = "Range of colors for each depth level in the tree.";

        /// Decleration for BVH coloring modes.
        static const std::vector<BVHColoringModeInfo> kAvailableBVHColoringModes = {
            {BVHColoringMode::VolumeType, kBVHColoringModeName_VolumeType, kBVHColoringModeDescription_VolumeType},
            {BVHColoringMode::TreeDepth, kBVHColoringModeName_TreeDepth, kBVHColoringModeDescription_TreeDepth},
        };

        static const char* kTraversalCounterModeName_TraversalLoopCount        = "Color traversal counters by loop count";
        static const char* kTraversalCounterModeDescription_TraversalLoopCount = "The number of iterations during traversal to get the closest hit.";
        static const char* kTraversalCounterModeName_InstanceHit               = "Color traversal counters by instance hit";
        static const char* kTraversalCounterModeDescription_InstanceHit        = "The count of intersections on instances.";
        static const char* kTraversalCounterModeName_BoxVolumeHit              = "Color traversal counters by box volume hit";
        static const char* kTraversalCounterModeDescription_BoxVolumeHit = "The count of intersections on box volumes. Box volumes contain smaller volumes.";
        static const char* kTraversalCounterModeName_BoxVolumeMiss       = "Color traversal counters by box volume miss";
        static const char* kTraversalCounterModeDescription_BoxVolumeMiss =
            "The count of intersections misses on box volumes. Box volumes contain smaller volumes.";
        static const char* kTraversalCounterModeName_BoxVolumeTest = "Color traversal counters by box volume test";
        static const char* kTraversalCounterModeDescription_BoxVolumeTest =
            "The count of intersections tests performed on box volumes. Box volumes contain smaller volumes.";
        static const char* kTraversalCounterModeName_TriangleHit         = "Color traversal counters by triangle hit";
        static const char* kTraversalCounterModeDescription_TriangleHit  = "The count of intersections on triangles.";
        static const char* kTraversalCounterModeName_TriangleMiss        = "Color traversal counters by triangle miss";
        static const char* kTraversalCounterModeDescription_TriangleMiss = "The count of intersection misses on triangles.";
        static const char* kTraversalCounterModeName_TriangleTest        = "Color traversal counters by triangle test";
        static const char* kTraversalCounterModeDescription_TriangleTest = "The count of intersection tests performed on triangles.";

        /// Decleration for BVH coloring modes.
        static const std::vector<TraversalCounterModeInfo> kAvailableTraversalCounterModes = {
            {TraversalCounterMode::TraversalLoopCount, kTraversalCounterModeName_TraversalLoopCount, kTraversalCounterModeDescription_TraversalLoopCount},
            {TraversalCounterMode::InstanceHit, kTraversalCounterModeName_InstanceHit, kTraversalCounterModeDescription_InstanceHit},
            {TraversalCounterMode::BoxVolumeHit, kTraversalCounterModeName_BoxVolumeHit, kTraversalCounterModeDescription_BoxVolumeHit},
            {TraversalCounterMode::BoxVolumeMiss, kTraversalCounterModeName_BoxVolumeMiss, kTraversalCounterModeDescription_BoxVolumeMiss},
            {TraversalCounterMode::BoxVolumeTest, kTraversalCounterModeName_BoxVolumeTest, kTraversalCounterModeDescription_BoxVolumeTest},
            {TraversalCounterMode::TriangleHit, kTraversalCounterModeName_TriangleHit, kTraversalCounterModeDescription_TriangleHit},
            {TraversalCounterMode::TriangleMiss, kTraversalCounterModeName_TriangleMiss, kTraversalCounterModeDescription_TriangleMiss},
            {TraversalCounterMode::TriangleTest, kTraversalCounterModeName_TriangleTest, kTraversalCounterModeDescription_TriangleTest},
        };

        class RendererVulkan;
        class MeshRenderModule;
        class BoundingVolumeRenderModule;
        class TraversalRenderModule;
        class SelectionRenderModule;

        /// @brief Declaration of the RenderStateAdapter class.
        class RenderStateAdapter : public RendererAdapter
        {
        public:
            /// @brief Constructor.
            ///
            /// @param [in] renderer The renderer instanced used to draw frames.
            /// @param [in] blas_mesh_module The BLAS mesh render module instance.
            /// @param [in] bounding_volume_module The bounding volume render module instance.
            RenderStateAdapter(RendererInterface*          renderer,
                               MeshRenderModule*           blas_mesh_module,
                               BoundingVolumeRenderModule* bounding_volume_module,
                               TraversalRenderModule*      traversal_render_module,
                               SelectionRenderModule*      selection_render_module);

            /// @brief Destructor.
            virtual ~RenderStateAdapter() = default;

            /// @brief Set whether or not to render BLAS geometry.
            ///
            /// @param [in] render_geometry A flag indicating if the BLAS geometry should be rendered or not.
            void SetRenderGeometry(bool render_geometry);

            /// @brief Get whether or not the BLAS geometry will be rendered.
            ///
            /// @returns True if the BLAS geometry should be rendered, or false if not.
            bool GetRenderGeometry() const;

            /// @brief Set the coloring mode index used to color BLAS meshes.
            ///
            /// @param [in] color_mode The coloring mode.
            void SetGeometryColoringMode(GeometryColoringMode coloring_mode);

            /// @brief Set the coloring mode index used to color BVHs.
            ///
            /// @param [in] color_mode The coloring mode index.
            void SetBVHColoringMode(int32_t color_mode);

            /// @brief Set the coloring mode index used to count traversal
            ///
            /// @param [in] counter_mode The traversal counter mode index.
            void SetTraversalCounterMode(int32_t counter_mode);

            /// @brief Adapt the traversal counter range to the view.
            ///
            /// @param [in] update_function The callback to use when the range has been acquired.
            void AdaptTraversalCounterRangeToView(std::function<void(uint32_t min, uint32_t max)> update_function);

            /// @brief Set the traversal counter continuous update function.
            ///
            /// @param [in] update_function The callback to use when the range has been acquired.
            void SetTraversalCounterContinuousUpdateFunction(std::function<void(uint32_t min, uint32_t max)> update_function);

            /// @brief Checks if the traversal counter continuous update function is set.
            ///
            /// @returns True if the update function is set.
            bool IsTraversalCounterContinuousUpdateFunctionSet();

            /// @brief Get the coloring mode index used to color BLAS meshes.
            ///
            /// @returns The coloring mode index.
            int32_t GetGeometryColoringMode() const;

            /// @brief Retrieve the list of available coloring modes for the given BVH type flags.
            ///
            /// @param [in] type Flags indicating the BVH types to retrieve valid coloring modes for.
            /// @param [out] coloring_modes A vector of coloring mode info to populate.
            void GetAvailableGeometryColoringModes(BvhTypeFlags type, std::vector<GeometryColoringModeInfo>& coloring_modes) const;

            /// @brief Set the culling mode used for BLAS geometry.
            ///
            /// @param [in] culling_mode The culling mode.
            void SetCullingMode(int culling_mode);

            /// @brief Get the culling mode used for rendering BLAS geometry.
            ///
            /// @returns The coloring mode type.
            int GetCullingMode() const;

            /// @brief Set whether or not to render a wireframe over the BLAS mesh.
            ///
            /// @param [in] render_wireframe A flag indicating whether or not the wireframe overlay is enabled.
            void SetRenderWireframe(bool render_wireframe);

            /// @brief Get whether or not the draw wireframe overlay is enabled.
            ///
            /// @returns True if the wireframe overlay is enabled, or false if not.
            bool GetRenderWireframe() const;

            /// @brief Set the traversal counter range.
            ///
            /// @param [in] min_value The minimum counter value.
            /// @param [in] max_value The maximum counter value.
            void SetTraversalCounterRange(uint32_t min_value, uint32_t max_value);

            /// @brief Get the traversal counter min.
            ///
            /// @returns The traversal counter min.
            uint32_t GetTraversalCounterMin() const;

            /// @brief Get the traversal counter max.
            ///
            /// @returns The traversal counter max
            uint32_t GetTraversalCounterMax() const;

            /// @brief Set whether or not to render the traversal.
            ///
            /// @param [in] render_traversal A flag indicating if the traversal should be rendered.
            void SetRenderTraversal(bool render_traversal);

            /// @brief Get whether or not the traversal will be rendered.
            ///
            /// @returns True if the traversal should be rendered, or false if not.
            bool GetRenderTraversal() const;

            /// @brief Set whether or not to render bounding volume geometry.
            ///
            /// @param [in] render_bounding_volumes A flag indicating if the BVH geometry should be rendered or not.
            void SetRenderBoundingVolumes(bool render_bounding_volumes);

            /// @brief Get whether or not the BVH volume geometry will be rendered.
            ///
            /// @returns True if the BVH geometry should be rendered, or false if not.
            bool GetRenderBoundingVolumes() const;

            /// @brief Set whether or not to render instance pretransform.
            ///
            /// @param [in] render_bounding_volumes A flag indicating if the instance pretransform should be rendered or not.
            void SetRenderInstancePretransform(bool render_instance_pretransform);

            /// @brief Set the heatmap data.
            ///
            /// @param [in] heatmap_data The raw heatmap data.
            void SetHeatmapData(HeatmapData heatmap_data);

            /// @brief  Add a heatmap update callback.
            ///
            /// @param [in] heatmap_update_callback The heatmap update callback to add.
            void AddHeatmapUpdateCallback(std::function<void(rra::renderer::HeatmapData)> heatmap_update_callback);

        private:
            RendererVulkan*                                              vulkan_renderer_         = nullptr;  ///< The renderer to alter the render state for.
            MeshRenderModule*                                            mesh_render_module_      = nullptr;  ///< The mesh render module instance.
            BoundingVolumeRenderModule*                                  bounding_volume_module_  = nullptr;  ///< The bounding volume module instance.
            TraversalRenderModule*                                       traversal_render_module_ = nullptr;  ///< The traversal render volume instance.
            SelectionRenderModule*                                       selection_render_module_ = nullptr;  ///< The selection render volume instance.
            std::vector<std::function<void(rra::renderer::HeatmapData)>> heatmap_update_callbacks_;           ///< The heatmap update callbacks.
            uint32_t traversal_counter_min_ = 0;  ///< The min traversal value to compare against to check if the renderer should re-render.
            uint32_t traversal_counter_max_ = 0;  ///< The max traversal value to compare against to check if the renderer should re-render.
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_RENDER_STATE_ADAPTER_H_
