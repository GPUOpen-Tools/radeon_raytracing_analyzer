//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for Vulkan utility functions.
//=============================================================================

#include <array>
#include <fstream>
#include <vector>

#include <QCoreApplication>
#include <QFileInfo>
#include "qt_common/utils/qt_util.h"

#include "util_vulkan.h"

#include "public/rra_assert.h"
#include "public/rra_print.h"

#include "framework/device.h"

#include "vk/vk_graphics_context.h"
#include "vk/framework/ext_debug_utils.h"

namespace rra
{
    namespace renderer
    {
        void CheckResult(VkResult result, const char* failure_message)
        {
            if (result != VK_SUCCESS)
            {
                if (GetVkGraphicsContext()->IsErrorWindowPrimed())
                {
                    // This means that the initial loading was successful. However, something went wrong during rendering.
                    QtCommon::QtUtils::ShowMessageBox(nullptr,
                                                      QMessageBox::Ok,
                                                      QMessageBox::Critical,
                                                      "RRA Has Crashed",
                                                      QString::fromStdString(GetVulkanErrorMessage(result, failure_message)));
                    exit(-1);
                }
                else
                {
                    // This means that the initial loading has failed. We can't show a window here since the initial loading happens in a loading thread.
                    GetVkGraphicsContext()->SetInitializationErrorMessage(GetVulkanErrorMessage(result, failure_message));
                }
            }
        }

        std::string GetVulkanErrorMessage(VkResult result, const char* failure_message)
        {
            std::string error_string = "Vulkan has returned a non VK_SUCCESS code.";

            if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
            {
                error_string =
                    "There is insufficient GPU memory to run this instance of RRA. Please close other instances of RRA or any other applications using the "
                    "GPU and try again.";
            }
            else if (result == VK_ERROR_OUT_OF_HOST_MEMORY || result == VK_ERROR_OUT_OF_POOL_MEMORY)
            {
                error_string =
                    "There is insufficient CPU memory to run this instance of RRA. Please close other instances of RRA or any other applications using the "
                    "CPU and try again.";
            }

            error_string += "\n\nAdditional information : " + std::string(failure_message) + " - VkResult(" + std::to_string(result) + ") ";
            return error_string;
        }

        void LoadShader(const char*                      shader_file_path,
                        Device*                          device,
                        VkShaderStageFlagBits            stage,
                        const char*                      function_name,
                        VkPipelineShaderStageCreateInfo& shader_stage_info)
        {
            // The shader binaries can be found in a 'shaders' directory living alongside the application executable.
            QString application_path = QCoreApplication::applicationDirPath();

            QString full_shader_path;
            full_shader_path.append(application_path);
            full_shader_path.append("/shaders/");
            full_shader_path.append(shader_file_path);

            QFile shader_source_stream(full_shader_path);
            VkShaderModule shader_module = VK_NULL_HANDLE;
            if (shader_source_stream.open(QIODevice::ReadOnly))
            {
                qint64 size = shader_source_stream.size();
                char* shader_code = new char[size];
                shader_source_stream.read(shader_code, size);
                shader_source_stream.close();

                assert(size > 0);

                VkShaderModuleCreateInfo module_create_info = {};
                module_create_info.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                module_create_info.codeSize                 = size;
                module_create_info.pCode                    = reinterpret_cast<uint32_t*>(shader_code);

                VkResult create_result = vkCreateShaderModule(device->GetDevice(), &module_create_info, nullptr, &shader_module);
                CheckResult(create_result, "Shader module creation failed.");
                SetObjectName(device->GetDevice(), VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)shader_module, shader_file_path);

                delete[] shader_code;
            }

            shader_stage_info        = {};
            shader_stage_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shader_stage_info.stage  = stage;
            shader_stage_info.module = shader_module;
            shader_stage_info.pName  = function_name;

            assert(shader_stage_info.module != VK_NULL_HANDLE);
        }
    }  // namespace renderer
}  // namespace rra
