//=============================================================================
// Copyright (c) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
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
        const char*              identifier                    = chunk_identifier_.c_str();
        static const std::string kAccelStructLabelPrefix       = "RRA_AS:";
        static const size_t      kAccelStructLabelPrefixLength = kAccelStructLabelPrefix.size();

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

            TraceChunkStringTableHeader header = {};  ///< The chunk data.
            std::vector<char>           data(payload_size);

            if (header_size > 0)
            {
                chunk_file.ReadChunkHeaderToBuffer(identifier, i, &header);
            }
            if (payload_size > 0)
            {
                chunk_file.ReadChunkDataToBuffer(identifier, i, data.data());
            }

            std::vector<std::string> stringTbl;
            auto                     data_begin = data.data();
            auto                     offsets    = reinterpret_cast<const uint32_t*>(data_begin);

            for (uint32_t n = 0; n < header.numStrings; n++)
            {
                const char* raw_str = data_begin + offsets[n];

                std::string std_str(raw_str);

                if (std_str.substr(0, kAccelStructLabelPrefixLength) == kAccelStructLabelPrefix)
                {
                    std_str = std_str.substr(kAccelStructLabelPrefixLength);

                    size_t colon_pos = std_str.find(':');
                    if (colon_pos != std::string::npos)
                    {
                        // Extract number and label
                        std::string address_str = std_str.substr(0, colon_pos);
                        std::string label_str   = std_str.substr(colon_pos + 1);

                        // Parse the address as decimal or hexadecimal
                        bool     is_hex  = (address_str.substr(0, 2) == "0x");
                        char*    end_ptr = nullptr;
                        uint64_t address = std::strtoull(address_str.c_str(), &end_ptr, is_hex ? 16 : 10);

                        bool is_valid_address = (end_ptr != address_str.c_str() && *end_ptr == '\0');
                        bool is_valid_label   = !label_str.empty();

                        if (is_valid_address && is_valid_label)
                        {
                            accel_struct_labels_[address] = label_str;
                        }
                    }
                }

                stringTbl.push_back(raw_str);
            }

            string_tables_[header.tableId] = std::move(stringTbl);
        }

        chunk_data_valid_ = true;

        return kRraOk;
    }

}  // namespace rra
