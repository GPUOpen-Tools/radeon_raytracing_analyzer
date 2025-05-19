//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the geometry info class.
//=============================================================================

#include "bvh/geometry_info.h"

namespace dxr
{
    // Check if headers and descriptions are trivially copyable (memcpy support).
    static_assert(std::is_trivially_copyable<dxr::amd::GeometryInfo>::value, "DXR::AMD::GeometryInfo must be a trivially copyable class.");

    // Check if the size of structs is as expected.
    static_assert(sizeof(amd::GeometryInfo) == amd::kGeometryInfoSize, "Size of GeometryDesc does not match the expected byte size");

    namespace amd
    {
        GeometryFlags GeometryInfo::GetGeometryFlags() const
        {
            return static_cast<GeometryFlags>(geometry_flags);
        }

        std::uint32_t GeometryInfo::GetPrimitiveCount() const
        {
            return primitive_count;
        }

        std::uint32_t GeometryInfo::GetPrimitiveNodePtrsOffset() const
        {
            return primitive_node_ptrs_offset_;
        }
    }  // namespace amd
}  // namespace dxr

