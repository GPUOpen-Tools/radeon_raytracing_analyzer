//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of the Graphics Context functions.
//=============================================================================

#ifndef RRA_RENDERER_GRAPHICS_CONTEXT_H_
#define RRA_RENDERER_GRAPHICS_CONTEXT_H_

#include <public/renderer_types.h>
#include "public/renderer_interface.h"
#include <QWidget>
#include <string>

namespace rra
{
    namespace renderer
    {
        /// @brief Create a graphics context.
        ///
        /// @param [in] parent qt widget information.
        /// @param [in] window_info The window information for the creation of graphics device, queues, and context.
        void CreateGraphicsContext(QWidget* parent);

        /// @brief Initialize the graphics context. Note: must be called after CreateGraphicsContext.
        ///
        /// @returns True if the context was initialized successfully and false in case of failure.
        bool InitializeGraphicsContext(const GraphicsContextSceneInfo& info);

        /// @brief Get the human readable initialization error.
        ///
        /// @returns A human readable initialization error.
        std::string GetGraphicsContextInitializationError();

        /// @brief Cleanup the graphics context.
        void CleanupGraphicsContext();

    }  // namespace renderer
}  // namespace rra

#endif
