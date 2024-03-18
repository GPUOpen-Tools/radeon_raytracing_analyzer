//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for Vulkan validation extensions functionality.
//=============================================================================

#ifndef RRA_RENDERER_VK_FRAMEWORK_EXT_VALIDATION_H_
#define RRA_RENDERER_VK_FRAMEWORK_EXT_VALIDATION_H_

#include "instance_properties.h"
#include "device_properties.h"

namespace rra
{
    namespace renderer
    {
        /// @brief Check if the debug report instance extension is supported.
        ///
        /// @param [in] instance_properties The instance properties object.
        ///
        /// @returns True if the debug report instance extension is supported, and false if it's not.
        bool ExtDebugReportCheckInstanceExtensions(InstanceProperties* instance_properties);

        /// @brief Initialize the Debug Report procedure addresses.
        ///
        /// @param [in] instance The Vulkan instance.
        void ExtDebugReportGetProcAddresses(VkInstance instance);

        /// @brief Initialize the Debug Report extension upon creation.
        ///
        /// @param [in] instance The Vulkan instance.
        void ExtDebugReportOnCreate(VkInstance instance);

        /// @brief Destroy the Debug Report extension.
        ///
        /// @param [in] instance The Vulkan instance.
        void ExtDebugReportOnDestroy(VkInstance instance);
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_FRAMEWORK_EXT_VALIDATION_H_
