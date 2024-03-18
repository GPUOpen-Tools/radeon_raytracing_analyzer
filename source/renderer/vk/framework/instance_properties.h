//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the Vulkan instance properties object.
//=============================================================================

#ifndef RRA_RENDERER_VK_FRAMEWORK_INSTANCE_PROPERTIES_H_
#define RRA_RENDERER_VK_FRAMEWORK_INSTANCE_PROPERTIES_H_

#include <vector>

#include <volk/volk.h>

namespace rra
{
    namespace renderer
    {
        /// @brief Declaration of the InstanceProperties type.
        ///
        /// This class simplifies the management of querying and configuring supported instance layers and extensions.
        class InstanceProperties
        {
        public:
            /// @brief Initialize the Instance properties object.
            ///
            /// @returns The initialization result code.
            VkResult Initialize();

            /// @brief Add the given layer to the list of layers initialized with the instance.
            ///
            /// @param [in] instance_layer_name The instance layer name.
            ///
            /// @returns True when the requested instance layer is supported, and false if it is not.
            bool AddInstanceLayerName(const char* instance_layer_name);

            /// @brief Add the given extension to the list of extensions initialized with the instance.
            ///
            /// @param [in] instance_extension_name The instance extension name.
            ///
            /// @returns True when the requested instance extension is supported, and false if it is not.
            bool AddInstanceExtensionName(const char* instance_extension_name);

            /// @brief Populate the provided vectors with the set of instance layer and extension names.
            ///
            /// @param [in] instance_layer_names The list of instance layer names.
            /// @param [in] instance_extension_names The list of instance extension names.
            void GetExtensionNamesAndConfigs(std::vector<const char*>& instance_layer_names, std::vector<const char*>& instance_extension_names);

            /// @brief Retrieve a pointer to the next VkValidationFeaturesEXT structure in the chain.
            ///
            /// @returns A pointer to the next VkValidationFeaturesEXT structure.
            void* GetNext() const;

            /// @brief Set a pointer to the next VkValidationFeaturesEXT structure in the chain.
            ///
            /// @param [in] next A pointer to the next VkValidationFeaturesEXT structure.
            void SetNewNext(void* next);

        private:
            /// @brief Check if a layer with the given name is supported.
            ///
            /// @param [in] layer_name The name of the layer to check support for.
            ///
            /// @returns True if the layer is supported, and false if it's not.
            bool IsLayerPresent(const char* layer_name);

            /// @brief Check if the extension with the given name is supported.
            ///
            /// @param [in] extension_name The name of the extension to check support for.
            ///
            /// @returns True if the extension is supported, and false if it's not.
            bool IsExtensionPresent(const char* extension_name);

            std::vector<VkLayerProperties>     instance_layer_properties_;      ///< The instance layer properties array.
            std::vector<VkExtensionProperties> instance_extension_properties_;  ///< The instance extension properties array.
            std::vector<const char*>           instance_layer_names_;           ///< The instance layer names array.
            std::vector<const char*>           instance_extension_names_;       ///< The instance extension names array.
            void*                              next_ = nullptr;
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_FRAMEWORK_INSTANCE_PROPERTIES_H_
