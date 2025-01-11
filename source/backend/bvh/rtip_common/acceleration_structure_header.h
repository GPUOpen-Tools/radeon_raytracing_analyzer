//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 1.1 acceleration structure header definition.
//=============================================================================

#include "bvh/rtip_common/i_acceleration_structure_header.h"

#include <cstring>  // --> Linux, memcpy

#ifndef RRA_BACKEND_BVH_ACCELERATION_STRUCTURE_HEADER_H_
#define RRA_BACKEND_BVH_ACCELERATION_STRUCTURE_HEADER_H_

namespace rta
{
    class RtIpCommonAccelerationStructureHeader : public IRtIpCommonAccelerationStructureHeader
    {
    public:
        // Use the default constructor when creating new acceleration structures.
        RtIpCommonAccelerationStructureHeader();

    private:
        const IRtIpCommonAccelerationStructurePostBuildInfo& GetPostBuildInfoImpl() const;
        std::uint32_t                                        GetMetaDataSizeImpl() const;
        std::uint32_t                                        GetFileSizeImpl() const;
        std::uint32_t                                        GetPrimitiveCountImpl() const;
        std::uint32_t                                        GetActivePrimitiveCountImpl() const;
        std::uint32_t                                        GetGeometryDescriptionCountImpl() const;
        BottomLevelBvhGeometryType                           GetGeometryTypeImpl() const;
        const AccelerationStructureBufferOffsets&            GetBufferOffsetsImpl() const;
        void                                                 SetBufferOffsetsImpl(const AccelerationStructureBufferOffsets& offsets);
        std::uint32_t                                        GetInteriorFp32NodeCountImpl() const;
        void                                                 SetInteriorFp32NodeCountImpl(const std::uint32_t interior_node_count);
        std::uint32_t                                        GetInteriorFp16NodeCountImpl() const;
        void                                                 SetInteriorFp16NodeCountImpl(const std::uint32_t interior_node_count);
        RayTracingBinaryVersion                              GetGpuRtDriverInterfaceVersionImpl() const override;
        std::uint32_t                                        GetInteriorNodeCountImpl() const;
        std::uint64_t                                        CalculateInteriorNodeBufferSizeImpl() const;
        std::uint64_t                                        CalculateWorstCaseInteriorNodeBufferSizeImpl(const std::uint32_t branching_factor = 8) const;
        std::uint32_t                                        GetLeafNodeCountImpl() const;
        void                                                 SetLeafNodeCountImpl(const std::uint32_t leaf_node_count);
        std::uint64_t CalculateWorstCaseLeafNodeBufferSizeImpl(const BvhTriangleCompressionMode tri_compression_mode) const override;
        std::uint64_t CalculateActualLeafNodeBufferSizeImpl() const override;
        std::uint64_t CalculateCompressionModeLeafNodeBufferSizeImpl(const BvhTriangleCompressionMode tri_compression_mode) const override;
        std::uint64_t CalculateLeafNodeBufferSizeImpl() const;

        bool IsValidImpl() const override;

    protected:
        AccelStructHeader                                              header_     = {};
        std::unique_ptr<IRtIpCommonAccelerationStructurePostBuildInfo> build_info_ = nullptr;
    };
}  // namespace rta
#endif  // RRA_BACKEND_BVH_ACCELERATION_STRUCTURE_HEADER_H_
