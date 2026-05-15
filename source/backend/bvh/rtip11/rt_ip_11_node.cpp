//=============================================================================
// Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the RTIP11 node class.
//=============================================================================

#include "bvh/rtip11/rt_ip_11_node.h"

#include "bvh/rtip31/internal_node.h"

namespace rta
{
    dxr::amd::AxisAlignedBoundingBox Rtip11Node::ComputeRootNodeBoundingBox(const IBvh* bvh) const
    {
        const auto& interior_nodes = bvh->GetInteriorNodesData();
        const auto  box_node       = reinterpret_cast<const dxr::amd::Float32BoxNode*>(&interior_nodes[0]);

        dxr::amd::AxisAlignedBoundingBox box = {};

        const auto& bbox_array  = box_node->GetBoundingBoxes();
        const auto& child_array = box_node->GetChildren();

        box.min.x = FLT_MAX;
        box.min.y = FLT_MAX;
        box.min.z = FLT_MAX;
        box.max.x = -FLT_MAX;
        box.max.y = -FLT_MAX;
        box.max.z = -FLT_MAX;

        for (auto child_index = 0; child_index < 4; child_index++)
        {
            if (!child_array[child_index].IsInvalid())
            {
                auto bbox_min = bbox_array[child_index].min;
                auto bbox_max = bbox_array[child_index].max;

                box.min.x = std::min(box.min.x, bbox_min.x);
                box.min.y = std::min(box.min.y, bbox_min.y);
                box.min.z = std::min(box.min.z, bbox_min.z);

                box.max.x = std::max(box.max.x, bbox_max.x);
                box.max.y = std::max(box.max.y, bbox_max.y);
                box.max.z = std::max(box.max.z, bbox_max.z);
            }
        }
        return box;
    }

    uint32_t Rtip11Node::GetMaxChildCount() const
    {
        return 4;
    }

    uint32_t Rtip11Node::GetNodeObbIndex(uint32_t node_id, const IBvh* bvh) const
    {
        RRA_UNUSED(node_id);
        RRA_UNUSED(bvh);

        // No OBBs in RtIP 1.1
        return ObbDisabled;
    }

    std::array<uint32_t, MAX_CHILD_NODES> Rtip11Node::GetChildNodeArray(const rta::IBvh* bvh, uint32_t root_id, uint32_t node_offset) const
    {
        const std::vector<uint8_t>& interior_nodes = bvh->GetInteriorNodesData();
        dxr::amd::NodePointer       root_node(root_id);

        RRA_ASSERT(root_node.IsBoxNode());
        if (root_node.IsFp32BoxNode())
        {
            const dxr::amd::Float32BoxNode*       box_node = reinterpret_cast<const dxr::amd::Float32BoxNode*>(&interior_nodes[node_offset]);
            std::array<uint32_t, MAX_CHILD_NODES> children_padded{};
            const auto&                           children = box_node->GetChildren();
            std::copy(children.begin(), children.end(), (dxr::amd::NodePointer*)children_padded.data());

            return children_padded;
        }
        else
        {
            const dxr::amd::Float16BoxNode*       box_node = reinterpret_cast<const dxr::amd::Float16BoxNode*>(&interior_nodes[node_offset]);
            std::array<uint32_t, MAX_CHILD_NODES> children_padded{};
            const auto&                           children = box_node->GetChildren();
            std::copy(children.begin(), children.end(), (dxr::amd::NodePointer*)children_padded.data());

            return children_padded;
        }
    }

}  // namespace rta

