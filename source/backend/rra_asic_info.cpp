//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the ASIC info interface.
//=============================================================================

#include "public/rra_asic_info.h"

#include "bvh/rtip11/encoded_rt_ip_11_top_level_bvh.h"
#include "rra_data_set.h"
#include "rra_tlas_impl.h"

// External reference to the global dataset.
extern RraDataSet data_set_;

const char* RraAsicInfoGetDeviceName()
{
    return data_set_.asic_info.GetDeviceName();
}

RraErrorCode RraAsicInfoGetDeviceID(int32_t* device_id)
{
    return data_set_.asic_info.GetDeviceID(device_id);
}

RraErrorCode RraAsicInfoIsDeviceNavi3(bool* out_is_navi_3)
{
    uint16_t     major_number;
    RraErrorCode result = data_set_.asic_info.GetGfxIpLevelMajor(&major_number);
    if (result == kRraOk)
    {
        *out_is_navi_3 = (major_number == 11);
    }
    return result;
}

RraErrorCode RraAsicInfoGetDeviceRevisionID(int32_t* device_revision_id)
{
    return data_set_.asic_info.GetDeviceRevisionID(device_revision_id);
}

RraErrorCode RraAsicInfoGetShaderCoreClockFrequency(uint64_t* out_clk_frequency)
{
    RraErrorCode result = data_set_.asic_info.GetShaderCoreClockFrequency(out_clk_frequency);
    if (result == kRraOk)
    {
        *out_clk_frequency /= 1000000;
    }
    return result;
}

RraErrorCode RraAsicInfoGetMaxShaderCoreClockFrequency(uint64_t* out_clk_frequency)
{
    RraErrorCode result = data_set_.asic_info.GetMaxShaderCoreClockFrequency(out_clk_frequency);
    if (result == kRraOk)
    {
        *out_clk_frequency /= 1000000;
    }
    return result;
}

RraErrorCode RraAsicInfoGetVRAMSize(int64_t* out_vram_size)
{
    return data_set_.asic_info.GetVRAMSize(out_vram_size);
}

RraErrorCode RraAsicInfoGetMemoryClockFrequency(uint64_t* out_clk_frequency)
{
    RraErrorCode result = data_set_.asic_info.GetMemoryClockFrequency(out_clk_frequency);
    if (result == kRraOk)
    {
        *out_clk_frequency /= 1000000;
    }
    return result;
}

RraErrorCode RraAsicInfoGetMaxMemoryClockFrequency(uint64_t* out_clk_frequency)
{
    RraErrorCode result = data_set_.asic_info.GetMaxMemoryClockFrequency(out_clk_frequency);
    if (result == kRraOk)
    {
        *out_clk_frequency /= 1000000;
    }
    return result;
}

RraErrorCode RraAsicInfoGetVideoMemoryBandwidth(uint64_t* out_memory_bandwidth)
{
    return data_set_.asic_info.GetVideoMemoryBandwidth(out_memory_bandwidth);
}

const char* RraAsicInfoGetVideoMemoryType()
{
    return data_set_.asic_info.GetVideoMemoryType();
}

RraErrorCode RraAsicInfoGetVideoMemoryBusWidth(int32_t* out_bus_width)
{
    return data_set_.asic_info.GetVideoMemoryBusWidth(out_bus_width);
}

RraErrorCode RraAsicInfoGetRaytracingVersion(uint16_t* out_version_major, uint16_t* out_version_minor)
{
    const rta::IBvh* tlas = RraTlasGetTlasFromTlasIndex(0);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const rta::IRtIpCommonAccelerationStructureHeader& header  = tlas->GetHeader();
    rta::RayTracingBinaryVersion                       version = header.GetGpuRtDriverInterfaceVersion();

    *out_version_major = version.GetMajor();
    *out_version_minor = version.GetMinor();

    return kRraOk;
}

