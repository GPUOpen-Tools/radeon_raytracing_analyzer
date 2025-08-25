//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test ASIC chunk implementation.
//=============================================================================

#include "rra_test_asic_chunk.h"

#include "rdf/rdf/inc/amdrdf.h"

#include "public/rra_asic_info.h"
#include "public/rra_macro.h"

#include "rra_test_config.h"

namespace backend_test
{
    RRATestAsicChunk::RRATestAsicChunk(const std::string& test_name)
        : RRATestBase(test_name)
    {
    }

    RRATestAsicChunk::~RRATestAsicChunk()
    {
    }

    bool RRATestAsicChunk::RunTests(const RRATestConfig& config)
    {
        AddTestStartMessage(config.log);

        bool result = true;

        if (TestDeviceName(config.log) == false)
        {
            result = false;
        }

        if (TestClocks(config.log) == false)
        {
            result = false;
        }

        if (TestMemoryBandwidth(config.log) == false)
        {
            result = false;
        }

        AddTestResultMessage(result, config.log);
        return result;
    }

    bool RRATestAsicChunk::TestDeviceName(const Log& log)
    {
        const char* device_name = RraAsicInfoGetDeviceName();
        if (device_name == nullptr)
        {
            log.Write(" ERROR: device_name is NULL");
            return false;
        }

        // Look for 'unknown' and 'ATI' in the device string. The 'U' of unknown is deliberately ignored due to strstr being case-sensitive.
        if (strstr(device_name, "nknown") == nullptr && strstr(device_name, "ATI") == nullptr)
        {
            log.Write(" GPU name: %s", device_name);
        }
        else
        {
            log.Write(" ERROR: name '%s' is invalid", device_name);
            return false;
        }
        return true;
    }

    bool RRATestAsicChunk::TestClocks(const Log& log)
    {
        uint64_t shader_core_clock     = 0;
        uint64_t max_shader_core_clock = 0;
        uint64_t memory_clock          = 0;
        uint64_t max_memory_clock      = 0;

        if (RraAsicInfoGetShaderCoreClockFrequency(&shader_core_clock) != kRraOk)
        {
            log.Write(" ERROR: unable to get shader core clock");
            return false;
        }
        if (RraAsicInfoGetMaxShaderCoreClockFrequency(&max_shader_core_clock) != kRraOk)
        {
            log.Write(" ERROR: unable to get max shader core clock");
            return false;
        }

        if (RraAsicInfoGetMemoryClockFrequency(&memory_clock) != kRraOk)
        {
            log.Write(" ERROR: unable to get memory clock");
            return false;
        }
        if (RraAsicInfoGetMaxMemoryClockFrequency(&max_memory_clock) != kRraOk)
        {
            log.Write(" ERROR: unable to get max memory clock");
            return false;
        }

        log.Write(" Shader clock frequency: %llu MHz", shader_core_clock);
        log.Write(" Max shader clock frequency: %llu MHz", max_shader_core_clock);

        log.Write(" Memory clock frequency: %llu MHz", memory_clock);
        log.Write(" Max memory clock frequency: %llu MHz", max_memory_clock);

        return true;
    }

    bool RRATestAsicChunk::TestMemoryBandwidth(const Log& log)
    {
        uint64_t bandwidth = 0;
        if (RraAsicInfoGetVideoMemoryBandwidth(&bandwidth) != kRraOk)
        {
            log.Write(" ERROR: unable to get memory bandwidth");
            return false;
        }

        // Convert bandwidth from bytes per second to GB per second.
        bandwidth /= (1024 * 1024 * 1024);
        if (bandwidth < 20)
        {
            log.Write(" ERROR: memory bandwidth very low at %llu GB/s", bandwidth);
            return false;
        }
        log.Write(" Memory bandwidth: %llu GB/s", bandwidth);

        return true;
    }

}  // namespace backend_test
