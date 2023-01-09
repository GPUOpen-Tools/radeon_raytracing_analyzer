//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a node pointer class.
//=============================================================================

#include "node_pointer.h"

#include "flags_util.h"

namespace dxr
{
    // Check if headers and descriptions are trivially copyable (memcpy support).
    static_assert(std::is_trivially_copyable<dxr::amd::NodePointer>::value, "DXR::AMD::NodePointer must be a trivially copyable class.");

    // Check if the size of structs is as expected.
    static_assert(sizeof(amd::NodePointer) == sizeof(std::uint32_t), "Node pointer size should match 4-Byte");

    namespace amd
    {
        NodePointer::NodePointer()
            : ptr_(kInvalidNode)
        {
        }

        NodePointer::NodePointer(const NodeType type, const std::uint32_t address)
            : type_(static_cast<std::uint32_t>(type))
            , address_(address >> 6)
        {
        }

        NodePointer::NodePointer(const std::uint32_t pointer)
            : ptr_(pointer)
        {
        }

        uint32_t NodePointer::GetByteOffset() const
        {
            // The first 3-bits ([0-2]) of this address are zero as they are reserved for
            // the node type. The shift is because we assume at least 64-byte (1 << 6)
            // alignment of the address. Hence, the bits [3-5] can also be neglected here.
            return address_ << 6;
        }

        std::uint32_t NodePointer::CalculateParentLinkIndex(const std::uint32_t parent_data_size, const TriangleCompressionMode compression_mode) const
        {
            return (parent_data_size - GetParentByteOffset(compression_mode)) / sizeof(NodePointer);
        }

        std::uint32_t NodePointer::GetGpuVirtualAddress() const
        {
            // The same with byte offset. To get the Gpu virtual address, we restore
            // the original address by shifting the value by 6 bits to the left. With this, we can
            // restore the 64-byte aligned virtual memory address.
            return address_ << 6;
        }

        NodeType NodePointer::GetType() const
        {
            return static_cast<NodeType>(type_);
        }

        std::uint32_t NodePointer::GetID(const std::uint32_t offset) const
        {
            const std::uint32_t byte_offset = GetByteOffset() - offset;

            if (IsInstanceNode())
            {
                // We need to subtract the As header size and the offset to the
                // internal nodes to get the correct index
                return byte_offset / kInstanceNodeSize;
            }
            else if (IsBoxNode())
            {
                if (GetType() == NodeType::kAmdNodeBoxFp32)
                {
                    // return ptr_ >> 4 - 1;
                    return byte_offset / kFp32BoxNodeSize;
                }
                else
                {
                    // - 2 because of the AS header offset.
                    //return ptr_ >> 3 - 2;
                    return byte_offset / kFp16BoxNodeSize;
                }
            }
            else if (IsTriangleNode())
            {
                // Or ptr_ >> 3 - nodeCount * 2 (for FP32) | - nodeCount (for FP16)
                return byte_offset / (kLeafNodeSize);
            }
            else
            {
                return UINT32_MAX;
            }
        }

        std::uint32_t NodePointer::GetParentByteOffset(const TriangleCompressionMode compression_mode) const
        {
            const auto          link_align_offset = (compression_mode == TriangleCompressionMode::kAmdTwoTriangleCompression) ? 1 : 0;
            const std::uint32_t link_index_offset   = address_ << link_align_offset;

            auto compressed_prim_offset = 0;
            if (IsTriangleNode() && compression_mode == TriangleCompressionMode::kAmdTwoTriangleCompression)
            {
                compressed_prim_offset = rta::to_underlying(GetType());
            }

            const auto link_index = link_index_offset + compressed_prim_offset - 1;

            const auto byte_offset = link_index * sizeof(dxr::amd::NodePointer);
            return static_cast<uint32_t>(byte_offset);
        }

        bool NodePointer::IsTriangleNode() const
        {
            return GetType() <= NodeType::kAmdNodeTriangle3;
        }

        bool NodePointer::IsBoxNode() const
        {
            return IsFp32BoxNode() || IsFp16BoxNode();
        }

        bool NodePointer::IsLeafNode() const
        {
            return IsInstanceNode() || IsTriangleNode() || IsProceduralNode();
        }

        bool NodePointer::IsFp32BoxNode() const
        {
            return GetType() == NodeType::kAmdNodeBoxFp32;
        }

        bool NodePointer::IsFp16BoxNode() const
        {
            return GetType() == NodeType::kAmdNodeBoxFp16;
        }

        bool NodePointer::IsRoot() const
        {
            return GetByteOffset() == dxr::amd::kAccelerationStructureHeaderSize;
        }

        bool NodePointer::IsProceduralNode() const
        {
            return GetType() == NodeType::kAmdNodeProcedural;
        }

        bool NodePointer::IsInstanceNode() const
        {
            return GetType() == NodeType::kAmdNodeInstance;
        }

        bool NodePointer::IsInvalid() const
        {
            return ptr_ == kInvalidNode;
        }

        std::uint32_t NodePointer::GetRawPointer() const
        {
            return ptr_;
        }

    }  // namespace amd
}  // namespace dxr
