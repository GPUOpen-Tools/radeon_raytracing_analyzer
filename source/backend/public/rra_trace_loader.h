//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the trace loader interface.
//=============================================================================

#ifndef RRA_BACKEND_PUBLIC_RRA_TRACE_LOADER_H_
#define RRA_BACKEND_PUBLIC_RRA_TRACE_LOADER_H_

#include <time.h>

#include "public/rra_error.h"

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

/// @brief Load a trace file.
///
/// @param [in] trace_file_name The name of the trace file to load.
///
/// @return kRraOk if trace file loaded OK, an RraErrorCode if an error occurred.
RraErrorCode RraTraceLoaderLoad(const char* trace_file_name);

/// @brief Unload (close) a trace file.
void RraTraceLoaderUnload();

/// @brief Is the trace data valid (has a trace been loaded).
///
/// @return true if trace file is valid, false if not.
bool RraTraceLoaderValid();

/// @brief Get the time the trace was created.
///
/// @return A time_t structure containing the time the trace was created.
time_t RraTraceLoaderGetCreateTime();

/// @brief Allocate memory and copy the Driver Overrides JSON string to the data set.
///
/// @param [in]  driver_overrides_string                    The JSON text to copy.
/// @param [in]  length                                     The length of the JSON text to copy.
///
/// @returns
/// kRmtOk                                      The operation completed successfully.
/// @retval
/// kRmtErrorInvalidPointer                     The operation failed due to <c><i>driver_overrides_string</i></c> being <c><i>NULL</i></c>.
/// @retval
/// kRmtErrorOutOfMemory                        The operation failed due to lack of free memory.
RraErrorCode RraTraceLoaderCopyDriverOverridesString(const char* driver_overrides_string, size_t length);

/// @brief  Get the Driver Overrides JSON string from the data set.
///
/// @returns                                    The Driver Overrides JSON text.
char* RraTraceLoaderGetDriverOverridesString();

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus
#endif  // RRA_BACKEND_PUBLIC_RRA_TRACE_LOADER_H_

