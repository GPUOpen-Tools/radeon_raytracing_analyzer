//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include "public/rra_assert.h"
#include "public/rra_error.h"
#include "public/rra_ray_history.h"
#include "public/rra_rtip_info.h"

#include "bvh/ibvh.h"
#include "bvh/rtip11/encoded_rt_ip_11_bottom_level_bvh.h"
#include "bvh/rtip11/encoded_rt_ip_11_top_level_bvh.h"
#include "bvh/rtip31/encoded_rt_ip_31_bottom_level_bvh.h"
#include "bvh/rtip31/encoded_rt_ip_31_top_level_bvh.h"

namespace rta
{
    constexpr std::uint32_t kDeprecatedBvhChunkVersion = 1;  ///< 1.0 Fallback version for chunk files with old index reference mappings.

    BvhBundle::BvhBundle(std::vector<std::unique_ptr<IBvh>>&&                 top_level_bvhs,
                         std::vector<std::unique_ptr<IBvh>>&&                 bottom_level_bvhs,
                         bool                                                 empty_placeholder,
                         uint64_t                                             missing_blas_count,
                         uint64_t                                             inactive_instance_count,
                         std::unordered_map<GpuVirtualAddress, std::uint64_t> blas_va_to_index_map)

        : top_level_bvhs_(std::move(top_level_bvhs))
        , bottom_level_bvhs_(std::move(bottom_level_bvhs))
        , blas_va_to_index_map_(blas_va_to_index_map)
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

    bool BvhBundle::HasEncoding(const RayTracingIpLevel encoding) const
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

    std::optional<uint64_t> BvhBundle::GetBlasIndexFromVirtualAddress(GpuVirtualAddress address)
    {
        auto it = blas_va_to_index_map_.find(address);
        if (it == blas_va_to_index_map_.end())
        {
            return std::nullopt;
        }
        return it->second;
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
    static std::unique_ptr<IBvh> LoadRawAccelStrucAtChunkFileIndex(rdf::ChunkFile&                     chunk_file,
                                                                   const std::int32_t                  chunk_index,
                                                                   const RawAccelStructRdfChunkHeader& chunk_header,
                                                                   const char* const                   chunk_identifier,
                                                                   const BvhBundleReadOption           import_option,
                                                                   std::unique_ptr<IBvh>&&             bvh)
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
    static std::unique_ptr<IBvh> CreateRawAccelStrucAtIndex(std::unique_ptr<IBvh>&& bvh, uint64_t index)
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
    template <typename Tlas, typename Blas>
    static std::unique_ptr<BvhBundle> LoadRawAccelStructBundleFromFile(rdf::ChunkFile&           chunk_file,
                                                                       const BvhBundleReadOption import_option,
                                                                       RraErrorCode*             io_error_code)
    {
        // Check if all expected identifiers are contained in the chunk file
        const char* bvh_identifier = nullptr;
        if (IdentifiersContainedInChunkFile({IBvh::kAccelChunkIdentifier1}, chunk_file))
        {
            bvh_identifier = IBvh::kAccelChunkIdentifier1;
        }
        else if (IdentifiersContainedInChunkFile({IBvh::kAccelChunkIdentifier2}, chunk_file))
        {
            bvh_identifier = IBvh::kAccelChunkIdentifier2;
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

        bottom_level_bvhs.emplace_back(CreateRawAccelStrucAtIndex(std::make_unique<Blas>(), 0));
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
                RawAccelStructRdfChunkHeader rdf_header;
                chunk_file.ReadChunkHeaderToBuffer(bvh_identifier, ci, &rdf_header);

                // Call the load function here, passing in a reference to the header as a parameter.
                if (rdf_header.flags.blas == 1)
                {
                    bottom_level_bvhs.emplace_back(
                        LoadRawAccelStrucAtChunkFileIndex(chunk_file, ci, rdf_header, bvh_identifier, import_option, std::make_unique<Blas>()));
                    if (bottom_level_bvhs.back() != nullptr)
                    {
                        // Add a mapping of GPU address to index.
                        auto        index   = bottom_level_bvhs.size() - 1;
                        const auto& as      = bottom_level_bvhs[index];
                        auto        address = as->GetVirtualAddress() + as->GetHeaderOffset();
                        blas_map.insert(std::make_pair(address, index));
                    }
                    else
                    {
                        // In case of error, remove the invalid BVH.
                        bottom_level_bvhs.pop_back();
                    }
                }
                else
                {
                    top_level_bvhs.emplace_back(
                        LoadRawAccelStrucAtChunkFileIndex(chunk_file, ci, rdf_header, bvh_identifier, import_option, std::make_unique<Tlas>()));
                    if (top_level_bvhs.back() == nullptr)
                    {
                        *io_error_code = kRraErrorMalformedData;
                        return nullptr;
                    }

                    // Add a mapping of GPU address to index.
                    auto        index   = top_level_bvhs.size() - 1;
                    const auto& as      = top_level_bvhs[index];
                    auto        address = as->GetVirtualAddress() + as->GetHeaderOffset();
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

            if ((RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == RayTracingIpLevel::RtIp3_1)
            {
                EncodedRtIp31TopLevelBvh* rtip3_tlas = (EncodedRtIp31TopLevelBvh*)top_level_bvh.get();
                rtip3_tlas->ConvertBlasAddressesToIndices(blas_map);
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

        for (std::unique_ptr<IBvh>& bvh : bottom_level_bvhs)
        {
            bvh->PreprocessParents();
        }

        for (std::unique_ptr<IBvh>& bvh : top_level_bvhs)
        {
            bvh->PreprocessParents();
        }

        *io_error_code = kRraOk;
        return std::make_unique<BvhBundle>(
            std::move(top_level_bvhs), std::move(bottom_level_bvhs), true, missing_blas_set.size(), inactive_instance_count, blas_map);
    }

    RayTracingIpLevel DecodeRtIpLevel(rdf::ChunkFile& chunk_file, RraErrorCode* io_error_code)
    {
        // Check if all expected identifiers are contained in the chunk file
        const char* bvh_identifier = nullptr;
        if (IdentifiersContainedInChunkFile({IBvh::kAccelChunkIdentifier1}, chunk_file))
        {
            bvh_identifier = IBvh::kAccelChunkIdentifier1;
        }
        else if (IdentifiersContainedInChunkFile({IBvh::kAccelChunkIdentifier2}, chunk_file))
        {
            bvh_identifier = IBvh::kAccelChunkIdentifier2;
        }
        else
        {
            *io_error_code = kRraErrorNoASChunks;
            return RayTracingIpLevel::RtIpNone;
        }

        const auto bvh_chunk_count = chunk_file.GetChunkCount(bvh_identifier);

        uint32_t highest_rtip_level{(uint32_t)RayTracingIpLevel::RtIpNone};
        for (auto ci = 0; ci < bvh_chunk_count; ++ci)
        {
            uint64_t header_size = chunk_file.GetChunkHeaderSize(bvh_identifier, ci);

            RRA_ASSERT(header_size > 0);

            if (header_size > 0)
            {
                RawAccelStructRdfChunkHeader header;
                chunk_file.ReadChunkHeaderToBuffer(bvh_identifier, ci, &header);

                const auto data_size = chunk_file.GetChunkDataSize(bvh_identifier, static_cast<uint32_t>(ci));

                std::vector<std::uint8_t> buffer(data_size);
                if (data_size > 0)
                {
                    chunk_file.ReadChunkDataToBuffer(bvh_identifier, static_cast<uint32_t>(ci), buffer.data());
                }

                *io_error_code      = kRraOk;
                size_t rt_ip_offset = offsetof(AccelStructHeader, rtIpLevel);
                size_t offset       = header.header_offset + rt_ip_offset;
                if (offset >= static_cast<uint64_t>(data_size))
                {
                    *io_error_code = kRraErrorMalformedData;
                    return RayTracingIpLevel::RtIpNone;
                }
                uint32_t current_rtip_level = *reinterpret_cast<uint32_t*>((char*)buffer.data() + offset);
                if (current_rtip_level > highest_rtip_level && current_rtip_level < (uint32_t)RayTracingIpLevel::RtIpCount)
                {
                    highest_rtip_level = current_rtip_level;
                }
            }
        }

        if (highest_rtip_level == (uint32_t)RayTracingIpLevel::RtIpNone)
        {
            *io_error_code = kRraErrorNoASChunks;
        }
        return (RayTracingIpLevel)highest_rtip_level;
    }

    RraErrorCode GetMaxMajorVersions(rdf::ChunkFile& chunk_file, int* max_as_major_version, int* max_dispatch_major_version)
    {
        // Check if all expected identifiers are contained in the chunk file
        const char* bvh_identifier = nullptr;
        if (IdentifiersContainedInChunkFile({IBvh::kAccelChunkIdentifier1}, chunk_file))
        {
            bvh_identifier = IBvh::kAccelChunkIdentifier1;
        }
        else if (IdentifiersContainedInChunkFile({IBvh::kAccelChunkIdentifier2}, chunk_file))
        {
            bvh_identifier = IBvh::kAccelChunkIdentifier2;
        }
        else
        {
            return kRraErrorNoASChunks;
        }

        auto dispatch_identifier = RRA_RAY_HISTORY_RAW_TOKENS_IDENTIFIER;

        const auto bvh_chunk_count      = chunk_file.GetChunkCount(bvh_identifier);
        const auto dispatch_chunk_count = chunk_file.GetChunkCount(dispatch_identifier);

        std::uint32_t max_as_mv   = 0;  // Acceleration structures.
        std::uint32_t max_disp_mv = 0;  // Dispatches

        for (auto ci = 0; ci < bvh_chunk_count; ++ci)
        {
            const std::uint32_t version       = chunk_file.GetChunkVersion(bvh_identifier, ci);
            std::uint32_t       major_version = version >> 16;
            max_as_mv                         = std::max(max_as_mv, major_version);
        }

        for (auto ci = 0; ci < dispatch_chunk_count; ++ci)
        {
            const std::uint32_t version       = chunk_file.GetChunkVersion(dispatch_identifier, ci);
            std::uint32_t       major_version = version >> 16;
            max_disp_mv                       = std::max(max_disp_mv, major_version);
        }

        *max_as_major_version       = max_as_mv;
        *max_dispatch_major_version = max_disp_mv;

        return kRraOk;
    }

    std::unique_ptr<BvhBundle> LoadBvhBundleFromFile(rdf::ChunkFile&           chunk_file,
                                                     const RayTracingIpLevel   encoding,
                                                     const BvhBundleReadOption import_option,
                                                     RraErrorCode*             io_error_code)
    {
        if (encoding == RayTracingIpLevel::RtIp3_1)
        {
            return LoadRawAccelStructBundleFromFile<EncodedRtIp31TopLevelBvh, EncodedRtIp31BottomLevelBvh>(chunk_file, import_option, io_error_code);
        }
        else
        {
            return LoadRawAccelStructBundleFromFile<EncodedRtIp11TopLevelBvh, EncodedRtIp11BottomLevelBvh>(chunk_file, import_option, io_error_code);
        }
    }

}  // namespace rta

