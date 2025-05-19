//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the checker styled clear module.
//=============================================================================

#ifndef RRA_RENDERER_VK_RENDER_MODULES_CHECKER_CLEAR_H_
#define RRA_RENDERER_VK_RENDER_MODULES_CHECKER_CLEAR_H_

#include "vk/render_module.h"

namespace rra
{
    namespace renderer
    {
        /// @brief Push constant containing colors of checker pattern.
        struct ClearColorPushConstants
        {
            glm::vec4 checker_color_1;  ///< Color of half the checker tiles.
            glm::vec4 checker_color_2;  ///< Color of half the checker tiles.
        };

        /// @brief Render module to render the checkerboard background
        class CheckerClearRenderModule : public RenderModule
        {
        public:
            /// @brief Constructor.
            CheckerClearRenderModule();

            /// @brief Destructor.
            virtual ~CheckerClearRenderModule() = default;

            /// @brief Initialize render module.
            ///
            /// @param [in] context The context to initialize.
            virtual void Initialize(const RenderModuleContext* context) override;

            /// @brief Draw onto given command buffer
            ///
            /// @param [in] context The frame context used to draw a new frame.
            virtual void Draw(const RenderFrameContext* context) override;

            /// @brief Cleanup render module.
            ///
            /// @param [in] context The context to cleanup.
            virtual void Cleanup(const RenderModuleContext* context) override;

        private:
            VkPipeline       checker_clear_pipeline_ = VK_NULL_HANDLE;  ///< The pipeline used for rendering the checker shaders.
            VkPipelineLayout pipeline_layout_        = VK_NULL_HANDLE;  ///< The pipeline layout used for rendering the checker shaders.
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_RENDER_MODULES_CHECKER_CLEAR_H_

