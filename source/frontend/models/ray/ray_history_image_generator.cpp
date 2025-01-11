//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Ray history image generator.
//=============================================================================

#include "ray_history_image_generator.h"
#include "vk/vk_graphics_context.h"
#include "public/renderer_types.h"

namespace rra
{
    void GraphicsContextCreateRayHistoryStatsBuffer(uint32_t dispatch_id, renderer::DispatchIdData* out_max_count)
    {
        auto vk_graphics_context = rra::renderer::GetVkGraphicsContext();
        vk_graphics_context->CreateRayHistoryStatsBuffer(dispatch_id, out_max_count);
    }

    QImage GraphicsContextRenderRayHistoryImage(uint32_t                      heatmap_min,
                                                uint32_t                      heatmap_max,
                                                uint32_t                      ray_index,
                                                uint32_t                      reshaped_x,
                                                uint32_t                      reshaped_y,
                                                uint32_t                      reshaped_z,
                                                renderer::RayHistoryColorMode color_mode,
                                                uint32_t                      slice_index,
                                                renderer::SlicePlane          slice_plane)
    {
        auto vk_graphics_context = rra::renderer::GetVkGraphicsContext();
        return vk_graphics_context->RenderRayHistoryImage(
            heatmap_min, heatmap_max, ray_index, reshaped_x, reshaped_y, reshaped_z, color_mode, slice_index, slice_plane);
    }

    void GraphicsContextSetRayHistoryHeatmapData(const renderer::HeatmapData& heatmap_data)
    {
        auto vk_graphics_context = rra::renderer::GetVkGraphicsContext();
        vk_graphics_context->SetRayHistoryHeatmapData(heatmap_data);
    }
}  // namespace rra
