//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the public raytracing IP level info interface.
//=============================================================================

#ifndef RRA_BACKEND_PUBLIC_RRA_RTIP_INFO_H_
#define RRA_BACKEND_PUBLIC_RRA_RTIP_INFO_H_

#include <stdint.h>

#include "bvh/gpu_def.h"

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

/// @brief Get the ray tracing IP level that was used when the application was captured.
///
/// @return The RtIp level.
uint32_t RraRtipInfoGetRaytracingIpLevel();

/// @brief Get whether or not the RtIp level of this capture supports oriented bounding boxes.
///
/// @return True if the current RtIp level supports oriented bounding boxes.
bool RraRtipInfoGetOBBSupported();

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus
#endif  // RRA_BACKEND_PUBLIC_RRA_RTIP_INFO_H_

