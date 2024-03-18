//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the Vulkan instance properties object.
//=============================================================================

#include <algorithm>
#include <cassert>
#include <string.h>

#include "instance_properties.h"

#include "../util_vulkan.h"

#include "public/rra_print.h"

namespace rra
{
    namespace renderer
    {
        VkResult InstanceProperties::Initialize()
        {
            // Query instance layers.
            uint32_t instance_layer_property_count = 0;

            VkResult result = vkEnumerateInstanceLayerProperties(&instance_layer_property_count, nullptr);
            CheckResult(result, "Failed to enumerate instance layer properties.");
            instance_layer_properties_.resize(instance_layer_property_count);

            if (instance_layer_property_count > 0)
            {
                result = vkEnumerateInstanceLayerProperties(&instance_layer_property_count, instance_layer_properties_.data());
                CheckResult(result, "Failed to populate instance layer properties array.");
            }

            // Query instance extensions.
            uint32_t instance_extension_property_count = 0;

            result = vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_property_count, nullptr);
            CheckResult(result, "Failed to enumerate instance extension properties.");
            instance_extension_properties_.resize(instance_extension_property_count);

            if (instance_extension_property_count > 0)
            {
                result = vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_property_count, instance_extension_properties_.data());
                CheckResult(result, "Failed to populate instance extension properties array.");
            }

            return result;
        }

        bool InstanceProperties::AddInstanceLayerName(const char* instance_layer_name)
        {
            if (IsLayerPresent(instance_layer_name))
            {
                instance_layer_names_.push_back(instance_layer_name);
                return true;
            }

            RraPrint("The instance layer '%s' has not been found.\n", instance_layer_name);

            return false;
        }

        bool InstanceProperties::AddInstanceExtensionName(const char* instance_extension_name)
        {
            if (IsExtensionPresent(instance_extension_name))
            {
                instance_extension_names_.push_back(instance_extension_name);
                return true;
            }

            RraPrint("The instance extension '%s' has not been found.\n", instance_extension_name);

            return false;
        }

        void InstanceProperties::GetExtensionNamesAndConfigs(std::vector<const char*>& instance_layer_names, std::vector<const char*>& instance_extension_names)
        {
            for (auto& name : instance_layer_names_)
            {
                instance_layer_names.push_back(name);
            }

            for (auto& name : instance_extension_names_)
            {
                instance_extension_names.push_back(name);
            }
        }

        void* InstanceProperties::GetNext() const
        {
            return next_;
        }

        void InstanceProperties::SetNewNext(void* next)
        {
            next_ = next;
        }

        bool InstanceProperties::IsLayerPresent(const char* layer_name)
        {
            return std::find_if(
                       instance_layer_properties_.begin(), instance_layer_properties_.end(), [layer_name](const VkLayerProperties& layerProps) -> bool {
                           return strcmp(layerProps.layerName, layer_name) == 0;
                       }) != instance_layer_properties_.end();
        }

        bool InstanceProperties::IsExtensionPresent(const char* extension_name)
        {
            return std::find_if(instance_extension_properties_.begin(),
                                instance_extension_properties_.end(),
                                [extension_name](const VkExtensionProperties& extension_properties) -> bool {
                                    return strcmp(extension_properties.extensionName, extension_name) == 0;
                                }) != instance_extension_properties_.end();
        }
    }  // namespace renderer
}  // namespace rra
