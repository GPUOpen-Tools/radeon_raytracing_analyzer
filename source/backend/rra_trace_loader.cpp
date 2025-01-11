//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
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

RraErrorCode RraTraceLoaderCopyDriverOverridesString(const char* driver_overrides_string, size_t length)
{
    RRA_RETURN_ON_ERROR(driver_overrides_string, kRraErrorInvalidPointer);
    RraErrorCode result = kRraOk;

    delete data_set_.driver_overrides_json_text;

    if ((driver_overrides_string == nullptr) || (length == 0))
    {
        data_set_.driver_overrides_json_text = nullptr;
    }
    else
    {
        data_set_.driver_overrides_json_text = new (std::nothrow) char[length + 1];
        if (data_set_.driver_overrides_json_text != nullptr)
        {
            memcpy(data_set_.driver_overrides_json_text, driver_overrides_string, length);
            data_set_.driver_overrides_json_text[length] = '\0';
        }
        else
        {
            result = kRraErrorOutOfMemory;
        }
    }

    return result;
}

char* RraTraceLoaderGetDriverOverridesString()
{
    return data_set_.driver_overrides_json_text;
}
