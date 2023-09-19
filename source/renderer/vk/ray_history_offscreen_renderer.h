//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for ray history offscreen renderer type.
//=============================================================================

#ifndef RRA_RAY_HISTORY_OFFSCREEN_RENDERER_H_
#define RRA_RAY_HISTORY_OFFSCREEN_RENDERER_H_

#include <vector>
#include <QImage>
#include <volk/volk.h>

#include "vma/include/vk_mem_alloc.h"
#include "public/rra_ray_history.h"
#include "public/renderer_types.h"
#include "public/renderer_interface.h"
#include "public/heatmap.h"

namespace rra
{
    namespace renderer
    {
        class Device;

        class RayHistoryOffscreenRenderer
        {
        public:
            /// @brief Initialize the ray history offscreen renderer.
            void Initialize(Device* device);

            /// @brief Clean up resources.
            void CleanUp();

            /// @brief Create the ray tracing statistics buffer to be used for subsequent renders.
            ///
            /// @param [in]  dispatch_id   The dispatch ID.
            /// @param [out] out_max_count Maximum count of each dispatch ID statistic.
            void CreateStatsBuffer(uint32_t dispatch_id, DispatchIdData* out_max_count);

            /// @brief Render a heatmap of the ray history data.
            ///
            /// @param cmd         The command buffer.
            /// @param heatmap_min The minimum heatmap slider value.
            /// @param heatmap_max The maximum heatmap slider value.
            /// @param reshaped_x  The dispatch width, after reshaping for 1D dispatches.
            /// @param reshaped_y  The dispatch height, after reshaping for 1D dispatches.
            /// @param reshaped_z  The dispatch depth, after reshaping for 1D dispatches.
            /// @param color_mode  The color mode to render the heatmap with.
            /// @param slice_index The slice of the 3D dispatch to be rendered.
            /// @param slice_plane  The plane of the 3D dispatch to be rendered.
            ///
            /// @return The rendered image in the form of a QImage, to be loaded by the QPixMap.
            QImage Render(uint32_t            heatmap_min,
                          uint32_t            heatmap_max,
                          uint32_t            reshaped_x,
                          uint32_t            reshaped_y,
                          uint32_t            reshaped_z,
                          RayHistoryColorMode color_mode,
                          uint32_t            slice_index,
                          SlicePlane          slice_plane);

            /// @brief Set the heatmap data.
            ///
            /// @param heatmap_data The raw data of the heatmap.
            void SetHeatmapData(const HeatmapData& heatmap_data);

        private:
            struct PushConstant
            {
                uint32_t color_mode;
                uint32_t reshape_width;
                uint32_t reshape_height;
                uint32_t reshape_depth;
                uint32_t slice_index;
                uint32_t slice_plane;
                uint32_t min_traversal_count_limit;
                uint32_t max_traversal_count_limit;
            };

            /// @brief Create the pipeline layout.
            void CreatePipelineLayout();

            /// @brief Create the descriptor set.
            void CreateDescriptorSet();

            /// @brief Make compute pipeline.
            void CreatePipeline();

            /// @brief Make the command buffer to record render commands to.
            void AllocateCommandBuffer();

            /// @brief Create the fence to be used with render submission.
            void CreateFence();

            struct ImageBuffer
            {
                VkBuffer      buffer;
                VmaAllocation allocation;
            };

            /// @brief Create the temporary color buffer to render to.
            ///
            /// @param  slice_plane The plane of the 3D dispatch to render.
            ///
            /// @return The color buffer.
            ImageBuffer CreateAndLinkColorBuffer(SlicePlane slice_plane);

            /// @brief Convert and image buffer to a CPU image.
            ///
            /// @param image_buffer The image buffer.
            /// @param slice_plane  The plane of the 3D dispatch to be rendered.
            ///
            /// @return The QT image.
            QImage ImageBufferToQImage(ImageBuffer image_buffer, SlicePlane slice_plane);

            /// @brief Get the width of the color image output.
            /// 
            /// @param slice_plane The plane of the 3D dispatch to render.
            ///
            /// @return The width.
            uint32_t GetColorBufferWidth(SlicePlane slice_plane) const;

            /// @brief Get the height of the color image output.
            ///
            /// @param slice_plane The plane of the 3D dispatch to render.
            /// 
            /// @return The height.
            uint32_t GetColorBufferHeight(SlicePlane slice_plane) const;

            ImageBuffer stats_buffer_{};  ///< The non-color ray history statisitcs that is used to draw the different heatmap color modes.

            uint32_t              width_{};                  ///< The x dimension of the RH image.
            uint32_t              height_{};                 ///< The y dimension of the RH image.
            uint32_t              depth_{};                  ///< The z dimension of the RH image.
            VkPipeline            pipeline_{};               ///< The pipeline for the RH rendering.
            VkPipelineLayout      pipeline_layout_{};        ///< The pipeline layout for the RH rendering.
            VkDescriptorSetLayout descriptor_set_layout_{};  ///< The descriptor set layout for the RH rendering.
            Device*               device_{};                 ///< The device used by all the Vulkan rendering in RRA.
            VkCommandPool         command_pool_{};           ///< The command pool.
            VkCommandBuffer       cmd_{};                    ///< The command buffer for rendering the heatmap image.
            VkFence               fence_{};                  ///< The fence used to wait for rendering to finish.
            uchar*                qt_image_data_{};          ///< The CPU side image to be returned to Qt.
            Heatmap*              heatmap_ = nullptr;        ///< The current heatmap.
            VulkanHeatmap         vulkan_heatmap_{};         ///< The Vulkan resources needed by the heatmap.

            VkDescriptorPool descriptor_pool_{};  ///< The descriptor pool.
            VkDescriptorSet  descriptor_set_{};   ///< The descriptor set.
        };
    }  // namespace renderer
}  // namespace rra
#endif  // RRA_RAY_HISTORY_OFFSCREEN_RENDERER_H_
