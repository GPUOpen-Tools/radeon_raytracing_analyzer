//=============================================================================
// Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a triangle node class.
//=============================================================================

#include "bvh/node_types/triangle_node.h"

#include <cassert>
#include <cmath>  // --> isnan, isinf
#include <cstring>  // --> Linux, memcpy

#include "bvh/flags_util.h"
#include "bvh/node_pointer.h"

#include "public/rra_assert.h"
#include "public/rra_macro.h"

namespace dxr
{
    // Check if headers and descriptions are trivially copyable (memcpy support).
    static_assert(std::is_trivially_copyable<dxr::amd::TriangleNode>::value, "DXR::AMD::TriangleNode must be a trivially copyable class.");

    // Check if the size of structs is as expected.
    static_assert(sizeof(amd::TriangleNode) == amd::kLeafNodeSize, "TriangleNode does not have the expected byte size.");

    namespace amd
    {
        TriangleNode::TriangleNode()
        {
            const auto geom_flags = GeometryFlags::kAmdFlagNone;
            UpdateTriangleId(NodeType::kAmdNodeTriangle0, geom_flags);
            SetPrimitiveIndex(0, NodeType::kAmdNodeTriangle0);
            SetPrimitiveIndex(0, NodeType::kAmdNodeTriangle1);
            SetGeometryIndexAndFlags(0, geom_flags);
        }

        std::uint32_t TriangleNode::CalculateTriangleId(const dxr::amd::NodeType node_type,
                                                        const dxr::GeometryFlags geometry_flags,
                                                        const std::uint32_t      id,
                                                        const std::uint32_t      rotation)
        {
            std::uint32_t triangle_id = id;

            const std::uint32_t id_i_shift      = 0;
            const std::uint32_t id_j_shift      = 2;
            const std::uint32_t id_bit_stride   = 8;
            const std::uint32_t id_opaque_shift = 7;

            const std::uint32_t triangle_shift = static_cast<uint32_t>(node_type) * id_bit_stride;

            // Compute the barycentric mapping table (mask)
            triangle_id |= ((rotation + 1) % 3) << (triangle_shift + id_i_shift);
            triangle_id |= ((rotation + 2) % 3) << (triangle_shift + id_j_shift);

            if (static_cast<std::uint32_t>(geometry_flags) & static_cast<std::uint32_t>(GeometryFlags::kAmdFlagOpaque))
            {
                triangle_id |= 1u << (triangle_shift + id_opaque_shift);
            }

            return triangle_id;
        }

        std::uint32_t TriangleNode::GetTriangleId() const
        {
            return triangle_id_;
        }

        void TriangleNode::UpdateTriangleId(const dxr::amd::NodeType node_type,
                                            const dxr::GeometryFlags geometry_flags,
                                            const std::uint32_t      id,
                                            const std::uint32_t      rotation)
        {
            triangle_id_ = CalculateTriangleId(node_type, geometry_flags, id, rotation);
        }

        const Float3& TriangleNode::GetVertex(const std::uint32_t index) const
        {
            return vertices_[index];
        }

        Triangle TriangleNode::GetTriangle(const NodeType node_type) const
        {
            switch (node_type)
            {
            case dxr::amd::NodeType::kAmdNodeTriangle0:
                return {GetVertex(0), GetVertex(1), GetVertex(2)};
            case dxr::amd::NodeType::kAmdNodeTriangle1:
                return {GetVertex(1), GetVertex(3), GetVertex(2)};
            case dxr::amd::NodeType::kAmdNodeTriangle2:
                return {GetVertex(2), GetVertex(3), GetVertex(4)};
            case dxr::amd::NodeType::kAmdNodeTriangle3:
                return {GetVertex(2), GetVertex(4), GetVertex(0)};
            default:
                assert(false);
                return {};
            }
        }

        const std::array<Float3, 5>& TriangleNode::GetVertices() const
        {
            return vertices_;
        }

        std::array<std::uint32_t, 3> TriangleNode::GetTriangleIndices(const NodeType node_type, const TriangleCompressionMode compression_mode) const
        {
            RRA_UNUSED(compression_mode);

            const std::uint32_t id_i_shift    = 0;
            const std::uint32_t id_j_shift    = 2;
            const std::uint32_t id_bit_stride = 8;

            const std::uint32_t triangle_shift = id_bit_stride * rta::to_underlying(node_type);

            const std::uint32_t vertex_swizzle_y = (triangle_id_ >> (triangle_shift + id_i_shift)) % 4;
            const std::uint32_t vertex_swizzle_z = (triangle_id_ >> (triangle_shift + id_j_shift)) % 4;
            const std::uint32_t vertex_swizzle_x = 3 - vertex_swizzle_y - vertex_swizzle_z;

            const std::uint32_t nodeVertexMapping[4] = {0x210, 0x231, 0x432, 0x042};

            const auto v0 = (nodeVertexMapping[rta::to_underlying(node_type)] >> (vertex_swizzle_x * 4)) & 0xf;
            const auto v1 = (nodeVertexMapping[rta::to_underlying(node_type)] >> (vertex_swizzle_y * 4)) & 0xf;
            const auto v2 = (nodeVertexMapping[rta::to_underlying(node_type)] >> (vertex_swizzle_z * 4)) & 0xf;

            return {v0, v1, v2};
        }

        Triangle TriangleNode::GetTriangle(const NodeType node_type, const TriangleCompressionMode compression_mode) const
        {
            if (compression_mode == TriangleCompressionMode::kAmdNoTriangleCompression)
            {
                return GetTriangle(node_type);
            }

            const auto v_indices = GetTriangleIndices(node_type, compression_mode);

            return {GetVertex(v_indices[0]), GetVertex(v_indices[1]), GetVertex(v_indices[2])};
        }

        void TriangleNode::SetPrimitiveIndex(const std::uint32_t primitive_index, const dxr::amd::NodeType node_type)
        {
            assert(node_type <= NodeType::kAmdNodeTriangle1);

            if (node_type == NodeType::kAmdNodeTriangle0)
            {
                std::memcpy(&vertices_[4].y, &primitive_index, sizeof(float));
            }
            else if (node_type == NodeType::kAmdNodeTriangle1)
            {
                std::memcpy(&vertices_[4].z, &primitive_index, sizeof(float));
            }
        }

        void TriangleNode::SetGeometryIndexAndFlags(const std::uint32_t geometry_index, const dxr::GeometryFlags geometry_flags)
        {
            const std::uint32_t index_and_flags = (static_cast<std::uint32_t>(geometry_flags) << 24) | (geometry_index & 0xFFFFFF);
            std::memcpy(&vertices_[4].x, &index_and_flags, sizeof(float));
        }

        std::uint32_t TriangleNode::GetPrimitiveIndex(const dxr::amd::NodeType node_type) const
        {
            assert(node_type <= NodeType::kAmdNodeTriangle1);

            std::uint32_t index = 0;
            if (node_type == NodeType::kAmdNodeTriangle0)
            {
                std::memcpy(&index, &vertices_[4].y, sizeof(float));
            }
            else if (node_type == NodeType::kAmdNodeTriangle1)
            {
                std::memcpy(&index, &vertices_[4].z, sizeof(float));
            }
            return index;
        }

        std::uint32_t TriangleNode::GetGeometryIndex() const
        {
            std::uint32_t index_and_flags = 0;
            std::memcpy(&index_and_flags, &vertices_[4].x, sizeof(float));

            return index_and_flags & 0xFFFFFF;
        }

        dxr::GeometryFlags TriangleNode::GetGeometryFlags() const
        {
            std::uint32_t index_and_flags = 0;
            std::memcpy(&index_and_flags, &vertices_[4].x, sizeof(float));

            // Flags are only stored in bits 25 and 26 in this float.
            // To be safe, just extract these bits with 0x3.
            return static_cast<dxr::GeometryFlags>((index_and_flags >> 24) & 0x3);
        }

        bool TriangleNode::IsInactive(NodeType node_type) const
        {
            const auto triangle = GetTriangle(node_type);

            for (auto i = 0; i < 3; ++i)
            {
                if (std::isnan(triangle[i].x))
                {
                    return true;
                }
            }

            return false;
        }

        bool TriangleNode::ContainsNaN(NodeType node_type) const
        {
            const auto triangle = GetTriangle(node_type);

            for (auto i = 0; i < 3; ++i)
            {
                if (std::isnan(triangle[i].x))
                {
                    return true;
                }
            }

            return false;
        }

        const Float3& Triangle::operator[](const std::int32_t i) const
        {
            assert(i >= 0 && i <= 2);
            return (i == 0) ? v0 : (i == 1) ? v1 : v2;
        }

        Float3& Triangle::operator[](const std::int32_t i)
        {
            assert(i >= 0 && i <= 2);
            return (i == 0) ? v0 : (i == 1) ? v1 : v2;
        }

    }  // namespace amd

}  // namespace dxr
