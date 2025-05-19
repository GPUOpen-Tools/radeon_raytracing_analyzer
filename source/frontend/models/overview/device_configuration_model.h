//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the Device configuration model.
//=============================================================================

#ifndef RRA_MODELS_OVERVIEW_DEVICE_CONFIGURATION_MODEL_H_
#define RRA_MODELS_OVERVIEW_DEVICE_CONFIGURATION_MODEL_H_

#include "qt_common/utils/model_view_mapper.h"

namespace rra
{
    /// @brief An enum of widgets used by the UI and model.
    ///
    /// Used to map UI widgets to their corresponding model data.
    enum DeviceConfigurationWidgets
    {
        kDeviceConfigurationCPUName,
        kDeviceConfigurationCPUSpeed,
        kDeviceConfigurationCPUPhysicalCores,
        kDeviceConfigurationCPULogicalCores,
        kDeviceConfigurationSystemMemorySize,

        kDeviceConfigurationApiName,
        kDeviceConfigurationRaytracingVersion,
        kDeviceConfigurationDeviceName,
        kDeviceConfigurationDeviceID,
        kDeviceConfigurationMemorySize,
        kDeviceConfigurationShaderCoreClockFrequency,
        kDeviceConfigurationMemoryClockFrequency,
        kDeviceConfigurationLocalMemoryBandwidth,
        kDeviceConfigurationLocalMemoryType,
        kDeviceConfigurationLocalMemoryBusWidth,

        kDeviceConfigurationDriverPackagingVersion,
        kDeviceConfigurationDriverSoftwareVersion,

        kDeviceConfigurationNumWidgets
    };

    /// @brief Container class that holds model data for the Device configuration pane.
    class DeviceConfigurationModel : public ModelViewMapper
    {
    public:
        /// @brief Constructor.
        explicit DeviceConfigurationModel();

        /// @brief Destructor.
        virtual ~DeviceConfigurationModel();

        /// @brief Initialize blank data for the model.
        void ResetModelValues();

        /// @brief Update the model with data from the back end.
        void Update();

        /// @brief Is the SystemInfo chunk available.
        ///
        /// @return true if SystemInfo available, false if not.
        bool SystemInfoAvailable();

        /// @brief Does the UI need to show the driver software version.
        ///
        /// This will be true if the loaded scene comes from a Windows
        /// machine.
        ///
        /// @return true if the driver software version is to be shown.
        bool IsDriverSoftwareVersionNeeded();

    private:
        /// @brief Update the device information from the system info chunk.
        void UpdateSystemInfo();

        /// @brief Update the device information from the ASIC info chunk.
        void UpdateAsicInfo();
    };

}  // namespace rra

#endif  // RRA_MODELS_OVERVIEW_DEVICE_CONFIGURATION_MODEL_H_

