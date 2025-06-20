//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of the Graphics Context functions.
//=============================================================================

#ifndef RRA_RENDERER_GRAPHICS_CONTEXT_H_
#define RRA_RENDERER_GRAPHICS_CONTEXT_H_

#include <string>

#include <QImage>
#include <QWidget>

#include "public/renderer_interface.h"
#include "public/renderer_types.h"

namespace rra
{
    namespace renderer
    {
        /// @brief Create a graphics context.
        ///
        /// @param [in] parent qt widget information.
        /// @param [in] window_info The window information for the creation of graphics device, queues, and context.
        void CreateGraphicsContext(QWidget* parent);

        /// @brief Initialize the graphics context. Note: must be called after CreateGraphicsContext.
        ///
        /// @returns True if the context was initialized successfully and false in case of failure.
        bool InitializeGraphicsContext(std::shared_ptr<GraphicsContextSceneInfo> info);

        /// @brief Get the human readable initialization error.
        ///
        /// @returns A human readable initialization error.
        std::string GetGraphicsContextInitializationError();

        /// @brief Create a buffer of ray history statistics needed for rendering a dispatch.
        ///
        /// @param dispatch_id   The dispatch ID.
        /// @param out_max_count The maximum of each dispatch statistic.
        void GraphicsContextCreateRayHistoryStatsBuffer(uint32_t dispatch_id, renderer::DispatchIdData* out_max_count);

        /// @brief Render the ray history image from the statistics buffer generated prior.
        ///
        /// @param heatmap_min The minimum heatmap value.
        /// @param heatmap_max The maximum heatmap value.
        /// @param ray_index   The index of the current ray.
        /// @param reshaped_x  The dispatch width, after reshaping for 1D dispatches.
        /// @param reshaped_y  The dispatch height, after reshaping for 1D dispatches.
        /// @param reshaped_z  The dispatch depth, after reshaping for 1D dispatches.
        /// @param color_mode  The color mode to render the heatmap with.
        /// @param slice_index The slice of the 3D dispatch to be rendered.
        /// @param slice_plane  The plane of the 3D dispatch to be rendered.
        ///
        /// @return The rendered image.
        QImage GraphicsContextRenderRayHistoryImage(uint32_t                      heatmap_min,
                                                    uint32_t                      heatmap_max,
                                                    uint32_t                      ray_index,
                                                    uint32_t                      reshaped_x,
                                                    uint32_t                      reshaped_y,
                                                    uint32_t                      reshaped_z,
                                                    renderer::RayHistoryColorMode color_mode,
                                                    uint32_t                      slice_index,
                                                    renderer::SlicePlane          slice_plane);

        /// @brief Set the color palette for the heatmap rendering.
        ///
        /// @param heatmap_data The heatmap data.
        void GraphicsContextSetRayHistoryHeatmapData(const renderer::HeatmapData& heatmap_data);

        /// @brief Cleanup the graphics context.
        void CleanupGraphicsContext();

    }  // namespace renderer
}  // namespace rra

#endif

