//=============================================================================
// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Class definition for UserMarkerHistory (a RDF chunk type) .
//=============================================================================

#ifndef RRA_BACKEND_USER_MARKER_HISTORY_H_
#define RRA_BACKEND_USER_MARKER_HISTORY_H_

#include "public/rra_error.h"

#include "rdf/rdf/inc/amdrdf.h"
#include <vector>
#include <unordered_map>
#include <stack>

namespace rra
{
    class UserMarkerHistory
    {
    public:
        UserMarkerHistory() = default;
        ~UserMarkerHistory() = default;

        // Global identifier of API info in chunk files.
        static constexpr const char* kChunkIdentifier = "UserMarkerHist";

        void Reset()
        {
            chunk_data_valid_ = false;
            user_marker_history_map_.clear();
        }

        /// @brief Load the chunk.
        ///
        /// @return kRraOk if loaded correctly, error code if not.
        RraErrorCode LoadChunk(rdf::ChunkFile& chunk_file);

        uint32_t GetStringTableId(uint32_t sqttCbId) const;

        std::stack<uint32_t> GetUserMarkerStringIndices(
            uint32_t sqttCbId,
            uint32_t userMarkerHistoryIndex) const;

    private:
        static constexpr uint32_t InvalidMarkerOp  = 0x0;
        static constexpr uint32_t PushMarkerOp     = 0x1;
        static constexpr uint32_t PopMarkerOp      = 0x2;
        static constexpr uint32_t SetMarkerOp      = 0x3;

        /// @brief The API Info chunk data format.
        struct TraceChunkUserMarkerHistory
        {
            uint32_t cbId;          // command buffer id
            uint32_t tableId;       // string table id
            uint32_t numOps;        // number of user marker operations
        };

        struct MarkerInfo
        {
            uint32_t opType   :  2;  // InvalidMarkerOp, PushMarkerOp, PopMarkerOp or SetMarkerOp
            uint32_t strIndex : 30;  // Index into the string table.
        };

        std::stack<uint32_t> RebuildUserMarkerStackUpUntil(
            const std::vector<MarkerInfo>& userMarkerHistory,
            uint32_t                       untilIdx) const;

        bool                        chunk_data_valid_ = false;      ///< Is the API info data valid.
        std::unordered_map<uint32_t, std::pair<uint32_t, std::vector<MarkerInfo>>> user_marker_history_map_;  ///< Contains pairs of (cb_id, (table_id, marker infos))
    };

}  // namespace rra

#endif  // RRA_BACKEND_USER_MARKER_HISTORY_H_

