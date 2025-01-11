//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the Vulkan renderer.
//=============================================================================

#ifndef RRA_RENDERER_VULKAN_RENDERER_VULKAN_H_
#define RRA_RENDERER_VULKAN_RENDERER_VULKAN_H_

#include "../public/renderer_interface.h"
#include "../public/heatmap.h"

#include "framework/device.h"
#include "framework/swap_chain.h"
#include "framework/command_buffer_ring.h"

#include "render_module.h"

#include <chrono>

namespace rra
{
    namespace renderer
    {
        /// @brief State tracking data store for RendererVulkan.
        struct RendererVulkanStateTracker
        {
            uint64_t           renderer_iteration_  = 0;   ///< The last renderer iteration.
            uint64_t           scene_iteration      = 0;   ///< The last scene iteration.
            SceneUniformBuffer scene_uniform_buffer = {};  ///< The last view projection matrix.

            /// @brief Check if the other state is equal to this.
            ///
            /// @param [in] other The other state to check against.
            ///
            /// @returns True if equal.
            ///
            ///
            bool Equal(const RendererVulkanStateTracker& other) const;
        };

        /// @brief Check if the scene uniform buffers are equal to one other.
        ///
        /// @param [in] a One of the buffers to check.
        /// @param [in] b The other buffer to check.
        ///
        /// @returns True if equal.
        bool EqualsSceneUniformBuffer(const SceneUniformBuffer& a, const SceneUniformBuffer& b);

        /// @brief The RendererVulkan implementation uses Vulkan to draw frames.
        /// The RendererVulkan takes ownership of all the modules passed during construction.
        /// It will delete the modules during destruction.
        class RendererVulkan : public RendererInterface
        {
        public:
            /// @brief Constructor.
            ///
            /// @param [in] render_modules The render modules to use for drawing.
            RendererVulkan(std::vector<RenderModule*> render_modules);

            /// @brief Destructor.
            virtual ~RendererVulkan();

            /// @brief Create the graphics device.
            ///
            /// @returns True if the device was intialized successfully, or false if initialization failed.
            virtual bool InitializeDevice() override;

            /// @brief Handle any synchronization required to advance to rendering the next frame.
            virtual void MoveToNextFrame() override;

            /// @brief Wait for all in flight operations in the GPU queue to complete.
            virtual void WaitForGpu() override;

            /// @brief Shut down the renderer interface.
            virtual void Shutdown() override;

            /// @brief Wait for the next swapchain image to become available.
            virtual void WaitForSwapchain() override;

            /// @brief Handle the renderer resizing.
            virtual void HandleDimensionsUpdated() override;

            /// @brief Draw the scene.
            virtual void DrawFrame() override;

            /// @brief Mark the scene as dirty.
            virtual void MarkAsDirty() override;

            /// @brief Retrieve a reference to the device object.
            ///
            /// @returns A reference to the device.
            Device& GetDevice();

            /// @brief Retrieve a reference to the command buffer ring object.
            ///
            /// @returns A reference to the command buffer ring object.
            CommandBufferRing& GetCommandBufferRing();

            /// @brief Retrieve a reference to the persistently-mapped scene uniform buffer.
            ///
            /// @returns A reference to the scene UBO.
            SceneUniformBuffer& GetSceneUbo();

        protected:
            /// @brief Handle updating the renderer after the scene has changed.
            virtual void HandleSceneChanged() override;

        private:
            /// @brief Builds and saves a command buffers to be presented for the current frame.
            void BuildScene();

            /// @brief Start the scene.
            ///
            /// @param [in] command_buffer The command buffer to record initial scene rendering commands to.
            void BeginScene(VkCommandBuffer command_buffer);

            /// @brief Start the render pass.
            ///
            /// @param [in] command_buffer The command buffer to record scene rendering commands to.
            /// @param [in] render_pass_hint The render pass hint to select and begin the correct render pass.
            void BeginRenderPass(VkCommandBuffer command_buffer, RenderPassHint render_pass_hint);

            /// @brief Finish the current render pass.
            ///
            /// @param [in] command_buffer The command buffer to record scene rendering commands to.
            void EndRenderPass(VkCommandBuffer command_buffer);

            /// @brief Finish the scene.
            ///
            /// @param [in] command_buffer The command buffer to record final scene rendering commands to.
            void EndScene(VkCommandBuffer command_buffer);

            /// @brief Finish rendering the scene.
            void PresentScene();

            /// @brief Initialize the scene uniform buffer object.
            void InitializeSceneUniformBuffer();

            /// @brief Update the persistently-mapped uniform buffer data.
            ///
            /// @param [in] frame_to_update The frame to update.
            void UpdateSceneUniformBuffer(uint32_t frame_to_update);

            /// @brief Update the heatmap.
            void UpdateHeatmap();

            /// @brief Cleanup heatmap.
            void CleanupHeatmap();

            /// @brief Copy the depth buffer to be read from a later render pass.
            ///
            /// @param [in] The command buffer rendering commands are being recorded to.
            void CopyDepthBuffer(VkCommandBuffer cmd);

            SceneUniformBuffer scene_uniform_buffer_ = {};  ///< A set of data used to update the scene uniform buffer memory.

            /// @brief The device buffer for the scene.
            struct SceneBuffer
            {
                VkBuffer      buffer     = VK_NULL_HANDLE;
                VmaAllocation allocation = VK_NULL_HANDLE;
            };

            std::vector<SceneBuffer> scene_ubos_;  ///< The scene buffer ubos per each frame.

            VulkanHeatmap vulkan_heatmap_ = {};  ///< The heatmap data to render with.

            std::vector<std::vector<VkCommandBuffer>> command_buffers_per_frame_;  ///< The command buffers for each frame in the swapchain.

            Device&           device_;               ///< The renderer device.
            SwapChain         swapchain_;            ///< The swapchain used to present images.
            CommandBufferRing command_buffer_ring_;  ///< The ring of command buffer objects.
            VkViewport        viewport_;             ///< The viewport dimensions.
            VkRect2D          scissor_;              ///< The scissor rectangle.
            float             clear_color_[4];       ///< The scene clear color.
            uint32_t          current_frame_index_;  ///< The current frame index.
            bool              initialized_;          ///< The flag used to track renderer initialization.

            RenderModuleContext        render_module_context_;  ///< The render module context to use across modules.
            std::vector<RenderModule*> render_modules_;         ///< The list of render modules to aid rendering.

            uint64_t renderer_iteration_ = 0;  ///< The renderer iteration to track state for when the renderer is marked dirty.

            std::vector<RendererVulkanStateTracker> states_;  ///< The state tracking to provide efficient rendering.
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VULKAN_RENDERER_VULKAN_H_
