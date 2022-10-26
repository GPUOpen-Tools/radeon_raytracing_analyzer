//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the RT IP 1.1 acceleration structure header class.
//=============================================================================

#include "bvh/irt_ip_11_acceleration_structure_header.h"
#include "bvh/rt_binary_file_defs.h"
#include "bvh/dxr_type_conversion.h"
#include "bvh/utils.h"

#include <string.h>  // for memcpy()

namespace rta
{
    static std::int32_t CalculateWorstCaseMemorySizeInteriorNodeCount(std::uint32_t primitive_count, const std::uint32_t branching_factor)
    {
        assert(branching_factor >= 2);

        // Special case if we have only one instance.
        if (primitive_count < 2)
        {
            // In case we have less than 2 primitives, we reserve space for at least 1 interior node
            // for at least one instance or geometry.
            return std::max(primitive_count, 1u);
        }
        else
        {
            if (branching_factor <= 2)
            {
                return primitive_count - 1;
            }
            else if (branching_factor == 4)
            {
                return (2 * primitive_count) / 3;
            }
            else
            {
                return static_cast<std::int32_t>((primitive_count / 2.0f) * (static_cast<float>(branching_factor) / (branching_factor - 1)));
            }
        }
    }

    static std::uint32_t CalculateWorstCaseMemorySizeInteriorHalfNodeCount(const std::uint32_t primitive_count, const std::uint32_t branching_factor)
    {
        assert(branching_factor >= 2);
        assert(branching_factor <= 4);
        if (branching_factor > 4)
        {
            //            throw std::runtime_error("Half box nodes are not supported for BVHs with branching factor > 4.");
        }

        return std::max(primitive_count / 2, 1u);
    }

    static std::uint64_t CalculateRequiredInteriorNodeBufferSize(const std::uint32_t                          primitive_count,
                                                                 const dxr::amd::BottomLevelFp16Mode          bottom_level_fp16_mode,
                                                                 const bool                                   half_fp32_box_nodes_enabled,
                                                                 const dxr::amd::AccelerationStructureBvhType bvh_type,
                                                                 const std::uint32_t                          branching_factor)
    {
        const auto box_node_per_interior_node_count = rta::ComputeBoxNodePerInteriorNodeCount(branching_factor);

        const auto worst_case_interior_node_count = (half_fp32_box_nodes_enabled)
                                                        ? CalculateWorstCaseMemorySizeInteriorHalfNodeCount(primitive_count, branching_factor)
                                                        : CalculateWorstCaseMemorySizeInteriorNodeCount(primitive_count, branching_factor);

        std::int32_t fp32_interior_node_count = 0;
        std::int32_t fp16_interior_node_count = 0;

        if (bvh_type == dxr::amd::AccelerationStructureBvhType::kAmdTopLevel)
        {
            fp32_interior_node_count = worst_case_interior_node_count;
        }
        else
        {
            // For bvhType == AsBvhType::kAmdBottomLevel
            if (bottom_level_fp16_mode == dxr::amd::BottomLevelFp16Mode::kLeafNodesOnly)
            {
                fp16_interior_node_count = primitive_count / 4;
                fp32_interior_node_count = worst_case_interior_node_count - fp16_interior_node_count;
                assert(fp32_interior_node_count >= 0);
            }
            else if (bottom_level_fp16_mode == dxr::amd::BottomLevelFp16Mode::kMixedWithFp32 || bottom_level_fp16_mode == dxr::amd::BottomLevelFp16Mode::kNone)
            {
                fp32_interior_node_count = worst_case_interior_node_count;
            }
            else if (bottom_level_fp16_mode == dxr::amd::BottomLevelFp16Mode::kAll)
            {
                fp16_interior_node_count = worst_case_interior_node_count - 1;
                fp32_interior_node_count = 1;
            }
        }

        // We at least require memory space for the root node
        fp32_interior_node_count = std::max(fp32_interior_node_count, 1);

        return box_node_per_interior_node_count * (static_cast<std::int64_t>(fp16_interior_node_count) * dxr::amd::kFp16BoxNodeSize +
                                                   static_cast<std::int64_t>(fp32_interior_node_count) * dxr::amd::kFp32BoxNodeSize);
    }

    IRtIp11AccelerationStructureHeader::~IRtIp11AccelerationStructureHeader()
    {
    }

    const IRtIp11AccelerationStructurePostBuildInfo& IRtIp11AccelerationStructureHeader::GetPostBuildInfo() const
    {
        return GetPostBuildInfoImpl();
    }

    std::uint32_t IRtIp11AccelerationStructureHeader::GetMetaDataSize() const
    {
        return GetMetaDataSizeImpl();
    }

    std::uint32_t IRtIp11AccelerationStructureHeader::GetFileSize() const
    {
        return GetFileSizeImpl();
    }

    std::uint32_t IRtIp11AccelerationStructureHeader::GetPrimitiveCount() const
    {
        return GetPrimitiveCountImpl();
    }

    std::uint32_t IRtIp11AccelerationStructureHeader::GetActivePrimitiveCount() const
    {
        return GetActivePrimitiveCountImpl();
    }

    std::uint32_t IRtIp11AccelerationStructureHeader::GetGeometryDescriptionCount() const
    {
        return GetGeometryDescriptionCountImpl();
    }

    BottomLevelBvhGeometryType IRtIp11AccelerationStructureHeader::GetGeometryType() const
    {
        return GetGeometryTypeImpl();
    }

    const AccelerationStructureBufferOffsets& IRtIp11AccelerationStructureHeader::GetBufferOffsets() const
    {
        return GetBufferOffsetsImpl();
    }

    void IRtIp11AccelerationStructureHeader::SetBufferOffsets(const AccelerationStructureBufferOffsets& offsets)
    {
        SetBufferOffsetsImpl(offsets);
    }

    std::uint32_t IRtIp11AccelerationStructureHeader::GetInteriorFp32NodeCount() const
    {
        return GetInteriorFp32NodeCountImpl();
    }

    void IRtIp11AccelerationStructureHeader::SetInteriorFp32NodeCount(const std::uint32_t interior_node_count)
    {
        SetInteriorFp32NodeCountImpl(interior_node_count);
    }

    std::uint32_t IRtIp11AccelerationStructureHeader::GetInteriorHalfFp32NodeCount() const
    {
        return GetInteriorHalfFp32NodeCountImpl();
    }

    void IRtIp11AccelerationStructureHeader::SetInteriorHalfFp32NodeCount(const std::uint32_t interior_node_count)
    {
        assert(IsHalfBoxNodeSupported());
        SetInteriorHalfFp32NodeCountImpl(interior_node_count);
    }

    std::uint32_t IRtIp11AccelerationStructureHeader::GetInteriorFp16NodeCount() const
    {
        return GetInteriorFp16NodeCountImpl();
    }

    void IRtIp11AccelerationStructureHeader::SetInteriorFp16NodeCount(const std::uint32_t interior_node_count)
    {
        SetInteriorFp16NodeCountImpl(interior_node_count);
    }

    RayTracingBinaryVersion IRtIp11AccelerationStructureHeader::GetGpuRtDriverInterfaceVersion() const
    {
        return GetGpuRtDriverInterfaceVersionImpl();
    }

    std::uint32_t IRtIp11AccelerationStructureHeader::GetInteriorNodeCount() const
    {
        return GetInteriorFp16NodeCount() + GetInteriorFp32NodeCount() + GetInteriorHalfFp32NodeCount();
    }

    std::uint64_t IRtIp11AccelerationStructureHeader::CalculateInteriorNodeBufferSize() const
    {
        return CalculateInteriorNodeBufferSizeImpl();
    }

    std::uint64_t IRtIp11AccelerationStructureHeader::CalculateWorstCaseInteriorNodeBufferSize(const std::uint32_t branching_factor) const
    {
        RRA_UNUSED(branching_factor);
        return CalculateWorstCaseInteriorNodeBufferSizeImpl();
    }

    std::uint32_t IRtIp11AccelerationStructureHeader::GetLeafNodeCount() const
    {
        return GetLeafNodeCountImpl();
    }

    void IRtIp11AccelerationStructureHeader::SetLeafNodeCount(const std::uint32_t leaf_node_count)
    {
        SetLeafNodeCountImpl(leaf_node_count);
    }

    std::uint64_t IRtIp11AccelerationStructureHeader::CalculateLeafNodeBufferSize() const
    {
        return CalculateLeafNodeBufferSizeImpl();
    }

    std::uint64_t IRtIp11AccelerationStructureHeader::CalculateActualLeafNodeBufferSize() const
    {
        return CalculateActualLeafNodeBufferSizeImpl();
    }

    std::uint64_t IRtIp11AccelerationStructureHeader::CalculateCompressionModeLeafNodeBufferSize(const BvhTriangleCompressionMode tri_compression_mode) const
    {
        return CalculateCompressionModeLeafNodeBufferSizeImpl(tri_compression_mode);
    }

    std::uint64_t IRtIp11AccelerationStructureHeader::CalculateWorstCaseLeafNodeBufferSize(const BvhTriangleCompressionMode tri_compression_mode) const
    {
        return CalculateWorstCaseLeafNodeBufferSizeImpl(tri_compression_mode);
    }

    void IRtIp11AccelerationStructureHeader::LoadFromBuffer(const std::uint64_t            size,
                                                            const void*                    buffer,
                                                            const RayTracingBinaryVersion& rt_binary_header_version)
    {
        LoadFromBufferImpl(size, buffer, rt_binary_header_version);
    }

    bool IRtIp11AccelerationStructureHeader::IsValid() const
    {
        return IsValidImpl();
    }

    bool IRtIp11AccelerationStructureHeader::IsHalfBoxNodeEnabled() const
    {
        return IsHalfBoxNodeEnabledImpl();
    }

    bool IRtIp11AccelerationStructureHeader::IsHalfBoxNodeSupported() const
    {
        return IsHalfBoxNodeSupportedImpl();
    }

    VulkanUniversalIdentifier::VulkanUniversalIdentifier(const std::uint32_t gfx_ip, const std::uint32_t build_time_hash)
        : gfx_ip_(gfx_ip)
        , build_time_hash_(build_time_hash)
    {
    }

    static_assert(sizeof(VulkanUniversalIdentifier) == 8, "VulkanUniversalIdentifier size does not match 8 Bytes.");

    struct DxrAccelerationStructureHeader
    {
        std::uint32_t                      build_info;                       ///< The build info flags.
        std::uint32_t                      meta_data_size_in_bytes;          ///< The size of the metadata, in bytes.
        std::uint32_t                      file_size_in_bytes;               ///< The size of the acceleration structure, in bytes.
        std::uint32_t                      primitive_count;                  ///< Num Primitives, 0 for Blas
        std::uint32_t                      active_primitive_count;           ///< The number of active primitives.
        std::uint32_t                      task_id_counter;                  ///< The task ID count.
        std::uint32_t                      desc_count;                       ///< The number of descriptors.
        dxr::GeometryType                  geometry_type;                    ///< Type of primitive contained in blas, invalid for tlas.
        AccelerationStructureBufferOffsets offsets;                          ///< Offsets to the node and primitive data relative to start of this header.
        std::uint32_t                      interior_fp32_node_count;         ///< Number of interior nodes in float32.
        std::uint32_t                      interior_fp16_node_count;         ///< Number of interior nodes in float16 (for compression).
        std::uint32_t                      leaf_node_count;                  ///< Number of leaf nodes (instances for tlas, geometry for blas).
        RayTracingBinaryVersion            driver_gpu_rt_interface_version;  ///< GpuRT version.
        VulkanUniversalIdentifier          universal_identifier;             ///< Vulkan-specific universal identifier.
        std::uint32_t                      interior_half_fp32_node_count;    ///< Number of half float32 box interior nodes.
        std::array<std::uint32_t, 13>      padding;                          ///< Padding for 128-byte alignment.
    };

    static_assert(sizeof(DxrAccelerationStructureHeader) == dxr::amd::kAccelerationStructureHeaderSize,
                  "DxrAccelerationStructureHeader does not have the expected byte size.");

    static_assert(std::is_trivially_copyable<DxrAccelerationStructureHeader>::value, "DxrAccelerationStructureHeader must be a trivially copyable class.");

    class DxrRtIp11AccelerationStructureHeader final : public IRtIp11AccelerationStructureHeader
    {
    public:
        // Use the default constructor when creating new acceleration structures.
        DxrRtIp11AccelerationStructureHeader();

    private:
        const IRtIp11AccelerationStructurePostBuildInfo& GetPostBuildInfoImpl() const;

        std::uint32_t GetMetaDataSizeImpl() const;

        std::uint32_t GetFileSizeImpl() const;

        std::uint32_t GetPrimitiveCountImpl() const;

        std::uint32_t GetActivePrimitiveCountImpl() const;

        std::uint32_t GetGeometryDescriptionCountImpl() const;

        BottomLevelBvhGeometryType GetGeometryTypeImpl() const;

        const AccelerationStructureBufferOffsets& GetBufferOffsetsImpl() const;

        void SetBufferOffsetsImpl(const AccelerationStructureBufferOffsets& offsets);

        std::uint32_t GetInteriorFp32NodeCountImpl() const;

        void SetInteriorFp32NodeCountImpl(const std::uint32_t interior_node_count);

        std::uint32_t GetInteriorHalfFp32NodeCountImpl() const override;

        void SetInteriorHalfFp32NodeCountImpl(const std::uint32_t interior_node_count) override;

        std::uint32_t GetInteriorFp16NodeCountImpl() const;

        void SetInteriorFp16NodeCountImpl(const std::uint32_t interior_node_count);

        RayTracingBinaryVersion GetGpuRtDriverInterfaceVersionImpl() const override;

        std::uint64_t CalculateInteriorNodeBufferSizeImpl() const;

        std::uint64_t CalculateWorstCaseInteriorNodeBufferSizeImpl(const std::uint32_t branching_factor = 4) const;

        std::uint32_t GetLeafNodeCountImpl() const;

        void SetLeafNodeCountImpl(const std::uint32_t leaf_node_count);

        std::uint64_t CalculateWorstCaseLeafNodeBufferSizeImpl(const BvhTriangleCompressionMode tri_compression_mode) const override;

        std::uint64_t CalculateActualLeafNodeBufferSizeImpl() const override;

        std::uint64_t CalculateCompressionModeLeafNodeBufferSizeImpl(const BvhTriangleCompressionMode tri_compression_mode) const override;

        std::uint64_t CalculateLeafNodeBufferSizeImpl() const;

        void LoadFromBufferImpl(const std::uint64_t            size,
                                const void*                    buffer,
                                const RayTracingBinaryVersion& rt_binary_header_version = kSupportedRayTracingBinaryHeaderVersion);

        bool IsValidImpl() const override;

        bool IsHalfBoxNodeEnabledImpl() const;

        bool IsHalfBoxNodeSupportedImpl() const;

    private:
        DxrAccelerationStructureHeader                             header_     = {};
        std::unique_ptr<IRtIp11AccelerationStructurePostBuildInfo> build_info_ = nullptr;
    };

    DxrRtIp11AccelerationStructureHeader::DxrRtIp11AccelerationStructureHeader()
        : build_info_(CreateRtIp11AccelerationStructurePostBuildInfo())
    {
    }

    const IRtIp11AccelerationStructurePostBuildInfo& DxrRtIp11AccelerationStructureHeader::GetPostBuildInfoImpl() const
    {
        return *build_info_;
    }

    std::uint32_t DxrRtIp11AccelerationStructureHeader::GetMetaDataSizeImpl() const
    {
        return header_.meta_data_size_in_bytes;
    }

    std::uint32_t DxrRtIp11AccelerationStructureHeader::GetFileSizeImpl() const
    {
        return header_.file_size_in_bytes;
    }

    std::uint32_t DxrRtIp11AccelerationStructureHeader::GetPrimitiveCountImpl() const
    {
        return header_.primitive_count;
    }

    std::uint32_t DxrRtIp11AccelerationStructureHeader::GetActivePrimitiveCountImpl() const
    {
        return header_.active_primitive_count;
    }

    std::uint32_t DxrRtIp11AccelerationStructureHeader::GetGeometryDescriptionCountImpl() const
    {
        return header_.desc_count;
    }

    BottomLevelBvhGeometryType DxrRtIp11AccelerationStructureHeader::GetGeometryTypeImpl() const
    {
        return ToBottomLevelBvhGeometryType(header_.geometry_type);
    }

    const AccelerationStructureBufferOffsets& DxrRtIp11AccelerationStructureHeader::GetBufferOffsetsImpl() const
    {
        return header_.offsets;
    }

    void DxrRtIp11AccelerationStructureHeader::SetBufferOffsetsImpl(const AccelerationStructureBufferOffsets& offsets)
    {
        header_.offsets = offsets;
    }

    std::uint32_t DxrRtIp11AccelerationStructureHeader::GetInteriorFp32NodeCountImpl() const
    {
        return header_.interior_fp32_node_count;
    }

    void DxrRtIp11AccelerationStructureHeader::SetInteriorFp32NodeCountImpl(const std::uint32_t interior_node_count)
    {
        header_.interior_fp32_node_count = interior_node_count;
    }

    std::uint32_t DxrRtIp11AccelerationStructureHeader::GetInteriorHalfFp32NodeCountImpl() const
    {
        return (header_.driver_gpu_rt_interface_version >= RayTracingBinaryVersion(11, 1)) ? header_.interior_half_fp32_node_count : 0;
    }

    void DxrRtIp11AccelerationStructureHeader::SetInteriorHalfFp32NodeCountImpl(const std::uint32_t interior_node_count)
    {
        header_.interior_half_fp32_node_count = interior_node_count;
    }

    std::uint32_t DxrRtIp11AccelerationStructureHeader::GetInteriorFp16NodeCountImpl() const
    {
        return header_.interior_fp16_node_count;
    }

    void DxrRtIp11AccelerationStructureHeader::SetInteriorFp16NodeCountImpl(const std::uint32_t interior_node_count)
    {
        header_.interior_fp16_node_count = interior_node_count;
    }

    RayTracingBinaryVersion DxrRtIp11AccelerationStructureHeader::GetGpuRtDriverInterfaceVersionImpl() const
    {
        return header_.driver_gpu_rt_interface_version;
    }

    std::uint32_t DxrRtIp11AccelerationStructureHeader::GetLeafNodeCountImpl() const
    {
        return header_.leaf_node_count;
    }

    void DxrRtIp11AccelerationStructureHeader::SetLeafNodeCountImpl(const std::uint32_t leaf_node_count)
    {
        header_.leaf_node_count = leaf_node_count;
    }

    std::uint64_t DxrRtIp11AccelerationStructureHeader::CalculateInteriorNodeBufferSizeImpl() const
    {
        return static_cast<std::uint64_t>(header_.interior_fp16_node_count) * dxr::amd::kFp16BoxNodeSize +
               static_cast<std::uint64_t>(header_.interior_fp32_node_count) * dxr::amd::kFp32BoxNodeSize +
               static_cast<std::uint64_t>(header_.interior_half_fp32_node_count) * dxr::amd::kFp32HalfBoxNodeSize;
    }

    std::uint64_t DxrRtIp11AccelerationStructureHeader::CalculateWorstCaseInteriorNodeBufferSizeImpl(const std::uint32_t branching_factor) const
    {
        return CalculateRequiredInteriorNodeBufferSize(GetPrimitiveCount(),
                                                       ToDxrBottomLevelFp16Mode(build_info_->GetBottomLevelFp16Mode()),
                                                       header_.interior_half_fp32_node_count > 0,
                                                       ToDxrBvhType(build_info_->GetBvhType()),
                                                       branching_factor);
    }

    std::uint64_t DxrRtIp11AccelerationStructureHeader::CalculateLeafNodeBufferSizeImpl() const
    {
        if (build_info_->IsTopLevel())
        {
            return static_cast<std::uint64_t>(header_.leaf_node_count) * dxr::amd::kInstanceNodeSize;
        }
        else
        {
            return static_cast<std::uint64_t>(header_.leaf_node_count) * dxr::amd::kLeafNodeSize;
        }
    }

    std::uint64_t DxrRtIp11AccelerationStructureHeader::CalculateActualLeafNodeBufferSizeImpl() const
    {
        // If it is enabled, we do not know how many triangles have actually been stored in the buffer.
        // Hence, use the offsets to calculate the actual buffer size.
        const auto gpuRtVersion = GetGpuRtDriverInterfaceVersion();

        if (GetPostBuildInfo().IsTopLevel())
        {
            if (gpuRtVersion >= RayTracingBinaryVersion(8, 0))
            {
                return GetBufferOffsets().prim_node_ptrs - GetBufferOffsets().leaf_nodes;
            }
            else
            {
                return GetFileSize() - GetMetaDataSize() - GetBufferOffsets().leaf_nodes;
            }
        }
        else
        {
            return GetBufferOffsets().geometry_info - GetBufferOffsets().leaf_nodes;
        }
    }

    std::uint64_t DxrRtIp11AccelerationStructureHeader::CalculateCompressionModeLeafNodeBufferSizeImpl(
        const BvhTriangleCompressionMode tri_compression_mode) const
    {
        // If triangle compression is disabled, just return the actual buffer size.
        if (tri_compression_mode == BvhTriangleCompressionMode::kNone)
        {
            return CalculateLeafNodeBufferSize();
        }

        // If it is enabled, we do not know how many triangles have actually been stored in the buffer.
        // Hence, use the offsets to calculate the actual buffer size.
        return CalculateActualLeafNodeBufferSize();
    }

    std::uint64_t DxrRtIp11AccelerationStructureHeader::CalculateWorstCaseLeafNodeBufferSizeImpl(const BvhTriangleCompressionMode tri_compression_mode) const
    {
        // If triangle compression is disabled, just return the actual buffer size.
        if (tri_compression_mode == BvhTriangleCompressionMode::kNone)
        {
            return CalculateLeafNodeBufferSize();
        }

        // If it is enabled, we do not know how many triangles have actually been stored in the buffer.
        // Hence, use the offsets to calculate the actual buffer size.
        const auto gpuRtVersion = GetGpuRtDriverInterfaceVersion();

        if (GetPostBuildInfo().IsTopLevel())
        {
            if (gpuRtVersion >= RayTracingBinaryVersion(8, 0))
            {
                return GetBufferOffsets().prim_node_ptrs - GetBufferOffsets().leaf_nodes;
            }
            else
            {
                return GetFileSize() - GetMetaDataSize() - GetBufferOffsets().leaf_nodes;
            }
        }
        else
        {
            return GetBufferOffsets().geometry_info - GetBufferOffsets().leaf_nodes;
        }
    }

    void DxrRtIp11AccelerationStructureHeader::LoadFromBufferImpl(const std::uint64_t            size,
                                                                  const void*                    buffer,
                                                                  const RayTracingBinaryVersion& rt_binary_header_version)
    {
        size_t struct_size = sizeof(DxrAccelerationStructureHeader);
        RRA_UNUSED(size);
        RRA_UNUSED(rt_binary_header_version);
        assert(size == struct_size);
        memcpy(&header_, buffer, struct_size);
        build_info_->LoadFromBuffer(sizeof(header_.build_info), &header_.build_info);

    }

    std::unique_ptr<IRtIp11AccelerationStructureHeader> CreateRtIp11AccelerationStructureHeader()
    {
        return std::make_unique<DxrRtIp11AccelerationStructureHeader>();
    }

    bool DxrRtIp11AccelerationStructureHeader::IsValidImpl() const
    {
        static constexpr std::uint32_t kMinimumFileSize = dxr::amd::kMetaDataAlignment + dxr::amd::kAccelerationStructureHeaderSize;

        // The minimum file size for RT IP 1.1 BVHs is 256B.
        if (header_.file_size_in_bytes < kMinimumFileSize)
        {
            return false;
        }
        // Meta data needs to be aligned with 128B.
        if (header_.meta_data_size_in_bytes < dxr::amd::kMetaDataAlignment)
        {
            return false;
        }
        // Check if there is any root node defined in this header.
        const bool has_root_node = (header_.interior_fp32_node_count > 0) || (header_.interior_half_fp32_node_count > 0);

        // If there are any active primitives in this BVH, make sure that the root node is allocated for this
        // BVH. Otherwise, it's invalid.
        if ((header_.file_size_in_bytes > kMinimumFileSize) && (header_.active_primitive_count > 0) && !has_root_node)
        {
            return false;
        }
        // Buffer offset values should be set in ascending order, according to the order they are
        // defined in the offset struct.
        if (header_.offsets.leaf_nodes < header_.offsets.interior_nodes || header_.offsets.prim_node_ptrs < header_.offsets.geometry_info)
        {
            return false;
        }
        // For top-level BVHs, geometry info can be 0 as it is not used.
        // For bottom-level BVHs, however, this offset must be defined properly.
        if (build_info_->IsBottomLevel() && header_.offsets.geometry_info < header_.offsets.leaf_nodes)
        {
            return false;
        }
        // The first interior node buffer offsets starts right after the acceleration structure header,
        // given that there is any root node stored in this BVH.
        if (has_root_node && header_.offsets.interior_nodes < dxr::amd::kAccelerationStructureHeaderSize)
        {
            return false;
        }
        // We cannot have more active primitives than stored in the BVH.
        if (header_.primitive_count < header_.active_primitive_count)
        {
            return false;
        }
        // Build info must be initialized.
        if (!build_info_)
        {
            return false;
        }

        return true;
    }

    bool DxrRtIp11AccelerationStructureHeader::IsHalfBoxNodeEnabledImpl() const
    {
        return (header_.interior_half_fp32_node_count > 0);
    }

    bool DxrRtIp11AccelerationStructureHeader::IsHalfBoxNodeSupportedImpl() const
    {
        return GetGpuRtDriverInterfaceVersion() > RayTracingBinaryVersion(11, 1);
    }

}  // namespace rta
