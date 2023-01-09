//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the public API info interface.
//=============================================================================

#ifndef RRA_BACKEND_PUBLIC_RRA_API_INFO_H_
#define RRA_BACKEND_PUBLIC_RRA_API_INFO_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

/// @brief Get the name of the API used when taking the trace.
///
/// @return The text string of the node name.
const char* RraApiInfoGetApiName();

/// @brief Get whether or not the captured application uses Vulkan.
/// 
/// @return True if the captured application uses Vulkan, false otherwise.
bool RraApiInfoIsVulkan();

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus
#endif  // RRA_BACKEND_PUBLIC_RRA_API_INFO_H_
