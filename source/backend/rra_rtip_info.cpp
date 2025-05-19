//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the raytracing IP level info interface.
//=============================================================================

#include "public/rra_rtip_info.h"

#include "rra_data_set.h"

// External reference to the global dataset.
extern RraDataSet data_set_;

uint32_t RraRtipInfoGetRaytracingIpLevel()
{
    return (uint32_t)data_set_.rtip_level;
}

bool RraRtipInfoGetOBBSupported()
{
    if (data_set_.rtip_level == rta::RayTracingIpLevel::RtIp3_1)
    {
        return true;
    }
    return false;
}

