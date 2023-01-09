//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the Device configuration model.
//=============================================================================

#include "models/overview/device_configuration_model.h"

#include <QVariant>

#include "public/rra_api_info.h"
#include "public/rra_asic_info.h"

#include "managers/trace_manager.h"
#include "util/string_util.h"
#include "views/widget_util.h"

namespace rra
{
    DeviceConfigurationModel::DeviceConfigurationModel()
        : ModelViewMapper(kDeviceConfigurationNumWidgets)
    {
    }

    DeviceConfigurationModel::~DeviceConfigurationModel()
    {
    }

    void DeviceConfigurationModel::ResetModelValues()
    {
        SetModelData(kDeviceConfigurationApiName, "-");
        SetModelData(kDeviceConfigurationRaytracingVersion, "-");
        SetModelData(kDeviceConfigurationDeviceName, "-");
        SetModelData(kDeviceConfigurationDeviceID, "-");
        SetModelData(kDeviceConfigurationMemorySize, "-");
        SetModelData(kDeviceConfigurationShaderCoreClockFrequency, "-");
        SetModelData(kDeviceConfigurationMemoryClockFrequency, "-");
        SetModelData(kDeviceConfigurationLocalMemoryBandwidth, "-");
        SetModelData(kDeviceConfigurationLocalMemoryType, "-");
        SetModelData(kDeviceConfigurationLocalMemoryBusWidth, "-");
    }

    void DeviceConfigurationModel::Update()
    {
        ResetModelValues();

        const char* api_name = RraApiInfoGetApiName();
        if (api_name != nullptr)
        {
            SetModelData(kDeviceConfigurationApiName, api_name);
        }

        const char* device_name = RraAsicInfoGetDeviceName();
        if (device_name != nullptr)
        {
            SetModelData(kDeviceConfigurationDeviceName, device_name);
        }

        int32_t device_id          = 0;
        int32_t device_revision_id = 0;
        if (RraAsicInfoGetDeviceID(&device_id) == kRraOk && RraAsicInfoGetDeviceRevisionID(&device_revision_id) == kRraOk)
        {
            const uint32_t compound_device_id = ((device_id & 0xffff) << 8) | (device_revision_id & 0xff);
            SetModelData(kDeviceConfigurationDeviceID, rra::string_util::ToUpperCase(QString::number(compound_device_id, 16)).rightJustified(6, '0'));
        }

        int64_t vram_size = 0;
        if (RraAsicInfoGetVRAMSize(&vram_size) == kRraOk)
        {
            SetModelData(kDeviceConfigurationMemorySize, rra::string_util::LocalizedValueMemory(vram_size, false, false));
        }

        uint64_t min_core_clock = 0;
        uint64_t max_core_clock = 0;
        if (RraAsicInfoGetShaderCoreClockFrequency(&min_core_clock) == kRraOk && RraAsicInfoGetMaxShaderCoreClockFrequency(&max_core_clock) == kRraOk)
        {
            SetModelData(kDeviceConfigurationShaderCoreClockFrequency,
                         rra::string_util::LocalizedValue(min_core_clock) + QString(" MHz (min) ") + rra::string_util::LocalizedValue(max_core_clock) +
                             QString(" MHz (max)"));
        }

        uint64_t min_memory_clock = 0;
        uint64_t max_memory_clock = 0;
        if (RraAsicInfoGetMemoryClockFrequency(&min_memory_clock) == kRraOk && RraAsicInfoGetMaxMemoryClockFrequency(&max_memory_clock) == kRraOk)
        {
            SetModelData(kDeviceConfigurationMemoryClockFrequency,
                         rra::string_util::LocalizedValue(min_memory_clock) + QString(" MHz (min) ") + rra::string_util::LocalizedValue(max_memory_clock) +
                             QString(" MHz (max)"));
        }

        uint64_t memory_bandwidth = 0;
        if (RraAsicInfoGetVideoMemoryBandwidth(&memory_bandwidth) == kRraOk)
        {
            SetModelData(kDeviceConfigurationLocalMemoryBandwidth, rra::string_util::LocalizedValueMemory(memory_bandwidth, true, true) + QString("/s"));
        }

        const char* memory_type = RraAsicInfoGetVideoMemoryType();
        if (memory_type != nullptr)
        {
            SetModelData(kDeviceConfigurationLocalMemoryType, memory_type);
        }

        int32_t bus_width = 0;
        if (RraAsicInfoGetVideoMemoryBusWidth(&bus_width) == kRraOk)
        {
            SetModelData(kDeviceConfigurationLocalMemoryBusWidth, QString::number(bus_width) + QString("-bit"));
        }
    }
}  // namespace rra
