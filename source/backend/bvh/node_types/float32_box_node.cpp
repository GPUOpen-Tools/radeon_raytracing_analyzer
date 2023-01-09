//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for a float 32 box node class.
//=============================================================================

#include "bvh/node_types/float32_box_node.h"
#include "bvh/node_types/box_node.h"

namespace dxr
{
    // Check if headers and descriptions are trivially copyable (memcpy support).
    static_assert(std::is_trivially_copyable<dxr::amd::Float32BoxNode>::value, "DXR::AMD::Float32BoxNode must be a trivially copyable class.");

    // Check if the size of structs is as expected.
    static_assert(sizeof(amd::Float32BoxNode) == amd::kFp32BoxNodeSize, "Float32BoxNode does not have the expected byte size.");

    namespace amd
    {
        const std::array<NodePointer, 4>& Float32BoxNode::GetChildren() const
        {
            return children_;
        }

        const std::array<AxisAlignedBoundingBox, 4>& Float32BoxNode::GetBoundingBoxes() const
        {
            return bounding_boxes_;
        }

        std::uint32_t Float32BoxNode::GetValidChildCount() const
        {
            return GetValidChildCountFromArray(children_);
        }
    }  // namespace amd
}  // namespace dxr