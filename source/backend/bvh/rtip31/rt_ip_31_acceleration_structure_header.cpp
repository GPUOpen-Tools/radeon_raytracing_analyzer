//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 3.1 (Navi4x) specific acceleration structure header
/// implementation.
//=============================================================================

#include "bvh/rtip31/rt_ip_31_acceleration_structure_header.h"

#include <cstring>  // --> Linux, memcpy

#include "bvh/dxr_type_conversion.h"
#include "bvh/utils.h"

namespace rta
{
    void DxrRtIp31AccelerationStructureHeader::LoadFromBufferImpl(const std::uint64_t            size,
                                                                  const void*                    buffer,
                                                                  const RayTracingBinaryVersion& rt_binary_header_version)
    {
        RRA_UNUSED(size);
        RRA_UNUSED(rt_binary_header_version);
        size_t struct_size = sizeof(AccelStructHeader);
        assert(size == struct_size);
        memcpy(&header_, buffer, struct_size);
        build_info_->LoadFromBuffer(sizeof(header_.info), &header_.info);
    }
}  // namespace rta

