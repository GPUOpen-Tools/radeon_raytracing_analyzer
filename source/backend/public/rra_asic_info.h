//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the public ASIC info interface.
//=============================================================================

#ifndef RRA_BACKEND_PUBLIC_RRA_ASIC_INFO_H_
#define RRA_BACKEND_PUBLIC_RRA_ASIC_INFO_H_

#include <stdbool.h>
#include <stdint.h>

#include "public/rra_error.h"

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

/// @brief Get the GPU Device name.
///
/// @return Pointer to a string containing the device string, or nullptr if invalid.
const char* RraAsicInfoGetDeviceName();

/// @brief Get the device ID.
///
/// @param [out] A variable to receive the device ID.
///
/// @return kRraOk if successful or error code if not.
RraErrorCode RraAsicInfoGetDeviceID(int32_t* device_id);

/// @brief Check if the device belongs to Navi 3 family.
///
/// @param [out] A variable to receive true or false value.
///
/// @return kRraOk if successful or error code if not.
RraErrorCode RraAsicInfoIsDeviceNavi3(bool* out_is_navi_3);

/// @brief Get the device revision ID.
///
/// @param [out] A variable to receive the device revision ID.
///
/// @return kRraOk if successful or error code if not.
RraErrorCode RraAsicInfoGetDeviceRevisionID(int32_t* device_revision_id);

/// @brief Get the shader core clock frequency, in MHz.
///
/// @param [out] A variable to receive the shader clock frequency.
///
/// @return kRraOk if successful or error code if not.
RraErrorCode RraAsicInfoGetShaderCoreClockFrequency(uint64_t* out_clk_frequency);

/// @brief Get the maximum shader core clock frequency, in MHz.
///
/// @param [out] A variable to receive the shader clock frequency.
///
/// @return kRraOk if successful or error code if not.
RraErrorCode RraAsicInfoGetMaxShaderCoreClockFrequency(uint64_t* out_clk_frequency);

/// @brief Get the Video RAM size, in bytes.
///
/// @param [out] A variable to receive the video RAM size.
///
/// @return kRraOk if successful or error code if not.
RraErrorCode RraAsicInfoGetVRAMSize(int64_t* out_vram_size);

/// @brief Get the memory clock frequency, in MHz.
///
/// @param [out] A variable to receive the memory clock frequency.
///
/// @return kRraOk if successful or error code if not.
RraErrorCode RraAsicInfoGetMemoryClockFrequency(uint64_t* out_clk_frequency);

/// @brief Get the maximum memory clock frequency, in MHz.
///
/// @param [out] A variable to receive the memory clock frequency.
///
/// @return kRraOk if successful or error code if not.
RraErrorCode RraAsicInfoGetMaxMemoryClockFrequency(uint64_t* out_clk_frequency);

/// @brief Get the video memory bandwidth, in bytes per second.
///
/// @param [out] A variable to receive the memory bandwidth.
///
/// @return kRraOk if successful or error code if not.
RraErrorCode RraAsicInfoGetVideoMemoryBandwidth(uint64_t* out_memory_bandwidth);

/// @brief Get the video memory type as a string.
///
/// @return Pointer to the video memory type string, or nullptr if invalid.
const char* RraAsicInfoGetVideoMemoryType();

/// @brief Get the video memory bus width, in bits.
///
/// @param [out] A variable to receive the memory bus width.
///
/// @return kRraOk if successful or error code if not.
RraErrorCode RraAsicInfoGetVideoMemoryBusWidth(int32_t* out_bus_width);

/// @brief Get the ray tracing binary version.
///
/// @param [out] out_version_major A variable to receive the ray tracing major version.
/// @param [out] out_version_minor A variable to receive the ray tracing minor version.
///
/// @return kRraOk if successful or error code if not.
RraErrorCode RraAsicInfoGetRaytracingVersion(uint16_t* out_version_major, uint16_t* out_version_minor);

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus
#endif  // RRA_BACKEND_PUBLIC_RRA_ASIC_INFO_H_
