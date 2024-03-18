//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the trace loader interface.
//=============================================================================

#include "public/rra_trace_loader.h"

#include <string.h>

#include "rra_data_set.h"

/// The one and only instance of the data set, which is initialized when loading in
/// a trace file.
RraDataSet data_set_ = {};

RraErrorCode RraTraceLoaderLoad(const char* trace_file_name)
{
    RraErrorCode error_code = RraDataSetInitialize(trace_file_name, &data_set_);

    if (error_code != kRraOk)
    {
        data_set_ = {};
    }
    return error_code;
}

void RraTraceLoaderUnload()
{
    if (RraTraceLoaderValid())
    {
        RraDataSetDestroy(&data_set_);
    }
    data_set_ = {};
}

bool RraTraceLoaderValid()
{
    return data_set_.file_loaded;
}

time_t RraTraceLoaderGetCreateTime()
{
    return data_set_.create_time;
}
