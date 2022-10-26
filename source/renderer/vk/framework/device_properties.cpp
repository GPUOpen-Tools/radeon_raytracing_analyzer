//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the Vulkan device properties type.
//=============================================================================

#include <algorithm>
#include <stdio.h>
#include <string.h>

#include "public/rra_print.h"

#include "device_properties.h"
#include "../util_vulkan.h"

namespace rra
{
    namespace renderer
    {
        VkResult DeviceProperties::Initialize(VkPhysicalDevice physical_device)
        {
            physical_device_ = physical_device;

            // Enumerate device extensions.
            uint32_t extension_count;
            VkResult result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);
            CheckResult(result, "Failed to enumerate device extensions.");

            device_extension_properties_.resize(extension_count);
            result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, device_extension_properties_.data());
            CheckResult(result, "Failed to populate device extensions array.");

            return result;
        }

        bool DeviceProperties::AddDeviceExtensionName(const char* device_extension_name)
        {
            if (IsExtensionPresent(device_extension_name))
            {
                device_extension_names_.push_back(device_extension_name);
                return true;
            }

            RraPrint("The device extension '%s' is not supported.", device_extension_name);

            return false;
        }

        VkPhysicalDevice DeviceProperties::GetPhysicalDevice() const
        {
            return physical_device_;
        }

        void DeviceProperties::GetExtensionNamesAndConfigs(std::vector<const char*>& device_extension_names)
        {
            for (auto& name : device_extension_names_)
            {
                device_extension_names.push_back(name);
            }
        }

        bool DeviceProperties::IsExtensionPresent(const char* device_extension_name)
        {
            return std::find_if(device_extension_properties_.begin(),
                                device_extension_properties_.end(),
                                [device_extension_name](const VkExtensionProperties& extension_properties) -> bool {
                                    return strcmp(extension_properties.extensionName, device_extension_name) == 0;
                                }) != device_extension_properties_.end();
        }
    }  // namespace renderer
}  // namespace rra
