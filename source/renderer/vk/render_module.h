//=============================================================================
// Copyright (c) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the Render Module.
///
/// The RenderModule is an abstract class that can be used by the Renderer to
/// define and run a Vulkan pipeline.
//=============================================================================

#ifndef RRA_RENDERER_VK_RENDER_MODULE_H_
#define RRA_RENDERER_VK_RENDER_MODULE_H_

#include "public/renderer_interface.h"
#include "framework/device.h"
#include "framework/swap_chain.h"
#include "framework/command_buffer_ring.h"

#include <functional>

namespace rra
{
    namespace renderer
    {
        /// @brief A structure to keep references to graphics handles.
        struct RenderModuleContext
        {
            Device*            device;               ///< The renderer device.
            SwapChain*         swapchain;            ///< The swapchain used to present images.
            CommandBufferRing* command_buffer_ring;  ///< The ring of command buffer objects.
            uint32_t           framebuffer_width;    ///< The framebuffer width.
            uint32_t           framebuffer_height;   ///< The framebuffer height.
        };

        /// @brief A structure to keep references to graphics handles.
        struct RenderFrameContext
        {
            Device*                device;              ///< The renderer device.
            uint32_t               current_frame;       ///< The index of the current frame being drawn in the swapchain.
            VkCommandBuffer        command_buffer;      ///< The command buffer to record commands into.
            VkDescriptorBufferInfo scene_ubo_info;      ///< The uniform buffer info for the scene.
            VkDescriptorImageInfo  heatmap_image_info;  ///< The heatmap image info.
            RendererSceneInfo*     scene_info;          ///< The scene being rendered.
            glm::mat4              view_projection;     ///< The view projection combined matrix for frustum culling.
            glm::vec3              camera_position;     ///< The camera position in the 3d world.
            float                  camera_fov;          ///< The camera fov.
            Camera                 camera;              ///< The camera.
            uint32_t               framebuffer_width;   ///< The framebuffer width.
            uint32_t               framebuffer_height;  ///< The framebuffer height.
            std::function<void()>  begin_render_pass;   ///< The function callback to start the render pass.
            std::function<void()>  end_render_pass;     ///< The function callback to end the render pass.
        };

        /// @brief The RenderModule class declaration.
        ///
        /// The RenderModule is an abstract class that can be extended
        /// to simplify the pipeline setup and execution.
        class RenderModule
        {
        public:
            /// @brief Constructor.
            ///
            /// @param [in] render_pass_hint The hint to set.
            RenderModule(RenderPassHint render_pass_hint);

            /// @brief Destructor.
            virtual ~RenderModule() = default;

            /// @brief Initialize render module.
            ///
            /// @param [in] context The context to initialize.
            virtual void Initialize(const RenderModuleContext* context) = 0;

            /// @brief Draw onto given command buffer
            ///
            /// @param [in] context The context used to draw the frame.
            virtual void Draw(const RenderFrameContext* context) = 0;

            /// @brief Cleanup render module.
            ///
            /// @param [in] context The context to cleanup.
            virtual void Cleanup(const RenderModuleContext* context) = 0;

            /// @brief Enable module to draw.
            void Enable();

            /// @brief Disable module to prevent draw.
            void Disable();

            /// @brief Check if module is enabled
            ///
            /// @return True if module is enabled.
            bool IsEnabled() const;

            /// @brief Get the render pass hint.
            ///
            /// @return The render pass hint.
            RenderPassHint GetRenderPassHint() const;

            /// @brief Set the render pass hint.
            ///
            /// @param [in] render_pass_hint The new hint.
            void SetRenderPassHint(RenderPassHint render_pass_hint);

            /// @brief Get the render interface for this module.
            ///
            /// @return The render interface that this module is currently working with.
            RendererInterface* GetRendererInterface();

            /// @brief Set the render interface for this module.
            ///
            /// @param [in] render_interface The render interface that this module will work with.
            /// Note: this operation is only valid if the render interface has never been set before.
            void SetRendererInterface(RendererInterface* render_interface);

        private:
            bool               enabled_          = true;                                      ///< The value to keep track if module is enabled.
            RenderPassHint     render_pass_hint_ = RenderPassHint::kRenderPassHintClearNone;  ///< The value that will select the render pass for the Renderer.
            RendererInterface* renderer_interface_ = nullptr;                                 ///< The render interface that uses this module.
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_RENDER_MODULE_H_
