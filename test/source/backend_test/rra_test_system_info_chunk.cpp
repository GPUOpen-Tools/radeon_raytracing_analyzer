//=============================================================================
// Copyright (c) 2024-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test system info implementation.
//=============================================================================

#include "rra_test_system_info_chunk.h"

#include <stdint.h>
#include <string.h>  // For strstr.
#include <string>

#include "public/rra_system_info.h"

namespace backend_test
{
    RRATestSystemInfoChunk::RRATestSystemInfoChunk(const std::string& test_name)
        : RRATestBase(test_name)
    {
    }

    RRATestSystemInfoChunk::~RRATestSystemInfoChunk()
    {
    }

    bool RRATestSystemInfoChunk::RunTests(const Log& log)
    {
        AddTestStartMessage(log);

        bool result = true;

        if (RraSystemInfoAvailable())
        {
            log.Write(" CPU Name: %s", RraSystemInfoGetCpuName());

            TestName(log, &result);
            TestMemoryType(log, &result);

            log.Write(" Driver packaging version: %s", RraSystemInfoGetDriverPackagingVersion());
            log.Write(" Driver software version: %s", RraSystemInfoGetDriverSoftwareVersion());
        }
        else
        {
            log.Write(" SystemInfo not available");
        }

        AddTestResultMessage(result, log);

        return result;
    }

    void RRATestSystemInfoChunk::TextUintValue(const char* text, const char* units, uint32_t value, uint32_t limit, const Log& log, bool* result)
    {
        if (value >= limit)
        {
            log.Write(" %s: %d %s", text, value, units);
        }
        else
        {
            log.Write(" ERROR: %s reported as %d %s", text, value, units);
            *result = false;
        }
    }

    void RRATestSystemInfoChunk::TestName(const Log& log, bool* result)
    {
        const char* device_name = RraSystemInfoGetGpuName();
        if (device_name == nullptr)
        {
            log.Write(" ERROR: GPU name is NULL");
            *result = false;
        }
        else
        {
            // Look for 'unknown' and 'ATI' in the device string. The 'U' of unknown is deliberately ignored due to strstr being case-sensitive.
            if (strstr(device_name, "nknown") == nullptr && strstr(device_name, "ATI") == nullptr)
            {
                log.Write(" GPU name: %s", device_name);
            }
            else
            {
                log.Write(" ERROR: name '%s' is invalid", device_name);
                *result = false;
            }
        }
    }

    void RRATestSystemInfoChunk::TestMemoryType(const Log& log, bool* result)
    {
        const char* memory_type = RraSystemInfoGetGpuMemoryType();

        if (memory_type == nullptr)
        {
            log.Write(" ERROR: GPU memory type is NULL");
            *result = false;
        }
        else
        {
            if (memory_type[0] != '\0')
            {
                log.Write(" memory type: %s", memory_type);
            }
            else
            {
                log.Write(" ERROR: memory type is null");
                *result = false;
            }
        }
    }

}  // namespace backend_test

