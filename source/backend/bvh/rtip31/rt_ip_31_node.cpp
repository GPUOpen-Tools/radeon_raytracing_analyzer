//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
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

    RraErrorCode Rtip31Node::GetChildNodeCount(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_count) const
    {
        const auto& interior_nodes = bvh->GetInteriorNodesData();

        const auto&            header_offsets = bvh->GetHeader().GetBufferOffsets();
        dxr::amd::NodePointer* node_ptr       = reinterpret_cast<dxr::amd::NodePointer*>(&parent_node);
        auto                   byte_offset    = node_ptr->GetByteOffset() - header_offsets.interior_nodes;

        if (interior_nodes.size() > byte_offset)
        {
            if (node_ptr->IsFp32BoxNode())
            {
                const auto node  = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[byte_offset]);
                *out_child_count = node->ValidChildCount();
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

    RraErrorCode Rtip31Node::GetChildNodes(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_nodes) const
    {
        const auto& interior_nodes = bvh->GetInteriorNodesData();

        const auto&            header_offsets = bvh->GetHeader().GetBufferOffsets();
        dxr::amd::NodePointer* node_ptr       = reinterpret_cast<dxr::amd::NodePointer*>(&parent_node);
        auto                   byte_offset    = node_ptr->GetByteOffset() - header_offsets.interior_nodes;

        if (interior_nodes.size() > byte_offset)
        {
            if (node_ptr->IsFp32BoxNode())
            {
                const auto node = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[byte_offset]);
                uint32_t   child_nodes[8]{};
                node->DecodeChildrenOffsets(child_nodes);

                // DecodeChildrenOffsets writes to all 8 children slots, but out_child_nodes only has allocated number of valid children.
                for (uint32_t i{0}; i < node->ValidChildCount(); ++i)
                {
                    if (!dxr::amd::NodePointer(child_nodes[i]).IsInvalid())
                    {
                        out_child_nodes[i] = child_nodes[i];
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

    RraErrorCode Rtip31Node::GetChildIndices(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_indices) const
    {
        const auto& interior_nodes = bvh->GetInteriorNodesData();

        const auto&            header_offsets = bvh->GetHeader().GetBufferOffsets();
        dxr::amd::NodePointer* node_ptr       = reinterpret_cast<dxr::amd::NodePointer*>(&parent_node);
        auto                   byte_offset    = node_ptr->GetByteOffset() - header_offsets.interior_nodes;

        if (interior_nodes.size() > byte_offset)
        {
            if (node_ptr->IsFp32BoxNode())
            {
                const auto node = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[byte_offset]);
                uint32_t   child_nodes[8]{};
                node->DecodeChildrenOffsets(child_nodes);

                // DecodeChildrenOffsets writes to all 8 children slots, but out_child_nodes only has allocated number of valid children.
                for (uint32_t i{0}; i < node->ValidChildCount(); ++i)
                {
                    if (!dxr::amd::NodePointer(child_nodes[i]).IsInvalid())
                    {
                        out_child_indices[i] = i;
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

    RraErrorCode Rtip31Node::GetChildNodePtr(const rta::IBvh* bvh, uint32_t parent_node, uint32_t child_index, uint32_t* out_node_id) const
    {
        const auto& interior_nodes = bvh->GetInteriorNodesData();

        const auto&            header_offsets = bvh->GetHeader().GetBufferOffsets();
        dxr::amd::NodePointer* node_ptr       = reinterpret_cast<dxr::amd::NodePointer*>(&parent_node);
        auto                   byte_offset    = node_ptr->GetByteOffset() - header_offsets.interior_nodes;

        if (interior_nodes.size() > byte_offset)
        {
            if (node_ptr->IsFp32BoxNode())
            {
                const auto node = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[byte_offset]);
                if (node->ValidChildCount() <= child_index)
                {
                    return kRraErrorIndexOutOfRange;
                }

                uint32_t child_pointers[8]{};
                node->DecodeChildrenOffsets(child_pointers);
                *out_node_id = child_pointers[child_index];
                return kRraOk;
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

    RraErrorCode Rtip31Node::GetNodeBoundingVolume(const rta::IBvh*                  bvh,
                                                   uint32_t                          node_id,
                                                   uint32_t                          child_index,
                                                   uint32_t                          global_child_index,
                                                   dxr::amd::AxisAlignedBoundingBox& out_bounding_box) const
    {
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

                // Node packing (RTIP3.1): multiple child slots may point to the same child node, each with its
                // own bounding box. Honor child_index so a packed slot returns its own box instead of the first
                // matching slot's box. Fall back to first-match when child_index doesn't identify this node.
                if (child_index < 4 && child_array[child_index].GetRawPointer() == node_ptr->GetRawPointer())
                {
                    out_bounding_box = bbox_array[child_index];
                    return kRraOk;
                }

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
                QuantizedBVH8BoxNode node = *reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[parent_index]);
                uint32_t             child_nodes[8]{};
                node.DecodeChildrenOffsets(child_nodes);

                // Node packing (RTIP3.1): multiple child slots may point to the same child node, each with its
                // own quantized box. Honor child_index so a packed slot returns its own box instead of the first
                // matching slot's box. Fall back to first-match when child_index doesn't identify this node.
                if (child_index < node.ValidChildCount() && child_nodes[child_index] == node_ptr->GetRawPointer())
                {
                    auto bbox = node.childInfos[child_index].DecodeBounds(node.Origin(), node.Exponents());

                    out_bounding_box.min.x = bbox.min.x;
                    out_bounding_box.min.y = bbox.min.y;
                    out_bounding_box.min.z = bbox.min.z;

                    out_bounding_box.max.x = bbox.max.x;
                    out_bounding_box.max.y = bbox.max.y;
                    out_bounding_box.max.z = bbox.max.z;
                    return kRraOk;
                }

                for (uint32_t i{0}; i < node.ValidChildCount(); ++i)
                {
                    if (child_nodes[i] == node_ptr->GetRawPointer())
                    {
                        auto bbox = node.childInfos[i].DecodeBounds(node.Origin(), node.Exponents());

                        out_bounding_box.min.x = bbox.min.x;
                        out_bounding_box.min.y = bbox.min.y;
                        out_bounding_box.min.z = bbox.min.z;

                        out_bounding_box.max.x = bbox.max.x;
                        out_bounding_box.max.y = bbox.max.y;
                        out_bounding_box.max.z = bbox.max.z;
                        return kRraOk;
                    }
                }
            }
        }
        return kRraErrorInvalidPointer;
    }

}  // namespace rta

