//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test file chunks implementation.
//=============================================================================

#include "rra_test_file_chunks.h"

#include "amdrdf.h"

#include "public/rra_macro.h"
#include "public/rra_ray_history.h"

#ifndef _WIN32
#include "linux/safe_crt.h"
#endif

#include "api_info.h"
#include "asic_info.h"
#include "bvh/ibvh.h"

namespace backend_test
{
    RRATestFileChunks::RRATestFileChunks(const std::string& test_name)
        : RRATestBase(test_name)
    {
    }

    RRATestFileChunks::~RRATestFileChunks()
    {
    }

    bool RRATestFileChunks::RunTests(const RRATestConfig& config)
    {
        AddTestStartMessage(config.log);

        bool result = true;

        rdf::ChunkFile chunk_file(config.trace_file_name.c_str());

        struct Version
        {
            uint32_t major;
            uint32_t minor;
        };

        // Mapping of chunk name to the version number supported by RRA.
        // IMPORTANT: If support is added to RRA for a new version number, this table needs to be updated.
        // The test will fail if a version number in a trace file is larger than one supported by RRA.
        // The RayHistoryToken and RayHistoryMetadata chunks share the same version number.
        std::unordered_map<std::string, Version> supported_version_map = {
            {rra::ApiInfo::kChunkIdentifier, {0, 2}},
            {rra::AsicInfo::kChunkIdentifier, {0, 3}},
            {rta::IBvh::kAccelChunkIdentifier2, {GPURT_ACCEL_STRUCT_MAJOR_VERSION, GPURT_ACCEL_STRUCT_MINOR_VERSION}},
            {RRA_RAY_HISTORY_TOKENS_METADATA_IDENTIFIER, {2, 0}},
            {RRA_RAY_HISTORY_RAW_TOKENS_IDENTIFIER, {2, 0}}};

        // Test to iterate over all chunks.
        auto it = chunk_file.GetIterator();
        for (;;)
        {
            if (it.IsAtEnd())
            {
                break;
            }

            char identifier[RDF_IDENTIFIER_SIZE + 1] = {};
            it.GetChunkIdentifier(identifier);
            identifier[RDF_IDENTIFIER_SIZE] = '\0';
            const auto index                = it.GetChunkIndex();

            const auto version = chunk_file.GetChunkVersion(identifier, index);

            // Check chunk version number.
            const auto version_it = supported_version_map.find(identifier);
            if (version_it != supported_version_map.end())
            {
                uint32_t       expected_major_version = version_it->second.major;
                uint32_t       expected_minor_version = version_it->second.minor;
                const uint32_t major_version          = version >> 16;
                const uint32_t minor_version          = version & 0xffff;

                if (minor_version > expected_minor_version)
                {
                    char buffer[1024] = {};
                    sprintf_s(buffer,
                              1024,
                              " WARNING: chunk '%s', found chunk minor version (%d,%d), expecting (%d,%d)",
                              identifier,
                              major_version,
                              minor_version,
                              expected_major_version,
                              expected_minor_version);
                    config.log.Write(buffer);
                }

                if (major_version > expected_major_version)
                {
                    char buffer[1024] = {};

                    sprintf_s(buffer,
                              1024,
                              " ERROR: chunk '%s', found chunk major version (%d,%d), expecting (%d,%d)",
                              identifier,
                              major_version,
                              minor_version,
                              expected_major_version,
                              expected_minor_version);
                    config.log.Write(buffer);
                    result = false;
                }
            }

            it.Advance();
        }

        // Check that there's only 1 ApiInfo chunk.
        auto api_info_chunk_count = chunk_file.GetChunkCount(rra::ApiInfo::kChunkIdentifier);
        if (api_info_chunk_count != 1)
        {
            config.log.Write(" ERROR: found %d ApiInfo chunks (should be 1)", api_info_chunk_count);
            result = false;
        }

        // Check that there's only 1 AsicInfo chunk.
        auto asic_info_chunk_count = chunk_file.GetChunkCount(rra::AsicInfo::kChunkIdentifier);
        if (asic_info_chunk_count <= 0)
        {
            config.log.Write(" ERROR: found %d AsicInfo chunks (should be at least 1)", asic_info_chunk_count);
            result = false;
        }

        // Make sure there's at least 1 BVH chunk.
        auto old_bvh_chunk_count = chunk_file.GetChunkCount(rta::IBvh::kAccelChunkIdentifier1);
        auto bvh_chunk_count     = chunk_file.GetChunkCount(rta::IBvh::kAccelChunkIdentifier2);
        if (old_bvh_chunk_count <= 0 && bvh_chunk_count <= 0)
        {
            config.log.Write(" ERROR: no acceleration structures found in the chunk file");
            result = false;
        }

        AddTestResultMessage(result, config.log);
        return result;
    }

}  // namespace backend_test

