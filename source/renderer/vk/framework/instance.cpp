//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the Vulkan instance wrapper object.
//=============================================================================

#include <algorithm>
#include <cassert>

#include "instance.h"
#include "instance_properties.h"
#include <volk/volk.h>
#include "ext_validation.h"
#include "ext_debug_utils.h"
#include "ext_gpu_validation.h"

#include "public/rra_macro.h"
#include "public/rra_print.h"

#include "../util_vulkan.h"

namespace rra
{
    namespace renderer
    {
        VkResult CreateInstance(const char*         app_name,
                                const char*         engine_name,
                                VkInstance*         instance,
                                VkPhysicalDevice*   physical_device,
                                InstanceProperties* instance_properties)
        {
            VkResult result = VK_ERROR_UNKNOWN;

            VkApplicationInfo app_info  = {};
            app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            app_info.pNext              = NULL;
            app_info.pApplicationName   = app_name;
            app_info.applicationVersion = 1;
            app_info.pEngineName        = engine_name;
            app_info.engineVersion      = 1;
            app_info.apiVersion         = VK_API_VERSION_1_2;

            // Attempt to create the instance.
            VkInstance new_instance = CreateInstance(app_info, instance_properties);

            if (new_instance != VK_NULL_HANDLE)
            {
                // Load the instance entrypoints.
                volkLoadInstance(new_instance);

                // Enumerate physical devices.
                uint32_t       gpu_count = 1;
                uint32_t const req_count = gpu_count;

                result = vkEnumeratePhysicalDevices(new_instance, &gpu_count, NULL);
                CheckResult(result, "Failed to enumerate physical devices.");

                std::vector<VkPhysicalDevice> gpus;
                gpus.resize(gpu_count);

                result = vkEnumeratePhysicalDevices(new_instance, &gpu_count, gpus.data());
                CheckResult(result, "Failed to populate physical device list.");
                assert(!result && gpu_count >= req_count);
                RRA_UNUSED(req_count);

                *instance        = new_instance;
                *physical_device = gpus[0];
            }
            else
            {
                result = VK_ERROR_INITIALIZATION_FAILED;
            }

            return result;
        }

        VkInstance CreateInstance(VkApplicationInfo app_info, InstanceProperties* instance_properties)
        {
            VkInstance instance = VK_NULL_HANDLE;

            // Prepare existing extensions and layer names into a buffer for vkCreateInstance.
            std::vector<const char*> instance_layer_names;
            std::vector<const char*> instance_extension_names;
            instance_properties->GetExtensionNamesAndConfigs(instance_layer_names, instance_extension_names);

            // Create the instance.
            VkInstanceCreateInfo instance_info    = {};
            instance_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            instance_info.pNext                   = instance_properties->GetNext();
            instance_info.flags                   = 0;
            instance_info.pApplicationInfo        = &app_info;
            instance_info.enabledLayerCount       = static_cast<uint32_t>(instance_layer_names.size());
            instance_info.ppEnabledLayerNames     = static_cast<uint32_t>(instance_layer_names.size()) ? instance_layer_names.data() : NULL;
            instance_info.enabledExtensionCount   = static_cast<uint32_t>(instance_extension_names.size());
            instance_info.ppEnabledExtensionNames = instance_extension_names.data();

            RraPrint("Instance extension names:");
            for (auto instance_extension_name : instance_extension_names)
            {
                RraPrint("\t%s", instance_extension_name);
            }

            VkResult result = vkCreateInstance(&instance_info, NULL, &instance);
            CheckResult(result, "Failed to create Vulkan instance.");

            if (result == VK_SUCCESS)
            {
                // Initialize the extensions (if they have been enabled successfully).
                ExtDebugReportGetProcAddresses(instance);
                ExtDebugReportOnCreate(instance);
            }

            return instance;
        }

        void DestroyInstance(VkInstance instance)
        {
            vkDestroyInstance(instance, nullptr);
        }
    }  // namespace renderer
}  // namespace rra
