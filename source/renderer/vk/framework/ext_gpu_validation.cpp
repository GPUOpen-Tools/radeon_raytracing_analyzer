//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for Vulkan GPU validation extensions functionality.
//=============================================================================

#include "ext_gpu_validation.h"

#include "public/rra_print.h"

#include <volk/volk.h>

namespace rra
{
    namespace renderer
    {
        static VkValidationFeaturesEXT      validation_features_ext       = {VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
        static VkValidationFeatureEnableEXT enabled_validation_features[] = {VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT};

        bool ExtGPUValidationCheckExtensions(InstanceProperties* instance_properties)
        {
            std::vector<const char*> required_extension_names = {VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME};

            for (auto& ext : required_extension_names)
            {
                if (instance_properties->AddInstanceExtensionName(ext) == false)
                {
                    RraPrint("GPU validation disabled, missing extension: %s.\n", ext);
                    return false;
                }
            }

            validation_features_ext.enabledValidationFeatureCount = 1;
            validation_features_ext.pEnabledValidationFeatures    = enabled_validation_features;
            validation_features_ext.pNext                         = instance_properties->GetNext();

            instance_properties->SetNewNext(&validation_features_ext);

            return true;
        }

    }  // namespace renderer
}  // namespace rra
