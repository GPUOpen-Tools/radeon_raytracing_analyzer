//=============================================================================
// Copyright (c) 2021-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 1.1 (Navi2x) specific bottom level acceleration structure
/// implementation.
//=============================================================================

#include "bvh/rtip11/encoded_rt_ip_11_bottom_level_bvh.h"

#include "public/rra_blas.h"

#include <float.h>
#include <math.h>
#include <vector>

#include "bvh/dxr_type_conversion.h"
#include "bvh/rtip11/rt_ip_11_acceleration_structure_header.h"
#include "bvh/rtip11/rt_ip_11_header.h"

#include "rra_blas_impl.h"
#include "rra_bvh_impl.h"

namespace rta
{
    /// @brief Find the minimum value of 3 provided values.
    ///
    /// @param [in] value1 The first value.
    /// @param [in] value2 The second value.
    /// @param [in] value3 The third value.
    ///
    /// @return The minimum value.
    static float Min(float value1, float value2, float value3)
    {
        float min_val = std::min({
            value1,
            value2,
            value3,
        });
        return min_val;
    }

    /// @brief Find the minimum value of 4 provided values.
    ///
    /// @param [in] value1 The first value.
    /// @param [in] value2 The second value.
    /// @param [in] value3 The third value.
    /// @param [in] value4 The fourth value.
    ///
    /// @return The minimum value.
    static float Min(float value1, float value2, float value3, float value4)
    {
        float min_val = std::min({
            value1,
            value2,
            value3,
            value4,
        });
        return min_val;
    }

    /// @brief Find the maximum value of 3 provided values.
    ///
    /// @param [in] value1 The first value.
    /// @param [in] value2 The second value.
    /// @param [in] value3 The third value.
    ///
    /// @return The maximum value.
    static float Max(float value1, float value2, float value3)
    {
        float max_val = std::max({
            value1,
            value2,
            value3,
        });
        return max_val;
    }

    /// @brief Find the maximum value of 4 provided values.
    ///
    /// @param [in] value1 The first value.
    /// @param [in] value2 The second value.
    /// @param [in] value3 The third value.
    /// @param [in] value4 The fourth value.
    ///
    /// @return The maximum value.
    static float Max(float value1, float value2, float value3, float value4)
    {
        float max_val = std::max({
            value1,
            value2,
            value3,
            value4,
        });
        return max_val;
    }

    EncodedRtIp11BottomLevelBvh::EncodedRtIp11BottomLevelBvh()
    {
        header_ = std::make_unique<DxrRtIp11AccelerationStructureHeader>();
    }

    EncodedRtIp11BottomLevelBvh::~EncodedRtIp11BottomLevelBvh()
    {
    }

    uint32_t EncodedRtIp11BottomLevelBvh::GetLeafNodeCount() const
    {
        return (uint32_t)(leaf_nodes_.size() / sizeof(dxr::amd::TriangleNode));
    }

    const std::vector<uint8_t>& EncodedRtIp11BottomLevelBvh::GetLeafNodesData() const
    {
        return leaf_nodes_;
    }

    const std::vector<dxr::amd::GeometryInfo>& EncodedRtIp11BottomLevelBvh::GetGeometryInfos() const
    {
        return geom_infos_;
    }

    const std::vector<dxr::amd::NodePointer>& EncodedRtIp11BottomLevelBvh::GetPrimitiveNodePtrs() const
    {
        return primitive_node_ptrs_;
    }

    bool EncodedRtIp11BottomLevelBvh::HasBvhReferences() const
    {
        return false;
    }

    std::uint64_t EncodedRtIp11BottomLevelBvh::GetBufferByteSizeImpl(const ExportOption export_option) const
    {
        auto file_size = header_->GetFileSize();
        if (export_option == ExportOption::kNoMetaData)
        {
            file_size -= meta_data_.GetByteSize();
        }
        auto min_file_size = kMinimumFileSize;
        return std::max(file_size, min_file_size);
    }

    void EncodedRtIp11BottomLevelBvh::UpdatePrimitiveNodePtrs()
    {
        auto                  byte_offset = header_->GetBufferOffsets().leaf_nodes;
        dxr::amd::NodePointer node_ptr;

        for (uint32_t i = 0; i < header_->GetLeafNodeCount(); ++i)
        {
            size_t geometry_index  = 0;
            auto   primitive_index = 0;

            if (is_procedural_)
            {
                const auto* procedural_nodes = reinterpret_cast<const dxr::amd::ProceduralNode*>(leaf_nodes_.data());
                const auto& procedural_node  = procedural_nodes[i];
                geometry_index               = procedural_node.GetGeometryIndex();
                primitive_index              = procedural_node.GetPrimitiveIndex();
                node_ptr                     = dxr::amd::NodePointer(dxr::amd::NodeType::kAmdNodeProcedural, byte_offset);
            }
            else
            {
                const auto* triangle_nodes = reinterpret_cast<const dxr::amd::TriangleNode*>(leaf_nodes_.data());
                const auto& triangle_node  = triangle_nodes[i];
                geometry_index             = triangle_node.GetGeometryIndex();
                primitive_index            = triangle_node.GetPrimitiveIndex(dxr::amd::NodeType::kAmdNodeTriangle0);
                node_ptr                   = dxr::amd::NodePointer(dxr::amd::NodeType::kAmdNodeTriangle0, byte_offset);
            }

            assert(geometry_index < geom_infos_.size());
            auto& geom_info = geom_infos_[geometry_index];

            const std::uint64_t base_prim_node_ptr_index = geom_info.GetPrimitiveNodePtrsOffset() / sizeof(dxr::amd::NodePointer);

            const std::uint32_t prim_node_ptr_index = static_cast<uint32_t>(base_prim_node_ptr_index + primitive_index);

            assert(prim_node_ptr_index < primitive_node_ptrs_.size());
            primitive_node_ptrs_[prim_node_ptr_index] = node_ptr;

            // Increment the byte offset
            byte_offset += dxr::amd::kLeafNodeSize;
        }
    }

    bool EncodedRtIp11BottomLevelBvh::LoadRawAccelStrucFromFile(rdf::ChunkFile&                     chunk_file,
                                                                const std::uint64_t                 chunk_index,
                                                                const RawAccelStructRdfChunkHeader& chunk_header,
                                                                const char* const                   chunk_identifier,
                                                                const BvhBundleReadOption           import_option)
    {
        const auto identifier     = chunk_identifier;
        const auto data_size      = chunk_file.GetChunkDataSize(identifier, static_cast<uint32_t>(chunk_index));
        uint8_t    no_meta_data   = (uint8_t)BvhBundleReadOption::kNoMetaData;
        const bool skip_meta_data = static_cast<std::uint8_t>(import_option) & no_meta_data;

        std::vector<std::uint8_t> buffer(data_size);
        if (data_size > 0)
        {
            chunk_file.ReadChunkDataToBuffer(identifier, static_cast<uint32_t>(chunk_index), buffer.data());
        }

        if (!skip_meta_data)
        {
            meta_data_  = {};
            size_t size = std::min(chunk_header.meta_header_size, dxr::amd::kMetaDataV1Size);
            memcpy(&meta_data_, buffer.data() + chunk_header.meta_header_offset, size);
        }

        if (buffer.size() < ((size_t)dxr::amd::kAccelerationStructureHeaderSize + chunk_header.header_offset))
        {
            return false;
        }

        header_->LoadFromBuffer(dxr::amd::kAccelerationStructureHeaderSize, buffer.data() + chunk_header.header_offset);
        if (!header_->IsValid())
        {
            return false;
        }

        is_procedural_ = header_->GetGeometryType() == BottomLevelBvhGeometryType::kAABB;

        uint64_t address = (static_cast<std::uint64_t>(chunk_header.accel_struct_base_va_hi) << 32) | chunk_header.accel_struct_base_va_lo;
        SetVirtualAddress(address);

        auto buffer_offset = chunk_header.header_offset + chunk_header.header_size;
        auto buffer_stream = rdf::Stream::FromReadOnlyMemory(buffer.size() - buffer_offset, buffer.data() + buffer_offset);

        auto metadata_size   = chunk_header.header_offset - chunk_header.meta_header_size;
        auto metadata_offset = chunk_header.meta_header_offset + chunk_header.meta_header_size;
        assert((header_->GetMetaDataSize() - chunk_header.meta_header_size) == metadata_size);
        auto metadata_stream = rdf::Stream::FromReadOnlyMemory(metadata_size, buffer.data() + metadata_offset);

        const auto& header_offsets            = header_->GetBufferOffsets();
        const auto  interior_node_buffer_size = header_offsets.leaf_nodes - header_offsets.interior_nodes;
        LoadBaseDataFromFile(metadata_stream, buffer_stream, static_cast<uint32_t>(interior_node_buffer_size), import_option);
        metadata_stream.Close();

        const auto leaf_node_buffer_size = header_offsets.geometry_info - header_offsets.leaf_nodes;
        leaf_nodes_                      = std::vector<std::uint8_t>(leaf_node_buffer_size);
        buffer_stream.Read(leaf_node_buffer_size, leaf_nodes_.data());

        const auto goem_info_size = sizeof(dxr::amd::GeometryInfo) * header_->GetGeometryDescriptionCount();
        geom_infos_               = std::vector<dxr::amd::GeometryInfo>(header_->GetGeometryDescriptionCount());
        buffer_stream.Read(goem_info_size, geom_infos_.data());

        const auto prim_node_ptrs_size = header_->GetPrimitiveCount() * sizeof(dxr::amd::NodePointer);
        primitive_node_ptrs_           = std::vector<dxr::amd::NodePointer>(header_->GetPrimitiveCount());
        buffer_stream.Read(prim_node_ptrs_size, primitive_node_ptrs_.data());

        buffer_stream.Close();

        // Set the root node offset.
        header_offset_ = static_cast<uint64_t>(chunk_header.header_offset);

        return true;
    }

    bool EncodedRtIp11BottomLevelBvh::PostLoad()
    {
        ScanTreeDepth();
        return true;
    }

    bool EncodedRtIp11BottomLevelBvh::Validate() const
    {
        if (header_->GetGeometryType() == rta::BottomLevelBvhGeometryType::kTriangle)
        {
            // Make sure the number of primitives in the BLAS and header match.
            uint32_t total_triangle_count = 0;
            for (auto geom_iter = geom_infos_.begin(); geom_iter != geom_infos_.end(); ++geom_iter)
            {
                total_triangle_count += geom_iter->GetPrimitiveCount();
            }

            if (total_triangle_count != header_->GetPrimitiveCount())
            {
                return false;
            }
        }
        return true;
    }

    float EncodedRtIp11BottomLevelBvh::GetSurfaceAreaHeuristic() const
    {
        return surface_area_heuristic_;
    }

    void EncodedRtIp11BottomLevelBvh::SetSurfaceAreaHeuristic(float surface_area_heuristic)
    {
        surface_area_heuristic_ = surface_area_heuristic;
    }

    RraErrorCode EncodedRtIp11BottomLevelBvh::GetNodeName(uint32_t node_id, const char** out_name) const
    {
        dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);

        switch ((uint32_t)node->GetType())
        {
        case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle0:
            *out_name = IsProcedural() ? "Procedural" : "Triangle";
            break;

        case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle1:
        case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle2:
        case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle3:
        case NODE_TYPE_TRIANGLE_4:
        case NODE_TYPE_TRIANGLE_5:
        case NODE_TYPE_TRIANGLE_6:
        case NODE_TYPE_TRIANGLE_7:
            *out_name = IsProcedural() ? "Procedural" : "Triangles";
            break;

        case (uint32_t)dxr::amd::NodeType::kAmdNodeBoxFp16:
            *out_name = "Box16";
            break;

        case (uint32_t)dxr::amd::NodeType::kAmdNodeBoxFp32:
            *out_name = "Box32";
            break;

        case (uint32_t)dxr::amd::NodeType::kAmdNodeInstance:
            *out_name = "Instance";
            break;

        case (uint32_t)dxr::amd::NodeType::kAmdNodeProcedural:
            *out_name = "Procedural";
            break;

        default:
            *out_name = "Unknown";
            break;
        }
        return kRraOk;
    }

    RraErrorCode EncodedRtIp11BottomLevelBvh::GetNodeNameToolTip(uint32_t node_id, const char** out_tooltip) const
    {
        static const char* proc_string = "A node containing procedural geometry data";
        static const char* tri_string  = "A node containing triangle geometry data";

        dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);

        switch ((uint32_t)node->GetType())
        {
        case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle0:
        case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle1:
        case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle2:
        case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle3:
        case NODE_TYPE_TRIANGLE_4:
        case NODE_TYPE_TRIANGLE_5:
        case NODE_TYPE_TRIANGLE_6:
        case NODE_TYPE_TRIANGLE_7:
            *out_tooltip = IsProcedural() ? proc_string : tri_string;
            break;

        case (uint32_t)dxr::amd::NodeType::kAmdNodeProcedural:
            *out_tooltip = proc_string;
            break;

        case (uint32_t)dxr::amd::NodeType::kAmdNodeBoxFp16:
            *out_tooltip = "A 16-bit floating point bounding volume node with up to 4 child nodes";
            break;

        case (uint32_t)dxr::amd::NodeType::kAmdNodeBoxFp32:
            *out_tooltip = "A 32-bit floating point bounding volume node with up to 4 child nodes";
            break;

        case (uint32_t)dxr::amd::NodeType::kAmdNodeInstance:
            *out_tooltip = "A node containing an instance of a BLAS";
            break;

        default:
            *out_tooltip = "";
            break;
        }
        return kRraOk;
    }

    RraErrorCode EncodedRtIp11BottomLevelBvh::GetGeometryIndex(uint32_t  node_id,
                                                               uint32_t  child_index,
                                                               uint32_t  global_child_index,
                                                               uint32_t* out_geometry_index) const
    {
        RRA_UNUSED(child_index);
        RRA_UNUSED(global_child_index);
        const dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);
        if (current_node->IsTriangleNode())
        {
            const auto& header_offsets = GetHeader().GetBufferOffsets();
            if (current_node->GetByteOffset() < header_offsets.leaf_nodes)
            {
                *out_geometry_index = 0;
                return kRraErrorInvalidPointer;
            }

            const dxr::amd::TriangleNode* triangle_node = GetTriangleNode(*current_node);

#ifdef _DEBUG
            const uint32_t index = (current_node->GetByteOffset() - header_offsets.leaf_nodes) / sizeof(dxr::amd::TriangleNode);

            const auto* triangle_nodes = reinterpret_cast<const dxr::amd::TriangleNode*>(GetLeafNodesData().data());
            const auto& tri_node       = triangle_nodes[index];

            RRA_ASSERT(&tri_node == triangle_node);
#endif  // DEBUG

            *out_geometry_index = triangle_node->GetGeometryIndex();
        }
        else
        {
            // The given node Id doesn't refer to a triangle node.
            // Don't output any geometry Id, and return an invalid child error code.
            return kRraErrorInvalidChildNode;
        }
        return kRraOk;
    }

    RraErrorCode EncodedRtIp11BottomLevelBvh::GetPrimitiveIndex(uint32_t  node_id,
                                                                uint32_t  child_index,
                                                                uint32_t  global_child_index,
                                                                uint32_t  local_primitive_index,
                                                                uint32_t* out_primitive_index) const
    {
        RRA_UNUSED(child_index);
        RRA_UNUSED(global_child_index);
        const dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);
        if (current_node->IsTriangleNode())
        {
            const dxr::amd::TriangleNode* triangle_node  = GetTriangleNode(*current_node);
            const auto&                   header_offsets = GetHeader().GetBufferOffsets();
            if (current_node->GetByteOffset() < header_offsets.leaf_nodes)
            {
                *out_primitive_index = 0;
                return kRraErrorInvalidPointer;
            }

#ifdef _DEBUG
            const uint32_t node_index = (current_node->GetByteOffset() - header_offsets.leaf_nodes) / sizeof(dxr::amd::TriangleNode);

            // Get the primitive index for this node index.
            const auto* triangle_nodes = reinterpret_cast<const dxr::amd::TriangleNode*>(GetLeafNodesData().data());
            const auto& tri_node       = triangle_nodes[node_index];

            RRA_ASSERT(&tri_node == triangle_node);
#endif  // DEBUG

            // No way to exctract more than 2 primitive indexes as of yet.
            RRA_ASSERT(local_primitive_index < 2);

            *out_primitive_index =
                triangle_node->GetPrimitiveIndex(local_primitive_index == 0 ? dxr::amd::NodeType::kAmdNodeTriangle0 : dxr::amd::NodeType::kAmdNodeTriangle1);
        }
        else
        {
            // The given node Id doesn't refer to a triangle node.
            // Don't output any geometry Id, and return an invalid child error code.
            return kRraErrorInvalidChildNode;
        }
        return kRraOk;
    }

    RraErrorCode EncodedRtIp11BottomLevelBvh::GetIsInactive(uint32_t node_id, uint32_t child_index, uint32_t global_child_index, bool* out_is_inactive) const
    {
        RRA_UNUSED(child_index);
        RRA_UNUSED(global_child_index);
        dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);

        if (current_node == nullptr)
        {
            return kRraErrorInvalidPointer;
        }

        if (current_node->IsTriangleNode())
        {
            const dxr::amd::TriangleNode* triangle_node = GetTriangleNode(*current_node);
            if (triangle_node == nullptr)
            {
                return kRraErrorInvalidPointer;
            }
            *out_is_inactive = triangle_node->IsInactive(current_node->GetType());
            return kRraOk;
        }
        else if (IsProcedural())
        {
            const dxr::amd::ProceduralNode* procedural_node = GetProceduralNode(*current_node);
            if (procedural_node == nullptr)
            {
                return kRraErrorInvalidPointer;
            }
            *out_is_inactive = procedural_node->IsInactive();
            return kRraOk;
        }
        return kRraErrorInvalidChildNode;
    }

    RraErrorCode EncodedRtIp11BottomLevelBvh::GetNodeTriangleCount(uint32_t  node_id,
                                                                   uint32_t  child_index,
                                                                   uint32_t  global_child_index,
                                                                   uint32_t* out_triangle_count) const
    {
        RRA_UNUSED(child_index);
        RRA_UNUSED(global_child_index);
        dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);

        if (current_node == nullptr)
        {
            return kRraErrorInvalidPointer;
        }

        if (!current_node->IsTriangleNode())
        {
            *out_triangle_count = 0;
            return kRraOk;
        }

        // The incoming node id should be a triangle node. If it's not, we can't extract triangle data.
        if (current_node->GetType() == dxr::amd::NodeType::kAmdNodeTriangle0)
        {
            *out_triangle_count = 1;
        }
        else if (current_node->GetType() == dxr::amd::NodeType::kAmdNodeTriangle1)
        {
            *out_triangle_count = 2;
        }
        else if (current_node->GetType() == dxr::amd::NodeType::kAmdNodeTriangle2)
        {
            *out_triangle_count = 3;
        }
        else if (current_node->GetType() == dxr::amd::NodeType::kAmdNodeTriangle3)
        {
            *out_triangle_count = 4;
        }
        else if ((int)current_node->GetType() == NODE_TYPE_TRIANGLE_4)
        {
            *out_triangle_count = 5;
        }
        else if ((int)current_node->GetType() == NODE_TYPE_TRIANGLE_5)
        {
            *out_triangle_count = 6;
        }
        else if ((int)current_node->GetType() == NODE_TYPE_TRIANGLE_6)
        {
            *out_triangle_count = 7;
        }
        else if ((int)current_node->GetType() == NODE_TYPE_TRIANGLE_7)
        {
            *out_triangle_count = 8;
        }
        else
        {
            *out_triangle_count = 0;
        }
        return kRraOk;
    }

    RraErrorCode EncodedRtIp11BottomLevelBvh::GetNodeTriangles(uint32_t          node_id,
                                                               uint32_t          child_index,
                                                               uint32_t          global_child_index,
                                                               TriangleVertices* out_triangles) const
    {
        RRA_UNUSED(child_index);
        RRA_UNUSED(global_child_index);
        dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);

        if (current_node == nullptr)
        {
            return kRraErrorInvalidPointer;
        }

        // The incoming node id should be a triangle node. If it's not, we can't extract triangle data.
        if (current_node->IsTriangleNode())
        {
            const dxr::amd::TriangleNode* triangle_node  = GetTriangleNode(*current_node);
            const auto&                   header_offsets = GetHeader().GetBufferOffsets();

            if (current_node->GetByteOffset() < header_offsets.leaf_nodes)
            {
                return kRraErrorInvalidPointer;
            }

#ifdef _DEBUG
            const uint32_t node_index = (current_node->GetByteOffset() - header_offsets.leaf_nodes) / sizeof(dxr::amd::TriangleNode);

            const auto* triangle_nodes = reinterpret_cast<const dxr::amd::TriangleNode*>(GetLeafNodesData().data());
            const auto& tri_node       = triangle_nodes[node_index];

            RRA_ASSERT(&tri_node == triangle_node);
#endif  // DEBUG

            const auto& verts = triangle_node->GetVertices();

            // Copy the triangle vertices to the output pointer.
            const size_t vertex_size = sizeof(VertexPosition);
            memcpy(&out_triangles->a, &verts[0], vertex_size);
            memcpy(&out_triangles->b, &verts[1], vertex_size);
            memcpy(&out_triangles->c, &verts[2], vertex_size);

            if (current_node->GetType() == dxr::amd::NodeType::kAmdNodeTriangle1)
            {
                out_triangles++;
                memcpy(&out_triangles->a, &verts[2], vertex_size);
                memcpy(&out_triangles->b, &verts[1], vertex_size);
                memcpy(&out_triangles->c, &verts[3], vertex_size);
            }
        }
        return kRraOk;
    }

    RraErrorCode EncodedRtIp11BottomLevelBvh::GetNodeVertexCount(uint32_t node_id, uint32_t child_index, uint32_t global_child_index, uint32_t* out_count) const
    {
        dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);

        if (current_node == nullptr)
        {
            return kRraErrorInvalidPointer;
        }

        // The incoming node id should be a triangle node. If it's not, we can't extract triangle data.
        if (current_node->IsTriangleNode())
        {
            uint32_t triangle_count{};
            GetNodeTriangleCount(node_id, child_index, global_child_index, &triangle_count);
            *out_count = (triangle_count == 1 ? 3 : 4);
        }
        return kRraOk;
    }

    RraErrorCode EncodedRtIp11BottomLevelBvh::GetNodeVertices(uint32_t               node_id,
                                                              uint32_t               child_index,
                                                              uint32_t               global_child_index,
                                                              struct VertexPosition* out_vertices) const
    {
        RRA_UNUSED(child_index);
        RRA_UNUSED(global_child_index);
        dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);

        if (current_node == nullptr)
        {
            return kRraErrorInvalidPointer;
        }

        // The incoming node id should be a triangle node. If it's not, we can't extract triangle data.
        if (current_node->IsTriangleNode())
        {
            const auto& header_offsets = GetHeader().GetBufferOffsets();

            const uint32_t node_index = (current_node->GetByteOffset() - header_offsets.leaf_nodes) / sizeof(dxr::amd::TriangleNode);
            // Get the triangle vertices for this node index.
            const auto* triangle_nodes = reinterpret_cast<const dxr::amd::TriangleNode*>(GetLeafNodesData().data());
            const auto& tri_node       = triangle_nodes[node_index];
            const auto& verts          = tri_node.GetVertices();

            // Copy the triangle vertices to the output pointer.
            const size_t vertex_size = sizeof(VertexPosition);
            memcpy(&out_vertices[0], &verts[0], vertex_size);
            memcpy(&out_vertices[1], &verts[1], vertex_size);
            memcpy(&out_vertices[2], &verts[2], vertex_size);

            if (current_node->GetType() == dxr::amd::NodeType::kAmdNodeTriangle1)
            {
                memcpy(&out_vertices[3], &verts[3], vertex_size);
            }
        }
        return kRraOk;
    }

    const dxr::amd::TriangleNode* EncodedRtIp11BottomLevelBvh::GetTriangleNode(const dxr::amd::NodePointer node_pointer, const int offset) const
    {
        assert(node_pointer.IsTriangleNode());
        if (header_->GetPostBuildInfo().IsBottomLevel() && node_pointer.GetByteOffset() >= header_->GetBufferOffsets().leaf_nodes)
        {
            return reinterpret_cast<const dxr::amd::TriangleNode*>(
                &GetLeafNodesData()[node_pointer.GetByteOffset() + (size_t)offset * dxr::amd::kLeafNodeSize - header_->GetBufferOffsets().leaf_nodes]);
        }
        else
        {
            // Bottom BHV cannot have instance nodes.
            return nullptr;
        }
    }

    const dxr::amd::ProceduralNode* EncodedRtIp11BottomLevelBvh::GetProceduralNode(const dxr::amd::NodePointer node_pointer, const int offset) const
    {
        assert(is_procedural_);
        if (header_->GetPostBuildInfo().IsBottomLevel())
        {
            return reinterpret_cast<const dxr::amd::ProceduralNode*>(
                &GetLeafNodesData()[node_pointer.GetByteOffset() + (size_t)offset * dxr::amd::kLeafNodeSize - header_->GetBufferOffsets().leaf_nodes]);
        }
        else
        {
            // Bottom BHV cannot have instance nodes
            assert(false);
            return nullptr;
        }
    }

    void EncodedRtIp11BottomLevelBvh::ComputeSurfaceAreaHeuristic()
    {
        if (IsEmpty())
        {
            SetSurfaceAreaHeuristic(0.0f);
            return;
        }

        // For each triangle node, calculate the SAH.
        const auto* triangle_nodes = reinterpret_cast<const dxr::amd::TriangleNode*>(GetLeafNodesData().data());
        const auto& header_offsets = GetHeader().GetBufferOffsets();

        std::vector<dxr::amd::NodePointer> tri_node_pointers;
        GetTriangleNodes(tri_node_pointers);

        for (const auto& node_ptr : tri_node_pointers)
        {
            if (node_ptr.GetByteOffset() < header_offsets.leaf_nodes)
            {
                // Bad address for a triangle.
                continue;
            }

            const uint32_t node_index = (node_ptr.GetByteOffset() - header_offsets.leaf_nodes) / sizeof(dxr::amd::TriangleNode);

            uint32_t tri_count{};
            RraBlasGetNodeTriangleCount(GetID(), node_ptr.GetID(), 0, 0, &tri_count);  // Pass 0 since this function is specific to RtIp11.

            if (node_ptr.GetType() == dxr::amd::NodeType::kAmdNodeTriangle0)
            {
                tri_count = 1;
            }
            else if (node_ptr.GetType() == dxr::amd::NodeType::kAmdNodeTriangle1)
            {
                tri_count = 2;
            }

            float aabb_surface_area         = CalculateTriangleAABBSurfaceArea(triangle_nodes[node_index], tri_count);
            float triangle_surface_area     = GetTriangleSurfaceArea(triangle_nodes[node_index], tri_count);
            float triangle_avg_surface_area = triangle_surface_area / tri_count;
            float sah                       = 0.0f;

            // Make sure the surface area of the triangle bounding volume is larger than the triangle surface area.
            if (aabb_surface_area >= triangle_surface_area && aabb_surface_area > FLT_MIN)
            {
                // Multiply triangle area by 2, to account for probability of ray going through front or back face.
                sah = (2.0f * triangle_avg_surface_area) / aabb_surface_area;

                // SAH is currently in the range [0.0, 0.5] since a triangle can occupy at most half the space of its bounding volume.
                // So multiply by 2.0 to normalize the SAH to a range [0.0, 1.0].
                sah *= 2.0f;
            }

            // Mathematically SAH should not ever be greater than 1.0, but with really problematic triangles (extremely long and thin)
            // floating point errors can push it over. I've seen as high as 1.454 in the Deathloop trace.
            if (!isnan(sah))
            {
                if (sah > 1.01f)
                {
                    // SAH has passed threshold, so assume this triangle is problematic and mark it as 0.
                    sah = 0.0f;
                }
                else
                {
                    // Otherwise it's only a small floating point error so clamp it to a valid value.
                    sah = std::min(sah, 1.0f);
                }
            }

            // Store the SAH back to the BLAS.
            SetLeafNodeSurfaceAreaHeuristic(node_ptr.GetRawPointer(), sah);
        }

        // Iterate over the box nodes and calculate their SAH values.
        // Top level node doesn't exist in the data so needs to be created. Assumed to be a Box32.
        dxr::amd::NodePointer root_node = dxr::amd::NodePointer(dxr::amd::NodeType::kAmdNodeBoxFp32, dxr::amd::kAccelerationStructureHeaderSize);
        float                 sah       = CalculateSAHForBlasNode(root_node);

        SetSurfaceAreaHeuristic(sah);
    }

    uint32_t EncodedRtIp11BottomLevelBvh::GetParentNode(uint32_t node_id, uint32_t global_child_index) const
    {
        RRA_UNUSED(global_child_index);
        dxr::amd::NodePointer* node_ptr = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);
        assert(!node_ptr->IsInvalid());

        const auto& parent_data       = parent_data_;
        const auto& parent_links      = parent_data.GetLinkData();
        const auto  compression_mode  = ToDxrTriangleCompressionMode(GetHeader().GetPostBuildInfo().GetTriangleCompressionMode());
        const auto  parent_link_index = node_ptr->CalculateParentLinkIndex(parent_data.GetSizeInBytes(), compression_mode);

        if (parent_link_index >= parent_data.GetLinkCount())
        {
            return {};
        }

        dxr::amd::NodePointer parent_node = parent_links[parent_link_index];

        return parent_node.GetRawPointer();
    }

    float EncodedRtIp11BottomLevelBvh::GetLeafNodeSurfaceAreaHeuristic(uint32_t node_id, uint32_t global_child_index) const
    {
        RRA_UNUSED(global_child_index);
        dxr::amd::NodePointer node_ptr    = dxr::amd::NodePointer(node_id);
        const uint32_t        byte_offset = node_ptr.GetByteOffset();
        const uint32_t        leaf_nodes  = GetHeader().GetBufferOffsets().leaf_nodes;
        if (byte_offset < leaf_nodes)
        {
            // Bad address for a triangle.
            return std::numeric_limits<float>::quiet_NaN();
        }
        const uint32_t index = (byte_offset - leaf_nodes) / sizeof(dxr::amd::TriangleNode);
        return triangle_surface_area_heuristic_.at(index);
    }

    void EncodedRtIp11BottomLevelBvh::SetLeafNodeSurfaceAreaHeuristic(uint32_t node_ptr, float surface_area_heuristic)
    {
        dxr::amd::NodePointer node        = (dxr::amd::NodePointer)node_ptr;
        const uint32_t        byte_offset = node.GetByteOffset();
        const uint32_t        leaf_nodes  = GetHeader().GetBufferOffsets().leaf_nodes;
        if (byte_offset < leaf_nodes)
        {
            // Bad address for a triangle.
            return;
        }
        const uint32_t index                    = (byte_offset - leaf_nodes) / sizeof(dxr::amd::TriangleNode);
        triangle_surface_area_heuristic_[index] = surface_area_heuristic;
    }

    RraErrorCode EncodedRtIp11BottomLevelBvh::GetTriangleNodes(std::vector<dxr::amd::NodePointer>& triangle_nodes)
    {
        uint64_t blas_index = GetID();
        uint32_t root_node  = UINT32_MAX;
        RRA_BUBBLE_ON_ERROR(RraBvhGetRootNodePtr(&root_node));

        uint32_t child_node_count;
        uint32_t triangle_count;

        uint32_t triangle_node_count{};
        GetTriangleNodeCount(&triangle_node_count);
        triangle_nodes.reserve(triangle_node_count);

        std::vector<uint32_t> traversal_stack{};
        traversal_stack.reserve(64);  // It is rare for the traversal stack to get deeper than ~28 so this should be sufficient memory to reserve.
        traversal_stack.push_back(root_node);

        // Memoized traversal of the tree.
        while (!traversal_stack.empty())
        {
            uint32_t current_node{traversal_stack.back()};
            traversal_stack.pop_back();

            // Add the triangles to the list.
            RRA_BUBBLE_ON_ERROR(RraBlasGetChildNodeCount(blas_index, current_node, &child_node_count));
            std::array<uint32_t, 8> child_nodes{};
            RRA_BUBBLE_ON_ERROR(RraBlasGetChildNodes(blas_index, current_node, child_nodes.data()));
            traversal_stack.insert(traversal_stack.end(), child_nodes.data(), child_nodes.data() + child_node_count);

            // Get the triangle nodes. If this is not a triangle the triangle count is 0.
            RRA_BUBBLE_ON_ERROR(GetNodeTriangleCount(current_node, 0, 0, &triangle_count));

            // Continue with processing the node if it's a triangle node with 1 or more triangles within.
            if (triangle_count > 0)
            {
                triangle_nodes.push_back(dxr::amd::NodePointer(current_node));
            }
        }
        return kRraOk;
    }

    float EncodedRtIp11BottomLevelBvh::CalculateTriangleAABBSurfaceArea(const dxr::amd::TriangleNode& triangle, uint32_t tri_count) const
    {
        // Calculate the triangle AABB.
        const auto& verts = triangle.GetVertices();

        BoundingVolumeExtents bounding_volume;
        if (tri_count == 1)
        {
            bounding_volume.min_x = Min(verts[0].x, verts[1].x, verts[2].x);
            bounding_volume.min_y = Min(verts[0].y, verts[1].y, verts[2].y);
            bounding_volume.min_z = Min(verts[0].z, verts[1].z, verts[2].z);
            bounding_volume.max_x = Max(verts[0].x, verts[1].x, verts[2].x);
            bounding_volume.max_y = Max(verts[0].y, verts[1].y, verts[2].y);
            bounding_volume.max_z = Max(verts[0].z, verts[1].z, verts[2].z);
        }
        else if (tri_count == 2)
        {
            bounding_volume.min_x = Min(verts[0].x, verts[1].x, verts[2].x, verts[3].x);
            bounding_volume.min_y = Min(verts[0].y, verts[1].y, verts[2].y, verts[3].y);
            bounding_volume.min_z = Min(verts[0].z, verts[1].z, verts[2].z, verts[3].z);
            bounding_volume.max_x = Max(verts[0].x, verts[1].x, verts[2].x, verts[3].x);
            bounding_volume.max_y = Max(verts[0].y, verts[1].y, verts[2].y, verts[3].y);
            bounding_volume.max_z = Max(verts[0].z, verts[1].z, verts[2].z, verts[3].z);
        }

        // Calculate the surface area.
        float aabb_surface_area = 0.0f;
        if (RraBvhGetBoundingVolumeSurfaceArea(&bounding_volume, &aabb_surface_area) == kRraOk)
        {
            return aabb_surface_area;
        }

        return 1.0f;
    }

    float EncodedRtIp11BottomLevelBvh::CalculateSAHForBlasNode(const dxr::amd::NodePointer root_node)
    {

        float sah          = 0.0f;
        float sub_tree_sah = 0.0f;

        if (root_node.IsBoxNode())
        {
            float total_child_area = 0.0f;

            const auto  node_offset    = root_node.GetByteOffset() - GetHeader().GetBufferOffsets().interior_nodes;
            const auto& interior_nodes = GetInteriorNodesData();

            if (interior_nodes.size() == 0)
            {
                return 1.0f;
            }
            const auto& child_array      = RraBvhGetChildNodeArray(this, root_node.GetRawPointer(), node_offset);
            float       out_surface_area = 0.0f;
            for (const auto& child_node : child_array)
            {
                // find SAH for child nodes.
                sub_tree_sah += CalculateSAHForBlasNode(child_node);
                if (RraBlasGetSurfaceAreaImpl(this, child_node, 0, 0, &out_surface_area) == kRraOk)  // Pass 0 since this is only called for RtIp11.
                {
                    total_child_area += out_surface_area;
                }
            }

            // Take that as ratio of the current node.
            out_surface_area = 0.0;
            if (RraBlasGetSurfaceAreaImpl(this, root_node.GetRawPointer(), 0, 0, &out_surface_area) == kRraOk)  // Pass 0 since this is only called for RtIp11.
            {
                sah = 0.25f * (total_child_area / out_surface_area);
            }

            if (out_surface_area == 0.0)
            {
                sah = 0.0f;
            }

            SetInteriorNodeSurfaceAreaHeuristic(root_node.GetRawPointer(), sah);
        }
        else if (root_node.IsTriangleNode())
        {
            // Get SAH from BLAS since it's already been computed for triangle nodes.
            sah = GetLeafNodeSurfaceAreaHeuristic(root_node.GetRawPointer(), 0);
        }

        return sah + sub_tree_sah;
    }

}  // namespace rta

