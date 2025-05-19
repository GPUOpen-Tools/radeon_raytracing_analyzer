//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the RT IP 1.1 acceleration structure header class.
//=============================================================================

#include <string.h>  // for memcpy()

#include "bvh/dxr_type_conversion.h"
#include "bvh/rt_binary_file_defs.h"
#include "bvh/rtip_common/gpurt_accel_struct.h"
#include "bvh/rtip_common/i_acceleration_structure_header.h"
#include "bvh/utils.h"

namespace rta
{

    IRtIpCommonAccelerationStructureHeader::~IRtIpCommonAccelerationStructureHeader()
    {
    }

    const IRtIpCommonAccelerationStructurePostBuildInfo& IRtIpCommonAccelerationStructureHeader::GetPostBuildInfo() const
    {
        return GetPostBuildInfoImpl();
    }

    std::uint32_t IRtIpCommonAccelerationStructureHeader::GetMetaDataSize() const
    {
        return GetMetaDataSizeImpl();
    }

    std::uint32_t IRtIpCommonAccelerationStructureHeader::GetFileSize() const
    {
        return GetFileSizeImpl();
    }

    std::uint32_t IRtIpCommonAccelerationStructureHeader::GetPrimitiveCount() const
    {
        return GetPrimitiveCountImpl();
    }

    std::uint32_t IRtIpCommonAccelerationStructureHeader::GetActivePrimitiveCount() const
    {
        return GetActivePrimitiveCountImpl();
    }

    std::uint32_t IRtIpCommonAccelerationStructureHeader::GetGeometryDescriptionCount() const
    {
        return GetGeometryDescriptionCountImpl();
    }

    BottomLevelBvhGeometryType IRtIpCommonAccelerationStructureHeader::GetGeometryType() const
    {
        return GetGeometryTypeImpl();
    }

    const AccelerationStructureBufferOffsets& IRtIpCommonAccelerationStructureHeader::GetBufferOffsets() const
    {
        return GetBufferOffsetsImpl();
    }

    void IRtIpCommonAccelerationStructureHeader::SetBufferOffsets(const AccelerationStructureBufferOffsets& offsets)
    {
        SetBufferOffsetsImpl(offsets);
    }

    std::uint32_t IRtIpCommonAccelerationStructureHeader::GetInteriorFp32NodeCount() const
    {
        return GetInteriorFp32NodeCountImpl();
    }

    void IRtIpCommonAccelerationStructureHeader::SetInteriorFp32NodeCount(const std::uint32_t interior_node_count)
    {
        SetInteriorFp32NodeCountImpl(interior_node_count);
    }

    std::uint32_t IRtIpCommonAccelerationStructureHeader::GetInteriorFp16NodeCount() const
    {
        return GetInteriorFp16NodeCountImpl();
    }

    void IRtIpCommonAccelerationStructureHeader::SetInteriorFp16NodeCount(const std::uint32_t interior_node_count)
    {
        SetInteriorFp16NodeCountImpl(interior_node_count);
    }

    RayTracingBinaryVersion IRtIpCommonAccelerationStructureHeader::GetGpuRtDriverInterfaceVersion() const
    {
        return GetGpuRtDriverInterfaceVersionImpl();
    }

    std::uint32_t IRtIpCommonAccelerationStructureHeader::GetInteriorNodeCount() const
    {
        return GetInteriorNodeCountImpl();
    }

    std::uint64_t IRtIpCommonAccelerationStructureHeader::CalculateInteriorNodeBufferSize() const
    {
        return CalculateInteriorNodeBufferSizeImpl();
    }

    std::uint64_t IRtIpCommonAccelerationStructureHeader::CalculateWorstCaseInteriorNodeBufferSize(const std::uint32_t branching_factor) const
    {
        return CalculateWorstCaseInteriorNodeBufferSizeImpl(branching_factor);
    }

    std::uint32_t IRtIpCommonAccelerationStructureHeader::GetLeafNodeCount() const
    {
        return GetLeafNodeCountImpl();
    }

    void IRtIpCommonAccelerationStructureHeader::SetLeafNodeCount(const std::uint32_t leaf_node_count)
    {
        SetLeafNodeCountImpl(leaf_node_count);
    }

    std::uint64_t IRtIpCommonAccelerationStructureHeader::CalculateLeafNodeBufferSize() const
    {
        return CalculateLeafNodeBufferSizeImpl();
    }

    std::uint64_t IRtIpCommonAccelerationStructureHeader::CalculateActualLeafNodeBufferSize() const
    {
        return CalculateActualLeafNodeBufferSizeImpl();
    }

    std::uint64_t IRtIpCommonAccelerationStructureHeader::CalculateCompressionModeLeafNodeBufferSize(
        const BvhTriangleCompressionMode tri_compression_mode) const
    {
        return CalculateCompressionModeLeafNodeBufferSizeImpl(tri_compression_mode);
    }

    std::uint64_t IRtIpCommonAccelerationStructureHeader::CalculateWorstCaseLeafNodeBufferSize(const BvhTriangleCompressionMode tri_compression_mode) const
    {
        return CalculateWorstCaseLeafNodeBufferSizeImpl(tri_compression_mode);
    }

    void IRtIpCommonAccelerationStructureHeader::LoadFromBuffer(const std::uint64_t            size,
                                                                const void*                    buffer,
                                                                const RayTracingBinaryVersion& rt_binary_header_version)
    {
        LoadFromBufferImpl(size, buffer, rt_binary_header_version);
    }

    bool IRtIpCommonAccelerationStructureHeader::IsValid() const
    {
        return IsValidImpl();
    }

#ifdef __cplusplus
    // Miscellaneous packed fields describing the acceleration structure and the build method.
    union AccelStructHeaderInfo2
    {
        struct
        {
            std::uint32_t compacted : 1;  // This BVH has been compacted
            std::uint32_t reserved : 31;  // Unused bits
        };

        std::uint32_t u32All;
    };
#else
    typedef uint32 AccelStructHeaderInfo2;
#endif

    VulkanUniversalIdentifier::VulkanUniversalIdentifier(const std::uint32_t gfx_ip, const std::uint32_t build_time_hash)
        : gfx_ip_(gfx_ip)
        , build_time_hash_(build_time_hash)
    {
    }

    static_assert(sizeof(VulkanUniversalIdentifier) == 8, "VulkanUniversalIdentifier size does not match 8 Bytes.");

}  // namespace rta

