//=============================================================================
// Copyright (c) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the public System info interface.
//=============================================================================

#ifndef RRA_BACKEND_PUBLIC_RRA_SYSTEM_INFO_H_
#define RRA_BACKEND_PUBLIC_RRA_SYSTEM_INFO_H_

#include <stdbool.h>
#include <stdint.h>

#include "public/rra_error.h"

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

/// @brief Get the CPU name as a string.
///
/// @return The CPU name.
const char* RraSystemInfoGetCpuName();

/// @brief Get the CPU clock speed.
///
/// @return The CPU clock speed, in MHz.
RraErrorCode RraSystemInfoGetCpuClockSpeed(uint32_t* out_clock_speed);

/// @brief Get the number of physical CPU cores.
///
/// @param [out] The number of physical cores.
///
/// @return kRraOk if successful or error code if not.
RraErrorCode RraSystemInfoGetCpuPhysicalCores(uint32_t* out_cores);

/// @brief Get the number of logical CPU cores.
///
/// @param [out] The number of logical cores.
///
/// @return kRraOk if successful or error code if not.
RraErrorCode RraSystemInfoGetCpuLogicalCores(uint32_t* out_cores);

/// @brief Get the amout of system memory.
///
/// @param [out] The amount od system memory, in bytes.
///
/// @return kRraOk if successful or error code if not.
RraErrorCode RraSystemInfoGetSystemMemorySize(uint64_t* out_memory_size);

/// @brief Get the type of system memory, as a string.
///
/// @return  The system memory type.
const char* RraSystemInfoGetSystemMemoryType();

/// @brief Get the operating system name, as a string.
///
/// @return  The operating system name.
const char* RraSystemInfoGetOsName();

/// @brief Get the driver packaging version, as a string.
///
/// @return The driver packaging version.
const char* RraSystemInfoGetDriverPackagingVersion();

/// @brief Get the driver software version, as a string.
///
/// @return The driver software version.
const char* RraSystemInfoGetDriverSoftwareVersion();

/// @brief Get the GPU name as a string.
///
/// @return The GPU name, or nullptr if error.
const char* RraSystemInfoGetGpuName();

/// @brief Get the type of video memory, as a string.
///
/// @return  The video memory type, or nullptr if error.
const char* RraSystemInfoGetGpuMemoryType();

/// @brief Is the system info data available.
///
/// @return true if system info is available, false if not.
bool RraSystemInfoAvailable();

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus
#endif  // RRA_BACKEND_PUBLIC_RRA_SYSTEM_INFO_H_

