//=============================================================================
// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for StringTables class (a RDF Chunk type)
//=============================================================================

#include "string_table.h"

namespace rra
{
    /// @brief Load the chunk.
    ///
    /// @return kRraOk if loaded correctly, error code if not.
    RraErrorCode StringTables::LoadChunk(rdf::ChunkFile& chunk_file)
    {
        const char* identifier = chunk_identifier_.c_str();

        chunk_data_valid_ = false;
        if (!chunk_file.ContainsChunk(identifier))
        {
            return kRraErrorMalformedData;
        }

        // Load in API info chunk.
        const auto chunk_count = chunk_file.GetChunkCount(identifier);

        std::uint32_t chunk_version = chunk_file.GetChunkVersion(identifier);
        RRA_UNUSED(chunk_version);

        for (auto i = 0; i < chunk_count; i++)
        {
            uint64_t header_size  = chunk_file.GetChunkHeaderSize(identifier, i);
            uint64_t payload_size = chunk_file.GetChunkDataSize(identifier, i);

            RRA_ASSERT(header_size == sizeof(TraceChunkStringTableHeader));

            TraceChunkStringTableHeader header = {};       ///< The chunk data.
            std::vector<char> data(payload_size);

            if (header_size > 0)
            {
                chunk_file.ReadChunkHeaderToBuffer(identifier, i, &header);
            }
            if (payload_size > 0)
            {
                chunk_file.ReadChunkDataToBuffer(identifier, i, data.data());
            }

            std::vector<std::string> stringTbl;
            auto pData    = data.data();
            auto pOffsets = reinterpret_cast<const uint32_t*>(pData);

            for (uint32_t n = 0; n < header.numStrings; n++)
            {
               const char* str = pData + pOffsets[n];
               stringTbl.push_back(str);
            }

            string_tables_[header.tableId] = std::move(stringTbl);
        }

        chunk_data_valid_ = true;

        return kRraOk;
    }

}  // namespace rra

