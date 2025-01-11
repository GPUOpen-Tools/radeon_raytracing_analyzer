//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for Vulkan debug extension utility functions.
//=============================================================================

#ifndef RRA_RENDERER_VK_FRAMEWORK_EXT_DEBUG_UTILS_H_
#define RRA_RENDERER_VK_FRAMEWORK_EXT_DEBUG_UTILS_H_

#include <volk/volk.h>

#include "instance_properties.h"

namespace rra
{
    namespace renderer
    {
        /// @brief Initialize the debug utils procedure addresses.
        ///
        /// @param [in] device The device handle.
        void ExtDebugUtilsGetProcAddresses(VkDevice device);

        /// @brief Check if the debug utils instance extension is available.
        ///
        /// @param [in] input The properties object used to manage instance extensions.
        ///
        /// @returns True if the debug utils instance extensions is supported.
        bool ExtDebugUtilsCheckInstanceExtensions(InstanceProperties* instance_properties);

        /// @brief Set the name string for a given object.
        ///
        /// @param [in] device The device used to create the given object.
        /// @param [in] object_type The type of object being named.
        /// @param [in] handle The handle to the object being named.
        /// @param [in] name The new name for the object.
        void SetObjectName(VkDevice device, VkObjectType object_type, uint64_t handle, const char* name);

        /// @brief Set the start of a performance label range.
        ///
        /// @param [in] command_buffer The command buffer to insert the begin perf label into.
        /// @param [in] message The perf label message string.
        void SetPerfLabelBegin(VkCommandBuffer command_buffer, const char* message);

        /// @brief Set the end of a performance label range.
        ///
        /// @param [in] command_buffer The command buffer to insert the end perf label into.
        void SetPerfLabelEnd(VkCommandBuffer command_buffer);
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_FRAMEWORK_EXT_DEBUG_UTILS_H_
