//=============================================================================
// Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for a float 16 box node class.
//=============================================================================

#include "bvh/node_types/float16_box_node.h"
#include "bvh/node_types/box_node.h"

#include "bvh/utils.h"

namespace dxr
{
    // Check if headers and descriptions are trivially copyable (memcpy support).
    static_assert(std::is_trivially_copyable<dxr::amd::Float16BoxNode>::value, "DXR::AMD::Float16BoxNode must be a trivially copyable class.");

    // Check if the size of structs is as expected.
    static_assert(sizeof(amd::Float16BoxNode) == amd::kFp16BoxNodeSize, "Float16BoxNode does not have the expected byte size.");

    namespace amd
    {
        const std::array<NodePointer, 4>& Float16BoxNode::GetChildren() const
        {
            return children_;
        }

        const std::array<AxisAlignedBoundingBox, 4> Float16BoxNode::GetBoundingBoxes() const
        {
            std::array<AxisAlignedBoundingBox, 4> fp32_bounding_boxes;

            rta::ConvertHalfToFloat(reinterpret_cast<const HalfFloat*>(bounding_boxes_.data()), reinterpret_cast<float*>(fp32_bounding_boxes.data()), 4 * 6);

            return fp32_bounding_boxes;
        }

        std::uint32_t Float16BoxNode::GetValidChildCount() const
        {
            return GetValidChildCountFromArray(children_);
        }

    }  // namespace amd
}  // namespace dxr