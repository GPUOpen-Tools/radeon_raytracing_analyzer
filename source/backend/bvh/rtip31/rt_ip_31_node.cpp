//=============================================================================
// Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the RTIP31 node class.
//=============================================================================

#include "bvh/rtip31/rt_ip_31_node.h"

#include "bvh/rtip31/internal_node.h"

namespace rta
{
    dxr::amd::AxisAlignedBoundingBox Rtip31Node::ComputeRootNodeBoundingBox(const IBvh* bvh) const
    {
        const auto& interior_nodes = bvh->GetInteriorNodesData();
        const auto  box_node       = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[0]);

        QuantizedBVH8BoxNode node = *box_node;

        dxr::amd::AxisAlignedBoundingBox box = {};
        box.min.x                            = FLT_MAX;
        box.min.y                            = FLT_MAX;
        box.min.z                            = FLT_MAX;
        box.max.x                            = -FLT_MAX;
        box.max.y                            = -FLT_MAX;
        box.max.z                            = -FLT_MAX;

        // DecodeChildrenOffsets writes to all 8 children slots, but out_child_nodes only has allocated number of valid children.
        for (uint32_t i{0}; i < box_node->ValidChildCount(); ++i)
        {
            auto child_node = node.childInfos[i];
            auto bbox       = child_node.DecodeBounds(node.Origin(), node.Exponents());

            auto bbox_min = bbox.min;
            auto bbox_max = bbox.max;

            box.min.x = std::min(box.min.x, bbox_min.x);
            box.min.y = std::min(box.min.y, bbox_min.y);
            box.min.z = std::min(box.min.z, bbox_min.z);

            box.max.x = std::max(box.max.x, bbox_max.x);
            box.max.y = std::max(box.max.y, bbox_max.y);
            box.max.z = std::max(box.max.z, bbox_max.z);
        }

        return box;
    }

    uint32_t Rtip31Node::GetMaxChildCount() const
    {
        return 8;
    }

    uint32_t Rtip31Node::GetNodeObbIndex(uint32_t node_id, const IBvh* bvh) const
    {
        const auto&            interior_nodes = bvh->GetInteriorNodesData();
        dxr::amd::NodePointer* node           = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);

        assert(node->IsFp32BoxNode());

        if (interior_nodes.empty())
        {
            return ObbDisabled;
        }

        auto                 byte_offset = node->GetByteOffset() - bvh->GetHeader().GetBufferOffsets().interior_nodes;
        QuantizedBVH8BoxNode box_node    = *reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[byte_offset]);

        return box_node.OBBMatrixIndex();
    }

    std::array<uint32_t, MAX_CHILD_NODES> Rtip31Node::GetChildNodeArray(const rta::IBvh* bvh, uint32_t root_id, uint32_t node_offset) const
    {
        const std::vector<uint8_t>& interior_nodes = bvh->GetInteriorNodesData();
        dxr::amd::NodePointer       root_node(root_id);

        RRA_ASSERT(root_node.IsBoxNode());
        if (root_node.IsFp32BoxNode())
        {
            const auto                            node = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[node_offset]);
            std::array<uint32_t, MAX_CHILD_NODES> children{};
            node->DecodeChildrenOffsets(children.data());
            return children;
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

