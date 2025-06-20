//=============================================================================
// Copyright (c) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Class definition for StringTables (a RDF chunk type).
//=============================================================================

#ifndef RRA_BACKEND_STRING_TABLE_H_
#define RRA_BACKEND_STRING_TABLE_H_

#include <map>
#include <string>
#include <vector>

#include "rdf/rdf/inc/amdrdf.h"

#include "public/rra_error.h"

namespace rra
{
    class StringTables
    {
    public:
        /// @brief Constructor.
        StringTables()
        {
            Reset();
        }

        /// @brief Destructor.
        ~StringTables()
        {
            Reset();
        }

        /// @brief Reset object to a default state.
        void Reset()
        {
            chunk_data_valid_ = false;
            string_tables_.clear();
        }

        /// @brief Load the chunk.
        ///
        /// @return kRraOk if loaded correctly, error code if not.
        RraErrorCode LoadChunk(rdf::ChunkFile& chunk_file);

        void SetChunkIdentifier(const char* chunk_identifier)
        {
            chunk_identifier_ = chunk_identifier;
        }

        /// @brief Get the API name.
        ///
        /// @return A pointer to a string containing the API name, or nullptr if invalid.
        const char* GetChunkIdentifier() const
        {
            return chunk_identifier_.c_str();
        }

        const std::vector<std::string>& GetStringTable(uint32_t table_id) const
        {
            return string_tables_.at(table_id);
        }

        const std::string GetAccelStructLabel(uint64_t accel_struct_id) const
        {
            auto it = accel_struct_labels_.find(accel_struct_id);

            if (it == accel_struct_labels_.end())
            {
                return "";
            }

            return it->second;
        }

    private:
        /// @brief The API Info chunk data format.
        struct TraceChunkStringTableHeader
        {
            uint32_t tableId;     /// The ID of the string table
            uint32_t numStrings;  /// The number of strings in the table
        };

        std::string                                  chunk_identifier_ = "StringTable";  ///< The chunk identifier.
        bool                                         chunk_data_valid_ = false;          ///< Is the API info data valid.
        std::map<uint32_t, std::vector<std::string>> string_tables_;                     ///< The string tables.
        std::map<uint64_t, std::string>              accel_struct_labels_;               ///< The labels for the acceleration structures.
    };

}  // namespace rra

#endif  // RRA_BACKEND_STRING_TABLE_H_

