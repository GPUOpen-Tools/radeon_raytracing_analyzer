//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the Vulkan render module.
//=============================================================================

#include "render_module.h"

namespace rra
{
    namespace renderer
    {
        RenderModule::RenderModule(RenderPassHint render_pass_hint)
        {
            render_pass_hint_ = render_pass_hint;
        }

        bool RenderModule::ShouldCopyDepthBuffer() const
        {
            return false;
        }

        void RenderModule::EveryFrameUpdate(Device* device, uint32_t current_frame)
        {
            // No-op by default.
            RRA_UNUSED(device);
            RRA_UNUSED(current_frame);
        }

        void RenderModule::Enable()
        {
            enabled_ = true;
        }

        void RenderModule::Disable()
        {
            enabled_ = false;
        }

        bool RenderModule::IsEnabled() const
        {
            return enabled_;
        }

        RenderPassHint RenderModule::GetRenderPassHint() const
        {
            return render_pass_hint_;
        }

        void RenderModule::SetRenderPassHint(RenderPassHint render_pass_hint)
        {
            render_pass_hint_ = render_pass_hint;
        }

        RendererInterface* RenderModule::GetRendererInterface()
        {
            return renderer_interface_;
        }

        void RenderModule::SetRendererInterface(RendererInterface* renderer_interface)
        {
            if (!renderer_interface_)
            {
                renderer_interface_ = renderer_interface;
            }
        }

    }  // namespace renderer
}  // namespace rra
