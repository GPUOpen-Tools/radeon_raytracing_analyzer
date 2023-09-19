//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Ray history image generator.
//=============================================================================

#include <QImage>
#include "public/renderer_types.h"
#include "public/renderer_interface.h"
#include "models/ray/ray_history_model.h"

namespace rra
{
    namespace renderer
    {
        struct DispatchIdData;
    }

    /// @brief Create a buffer of ray history statistics needed for rendering a dispatch.
    ///
    /// @param dispatch_id   The dispatch ID.
    /// @param out_max_count The maximum of each dispatch statistic.
    void GraphicsContextCreateRayHistoryStatsBuffer(uint32_t dispatch_id, renderer::DispatchIdData* out_max_count);

    /// @brief Render the ray history image from the statistics buffer generated prior.
    ///
    /// @param heatmap_min The minimum heatmap value.
    /// @param heatmap_max The maximum heatmap value.
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
}  // namespace rra