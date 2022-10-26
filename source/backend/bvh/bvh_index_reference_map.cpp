//=============================================================================
// Copyright (c) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  BVH Index reference map implementation.
//=============================================================================

#include "bvh/bvh_index_reference_map.h"

#include "rdf/rdf/inc/amdrdf.h"

#include "public/rra_assert.h"

namespace rta
{
    // Collection of bvhs of different acceleration structure types.
    struct IndexReferenceEntry
    {
        std::uint64_t  index;
        std::uintptr_t ptr;
    };

    BvhIndexReferenceMap::BvhIndexReferenceMap(const std::string& identifier)
        : chunk_identifier_(identifier)
    {
    }

    BvhIndexReferenceMap::~BvhIndexReferenceMap()
    {
    }

    void BvhIndexReferenceMap::LoadFromFile(rdf::ChunkFile& chunk_file)
    {
#ifdef _DEBUG
        const auto header_size = chunk_file.GetChunkHeaderSize(chunk_identifier_.c_str());
        RRA_ASSERT(header_size == sizeof(std::uint64_t));
#endif

        std::uint64_t index_reference_count = 0;
        chunk_file.ReadChunkHeaderToBuffer(chunk_identifier_.c_str(), &index_reference_count);

        std::vector<IndexReferenceEntry> index_reference_buffer(index_reference_count);
        chunk_file.ReadChunkDataToBuffer(chunk_identifier_.c_str(), index_reference_buffer.data());

        for (const auto& entry : index_reference_buffer)
        {
            InsertIndexReference(entry.index, entry.ptr);
        }
        RRA_ASSERT(index_reference_map_.size() == index_reference_count);
    }

    void BvhIndexReferenceMap::InsertIndexReference(const std::uint64_t index, const std::uintptr_t reference)
    {
        index_reference_map_.emplace(index, reference);
    }

    std::uintptr_t BvhIndexReferenceMap::GetReferenceByIndex(const std::uint64_t index) const
    {
        auto it = index_reference_map_.find(index);
        if (it == index_reference_map_.end())
        {
            RRA_ASSERT_FAIL("Could not find reference in index-reference map.");
        }

        return it->second;
    }

    const std::unordered_map<std::uint64_t, std::uintptr_t>& BvhIndexReferenceMap::GetMap() const
    {
        return index_reference_map_;
    }

}  // namespace rta
