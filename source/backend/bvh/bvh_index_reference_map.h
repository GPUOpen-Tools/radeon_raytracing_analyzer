//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  BVH Index reference map definition.
//=============================================================================

#ifndef RRA_BACKEND_BVH_BVH_INDEX_REFERENCE_MAP_H_
#define RRA_BACKEND_BVH_BVH_INDEX_REFERENCE_MAP_H_

#include <unordered_map>

#include "rdf/rdf/inc/amdrdf.h"

namespace rta
{
    class BvhIndexReferenceMap final
    {
    public:
        /// @brief Constructor.
        ///
        /// @param [in] identifier The identifier.
        BvhIndexReferenceMap(const std::string& identifier);

        /// @brief Destructor.
        ~BvhIndexReferenceMap();

        /// @brief Load the reference map from a file.
        ///
        /// @param [in] chunk_file Reference to a ChunkFile object which describes the file being loaded.
        void LoadFromFile(rdf::ChunkFile& chunk_file);

        /// @brief Insert an index reference into the index reference map.
        ///
        /// @param [in] index     The index indicating where to insert the reference.
        /// @param [in] reference The reference to insert.
        void InsertIndexReference(const std::uint64_t index, const std::uintptr_t reference);

        /// @brief Get a reference at a specified index in the reference map.
        ///
        /// @param [in] index The index in the map.
        ///
        /// @return The reference.
        std::uintptr_t GetReferenceByIndex(const std::uint64_t index) const;

        /// @brief Get the reference map.
        ///
        /// @return The index reference map.
        const std::unordered_map<std::uint64_t, std::uintptr_t>& GetMap() const;

    private:
        std::string                                       chunk_identifier_;     ///< The index map file chunk identifier.
        std::unordered_map<std::uint64_t, std::uintptr_t> index_reference_map_;  ///< The index reference map.
    };

}  // namespace rta

#endif  // RRA_BACKEND_BVH_BVH_INDEX_REFERENCE_MAP_H_
