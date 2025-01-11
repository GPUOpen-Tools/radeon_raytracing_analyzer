//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a node pointer class.
//=============================================================================

#include "node_pointer.h"

#include "flags_util.h"
#include "public/rra_rtip_info.h"
#include "bvh/gpu_def.h"
#include "public/rra_assert.h"
#include "bvh/rtip31/ray_tracing_defs.h"

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
        {
            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
            {
                rtip31_type_    = static_cast<std::uint32_t>(type);
                rtip31_address_ = address >> 7;
            }
            else
            {
                type_    = static_cast<std::uint32_t>(type);
                address_ = address >> 6;
            }
        }

        NodePointer::NodePointer(const std::uint32_t pointer)
            : ptr_(pointer)
        {
        }

        uint32_t NodePointer::GetByteOffset() const
        {
            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
            {
                return ExtractNodePointerOffset3_1(ptr_);
            }
            else
            {
                // The first 3-bits ([0-2]) of this address are zero as they are reserved for
                // the node type. The shift is because we assume at least 64-byte (1 << 6)
                // alignment of the address. Hence, the bits [3-5] can also be neglected here.
                return GetShiftedAddress();
            }
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
            return GetShiftedAddress();
        }

        NodeType NodePointer::GetType() const
        {
            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
            {
                return static_cast<NodeType>(rtip31_type_);
            }
            else
            {
                return static_cast<NodeType>(type_);
            }
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
                    if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() != rta::RayTracingIpLevel::RtIp3_1)
                    {
                        return byte_offset / kBvh8BoxNodeSize;
                    }
                    else
                    {
                        return byte_offset / kFp32BoxNodeSize;
                    }
                }
                else
                {
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
            const std::uint32_t link_index_offset = GetAddress() << link_align_offset;

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
            bool is_triangle_node = GetType() <= NodeType::kAmdNodeTriangle3;

            is_triangle_node = is_triangle_node || ((int)GetType() >= NODE_TYPE_TRIANGLE_4 && (int)GetType() <= NODE_TYPE_TRIANGLE_7);
            return is_triangle_node;
        }

        uint32_t NodePointer::GetTrianglePairIndex() const
        {
            assert(IsTriangleNode());

            switch ((uint32_t)GetType())
            {
            case (uint32_t)NodeType::kAmdNodeTriangle0:
                return 0;
            case (uint32_t)NodeType::kAmdNodeTriangle1:
                return 1;
            case (uint32_t)NodeType::kAmdNodeTriangle2:
                return 2;
            case (uint32_t)NodeType::kAmdNodeTriangle3:
                return 3;
            case NODE_TYPE_TRIANGLE_4:
                return 4;
            case NODE_TYPE_TRIANGLE_5:
                return 5;
            case NODE_TYPE_TRIANGLE_6:
                return 6;
            case NODE_TYPE_TRIANGLE_7:
                return 7;
            }

            // Not a triangle node.
            return 0xFFFF;
        }

        bool NodePointer::IsBoxNode() const
        {
            return IsFp32BoxNode() || IsFp16BoxNode();
        }

        bool NodePointer::IsLeafNode() const
        {
            bool is_procedural_node = false;
            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() != rta::RayTracingIpLevel::RtIp3_1)
            {
                is_procedural_node = GetType() == NodeType::kAmdNodeProcedural;
            }

            return IsInstanceNode() || IsTriangleNode() || is_procedural_node;
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

        uint32_t NodePointer::GetAddress() const
        {
            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
            {
                return rtip31_address_ << 1;
            }
            else
            {
                return address_;
            }
        }

        uint32_t NodePointer::GetShiftedAddress() const
        {
            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
            {
                return rtip31_address_ << 7;
            }
            else
            {
                return address_ << 6;
            }
        }

    }  // namespace amd
}  // namespace dxr
