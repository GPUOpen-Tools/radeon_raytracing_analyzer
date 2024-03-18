//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the Graphics Context functions.
//=============================================================================

#include "public/graphics_context.h"

#include "vk/vk_graphics_context.h"

namespace rra
{
    namespace renderer
    {
        void CreateGraphicsContext(QWidget* parent)
        {
            auto vk_graphics_context = rra::renderer::GetVkGraphicsContext();

            rra::renderer::WindowInfo window_info = {};
#if defined(VK_USE_PLATFORM_WIN32_KHR)
            window_info.window_handle = reinterpret_cast<HWND>(parent->winId());
#elif defined(VK_USE_PLATFORM_XCB_KHR)
            QPlatformNativeInterface* native_interface = QGuiApplication::platformNativeInterface();
            Q_ASSERT(native_interface);
            xcb_connection_t* connection = static_cast<xcb_connection_t*>(native_interface->nativeResourceForWindow("connection", parent->windowHandle()));
            Q_ASSERT(connection);
            xcb_window_t xcb_window = static_cast<xcb_window_t>(parent->winId());
            Q_ASSERT(xcb_window);
            window_info.window     = xcb_window;
            window_info.connection = connection;
#endif

            vk_graphics_context->SetWindowInfo(window_info);
        }

        bool InitializeGraphicsContext(std::shared_ptr<GraphicsContextSceneInfo> info)
        {
            auto vk_graphics_context = rra::renderer::GetVkGraphicsContext();
            return vk_graphics_context->Initialize(info);
        }

        std::string GetGraphicsContextInitializationError()
        {
            auto vk_graphics_context = rra::renderer::GetVkGraphicsContext();
            return vk_graphics_context->GetInitializationErrorMessage();
        }

        void CleanupGraphicsContext()
        {
            auto vk_graphics_context = rra::renderer::GetVkGraphicsContext();
            vk_graphics_context->Cleanup();
            DeleteVkGraphicsContext();
        }
    }  // namespace renderer
}  // namespace rra
