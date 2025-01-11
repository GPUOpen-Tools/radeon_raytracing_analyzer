//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of functions for working with a data set.
//=============================================================================

#include "rra_data_set.h"

#include <string.h>  // for memcpy()
#include <stdlib.h>  // for malloc() / free()
#include <time.h>
#include <iterator>
#include <fstream>

#include "public/rra_assert.h"
#include "public/rra_print.h"
#include "public/rra_ray_history.h"
#include "public/rra_trace_loader.h"

#include "string_table.h"
#include "user_marker_history.h"

#include "system_info_utils/source/driver_overrides_reader.h"

#include "rdf/rdf/inc/amdrdf.h"

#include "ray_history/raytracing_counter.h"

#include "surface_area_heuristic.h"

#ifndef _WIN32
#include "public/linux/safe_crt.h"
#include <stddef.h>  // for offsetof macro.
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <map>
#include <algorithm>

static bool ContainsChunk(const char* identifier, const rdf::ChunkFile& chunk_file)
{
    try
    {
        auto result = chunk_file.ContainsChunk(identifier);
        return result;
    }
    catch (...)
    {
        return false;
    }
}

static std::vector<std::shared_ptr<RraAsyncRayHistoryLoader>> LaunchAsyncRayHistoryLoaders(rdf::ChunkFile& chunk_file, const char* file_path)
{
    std::vector<std::shared_ptr<RraAsyncRayHistoryLoader>> loaders;

    int64_t dispatch_count = chunk_file.GetChunkCount(RRA_RAY_HISTORY_RAW_TOKENS_IDENTIFIER);

    for (int64_t i = 0; i < dispatch_count; i++)
    {
        auto loader = std::make_shared<RraAsyncRayHistoryLoader>(file_path, i);
        loaders.push_back(loader);
    }

    return loaders;
}

RraErrorCode CheckMajorVersions(int as_major, int dispatch_major)
{
    if (as_major > GPURT_ACCEL_STRUCT_MAJOR_VERSION)
    {
        return kRraMajorVersionIncompatible;
    }

    if (dispatch_major > GPURT_COUNTER_MAJOR_VERSION)
    {
        return kRraMajorVersionIncompatible;
    }

    return kRraOk;
}

// Load the Driver Overrides chunk.
static RraErrorCode LoadDriverOverridesChunk(rdfChunkFile* chunk_file)
{
    RRA_RETURN_ON_ERROR(chunk_file != nullptr, kRraErrorInvalidPointer);
    RraErrorCode result = kRraErrorMalformedData;

    // Attempt to load Driver Overrides chunk.
    std::string json_text;
    if (driver_overrides_utils::DriverOverridesReader::Parse(chunk_file, json_text))
    {
        result = RraTraceLoaderCopyDriverOverridesString(json_text.c_str(), json_text.size());
    }

    return result;
}

static RraErrorCode ParseRdf(const char* path, RraDataSet* data_set)
{
    auto         file       = rdf::Stream::OpenFile(path);
    RraErrorCode error_code = kRraOk;

    rdf::ChunkFile chunk_file = rdf::ChunkFile(file);

    // Load the API Info and ASIC info chunks if they exist in the file.
    if (ContainsChunk(rra::ApiInfo::kChunkIdentifier, chunk_file))
    {
        error_code = data_set->api_info.LoadChunk(chunk_file);
        if (error_code != kRraOk)
        {
            return error_code;
        }
    }
    if (ContainsChunk(rra::AsicInfo::kChunkIdentifier, chunk_file))
    {
        error_code = data_set->asic_info.LoadChunk(chunk_file);
        if (error_code != kRraOk)
        {
            return error_code;
        }
    }

    error_code = LoadDriverOverridesChunk(chunk_file.operator rdfChunkFile*());
    if (error_code != kRraOk)
    {
        return error_code;
    }

    if (ContainsChunk("StringTable", chunk_file))
    {
        data_set->user_marker_string_tables->LoadChunk(chunk_file);
    }
    if (ContainsChunk("UserMarkerHist", chunk_file))
    {
        data_set->user_marker_histories->LoadChunk(chunk_file);
    }

    int max_as_major_version       = 0;
    int max_dispatch_major_version = 0;
    error_code                     = rta::GetMaxMajorVersions(chunk_file, &max_as_major_version, &max_dispatch_major_version);
    if (error_code != kRraOk)
    {
        return error_code;
    }

    error_code = CheckMajorVersions(max_as_major_version, max_dispatch_major_version);
    if (error_code != kRraOk)
    {
        return error_code;
    }

    bool system_info_result = system_info_utils::SystemInfoReader::Parse(chunk_file, *data_set->system_info);
    RRA_UNUSED(system_info_result);

    data_set->rtip_level = rta::DecodeRtIpLevel(chunk_file, &error_code);
    if (error_code != kRraOk)
    {
        return error_code;
    }

    RRA_ASSERT(data_set->rtip_level != rta::RayTracingIpLevel::RtIpReserved);

    if (data_set->rtip_level >= rta::RayTracingIpLevel::RtIpCount)
    {
        return kRraErrorUnrecognizedRtIpLevel;
    }

    // Load the BVH chunks.
    data_set->bvh_bundle = rta::LoadBvhBundleFromFile(chunk_file, data_set->rtip_level, rta::BvhBundleReadOption::kDefault, &error_code);
    if (error_code != kRraOk)
    {
        return error_code;
    }

    // Launch ray history loaders after the dataset has loaded.
    // If an error is returned before here, the dataset will be cleared, meaning the ray history loaders will be dangling.
    data_set->async_ray_histories.clear();
    data_set->async_ray_histories = LaunchAsyncRayHistoryLoaders(chunk_file, path);

    return error_code;
}

// Initialize the data set by reading the header chunks, and setting up the streams.
RraErrorCode RraDataSetInitialize(const char* path, RraDataSet* data_set)
{
    RRA_ASSERT(path);
    RRA_ASSERT(data_set);
    RRA_RETURN_ON_ERROR(path, kRraErrorInvalidPointer);
    RRA_RETURN_ON_ERROR(data_set, kRraErrorInvalidPointer);

    // Copy the path
    const size_t path_length = strlen(path);
    memcpy(data_set->file_path, path, RRA_MINIMUM(RRA_MAXIMUM_FILE_PATH, path_length));

    data_set->driver_overrides_json_text = nullptr;
    data_set->file_loaded                = false;
    data_set->system_info                = new system_info_utils::SystemInfo();
    data_set->user_marker_string_tables  = new rra::StringTables();
    data_set->user_marker_histories      = new rra::UserMarkerHistory();

    // Error-checking to make sure the chunk file is valid.
    rdfChunkFile* chunk_file = nullptr;
    if (rdfChunkFileOpenFile(path, &chunk_file) != rdfResult::rdfResultOk)
    {
        return kRraErrorFileNotOpen;
    }
    rdfChunkFileClose(&chunk_file);

    // Parse all the chunk headers from the file.
    RraErrorCode error_code = ParseRdf(path, data_set);

    RRA_ASSERT(error_code == kRraOk);
    RRA_RETURN_ON_ERROR(error_code == kRraOk, error_code);

    data_set->file_loaded = true;

    rra::CalculateSurfaceAreaHeuristics(*data_set);

    return kRraOk;
}

// Destroy the data set.
RraErrorCode RraDataSetDestroy(RraDataSet* data_set)
{
    data_set->file_loaded = false;
    data_set->bvh_bundle.reset();
    delete data_set->system_info;
    data_set->system_info = nullptr;

    delete data_set->driver_overrides_json_text;
    data_set->driver_overrides_json_text = nullptr;

    delete data_set->user_marker_string_tables;
    data_set->user_marker_string_tables = nullptr;

    delete data_set->user_marker_histories;
    data_set->user_marker_histories = nullptr;

    return kRraOk;
}
