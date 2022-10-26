//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the API info interface.
//=============================================================================

#include "public/rra_api_info.h"

#include "rra_data_set.h"

// External reference to the global dataset.
extern RraDataSet data_set_;

const char* RraApiInfoGetApiName()
{
    return data_set_.api_info.GetApiName();
}

bool RraApiInfoIsVulkan()
{
    return data_set_.api_info.IsVulkan();
}
