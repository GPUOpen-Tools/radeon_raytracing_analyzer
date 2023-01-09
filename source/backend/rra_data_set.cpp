//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of functions for working with a data set.
//=============================================================================

#include "rra_data_set.h"

#include <string.h>  // for memcpy()
#include <stdlib.h>  // for malloc() / free()
#include <time.h>

#include "public/rra_assert.h"
#include "public/rra_print.h"

#include "rdf/rdf/inc/amdrdf.h"

#ifndef _WIN32
#include "public/linux/safe_crt.h"
#include <stddef.h>  // for offsetof macro.
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static bool ContainsChunk(const char* identifier, const rdf::ChunkFile& chunk_file)
{
    try
    {
        chunk_file.ContainsChunk(identifier);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

static RraErrorCode ParseRdf(const char* path, RraDataSet* data_set)
{
    auto         file       = rdf::Stream::OpenFile(path);
    RraErrorCode error_code = kRraOk;

    rdf::ChunkFile chunk_file = rdf::ChunkFile(file);

    // Load the API Info and ASIC info chunks if they exist in the file.
    if (ContainsChunk(rra::ApiInfo::kChunkIdentifier, chunk_file))
    {
        data_set->api_info.LoadChunk(chunk_file);
    }
    if (ContainsChunk(rra::AsicInfo::kChunkIdentifier, chunk_file))
    {
        data_set->asic_info.LoadChunk(chunk_file);
    }

    // Load the BVH chunks.
    data_set->bvh_bundle = rta::LoadBvhBundleFromFile(chunk_file, rta::BvhEncoding::kAmdRtIp_1_1, rta::BvhBundleReadOption::kDefault, &error_code);

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

    data_set->file_loaded = false;

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

    return kRraOk;
}

// Destroy the data set.
RraErrorCode RraDataSetDestroy(RraDataSet* data_set)
{
    data_set->file_loaded = false;
    data_set->bvh_bundle.reset();
    return kRraOk;
}
