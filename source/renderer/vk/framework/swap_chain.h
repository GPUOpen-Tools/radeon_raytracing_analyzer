//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the Vulkan swapchain object.
//=============================================================================

#ifndef RRA_RENDERER_VK_FRAMEWORK_SWAP_CHAIN_H_
#define RRA_RENDERER_VK_FRAMEWORK_SWAP_CHAIN_H_

#include "../../public/renderer_types.h"
#include "device.h"

namespace rra
{
    namespace renderer
    {
        /// @brief A set of render pass hints for the swapchain.
        enum class RenderPassHint
        {
            kRenderPassHintClearColorAndDepth,
            kRenderPassHintClearDepthOnly,
            kRenderPassHintClearNone,
            kRenderPassHintClearNoneDepthInput,
            kRenderPassHintResolve,
        };

        /// @brief Declaration of the SwapChain object.
        ///
        /// This type manages the chain of images that are drawn into and presented to the screen.
        class SwapChain
        {
        public:
            /// @brief Initialize the swapchain internals upon creation.
            ///
            /// @param [in] device The device used to create the swapchain resources.
            /// @param [in] back_buffer_count The number of back buffers in the swapchain.
            /// @param [in] window_info The window info.
            void OnCreate(Device* device, uint32_t back_buffer_count, const WindowInfo* window_info);

            /// @brief Destroy the swapchain.
            void OnDestroy();

            /// @brief Create all resources that depend on window resolution.
            ///
            /// @param [in] width The viewport width.
            /// @param [in] height The viewport height.
            /// @param [in] v_sync_enabled True if v-sync should be enabled, and false if not.
            void OnCreateWindowSizeDependentResources(uint32_t width, uint32_t height, bool v_sync_enabled);

            /// @brief Destroy all resources that depend on window size.
            void OnDestroyWindowSizeDependentResources();

            /// @brief Retrieve swapchain semaphore handles.
            ///
            /// @param [in] image_available_semaphore The image available semaphore handle.
            /// @param [in] render_finished_semaphore The rendering finished semaphore handle.
            /// @param [in] cmd_buffer_executed_fences The executed command buffer semaphore handle.
            void GetSemaphores(VkSemaphore* image_available_semaphore, VkSemaphore* render_finished_semaphore, VkFence* cmd_buffer_executed_fences);

            /// @brief Present the swapchain image to the screen.
            void Present();

            /// @brief Wait for the swapchain to be ready before moving to the next frame.
            ///
            /// @returns The index within the swapchain of the next back buffer to use.
            uint32_t WaitForSwapChain();

            /// @brief Retrieve a handle to the current back buffer image.
            ///
            /// @returns A handle to the current back buffer image.
            VkImage GetCurrentBackBuffer();

            /// @brief Retrieve a handle to the current back buffer render target view.
            ///
            /// @returns A handle to the current back buffer render target view.
            VkImageView GetCurrentBackBufferRTV();

            /// @brief Get the handle to the swapchain.
            ///
            /// @returns The handle to the swapchain.
            VkSwapchainKHR GetSwapChain() const;

            /// @brief Get the swapchain format.
            ///
            /// @returns The swapchain format.
            VkFormat GetFormat() const;

            /// @brief Get a handle to the render pass.
            ///
            /// @param [in] render_pass_hint The type of the render pass requested.
            ///
            /// @returns The render pass handle.
            VkRenderPass GetRenderPass(RenderPassHint render_pass_hint);

            /// @brief Get a handle to the frame buffer associated with a render pass.
            ///
            /// @param [in] render_pass_hint The type of the render pass requested.
            /// @param [in] image_index The index of the frame buffer to retrieve.
            ///
            /// @returns A handle to the frame buffer with the given index and render pass hint.
            VkFramebuffer GetFramebuffer(RenderPassHint render_pass_hint, int image_index) const;

            /// @brief Get the index of the current image in the swapchain.
            ///
            /// @returns The index for the current image in the swapchain.
            uint32_t GetImageIndex() const;

            /// @brief Get the depth/stencil image handle.
            ///
            /// @returns A handle to the depth/stencil image.
            VkImage GetDepthStencilImage() const;

            /// @brief Get the back buffer count.
            ///
            /// @returns The number of back buffers, aka frame count.
            uint32_t GetBackBufferCount() const;

            /// @brief Get the MSAA samples.
            ///
            /// @returns The MSAA samples.
            VkSampleCountFlagBits GetMSAASamples() const;

            /// @brief Get the depth input image to read from.
            ///
            /// @return The depth input image.
            VkImage GetDepthInputImage() const;

            /// @brief Get the depth input image view to read from.
            ///
            /// @return The depth input image view.
            const VkImageView& GetDepthInputImageView() const;

            /// @brief Get the size of the swapchain image.
            ///
            /// @return The swapchain extent.
            VkExtent2D GetSwapchainExtent() const;

        private:
            /// @brief Create a render target view for each back buffer image.
            void CreateRTV();

            /// @brief Destroy all SwapChain render target view handles.
            void DestroyRTV();

            /// @brief Create the render pass.
            void CreateRenderPass();

            /// @brief Destroy the render pass.
            void DestroyRenderPass();

            /// @brief Create the internal frame buffer resources with the provided dimensions.
            ///
            /// @param [in] width The frame buffer width.
            /// @param [in] height The frame buffer height.
            void CreateFramebuffers(uint32_t width, uint32_t height);

            /// @brief Destroy the internal frame buffer resources.
            void DestroyFramebuffers();

            /// @brief Destroy the color image resource.
            void DestroyColorImage();

            /// @brief Create the color image resource.
            void CreateColorImage();

            /// @brief Destroy the depth/stencil image resource.
            void DestroyDepthStencil();

            /// @brief Create the depth/stencil image resource.
            void CreateDepthStencil();

            std::vector<VkImage>     images_;                          ///< The vector of images within the swapchain.
            std::vector<VkImageView> image_views_;                     ///< The vector of swapchain image views.
            std::vector<VkFence>     command_buffer_executed_fences_;  ///< The fences used to monitor command buffer execution.
            std::vector<VkSemaphore> image_available_semaphores_;      ///< The semaphores used to monitor swapchain image availability.
            std::vector<VkSemaphore> render_finished_semaphores_;      ///< The semaphores used to monitor completion of rendering.
            VkSurfaceFormatKHR       swapchain_format_;                ///< The swapchain surface format.
            const WindowInfo*        window_info_   = nullptr;         ///< The platform's application window info.
            Device*                  device_        = nullptr;         ///< The device used to initialize the swapchain.
            VkSwapchainKHR           swapchain_     = VK_NULL_HANDLE;  ///< Handle to the swapchain instance.
            VkQueue                  present_queue_ = VK_NULL_HANDLE;  ///< Handle to the queue used to present the swapchain images.
            VkSurfaceKHR             surface_       = VK_NULL_HANDLE;  ///< The surface used to present graphics.

            VkRenderPass render_pass_clear_color_and_depth_  = VK_NULL_HANDLE;  ///< Handle to the render pass with kRenderPassHintClearColorAndDepth setting.
            VkRenderPass render_pass_clear_depth_only_       = VK_NULL_HANDLE;  ///< Handle to the render pass with kRenderPassHintClearDepthOnly setting.
            VkRenderPass render_pass_clear_none_             = VK_NULL_HANDLE;  ///< Handle to the render pass with kRenderPassHintClearNone setting.
            VkRenderPass render_pass_clear_none_depth_input_ = VK_NULL_HANDLE;  ///< Handle to the render pass with kRenderPassHintClearNoneDepthInput setting.
            VkRenderPass render_pass_resolve_                = VK_NULL_HANDLE;  ///< Handle to the render pass that will resolve the pipeline if MSAA is active.

            std::vector<VkFramebuffer>
                frame_buffers_clear_color_and_depth_;                    ///< The vector of frame buffer handles for kRenderPassHintClearColorAndDepth setting.
            std::vector<VkFramebuffer> frame_buffers_clear_depth_only_;  ///< The vector of frame buffer handles for kRenderPassHintClearDepthOnly setting.
            std::vector<VkFramebuffer> frame_buffers_clear_none_;        ///< The vector of frame buffer handles for kRenderPassHintClearNone setting.
            std::vector<VkFramebuffer>
                frame_buffers_clear_none_depth_input_;          ///< The vector of frame buffer handles for kRenderPassHintClearNoneDepthInput setting.
            std::vector<VkFramebuffer> frame_buffers_resolve_;  ///< The vector of frame buffer handles for resolve.

            VkSampleCountFlagBits msaa_samples_ = VK_SAMPLE_COUNT_1_BIT;  ///< The desired msaa samples for rendering.

            /// @brief A helper type used to group image resources together.
            struct Image
            {
                VkImage       image;       ///< Handle to the image.
                VmaAllocation allocation;  ///< Handle to the image memory.
                VkImageView   view;        ///< Handle to the image view.
            };

            Image      depth_stencil_       = {};  ///< The depth/stencil image handles.
            Image      geometry_depth_copy_ = {};  ///< A copy of the geometry's depth buffer to be used as an input attachment.
            Image      color_image_         = {};  ///< The color image handles.
            VkExtent2D swapchain_extent_;          ///< The dimension of the swapchain images.
            uint32_t   image_index_ = 0;           ///< The current back buffer image index.
            uint32_t   back_buffer_count_;         ///< The number of back buffers in the swapchain.
            uint32_t   semaphore_index_      = 0;  ///< The current frame semaphore index.
            uint32_t   prev_semaphore_index_ = 0;  ///< The previous frame semaphore index.

            bool v_sync_enabled_ = false;  ///< A flag used to indicate if v-sync is enabled.
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_FRAMEWORK_SWAP_CHAIN_H_
