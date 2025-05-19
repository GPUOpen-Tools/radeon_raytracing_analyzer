//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the Render State Adapter interface. This type can
///         be used by frontend to query and alter renderer display state.
//=============================================================================

#ifndef RRA_RENDERER_RENDER_STATE_ADAPTER_H_
#define RRA_RENDERER_RENDER_STATE_ADAPTER_H_

#include <functional>

#include "public/renderer_adapter.h"
#include "public/renderer_interface.h"

namespace rra
{
    namespace renderer
    {
        static const char* kBVHColoringModeName_VolumeType        = "Color BVH by volume type";
        static const char* kBVHColoringModeDescription_VolumeType = "Constant coloring for each volume type.";
        static const char* kBVHColoringModeName_TreeDepth         = "Color BVH by tree depth";
        static const char* kBVHColoringModeDescription_TreeDepth  = "Range of colors for each depth level in the tree.";

        /// Declaration for BVH coloring modes.
        static const std::vector<BVHColoringModeInfo> kAvailableBVHColoringModes{
            {BVHColoringMode::VolumeType, kBVHColoringModeName_VolumeType, kBVHColoringModeDescription_VolumeType},
            {BVHColoringMode::TreeDepth, kBVHColoringModeName_TreeDepth, kBVHColoringModeDescription_TreeDepth},
        };

        class RendererVulkan;
        class MeshRenderModule;
        class BoundingVolumeRenderModule;
        class TraversalRenderModule;
        class SelectionRenderModule;
        class RayInspectorOverlayRenderModule;

        /// @brief Declaration of the RenderStateAdapter class.
        class RenderStateAdapter : public RendererAdapter
        {
        public:
            /// @brief Constructor.
            ///
            /// @param [in] renderer The renderer instanced used to draw frames.
            /// @param [in] blas_mesh_module The BLAS mesh render module instance.
            /// @param [in] bounding_volume_module The bounding volume render module instance.
            RenderStateAdapter(RendererInterface*               renderer,
                               MeshRenderModule*                blas_mesh_module,
                               BoundingVolumeRenderModule*      bounding_volume_module,
                               TraversalRenderModule*           traversal_render_module,
                               SelectionRenderModule*           selection_render_module,
                               RayInspectorOverlayRenderModule* ray_inspector_overlay_module);

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

            /// @brief Sets the histogram data update function to populate histogram from traversal counter.
            ///
            /// @param [in] update_function The callback to use after the traversal shader runs to populate histogram.
            /// @param [in] traversal_max_setting The maximum traversal count set in the settings.
            void SetHistogramUpdateFunction(std::function<void(const std::vector<uint32_t>&, uint32_t, uint32_t)> update_function,
                                            uint32_t                                                              traversal_max_setting);

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

            /// @brief Retrieve the list of available traversal counter modes for the given BVH type flags.
            ///
            /// @param [in] type Flags indicating the BVH types to retrieve valid traversal counter modes for.
            /// @param [out] coloring_modes A vector of traversal counter mode info to populate.
            void GetAvailableTraversalCounterModes(BvhTypeFlags type, std::vector<TraversalCounterModeInfo>& counter_modes) const;

            /// @brief Set the viewport culling mode used in geometry rendering mode.
            ///
            /// @param [in] culling_mode The culling mode.
            void SetViewportCullingMode(int culling_mode);

            /// @brief Get the viewport culling mode used in geometry rendering mode.
            ///
            /// @returns The culling mode type.
            int GetViewportCullingMode() const;

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
            /// @param [in] settings_value The maximum counter value set in the settings.
            void SetTraversalCounterRange(uint32_t min_value, uint32_t max_value, uint32_t settings_max);

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

            /// @brief Get whether or not the instance pretransform is rendered.
            ///
            /// @returns True if the instance pretransform should be rendered, or false if not.
            bool GetRenderInstancePretransform();

            /// @brief Set whether or not to render instance pretransform.
            ///
            /// @param [in] render_bounding_volumes A flag indicating if the instance pretransform should be rendered or not.
            void SetRenderInstancePretransform(bool render_instance_pretransform);

            /// @brief Set the heatmap data.
            ///
            /// @param [in] heatmap_data The raw heatmap data.
            void SetHeatmapData(const HeatmapData& heatmap_data);

            /// @brief  Add a heatmap update callback.
            ///
            /// @param [in] heatmap_update_callback The heatmap update callback to add.
            void AddHeatmapUpdateCallback(std::function<void(rra::renderer::HeatmapData)> heatmap_update_callback);

            /// @brief Set the current Architecture to navi 2.
            void SetArchitectureToNavi2();

            /// @brief Set the current Architecture to navi 3
            void SetArchitectureToNavi3();

            /// @brief Check if the current Architecture is Navi 3.
            bool IsUsingNavi3() const;

            /// @brief Set accept first hit ray flag.
            void SetRayFlagAcceptFirstHit(bool accept_first_hit);

            /// @brief Set cull back facing triangles ray flag.
            void SetRayFlagCullBackFacingTriangles(bool cull_back_facing_tris);

            /// @brief Set cull front facing triangles ray flag.
            void SetRayFlagCullFrontFacingTriangles(bool cull_front_facing_tris);

            /// @brief Get the boxt sort heuristic value.
            ///
            /// @return The box sort heuristic currently in use depending on the view parameters.
            uint32_t GetBoxSortHeuristic();

            /// @brief Update ray parameters.
            void UpdateRayParameters();

        private:
            RendererVulkan*                                              vulkan_renderer_         = nullptr;  ///< The renderer to alter the render state for.
            MeshRenderModule*                                            mesh_render_module_      = nullptr;  ///< The mesh render module instance.
            BoundingVolumeRenderModule*                                  bounding_volume_module_  = nullptr;  ///< The bounding volume module instance.
            TraversalRenderModule*                                       traversal_render_module_ = nullptr;  ///< The traversal render volume instance.
            SelectionRenderModule*                                       selection_render_module_ = nullptr;  ///< The selection render volume instance.
            RayInspectorOverlayRenderModule*                             ray_inspector_overlay_module_ = nullptr;  ///< The ray inspector module to draw rays.
            std::vector<std::function<void(rra::renderer::HeatmapData)>> heatmap_update_callbacks_;                ///< The heatmap update callbacks.
            uint32_t traversal_counter_min_ = 0;      ///< The min traversal value to compare against to check if the renderer should re-render.
            uint32_t traversal_counter_max_ = 0;      ///< The max traversal value to compare against to check if the renderer should re-render.
            bool     using_navi_3_          = false;  ///< The flag to determine which Architecture to render for.
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_RENDER_STATE_ADAPTER_H_

