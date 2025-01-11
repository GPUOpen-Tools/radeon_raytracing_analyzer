//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 1.1 (Navi2x) specific acceleration structure header
/// definition.
//=============================================================================

#include "bvh/rtip_common/acceleration_structure_header.h"

#ifndef RRA_BACKEND_BVH_RT_IP_11_ACCELERATION_STRUCTURE_HEADER_H_
#define RRA_BACKEND_BVH_RT_IP_11_ACCELERATION_STRUCTURE_HEADER_H_

namespace rta
{
    class DxrRtIp11AccelerationStructureHeader final : public RtIpCommonAccelerationStructureHeader
    {
    private:
        void LoadFromBufferImpl(const std::uint64_t            size,
                                const void*                    buffer,
                                const RayTracingBinaryVersion& rt_binary_header_version = kSupportedRayTracingBinaryHeaderVersion);
    };
}  // namespace rta
#endif  // RRA_BACKEND_BVH_RT_IP_11_ACCELERATION_STRUCTURE_HEADER_H_
