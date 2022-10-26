//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for Vulkan GPU validation extensions functionality.
//=============================================================================

#ifndef RRA_RENDERER_VK_FRAMEWORK_EXT_GPU_VALIDATION_H_
#define RRA_RENDERER_VK_FRAMEWORK_EXT_GPU_VALIDATION_H_

#include "device_properties.h"
#include "instance_properties.h"

namespace rra
{
    namespace renderer
    {
        /// @brief Initialize the GPU validation extensions if they're available.
        ///
        /// @param [in] instance_properties The instance properties object.
        ///
        /// @returns True if the GPU validation extensions are supported, and false if not.
        bool ExtGPUValidationCheckExtensions(InstanceProperties* instance_properties);
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_FRAMEWORK_EXT_GPU_VALIDATION_H_
