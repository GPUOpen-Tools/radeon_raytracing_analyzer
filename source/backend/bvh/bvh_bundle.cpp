//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  BVH bundle implementation.
///
/// A BVH bundle consists of a TLAS array, a BLAS array and index maps for
/// the TLAS & BLAS arrays.
//=============================================================================

#include "bvh/bvh_bundle.h"

#include <iostream>
#include <unordered_set>

#include "rdf/rdf/inc/amdrdf.h"
#include "bvh/encoded_rt_ip_11_bottom_level_bvh.h"
#include "bvh/encoded_rt_ip_11_top_level_bvh.h"

#include "public/rra_assert.h"
#include "public/rra_error.h"

namespace rta
{
    constexpr std::uint32_t kDeprecatedBvhChunkVersion = 1;  ///< 1.0 Fallback version for chunk files with old index reference mappings.

    BvhBundle::BvhBundle(std::vector<std::unique_ptr<IBvh>>&& top_level_bvhs,
                         std::vector<std::unique_ptr<IBvh>>&& bottom_level_bvhs,
                         bool                                 empty_placeholder,
                         uint64_t                             missing_blas_count,
                         uint64_t                             inactive_instance_count)

        : top_level_bvhs_(std::move(top_level_bvhs))
        , bottom_level_bvhs_(std::move(bottom_level_bvhs))
        , empty_placeholder_(empty_placeholder)
        , missing_blas_count_(missing_blas_count)
        , inactive_instance_count_(inactive_instance_count)
    {
        empty_blas_count_  = 0;
        size_t start_index = (empty_placeholder) ? 1 : 0;

        for (size_t count = start_index; count < bottom_level_bvhs_.size(); count++)
        {
            if (bottom_level_bvhs_[count]->IsEmpty())
            {
                empty_blas_count_++;
            }
        }
    }

    BvhBundle::~BvhBundle()
    {
    }

    bool BvhBundle::HasEncoding(const BvhEncoding encoding) const
    {
        for (const auto& tlas : top_level_bvhs_)
        {
            if (tlas->GetFormat().encoding != encoding)
            {
                return false;
            }
        }

        for (const auto& blas : bottom_level_bvhs_)
        {
            if (blas->GetFormat().encoding != encoding)
            {
                return false;
            }
        }

        return true;
    }

    BvhFormat BvhBundle::GetFormat() const
    {
        BvhFormat format = {};
        RRA_ASSERT(!top_level_bvhs_.empty());

        format.encoding = top_level_bvhs_.front()->GetFormat().encoding;
        if (!HasEncoding(format.encoding))
        {
            RRA_ASSERT_FAIL("Encoding mismatch in BVH bundle.");
        }

        return format;
    }

    const std::vector<std::unique_ptr<IBvh>>& BvhBundle::GetTopLevelBvhs() const
    {
        return top_level_bvhs_;
    }

    const std::vector<std::unique_ptr<IBvh>>& BvhBundle::GetBottomLevelBvhs() const
    {
        return bottom_level_bvhs_;
    }

    size_t BvhBundle::GetBlasCount() const
    {
        auto size = bottom_level_bvhs_.size();
        if (empty_placeholder_ && size)
        {
            return size - 1;
        }
        return size;
    }

    size_t BvhBundle::GetTotalBlasCount() const
    {
        return bottom_level_bvhs_.size();
    }

    uint64_t BvhBundle::GetMissingBlasCount() const
    {
        return missing_blas_count_;
    }

    uint64_t BvhBundle::GetInactiveInstanceCount() const
    {
        return inactive_instance_count_;
    }

    uint64_t BvhBundle::GetEmptyBlasCount() const
    {
        return empty_blas_count_;
    }

    bool BvhBundle::ContainsEmptyPlaceholder() const
    {
        return empty_placeholder_;
    }

    /// @brief Load a "RawAccelStruct" chunk.
    ///
    /// @param [in]  chunk_file        The chunk file to load from.
    /// @param [in]  chunk_index       The chunk index in the chunk file.
    /// @param [in]  chunk_header      Reference to the chunk header data.
    /// @param [in]  chunk_identifier  The BVH chunk name.
    /// @param [in]  import_option     Flag indicating which sections of the chunk to load/discard.
    /// @param [out] bvh               The loaded BVH data.
    ///
    /// @return The BVH object loaded in if successful or nullptr if error.
    static std::unique_ptr<IBvh> LoadRtIp11RawAccelStrucAtChunkFileIndex(rdf::ChunkFile&                      chunk_file,
                                                                         const std::int32_t                   chunk_index,
                                                                         const RawAccelStructRdfChunkHeader&  chunk_header,
                                                                         const char* const                    chunk_identifier,
                                                                         const BvhBundleReadOption            import_option,
                                                                         std::unique_ptr<IEncodedRtIp11Bvh>&& bvh)
    {
        const std::uint32_t version       = chunk_file.GetChunkVersion(chunk_identifier, chunk_index);
        std::uint32_t       major_version = version >> 16;
        if (major_version <= GPURT_ACCEL_STRUCT_MAJOR_VERSION)
        {
            if (bvh->LoadRawAccelStrucFromFile(chunk_file, chunk_index, chunk_header, chunk_identifier, import_option) == true)
            {
                return std::move(bvh);
            }
        }
        return nullptr;
    }

    /// Create an empty BVH structure
    static std::unique_ptr<IBvh> CreateRtIp11RawAccelStrucAtIndex(std::unique_ptr<IEncodedRtIp11Bvh>&& bvh, uint64_t index)
    {
        bvh->SetID(index);
        assert(!bvh->HasBvhReferences());
        return std::move(bvh);
    }

    static bool IdentifiersContainedInChunkFile(const std::vector<const char*> identifiers, const rdf::ChunkFile& chunk_file)
    {
        for (const char* identifier : identifiers)
        {
            try
            {
                bool result = chunk_file.ContainsChunk(identifier);
                if (result == false)
                {
                    return result;
                }
            }
            catch (...)
            {
                return false;
            }
        }

        return true;
    }

    /// @brief Load in all the "RawAccelStruc" chunks from the file provided.
    ///
    /// @param [in] chunk_file     The chunk file to load from.
    /// @param [in] import_option  Flag indicating which sections of the chunk to load/discard.
    /// @param [out] io_error_code Variable to receive an error code if the load failed.
    ///
    /// @return The BVH object loaded in if successful or nullptr if error.
    static std::unique_ptr<BvhBundle> LoadRtIp11RawAccelStructBundleFromFile(rdf::ChunkFile&           chunk_file,
                                                                             const BvhBundleReadOption import_option,
                                                                             RraErrorCode*             io_error_code)
    {
        // Check if all expected identifiers are contained in the chunk file
        const char* bvh_identifier = nullptr;
        if (IdentifiersContainedInChunkFile({IEncodedRtIp11Bvh::kAccelChunkIdentifier1}, chunk_file))
        {
            bvh_identifier = IEncodedRtIp11Bvh::kAccelChunkIdentifier1;
        }
        else if (IdentifiersContainedInChunkFile({IEncodedRtIp11Bvh::kAccelChunkIdentifier2}, chunk_file))
        {
            bvh_identifier = IEncodedRtIp11Bvh::kAccelChunkIdentifier2;
        }
        else
        {
            *io_error_code = kRraErrorNoASChunks;
            return nullptr;
        }

        std::vector<std::unique_ptr<IBvh>> top_level_bvhs;
        std::vector<std::unique_ptr<IBvh>> bottom_level_bvhs;

        const auto bvh_chunk_count = chunk_file.GetChunkCount(bvh_identifier);

        std::unordered_map<GpuVirtualAddress, std::uint64_t> tlas_map;
        std::unordered_map<GpuVirtualAddress, std::uint64_t> blas_map;

        bottom_level_bvhs.emplace_back(CreateRtIp11RawAccelStrucAtIndex(std::make_unique<EncodedRtIp11BottomLevelBvh>(), 0));
        // Add a mapping of GPU address to index.
        {
            auto        index   = bottom_level_bvhs.size() - 1;
            const auto& as      = bottom_level_bvhs[index];
            auto        address = as->GetVirtualAddress();
            blas_map.insert(std::make_pair(address, index));
        }

        for (auto ci = 0; ci < bvh_chunk_count; ++ci)
        {
            uint64_t header_size = chunk_file.GetChunkHeaderSize(bvh_identifier, ci);

            RRA_ASSERT(header_size > 0);

            if (header_size > 0)
            {
                RawAccelStructRdfChunkHeader header;
                chunk_file.ReadChunkHeaderToBuffer(bvh_identifier, ci, &header);

                // Call the load function here, passing in a reference to the header as a parameter.
                if (header.flags.blas == 1)
                {
                    bottom_level_bvhs.emplace_back(LoadRtIp11RawAccelStrucAtChunkFileIndex(
                        chunk_file, ci, header, bvh_identifier, import_option, std::make_unique<EncodedRtIp11BottomLevelBvh>()));
                    if (bottom_level_bvhs.back() == nullptr)
                    {
                        *io_error_code = kRraErrorMalformedData;
                        return nullptr;
                    }

                    // Add a mapping of GPU address to index.
                    auto        index   = bottom_level_bvhs.size() - 1;
                    const auto& as      = bottom_level_bvhs[index];
                    auto        address = as->GetVirtualAddress();
                    blas_map.insert(std::make_pair(address, index));
                }
                else
                {
                    top_level_bvhs.emplace_back(LoadRtIp11RawAccelStrucAtChunkFileIndex(
                        chunk_file, ci, header, bvh_identifier, import_option, std::make_unique<EncodedRtIp11TopLevelBvh>()));
                    if (top_level_bvhs.back() == nullptr)
                    {
                        *io_error_code = kRraErrorMalformedData;
                        return nullptr;
                    }

                    // Add a mapping of GPU address to index.
                    auto        index   = top_level_bvhs.size() - 1;
                    const auto& as      = top_level_bvhs[index];
                    auto        address = as->GetVirtualAddress();
                    tlas_map.insert(std::make_pair(address, index));
                }
            }
        }

        // Replace absolute addresses in the TLAS with indices. Additionally, the instance nodes
        // in the TLAS refer to BLAS instances, and these addresses also need converting to indices.
        std::unordered_set<GpuVirtualAddress> missing_tlas_set;
        std::unordered_set<GpuVirtualAddress> missing_blas_set;

        uint64_t inactive_instance_count = 0;

        for (std::size_t t = 0; t < top_level_bvhs.size(); ++t)
        {
            auto& top_level_bvh = top_level_bvhs[t];
            top_level_bvh->SetRelativeReferences(tlas_map, true, missing_tlas_set);
            top_level_bvh->SetRelativeReferences(blas_map, false, missing_blas_set);
            inactive_instance_count += top_level_bvh->GetInactiveInstanceCount();
            if (top_level_bvh->PostLoad() == false)
            {
                *io_error_code = kRraErrorMalformedData;
                return nullptr;
            }
        }

        // Replace absolute addresses in the BLAS with indices.
        for (std::size_t t = 0; t < bottom_level_bvhs.size(); ++t)
        {
            auto& bottom_level_bvh = bottom_level_bvhs[t];
            bottom_level_bvh->SetRelativeReferences(blas_map, true, missing_tlas_set);
            if (bottom_level_bvh->PostLoad() == false)
            {
                *io_error_code = kRraErrorMalformedData;
                return nullptr;
            }
        }

        *io_error_code = kRraOk;
        return std::make_unique<BvhBundle>(std::move(top_level_bvhs), std::move(bottom_level_bvhs), true, missing_blas_set.size(), inactive_instance_count);
    }

    std::unique_ptr<BvhBundle> LoadBvhBundleFromFile(rdf::ChunkFile&           chunk_file,
                                                     const BvhEncoding         encoding,
                                                     const BvhBundleReadOption import_option,
                                                     RraErrorCode*             io_error_code)
    {
        if (encoding == BvhEncoding::kAmdRtIp_1_1)
        {
            return LoadRtIp11RawAccelStructBundleFromFile(chunk_file, import_option, io_error_code);
        }

        return nullptr;
    }

}  // namespace rta
