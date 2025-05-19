//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the Vulkan instance wrapper object.
//=============================================================================

#ifndef RRA_RENDERER_VK_FRAMEWORK_INSTANCE_H_
#define RRA_RENDERER_VK_FRAMEWORK_INSTANCE_H_

#include "volk/volk.h"

#include "vk/framework/instance_properties.h"

namespace rra
{
    namespace renderer
    {
        /// @brief Create the Vulkan instance.
        ///
        /// @param [in] app_name The application name string.
        /// @param [in] engine_name The engine name string.
        /// @param [out] instance The handle to the new Vulkan instance.
        /// @param [out] physical_device The handle to the physical device.
        /// @param [in] instance_properties The instance properties object.
        ///
        /// @returns The instance creation return code.
        VkResult CreateInstance(const char*         app_name,
                                const char*         engine_name,
                                VkInstance*         instance,
                                VkPhysicalDevice*   physical_device,
                                InstanceProperties* instance_properties);

        /// @brief Create the Vulkan instance, enabling requested instance layers & extensions where supported.
        ///
        /// @param [in] app_info The application name and version info.
        /// @param [in] instance_properties The instance properties object.
        ///
        /// @returns A handle to the new instance.
        VkInstance CreateInstance(VkApplicationInfo app_info, InstanceProperties* instance_properties);

        /// @brief Destroy the given instance.
        ///
        /// @param [in] instance The instance to be destroyed.
        void DestroyInstance(VkInstance instance);
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_FRAMEWORK_INSTANCE_H_

