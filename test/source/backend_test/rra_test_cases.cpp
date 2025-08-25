//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test cases implementation.
//=============================================================================

#include "rra_test_cases.h"

#include <string.h>  // for memset

#include "public/rra_trace_loader.h"

#include "rra_test_asic_chunk.h"
#include "rra_test_blas.h"
#include "rra_test_file_chunks.h"
#include "rra_test_hlsl.h"
#include "rra_test_ray_history.h"
#include "rra_test_system_info_chunk.h"
#include "rra_test_tlas.h"
#include "rra_test_tlas_blas_mapping.h"

RRATestCases* RRATestCases::instance_ = nullptr;

RRATestCases::RRATestCases()
    : config_{}
{
}

RRATestCases::~RRATestCases()
{
    delete instance_;
}

RRATestCases* RRATestCases::Get()
{
    if (instance_ == nullptr)
    {
        instance_ = new RRATestCases();
    }
    return instance_;
}

bool RRATestCases::Initialize(const RRATestConfig& config)
{
    config_ = config;

    // Initialize the log file.
    config_.log.Open(config.trace_file_name.c_str(), "_test.log");

    // Check the rra file has been specified.
    if (config.trace_file_name.empty())
    {
        config_.log.WriteConsole("ERROR: Missing path to the trace file");
        return false;
    }

    // Initialize the data set.
    RraErrorCode error_code = RraTraceLoaderLoad(config.trace_file_name.c_str());

    if (error_code == kRraErrorNoASChunks)
    {
        config_.log.WriteConsole("ERROR: No acceleration structures found in the trace file");
        return false;
    }
    else if (error_code == kRraErrorMalformedData)
    {
        config_.log.WriteConsole("ERROR: The trace file contains malformed data (missing BVH data, possible buffer reuse)");
        return false;
    }
    else if (error_code == kRraMajorVersionIncompatible)
    {
        config_.log.WriteConsole("ERROR: The trace file contains structures with major version incompatibility.");
        return false;
    }
    else if (error_code != kRraOk)
    {
        config_.log.WriteConsole("ERROR: The trace contains malformed data");
        return false;
    }

    return true;
}

void RRATestCases::Log(const char* log_message, ...)
{
    config_.log.Write(log_message);
}

void RRATestCases::Shutdown()
{
    RraTraceLoaderUnload();
}

bool RRATestCases::TestFileChunks(const char* test_name) const
{
    backend_test::RRATestFileChunks file_chunks_test(test_name);
    bool                            result = file_chunks_test.RunTests(config_);
    return result;
}

bool RRATestCases::TestTlasBlasMapping(const char* test_name) const
{
    backend_test::RRATestTlasBlasMapping mapping_test(test_name);
    bool                                 result = mapping_test.RunTests(config_);
    return result;
}

bool RRATestCases::TestHlsl(const char* test_name) const
{
    backend_test::RRATestHlsl hlsl_test(test_name);
    bool                      result = hlsl_test.RunTests(config_);
    return result;
}

bool RRATestCases::TestAsicChunk(const char* test_name) const
{
    backend_test::RRATestAsicChunk asic_test(test_name);
    bool                           result = asic_test.RunTests(config_);
    return result;
}

bool RRATestCases::TestSystemInfoChunk(const char* test_name) const
{
    backend_test::RRATestSystemInfoChunk system_info_test(test_name);
    bool                                 result = system_info_test.RunTests(config_.log);
    return result;
}

bool RRATestCases::TestBlas(const char* test_name) const
{
    backend_test::RRATestBlas blas_test(test_name);
    bool                      result = blas_test.RunTests(config_);
    return result;
}

bool RRATestCases::TestTlas(const char* test_name) const
{
    backend_test::RRATestTlas tlas_test(test_name);
    bool                      result = tlas_test.RunTests(config_.log);
    return result;
}

bool RRATestCases::TestRayHistory(const char* test_name) const
{
    backend_test::RRATestRayHistory ray_history_test(test_name);
    bool                            result = ray_history_test.RunTests(config_.log);
    return result;
}
