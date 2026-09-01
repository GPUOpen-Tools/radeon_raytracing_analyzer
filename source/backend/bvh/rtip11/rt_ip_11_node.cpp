//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
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

    RraErrorCode Rtip11Node::GetChildNodeCount(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_count) const
    {
        const auto& interior_nodes = bvh->GetInteriorNodesData();

        const auto&            header_offsets = bvh->GetHeader().GetBufferOffsets();
        dxr::amd::NodePointer* node_ptr       = reinterpret_cast<dxr::amd::NodePointer*>(&parent_node);
        auto                   byte_offset    = node_ptr->GetByteOffset() - header_offsets.interior_nodes;

        if (interior_nodes.size() > byte_offset)
        {
            if (node_ptr->IsFp32BoxNode())
            {
                const auto node  = reinterpret_cast<const dxr::amd::Float32BoxNode*>(&interior_nodes[byte_offset]);
                *out_child_count = node->GetValidChildCount();
            }
            else if (node_ptr->IsFp16BoxNode())
            {
                const auto node  = reinterpret_cast<const dxr::amd::Float16BoxNode*>(&interior_nodes[byte_offset]);
                *out_child_count = node->GetValidChildCount();
            }
            else
            {
                *out_child_count = 0;
            }
        }
        else
        {
            *out_child_count = 0;
        }
        return kRraOk;
    }

    RraErrorCode Rtip11Node::GetChildNodes(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_nodes) const
    {
        const auto& interior_nodes = bvh->GetInteriorNodesData();

        const auto&            header_offsets = bvh->GetHeader().GetBufferOffsets();
        dxr::amd::NodePointer* node_ptr       = reinterpret_cast<dxr::amd::NodePointer*>(&parent_node);
        auto                   byte_offset    = node_ptr->GetByteOffset() - header_offsets.interior_nodes;

        if (interior_nodes.size() > byte_offset)
        {
            if (node_ptr->IsFp32BoxNode())
            {
                const auto node     = reinterpret_cast<const dxr::amd::Float32BoxNode*>(&interior_nodes[byte_offset]);
                const auto children = node->GetChildren();
                for (size_t i = 0; i < children.size(); i++)
                {
                    if (!children[i].IsInvalid())
                    {
                        *out_child_nodes = children[i].GetRawPointer();
                        out_child_nodes++;
                    }
                }
            }
            else if (node_ptr->IsFp16BoxNode())
            {
                const auto node     = reinterpret_cast<const dxr::amd::Float16BoxNode*>(&interior_nodes[byte_offset]);
                const auto children = node->GetChildren();
                for (size_t i = 0; i < children.size(); i++)
                {
                    if (!children[i].IsInvalid())
                    {
                        *out_child_nodes = children[i].GetRawPointer();
                        out_child_nodes++;
                    }
                }
            }
        }
        return kRraOk;
    }

    RraErrorCode Rtip11Node::GetChildIndices(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_indices) const
    {
        const auto& interior_nodes = bvh->GetInteriorNodesData();

        const auto&            header_offsets = bvh->GetHeader().GetBufferOffsets();
        dxr::amd::NodePointer* node_ptr       = reinterpret_cast<dxr::amd::NodePointer*>(&parent_node);
        auto                   byte_offset    = node_ptr->GetByteOffset() - header_offsets.interior_nodes;

        if (interior_nodes.size() > byte_offset)
        {
            if (node_ptr->IsFp32BoxNode())
            {
                const auto node     = reinterpret_cast<const dxr::amd::Float32BoxNode*>(&interior_nodes[byte_offset]);
                const auto children = node->GetChildren();
                for (size_t i = 0; i < children.size(); i++)
                {
                    if (!children[i].IsInvalid())
                    {
                        out_child_indices[i] = (uint32_t)i;
                    }
                }
            }
            else if (node_ptr->IsFp16BoxNode())
            {
                const auto node     = reinterpret_cast<const dxr::amd::Float16BoxNode*>(&interior_nodes[byte_offset]);
                const auto children = node->GetChildren();
                for (size_t i = 0; i < children.size(); i++)
                {
                    if (!children[i].IsInvalid())
                    {
                        out_child_indices[i] = (uint32_t)i;
                    }
                }
            }
        }
        return kRraOk;
    }

    RraErrorCode Rtip11Node::GetChildNodePtr(const rta::IBvh* bvh, uint32_t parent_node, uint32_t child_index, uint32_t* out_node_id) const
    {
        const auto& interior_nodes = bvh->GetInteriorNodesData();

        const auto&            header_offsets = bvh->GetHeader().GetBufferOffsets();
        dxr::amd::NodePointer* node_ptr       = reinterpret_cast<dxr::amd::NodePointer*>(&parent_node);
        auto                   byte_offset    = node_ptr->GetByteOffset() - header_offsets.interior_nodes;

        if (interior_nodes.size() > byte_offset)
        {
            if (node_ptr->IsFp32BoxNode())
            {
                const auto node = reinterpret_cast<const dxr::amd::Float32BoxNode*>(&interior_nodes[byte_offset]);
                if (node->GetChildren().size() <= child_index)
                {
                    return kRraErrorIndexOutOfRange;
                }

                const auto& ptr = node->GetChildren()[child_index];
                if (!ptr.IsInvalid())
                {
                    *out_node_id = ptr.GetRawPointer();
                    return kRraOk;
                }
                else
                {
                    return kRraErrorInvalidChildNode;
                }
            }
            else if (node_ptr->IsFp16BoxNode())
            {
                const auto node = reinterpret_cast<const dxr::amd::Float16BoxNode*>(&interior_nodes[byte_offset]);
                if (node->GetChildren().size() <= child_index)
                {
                    return kRraErrorIndexOutOfRange;
                }

                const auto& ptr = node->GetChildren()[child_index];
                if (!ptr.IsInvalid())
                {
                    *out_node_id = ptr.GetRawPointer();
                    return kRraOk;
                }
                else
                {
                    return kRraErrorInvalidChildNode;
                }
            }
        }
        return kRraErrorIndexOutOfRange;
    }

    RraErrorCode Rtip11Node::GetNodeBoundingVolume(const rta::IBvh*                  bvh,
                                                   uint32_t                          node_id,
                                                   uint32_t                          child_index,
                                                   uint32_t                          global_child_index,
                                                   dxr::amd::AxisAlignedBoundingBox& out_bounding_box) const
    {
        RRA_UNUSED(child_index);
        const auto& interior_nodes = bvh->GetInteriorNodesData();

        dxr::amd::NodePointer* node_ptr = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);

        if (node_ptr->IsInvalid())
        {
            return kRraErrorInvalidPointer;
        }

        if (interior_nodes.size() == 0)
        {
            return kRraErrorInvalidPointer;
        }

        if (bvh->IsEmpty())
        {
            return kRraErrorInvalidPointer;
        }

        dxr::amd::NodePointer parent_node = bvh->GetParentNode(node_ptr->GetRawPointer(), global_child_index);

        // Need to get the node parent, and look for the node in the children of the parent, since that's where
        // the bounding box info is stored.
        if (parent_node.IsInvalid())
        {
            out_bounding_box = ComputeRootNodeBoundingBox(bvh);
            return kRraOk;
        }
        else
        {
            uint64_t parent_index;
            if (parent_node.IsBoxNode())
            {
                parent_index = parent_node.GetByteOffset() - bvh->GetHeader().GetBufferOffsets().interior_nodes;
            }
            else
            {
                parent_index = parent_node.GetByteOffset() - bvh->GetHeader().GetBufferOffsets().leaf_nodes;
            }

            if (parent_node.IsFp16BoxNode())
            {
                const dxr::amd::Float16BoxNode* box_node    = reinterpret_cast<const dxr::amd::Float16BoxNode*>(&interior_nodes[parent_index]);
                const auto&                     child_array = box_node->GetChildren();
                const auto&                     bbox_array  = box_node->GetBoundingBoxes();
                for (auto i = 0; i < 4; i++)
                {
                    if (child_array[i].GetRawPointer() == node_ptr->GetRawPointer())
                    {
                        out_bounding_box = bbox_array[i];
                        return kRraOk;
                    }
                }
            }
            else if (parent_node.IsFp32BoxNode())
            {
                const auto  box_node    = reinterpret_cast<const dxr::amd::Float32BoxNode*>(&interior_nodes[parent_index]);
                const auto& child_array = box_node->GetChildren();
                const auto& bbox_array  = box_node->GetBoundingBoxes();
                for (auto i = 0; i < 4; i++)
                {
                    if (child_array[i].GetRawPointer() == node_ptr->GetRawPointer())
                    {
                        out_bounding_box = bbox_array[i];
                        return kRraOk;
                    }
                }
            }
        }
        return kRraErrorInvalidPointer;
    }

}  // namespace rta

