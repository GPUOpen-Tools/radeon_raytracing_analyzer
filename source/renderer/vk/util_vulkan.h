//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for Vulkan utility functions.
//=============================================================================

#ifndef RRA_RENDERER_VULKAN_UTIL_VULKAN_H_
#define RRA_RENDERER_VULKAN_UTIL_VULKAN_H_

#include <volk/volk.h>
#include <string>

namespace rra
{
    namespace renderer
    {
        class Device;

        /// @brief Check that a Vulkan result code is successful, and log any failures.
        ///
        /// @param [in] result The result code to check for success.
        /// @param [in] failure_message The message string logged if a failure occurs.
        void CheckResult(VkResult result, const char* failure_message);

        /// @brief Get an error message based on VKResult
        ///
        /// @param [in] result The result code to generate a message.
        /// @param [in] failure_message The message string logged if a failure occurs.
        std::string GetVulkanErrorMessage(VkResult result, const char* failure_message);

        /// Load the given shader binary file and create a shader module creation info structure.
        ///
        /// @param [in] shader_file_path The full file path to the SPV shader binary.
        /// @param [in] device The device to use to create the shader module.
        /// @param [in] stage The target pipeline stage that the module will be used for.
        /// @param [in] function_name The name of the function for the shader stage.
        /// @param [out] shader_stage_info The full pipeline shader stage create info structure.
        void LoadShader(const char*                      shader_file_path,
                        Device*                          device,
                        VkShaderStageFlagBits            stage,
                        const char*                      function_name,
                        VkPipelineShaderStageCreateInfo& shader_stage_info);
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VULKAN_UTIL_VULKAN_H_
