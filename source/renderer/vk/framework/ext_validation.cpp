//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for Vulkan validation extensions functionality.
//=============================================================================

#include <cassert>

#include "instance.h"
#include "instance_properties.h"
#include "device_properties.h"
#include "ext_validation.h"
#include "../util_vulkan.h"

#include "public/rra_macro.h"
#include "public/rra_print.h"

#include <volk/volk.h>

namespace rra
{
    namespace renderer
    {
        static PFN_vkCreateDebugReportCallbackEXT  vkCreateDebugReportCallbackEXT  = NULL;
        static PFN_vkDebugReportMessageEXT         vkDebugReportMessageEXT         = NULL;
        static PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = NULL;
        static VkDebugReportCallbackEXT            DebugReportCallback             = NULL;

        static bool can_use_debug_report = false;

        static VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback(VkDebugReportFlagsEXT      flags,
                                                                    VkDebugReportObjectTypeEXT object_type,
                                                                    uint64_t                   object,
                                                                    size_t                     location,
                                                                    int32_t                    message_code,
                                                                    const char*                layer_prefix,
                                                                    const char*                message,
                                                                    void*                      usage_data)
        {
            RRA_UNUSED(flags);
            RRA_UNUSED(object_type);
            RRA_UNUSED(object);
            RRA_UNUSED(location);
            RRA_UNUSED(message_code);
            RRA_UNUSED(layer_prefix);
            RRA_UNUSED(usage_data);

            if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
            {
                RraPrint("%s\n\n", message);
            }
            return VK_FALSE;
        }

        VkValidationFeatureEnableEXT enables[] = {VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT};
        VkValidationFeaturesEXT      features  = {};

        bool ExtDebugReportCheckInstanceExtensions(InstanceProperties* instance_properties)
        {
            can_use_debug_report = instance_properties->AddInstanceLayerName("VK_LAYER_KHRONOS_validation") &&
                                   instance_properties->AddInstanceExtensionName(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

            if (can_use_debug_report)
            {
                features.sType                         = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
                features.pNext                         = instance_properties->GetNext();
                features.enabledValidationFeatureCount = 1;
                features.pEnabledValidationFeatures    = enables;

                instance_properties->SetNewNext(&features);
            }

            return can_use_debug_report;
        }

        void ExtDebugReportGetProcAddresses(VkInstance instance)
        {
            if (can_use_debug_report)
            {
                vkCreateDebugReportCallbackEXT  = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
                vkDebugReportMessageEXT         = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT");
                vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");

                assert(vkCreateDebugReportCallbackEXT);
                assert(vkDebugReportMessageEXT);
                assert(vkDestroyDebugReportCallbackEXT);
            }
        }

        void ExtDebugReportOnCreate(VkInstance instance)
        {
            if (vkCreateDebugReportCallbackEXT)
            {
                VkDebugReportCallbackCreateInfoEXT debugReportCallbackInfo = {};
                debugReportCallbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
                debugReportCallbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
                debugReportCallbackInfo.pfnCallback = MyDebugReportCallback;

                VkResult result = vkCreateDebugReportCallbackEXT(instance, &debugReportCallbackInfo, nullptr, &DebugReportCallback);
                CheckResult(result, "Failed to register Debug Report callback.");
            }
        }

        void ExtDebugReportOnDestroy(VkInstance instance)
        {
            if (DebugReportCallback)
            {
                vkDestroyDebugReportCallbackEXT(instance, DebugReportCallback, nullptr);
                DebugReportCallback = nullptr;
            }
        }
    }  // namespace renderer
}  // namespace rra
