//=============================================================================
// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the System info interface.
//=============================================================================

#include "public/rra_system_info.h"

#include "rra_data_set.h"

// External reference to the global dataset.
extern RraDataSet data_set_;

static const int active_gpu = 0;
static const int active_cpu = 0;

const char* RraSystemInfoGetCpuName()
{
    return data_set_.system_info->cpus[active_cpu].name.c_str();
}

RraErrorCode RraSystemInfoGetCpuClockSpeed(uint32_t* out_clock_speed)
{
    *out_clock_speed = data_set_.system_info->cpus[active_cpu].max_clock_speed;
    return kRraOk;
}

RraErrorCode RraSystemInfoGetCpuPhysicalCores(uint32_t* out_cores)
{
    *out_cores = data_set_.system_info->cpus[active_cpu].num_physical_cores;
    return kRraOk;
}

RraErrorCode RraSystemInfoGetCpuLogicalCores(uint32_t* out_cores)
{
    *out_cores = data_set_.system_info->cpus[active_cpu].num_logical_cores;
    return kRraOk;
}

RraErrorCode RraSystemInfoGetSystemMemorySize(uint64_t* out_memory_size)
{
    *out_memory_size = data_set_.system_info->os.memory.physical;
    return kRraOk;
}

const char* RraSystemInfoGetSystemMemoryType()
{
    return data_set_.system_info->os.memory.type.c_str();
}

const char* RraSystemInfoGetDriverPackagingVersion()
{
    return data_set_.system_info->driver.packaging_version.c_str();
}

const char* RraSystemInfoGetDriverSoftwareVersion()
{
    return data_set_.system_info->driver.software_version.c_str();
}

bool RraSystemInfoAvailable()
{
    return data_set_.system_info->version.major > 0;
}
