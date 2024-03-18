//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the Vulkan device properties type.
//=============================================================================

#ifndef RRA_RENDERER_VK_FRAMEWORK_DEVICE_PROPERTIES_H_
#define RRA_RENDERER_VK_FRAMEWORK_DEVICE_PROPERTIES_H_

#include <vector>

#include <volk/volk.h>

namespace rra
{
    namespace renderer
    {
        /// @brief Declaration of the device properties type.
        ///
        /// This object simplifies querying and management of device extensions.
        class DeviceProperties
        {
        public:
            /// @brief Initialize the device properties instance.
            ///
            /// @param [in] physical_device The physical device to initialize with.
            ///
            /// @returns The initialization result code.
            VkResult Initialize(VkPhysicalDevice physical_device);

            /// @brief Add the named device extension to the set of extensions to enable on device creation.
            ///
            /// @param [in] device_extension_name The device extension name.
            ///
            /// @returns True if the named extension was successfully added to the list of extensions to enable.
            bool AddDeviceExtensionName(const char* device_extension_name);

            /// @brief Retrieve a handle to the physical device.
            ///
            /// @returns A handle to the physical device.
            VkPhysicalDevice GetPhysicalDevice() const;

            /// @brief Populate a vector with the names of device extensions to enable.
            ///
            /// @param [out] device_extension_names The vector to be populated with device extension names.
            void GetExtensionNamesAndConfigs(std::vector<const char*>& device_extension_names);

        private:
            /// @brief Check if the given extension is supported on the current device.
            ///
            /// @param [in] device_extension_name The extension name string.
            ///
            /// @returns True if the named device extension is supported, or false if it's not.
            bool IsExtensionPresent(const char* device_extension_name);

            VkPhysicalDevice                   physical_device_{};              ///< The physical device handle.
            std::vector<const char*>           device_extension_names_{};       ///< The set of device extensions to enable.
            std::vector<VkExtensionProperties> device_extension_properties_{};  ///< The set of device extension properties.
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_FRAMEWORK_DEVICE_PROPERTIES_H_
