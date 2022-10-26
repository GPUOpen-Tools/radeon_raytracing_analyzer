//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the trace loader interface.
//=============================================================================

#ifndef RRA_BACKEND_PUBLIC_RRA_TRACE_LOADER_H_
#define RRA_BACKEND_PUBLIC_RRA_TRACE_LOADER_H_

#include <time.h>

#include "rra_error.h"

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

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus
#endif  // RRA_BACKEND_PUBLIC_RRA_TRACE_LOADER_H_
