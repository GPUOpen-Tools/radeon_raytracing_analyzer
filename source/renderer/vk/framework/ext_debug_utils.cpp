//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for Vulkan debug extension utility functions.
//=============================================================================

#include <cassert>
#include <mutex>
#include <string.h>

#ifndef _WIN32
#include <public/linux/safe_crt.h>
#endif

#include "public/rra_print.h"
#include "ext_debug_utils.h"

namespace rra
{
    namespace renderer
    {
        static PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectName = nullptr;
        static PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabel = nullptr;
        static PFN_vkCmdEndDebugUtilsLabelEXT   vkCmdEndDebugUtilsLabel   = nullptr;

        static bool       can_use_debug_utils = false;  ///< This flag is set to true upon verifying that the debug utils extension is available.
        static std::mutex object_name_mutex;            ///< This mutex manages invocation of the vkSetDebugUtilsSetObjectName function.

        bool ExtDebugUtilsCheckInstanceExtensions(InstanceProperties* instance_properties)
        {
            can_use_debug_utils = instance_properties->AddInstanceExtensionName("VK_EXT_debug_utils");
            if (can_use_debug_utils)
            {
                RraPrint("Note that the extension 'VK_EXT_debug_utils' is only available under tools that enable them, like RenderDoc\n");
            }

            return can_use_debug_utils;
        }

        void ExtDebugUtilsGetProcAddresses(VkDevice device)
        {
            if (can_use_debug_utils)
            {
                vkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
                vkCmdBeginDebugUtilsLabel = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT");
                vkCmdEndDebugUtilsLabel   = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT");
            }
        }

        void SetObjectName(VkDevice device, VkObjectType object_type, uint64_t handle, const char* name)
        {
            assert(handle != 0);

            size_t size     = strlen(name);
            char*  uni_name = (char*)malloc(size + 1);  // Yes, this will cause leaks!
            if (uni_name)
            {
                strcpy_s(uni_name, size + 1, name);
            }

            VkDebugUtilsObjectNameInfoEXT name_info = {};
            name_info.sType                         = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            name_info.pNext                         = nullptr;
            name_info.objectType                    = object_type;
            name_info.objectHandle                  = handle;
            name_info.pObjectName                   = uni_name;

            if (vkSetDebugUtilsObjectName != nullptr)
            {
                std::unique_lock<std::mutex> lock(object_name_mutex);
                vkSetDebugUtilsObjectName(device, &name_info);
            }
        }

        void SetPerfLabelBegin(VkCommandBuffer command_buffer, const char* message)
        {
            if (vkCmdBeginDebugUtilsLabel)
            {
                VkDebugUtilsLabelEXT label_info = {};
                label_info.sType                = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;

                // Color to display this region with (if supported by debugger).
                float color[4] = {1.0f, 0.0f, 0.0f, 1.0f};
                memcpy(label_info.color, &color[0], sizeof(float) * 4);

                // Name of the region displayed by the debugging application.
                label_info.pLabelName = message;
                vkCmdBeginDebugUtilsLabel(command_buffer, &label_info);
            }
        }

        void SetPerfLabelEnd(VkCommandBuffer command_buffer)
        {
            if (vkCmdEndDebugUtilsLabel)
            {
                vkCmdEndDebugUtilsLabel(command_buffer);
            }
        }

    }  // namespace renderer
}  // namespace rra
