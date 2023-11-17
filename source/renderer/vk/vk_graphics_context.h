//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for graphics context for the Vulkan API.
//=============================================================================

#ifndef RRA_RENDERER_VK_GRAPHICS_CONTEXT_H_
#define RRA_RENDERER_VK_GRAPHICS_CONTEXT_H_

#include "framework/device.h"
#include "public/renderer_interface.h"
#include "public/renderer_types.h"
#include <map>
#include <memory>
#include <QImage>

namespace rra
{
    namespace renderer
    {
        class RayHistoryOffscreenRenderer;
        struct DispatchIdData;

        /// @brief An instruction to use when drawing a blas.
        struct BlasDrawInstruction
        {
            VkBuffer vertex_buffer = VK_NULL_HANDLE;
            uint32_t vertex_index  = 0;
            uint32_t vertex_count  = 0;
        };

        /// @brief The traversal tree for vulkan.
        struct VkTraversalTree
        {
            VkBuffer      volume_buffer     = VK_NULL_HANDLE;
            VmaAllocation volume_allocation = VK_NULL_HANDLE;
            VkBuffer      vertex_buffer     = VK_NULL_HANDLE;
            VmaAllocation vertex_allocation = VK_NULL_HANDLE;
        };

        /// @brief The Vulkan specific graphics context.
        class VkGraphicsContext
        {
        public:
            /// @brief Initializes the Vulkan context and device.
            ///
            /// @returns True if the graphics context was initialized successfully, or false if a failure occurred.
            bool Initialize(std::shared_ptr<GraphicsContextSceneInfo> info);

            /// @brief Cleans up the context.
            void Cleanup();

            /// @brief Set the window information for this context.
            ///
            /// @param [in] window_info The window information.
            void SetWindowInfo(WindowInfo window_info);

            /// @brief Get the draw instruction for a blas index.
            ///
            /// @param [in] blas_index The blas index.
            ///
            /// @returns An instruction on how to draw the blas.
            BlasDrawInstruction GetBlasDrawInstruction(uint64_t blas_index);

            /// @brief Get the uploaded traversal trees for a blases.
            ///
            /// @returns The loaded memory of the traversal tree.
            std::vector<VkTraversalTree> GetBlases() const;

            /// @brief Get the device for this context.
            ///
            /// @returns A device.
            Device& GetDevice();

            /// @brief Check if the context has been initialized successfully and is ready to use.
            ///
            /// @returns True if the graphics context has been initialized successfully.
            bool IsInitialized() const;

            /// @brief Check if the error window has been primed.
            ///
            /// @returns True if the error window has been primed.
            bool IsErrorWindowPrimed();

            /// @brief Check if the context is healthy.
            ///
            /// @returns True if the context is healthy.
            bool IsHealthy();

            /// @brief Get the initialization error message in the event of startup failure.
            ///
            /// @returns The initialization error message.
            std::string GetInitializationErrorMessage();

            /// @brief Set the initialization error message.
            ///
            /// @param [in] message The initialization error message to send.
            void SetInitializationErrorMessage(const std::string& message);

            /// @brief Set the scene info the graphics context will use.
            /// @param scene_info The scene info used for renderering.
            void SetSceneInfo(RendererSceneInfo* scene_info);

            /// @brief Create the device buffer containing traversal data that can be rendered.
            ///
            /// @param dispatch_id The dispatch ID.
            /// @param [out] out_max_count Maximum count of each dispatch ID statistic.
            void CreateRayHistoryStatsBuffer(uint32_t dispatch_id, DispatchIdData* out_max_count);

            /// @brief Render the previously created render image.
            ///
            /// @param heatmap_min The minimum heatmap value selected from the heatmap slider.
            /// @param heatmap_max The maximum heatmap value selected from the heatmap slider.
            /// @param ray_index   The index of the current ray.
            /// @param reshaped_x  The dispatch width, after reshaping for 1D dispatches.
            /// @param reshaped_y  The dispatch height, after reshaping for 1D dispatches.
            /// @param reshaped_z  The dispatch depth, after reshaping for 1D dispatches.
            /// @param color_mode  The ray history color mode to render with.
            /// @param slice_index The slice of the 3D dispatch to be rendered.
            /// @param slice_plane  The plane of the 3D dispatch to be rendered.
            ///
            /// @return The QT image.
            QImage RenderRayHistoryImage(uint32_t            heatmap_min,
                                         uint32_t            heatmap_max,
                                         uint32_t            ray_index,
                                         uint32_t            reshaped_x,
                                         uint32_t            reshaped_y,
                                         uint32_t            reshaped_z,
                                         RayHistoryColorMode color_mode,
                                         uint32_t            slice_index,
                                         SlicePlane          slice_plane);

            /// @brief Set the heatmap data.
            ///
            /// @param [in] heatmap_data The raw data of the heatmap.
            void SetRayHistoryHeatmapData(const HeatmapData& heatmap_data);

            /// @brief Create Vulkan image and sampler for the heatmap.
            ///
            /// @param cmd         The command buffer.
            /// @param heatmap     The heatmap data.
            /// @param for_compute True if these resources will be used with a compute pipeline.
            ///
            /// @return The Vulkan heatmap resources.
            VulkanHeatmap CreateVulkanHeatmapResources(VkCommandBuffer cmd, Heatmap* heatmap, bool for_compute);

        private:
            /// @brief Collects and uploads the traversal trees to the device.
            ///
            /// returns True on successful upload.
            bool CollectAndUploadTraversalTrees(std::shared_ptr<GraphicsContextSceneInfo> info);

            Device                           device_{};                 ///< The renderer device.
            WindowInfo                       window_info_{};            ///< The window information.
            std::vector<BlasDrawInstruction> geometry_instructions_{};  ///< Mapping from BLAS to a geometry address.
            std::vector<VkTraversalTree>     blases_{};                 ///< The traversal tree.
            RendererSceneInfo*               scene_info_{};             ///< Information needed to render the scene.

            /// We load our contents in a seperate thread so we can't show the error window and exit until we join main thread.
            bool error_window_primed_ = false;  /// A flag to track if the error window can be shown if the vulkan crashes after the loading has been complete.
            std::string initialization_error_message_ = "";  /// The error message in case of a crash before rendering starts (aka loading).

            bool                         initialized_ = false;  ///< A flag used to track if the context has been initialized.
            RayHistoryOffscreenRenderer* rh_renderer_{};
        };

        /// @brief A function to get the global graphics context.
        ///
        /// @return A Vulkan graphics context.
        VkGraphicsContext* GetVkGraphicsContext();

        /// @brief A function to delete the global graphics context.
        void DeleteVkGraphicsContext();

    }  // namespace renderer

}  // namespace rra

#endif  // RRA_RENDERER_VK_GRAPHICS_CONTEXT_H_
