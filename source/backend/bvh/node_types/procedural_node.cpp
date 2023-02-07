//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for a procedural node class.
//=============================================================================

#include "bvh/node_types/procedural_node.h"

#include <cmath>  // --> isnan, isinf

namespace dxr
{
    // Check if headers and descriptions are trivially copyable (memcpy support).
    static_assert(std::is_trivially_copyable<dxr::amd::ProceduralNode>::value, "DXR::AMD::ProceduralNode must be a trivially copyable class.");

    // Check if the size of structs is as expected.
    static_assert(sizeof(amd::ProceduralNode) == amd::kLeafNodeSize, "ProceduralNode does not have the expected byte size.");

    namespace amd
    {
        const AxisAlignedBoundingBox& ProceduralNode::GetBoundingBox() const
        {
            return bounding_box_;
        }

        std::uint32_t ProceduralNode::GetGeometryIndex() const
        {
            return geometry_index;
        }

        GeometryFlags ProceduralNode::GetGeometryFlags() const
        {
            return static_cast<GeometryFlags>(geometry_flags);
        }

        std::uint32_t ProceduralNode::GetPrimitiveIndex() const
        {
            return primitive_index_;
        }

        bool ProceduralNode::IsInactive() const
        {
            return std::isnan(bounding_box_.min.x);
        }

        bool ProceduralNode::ContainsNaN() const
        {
            return std::isnan(bounding_box_.min.x) || std::isnan(bounding_box_.min.y) || std::isnan(bounding_box_.min.z) || std::isnan(bounding_box_.max.x) ||
                   std::isnan(bounding_box_.max.y) || std::isnan(bounding_box_.max.z);
        }

    }  // namespace amd
}  // namespace dxr
