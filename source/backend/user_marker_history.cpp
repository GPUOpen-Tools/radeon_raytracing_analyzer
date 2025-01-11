//=============================================================================
// Copyright (c) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for UserMarkerHistory class (a RDF chunk type).
//=============================================================================

#include "user_marker_history.h"

namespace rra
{
    /// @brief Load the chunk.
    ///
    /// @return kRraOk if loaded correctly, error code if not.
    RraErrorCode UserMarkerHistory::LoadChunk(rdf::ChunkFile& chunk_file)
    {
        const auto identifier = kChunkIdentifier;

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

            TraceChunkUserMarkerHistory header = {};
            std::vector<char>           data(payload_size);

            if (header_size > 0)
            {
                chunk_file.ReadChunkHeaderToBuffer(identifier, i, &header);
            }
            if (payload_size > 0)
            {
                chunk_file.ReadChunkDataToBuffer(identifier, i, data.data());
            }

            auto                    userMarkerOpsBegin = reinterpret_cast<const MarkerInfo*>(data.data());
            auto                    userMarkerOpsEnd   = userMarkerOpsBegin + header.numOps;
            std::vector<MarkerInfo> userMarkerOps(userMarkerOpsBegin, userMarkerOpsEnd);

            user_marker_history_map_[header.cbId] = std::make_pair(header.tableId, std::move(userMarkerOps));
        }

        chunk_data_valid_ = true;
        return kRraOk;
    }

    uint32_t UserMarkerHistory::GetStringTableId(uint32_t cbId) const
    {
        if (auto it = user_marker_history_map_.find(cbId); it != user_marker_history_map_.end())
        {
            return it->second.first;
        }
        return 0;  // 0 is an invalid table id.
    }

    std::stack<uint32_t> UserMarkerHistory::GetUserMarkerStringIndices(uint32_t cbId, uint32_t userMarkerHistoryIndex) const
    {
        if (auto it = user_marker_history_map_.find(cbId); it != user_marker_history_map_.end())
        {
            const auto& userMarkerHistory = it->second.second;

            return RebuildUserMarkerStackUpUntil(userMarkerHistory, userMarkerHistoryIndex);
        }

        return {};
    }

    std::stack<uint32_t> UserMarkerHistory::RebuildUserMarkerStackUpUntil(const std::vector<MarkerInfo>& userMarkerHistory, uint32_t untilIdx) const
    {
        RRA_ASSERT(untilIdx <= userMarkerHistory.size());

        std::stack<uint32_t> stack;

        for (auto idx = 0ul; idx < untilIdx; idx++)
        {
            const MarkerInfo& markerInfo = userMarkerHistory[idx];

            if (markerInfo.opType == PushMarkerOp)
            {
                stack.push(markerInfo.strIndex - 1);
            }
            else if (markerInfo.opType == PopMarkerOp)
            {
                if (!stack.empty())
                {
                    stack.pop();
                }
            }
        }

        return stack;
    }

}  // namespace rra
