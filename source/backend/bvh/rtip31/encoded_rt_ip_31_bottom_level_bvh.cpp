//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 3.1 (Navi4x) specific bottom level acceleration structure
/// implementation.
//=============================================================================

#include "bvh/rtip31/encoded_rt_ip_31_bottom_level_bvh.h"

#include <cassert>
#include <cmath>  // --> isnan, isinf, ceil
#include <deque>
#include <iostream>
#include <limits>
#include <vector>

#include "public/rra_assert.h"
#include "public/rra_blas.h"

#include "bvh/dxr_definitions.h"
#include "bvh/rtip31/internal_node.h"
#include "bvh/rtip31/rt_ip_31_acceleration_structure_header.h"
#include "bvh/rtip_common/ray_tracing_defs.h"  // NodePointer64, for CBLAS cluster-ref childBasePtr decode.
#include "rra_blas_impl.h"
#include "surface_area_heuristic.h"

namespace rta
{
    EncodedRtIp31BottomLevelBvh::EncodedRtIp31BottomLevelBvh()
    {
        header_              = std::make_unique<DxrRtIp31AccelerationStructureHeader>();
        interior_node_count_ = 0;
        leaf_node_count_     = 0;
    }

    EncodedRtIp31BottomLevelBvh::~EncodedRtIp31BottomLevelBvh()
    {
    }

    uint32_t EncodedRtIp31BottomLevelBvh::GetLeafNodeCount() const
    {
        return leaf_node_count_;
    }

    const std::vector<dxr::amd::GeometryInfo>& EncodedRtIp31BottomLevelBvh::GetGeometryInfos() const
    {
        return geom_infos_;
    }

    const std::vector<dxr::amd::NodePointer>& EncodedRtIp31BottomLevelBvh::GetPrimitiveNodePtrs() const
    {
        return primitive_node_ptrs_;
    }

    bool EncodedRtIp31BottomLevelBvh::HasBvhReferences() const
    {
        return false;
    }

    std::uint64_t EncodedRtIp31BottomLevelBvh::GetBufferByteSizeImpl(const ExportOption export_option) const
    {
        auto file_size = header_->GetFileSize();
        if (export_option == ExportOption::kNoMetaData)
        {
            file_size -= meta_data_.GetByteSize();
        }
        auto min_file_size = kMinimumFileSize;
        return std::max(file_size, min_file_size);
    }

    bool EncodedRtIp31BottomLevelBvh::LoadRawAccelStrucFromFile(rdf::ChunkFile&                     chunk_file,
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
        const auto  interior_node_buffer_size = header_offsets.geometry_info - header_offsets.interior_nodes;
        LoadBaseDataFromFile(metadata_stream, buffer_stream, static_cast<uint32_t>(interior_node_buffer_size), import_option);
        metadata_stream.Close();

        const auto goem_info_size = sizeof(dxr::amd::GeometryInfo) * header_->GetGeometryDescriptionCount();
        geom_infos_               = std::vector<dxr::amd::GeometryInfo>(header_->GetGeometryDescriptionCount());
        buffer_stream.Read(goem_info_size, geom_infos_.data());

        // Cluster BLASes (geometryType == Instances) and CLASes (ClusterLevel) do not store a
        // prim-node-pointer array (offsets.prim_node_ptrs == 0); reading one would consume trailing
        // padding as garbage pointers. Only read the array when it is actually present.
        if (header_offsets.prim_node_ptrs != 0)
        {
            const auto prim_node_ptrs_size = header_->GetPrimitiveCount() * sizeof(dxr::amd::NodePointer);
            primitive_node_ptrs_           = std::vector<dxr::amd::NodePointer>(header_->GetPrimitiveCount());
            buffer_stream.Read(prim_node_ptrs_size, primitive_node_ptrs_.data());
        }

        buffer_stream.Close();

        // Set the root node offset.
        header_offset_ = static_cast<uint64_t>(chunk_header.header_offset);

        CountNodes();

        return true;
    }

    bool EncodedRtIp31BottomLevelBvh::Validate()
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

    float EncodedRtIp31BottomLevelBvh::GetSurfaceAreaHeuristic() const
    {
        return surface_area_heuristic_;
    }

    void EncodedRtIp31BottomLevelBvh::SetSurfaceAreaHeuristic(float surface_area_heuristic)
    {
        surface_area_heuristic_ = surface_area_heuristic;
    }

    RraErrorCode EncodedRtIp31BottomLevelBvh::GetNodeName(uint32_t node_id, const char** out_name) const
    {
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
            *out_name = IsProcedural() ? "Procedural" : "Triangles";
            break;

        case (uint32_t)dxr::amd::NodeType::kAmdNodeBoxFp16:
            *out_name = "Box16";
            break;

        case (uint32_t)dxr::amd::NodeType::kAmdNodeBoxFp32:
            *out_name = "Bvh8";
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

    RraErrorCode EncodedRtIp31BottomLevelBvh::GetNodeNameToolTip(uint32_t node_id, const char** out_tooltip) const
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
            *out_tooltip = "A compressed bounding volume node with up to 8 child nodes";
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

    RraErrorCode EncodedRtIp31BottomLevelBvh::GetGeometryIndex(uint32_t  node_id,
                                                               uint32_t  child_index,
                                                               uint32_t  global_child_index,
                                                               uint32_t* out_geometry_index) const
    {
        RRA_UNUSED(child_index);
        RRA_UNUSED(global_child_index);
        const dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);
        if (current_node->IsTriangleNode())
        {
            uint32_t pair_indices_count{};
            auto     pair_indices = GetTrianglePairIndices(node_id, &pair_indices_count);

            auto pair_index = uint32_t(current_node->GetType());
            RRA_UNUSED(pair_index);

            *out_geometry_index = (pair_indices_count == 0) ? 0 : pair_indices.front().first->UnpackGeometryIndex(pair_indices.front().second, 0);
        }
        else
        {
            // The given node Id doesn't refer to a triangle node.
            // Don't output any geometry Id, and return an invalid child error code.
            return kRraErrorInvalidChildNode;
        }
        return kRraOk;
    }

    RraErrorCode EncodedRtIp31BottomLevelBvh::GetPrimitiveIndex(uint32_t  node_id,
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
            uint32_t pair_indices_count{};
            auto     pair_indices = GetTrianglePairIndices(node_id, &pair_indices_count);

            *out_primitive_index =
                (pair_indices_count == 0) ? 0 : pair_indices.front().first->UnpackPrimitiveIndex(pair_indices.front().second, local_primitive_index);
        }
        else
        {
            // The given node Id doesn't refer to a triangle node.
            // Don't output any geometry Id, and return an invalid child error code.
            return kRraErrorInvalidChildNode;
        }
        return kRraOk;
    }

    RraErrorCode EncodedRtIp31BottomLevelBvh::GetIsInactive(uint32_t node_id, uint32_t child_index, uint32_t global_child_index, bool* out_is_inactive) const
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
            uint32_t pair_indices_count{};
            auto     pair_indices = GetTrianglePairIndices(node_id, &pair_indices_count);

            *out_is_inactive =
                (pair_indices_count == 0) ? true : std::isnan(pair_indices.front().first->UnpackTriangleVertices(pair_indices.front().second, 0).v0.x);
            return kRraOk;
        }
        else if (IsProcedural())
        {
            uint32_t pair_indices_count{};
            auto     pair_indices = GetTrianglePairIndices(node_id, &pair_indices_count);
            if ((pair_indices_count == 0) || pair_indices.front().first == nullptr || !pair_indices.front().first->IsProcedural(pair_indices.front().second, 0))
            {
                return kRraErrorInvalidPointer;
            }
            *out_is_inactive = std::isnan(pair_indices.front().first->UnpackTriangleVertices(pair_indices.front().second, 0).v0.x);
            return kRraOk;
        }
        return kRraErrorInvalidChildNode;
    }

    RraErrorCode EncodedRtIp31BottomLevelBvh::GetNodeTriangleCount(uint32_t  node_id,
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

        uint32_t pair_indices_count{};
        auto     pair_indices = GetTrianglePairIndices(node_id, &pair_indices_count);

        uint32_t tri_count{};
        for (uint32_t i = 0; i < pair_indices_count; ++i)
        {
            tri_count += 1 + pair_indices[i].first->ReadTrianglePairDesc(pair_indices[i].second).Tri1Valid();
        }
        *out_triangle_count = tri_count;

        return kRraOk;
    }

    RraErrorCode EncodedRtIp31BottomLevelBvh::GetNodeTriangles(uint32_t          node_id,
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
            uint32_t pair_indices_count{};
            auto     pair_indices = GetTrianglePairIndices(node_id, &pair_indices_count);

            uint32_t triangle_idx = 0;

            for (uint32_t i = 0; i < pair_indices_count; ++i)
            {
                auto& pair_index{pair_indices[i]};
                auto  pair_descriptor = pair_index.first->ReadTrianglePairDesc(pair_index.second);

                auto tri0_data                  = pair_index.first->UnpackTriangleVertices(pair_index.second, 0);
                out_triangles[triangle_idx].a.x = tri0_data.v0.x;
                out_triangles[triangle_idx].a.y = tri0_data.v0.y;
                out_triangles[triangle_idx].a.z = tri0_data.v0.z;
                out_triangles[triangle_idx].b.x = tri0_data.v1.x;
                out_triangles[triangle_idx].b.y = tri0_data.v1.y;
                out_triangles[triangle_idx].b.z = tri0_data.v1.z;
                out_triangles[triangle_idx].c.x = tri0_data.v2.x;
                out_triangles[triangle_idx].c.y = tri0_data.v2.y;
                out_triangles[triangle_idx].c.z = tri0_data.v2.z;
                triangle_idx++;

                if (pair_descriptor.Tri1Valid())
                {
                    auto tri1_data                  = pair_index.first->UnpackTriangleVertices(pair_index.second, 1);
                    out_triangles[triangle_idx].a.x = tri1_data.v0.x;
                    out_triangles[triangle_idx].a.y = tri1_data.v0.y;
                    out_triangles[triangle_idx].a.z = tri1_data.v0.z;
                    out_triangles[triangle_idx].b.x = tri1_data.v1.x;
                    out_triangles[triangle_idx].b.y = tri1_data.v1.y;
                    out_triangles[triangle_idx].b.z = tri1_data.v1.z;
                    out_triangles[triangle_idx].c.x = tri1_data.v2.x;
                    out_triangles[triangle_idx].c.y = tri1_data.v2.y;
                    out_triangles[triangle_idx].c.z = tri1_data.v2.z;
                    triangle_idx++;
                }
            }
        }
        return kRraOk;
    }

    RraErrorCode EncodedRtIp31BottomLevelBvh::GetNodeVertexCount(uint32_t node_id, uint32_t child_index, uint32_t global_child_index, uint32_t* out_count) const
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
            uint32_t pair_indices_count{};
            auto     pair_indices = GetTrianglePairIndices(node_id, &pair_indices_count);

            // Leftmost 32 bits are pair_indices_idx and rightmost are vertex index. Since vertices may index into separate PrimitiveStructures.
            std::unordered_set<uint64_t> vertex_index_set{};
            uint32_t                     pair_indices_idx{0};
            for (uint32_t i = 0; i < pair_indices_count; ++i)
            {
                TrianglePairDesc desc{pair_indices[i].first->ReadTrianglePairDesc(pair_indices[i].second)};
                vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri0V0());
                vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri0V1());
                vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri0V2());

                if (desc.Tri1Valid())
                {
                    vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri1V0());
                    vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri1V1());
                    vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri1V2());
                }
                ++pair_indices_idx;
            }

            *out_count = (uint32_t)vertex_index_set.size();
        }
        return kRraOk;
    }

    RraErrorCode EncodedRtIp31BottomLevelBvh::GetNodeVertices(uint32_t               node_id,
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
            uint32_t pair_indices_count{};
            auto     pair_indices = GetTrianglePairIndices(node_id, &pair_indices_count);

            // Leftmost 32 bits are pair_indices_idx and rightmost are vertex index. Since vertices may index into separate PrimitiveStructures.
            std::unordered_set<uint64_t> vertex_index_set{};
            {
                uint32_t pair_indices_idx{0};
                for (uint32_t i = 0; i < pair_indices_count; ++i)
                {
                    TrianglePairDesc desc{pair_indices[i].first->ReadTrianglePairDesc(pair_indices[i].second)};
                    vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri0V0());
                    vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri0V1());
                    vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri0V2());

                    if (desc.Tri1Valid())
                    {
                        vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri1V0());
                        vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri1V1());
                        vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri1V2());
                    }
                    ++pair_indices_idx;
                }
            }

            uint32_t vertex_idx{0};
            for (uint64_t vertex_index_pair : vertex_index_set)
            {
                uint32_t  pair_indices_idx = (uint32_t)(vertex_index_pair >> 32);
                uint32_t  vertex_index     = (uint32_t)vertex_index_pair;
                glm::vec3 v                = pair_indices[pair_indices_idx].first->ReadVertex(vertex_index, false);

                out_vertices[vertex_idx++] = {v.x, v.y, v.z};
            }
        }
        return kRraOk;
    }

    std::array<std::pair<const PrimitiveStructure*, uint32_t>, 8> EncodedRtIp31BottomLevelBvh::GetTrianglePairIndices(dxr::amd::NodePointer node_ptr,
                                                                                                                      uint32_t*             out_count) const
    {
        auto                        byte_offset    = node_ptr.GetByteOffset() - header_->GetBufferOffsets().interior_nodes;
        const std::vector<uint8_t>& interior_nodes = GetInteriorNodesData();

        // Get full range of tri pairs.
        std::array<std::pair<const PrimitiveStructure*, uint32_t>, 8> tri_pair_indices{};
        const PrimitiveStructure* prim_structure = reinterpret_cast<const PrimitiveStructure*>(&interior_nodes[byte_offset]);
        uint32_t                  pair_index     = node_ptr.GetTrianglePairIndex();

        uint32_t idx{0};
        bool     should_stop{false};
        do
        {
            TrianglePairDesc desc{prim_structure->ReadTrianglePairDesc(pair_index)};
            should_stop             = desc.PrimRangeStopBit();
            tri_pair_indices[idx++] = {prim_structure, pair_index};
            ++pair_index;

            // Triangle pairs may span across multiple PrimitiveStructures.
            if (!should_stop && (pair_index == prim_structure->TrianglePairCount()))
            {
                pair_index = 0;
                byte_offset += sizeof(PrimitiveStructure);
                prim_structure = reinterpret_cast<const PrimitiveStructure*>(&interior_nodes[byte_offset]);
            }
        } while (!should_stop);

        *out_count = idx;
        return tri_pair_indices;
    }

    std::array<uint32_t, 8> EncodedRtIp31BottomLevelBvh::GetPrimitiveStructureOffsets(dxr::amd::NodePointer node_ptr, uint32_t* out_count)
    {
        auto                  byte_offset    = node_ptr.GetByteOffset() - header_->GetBufferOffsets().interior_nodes;
        std::vector<uint8_t>& interior_nodes = GetInteriorNodesData();

        // Get full range of tri pairs.
        std::array<uint32_t, 8> byte_offsets{};
        PrimitiveStructure*     prim_structure = reinterpret_cast<PrimitiveStructure*>(&interior_nodes[byte_offset]);
        uint32_t                pair_index     = node_ptr.GetTrianglePairIndex();

        uint32_t idx{0};
        bool     should_stop{false};
        do
        {
            TrianglePairDesc desc{prim_structure->ReadTrianglePairDesc(pair_index)};
            should_stop         = desc.PrimRangeStopBit();
            byte_offsets[idx++] = byte_offset;
            ++pair_index;

            // Triangle pairs may span across multiple PrimitiveStructures.
            if (!should_stop && (pair_index == prim_structure->TrianglePairCount()))
            {
                pair_index = 0;
                byte_offset += sizeof(PrimitiveStructure);
                prim_structure = reinterpret_cast<PrimitiveStructure*>(&interior_nodes[byte_offset]);
            }
        } while (!should_stop);

        *out_count = idx;
        return byte_offsets;
    }

    std::uint32_t EncodedRtIp31BottomLevelBvh::GetNodeCount(const BvhNodeFlags flag)
    {
        if (flag == BvhNodeFlags::kIsInteriorNode)
        {
            return interior_node_count_;
        }
        else if (flag == BvhNodeFlags::kIsLeafNode)
        {
            return leaf_node_count_;
        }
        else
        {
            return interior_node_count_ + leaf_node_count_;
        }
    }

    bool EncodedRtIp31BottomLevelBvh::PostLoad()
    {
        ScanTreeDepth();
        return true;
    }

    void EncodedRtIp31BottomLevelBvh::CountNodes()
    {
        if (interior_nodes_.empty())
        {
            return;
        }

        // We calculate node count through traversal because for RtIp3.1, the leaf node count in the header is actually the number
        // of primitive packets (instances of PrimitiveStructure), and multiple leaf nodes can reference the same primitive packet.
        interior_node_count_ = 0;
        leaf_node_count_     = 0;
        std::deque<dxr::amd::NodePointer> traversal_stack;

        // Top level node doesn't exist in the data so needs to be created. Assumed to be a BVH8.
        dxr::amd::NodePointer root_ptr = dxr::amd::NodePointer(dxr::amd::NodeType::kAmdNodeBoxFp32, dxr::amd::kAccelerationStructureHeaderSize);
        traversal_stack.push_back(root_ptr);

        const auto& header_offsets = header_->GetBufferOffsets();
        while (!traversal_stack.empty())
        {
            dxr::amd::NodePointer node_ptr = traversal_stack.back();
            traversal_stack.pop_back();

            // Get the byte offset relative to the internal node buffer.
            auto byte_offset = node_ptr.GetByteOffset() - header_offsets.interior_nodes;
            if (node_ptr.IsFp32BoxNode())
            {
                ++interior_node_count_;

                // The quantized BVH8 node uses the same enum value as Fp32 Box node.
                const auto            node = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes_[byte_offset]);
                dxr::amd::NodePointer child_ptrs[8]{};
                node->DecodeChildrenOffsets((uint32_t*)child_ptrs);

                const uint32_t valid_child_count = node->ValidChildCount();
                for (uint32_t i = 0; i < valid_child_count; ++i)
                {
                    if (!child_ptrs[i].IsInvalid())
                    {
                        traversal_stack.push_back(child_ptrs[i]);

                        // Node packing: two of this box's slots decode to the same child pointer (the second slot had
                        // NodeRangeLength()==0, so DecodeChildrenOffsets did not advance the running offset). Flag it so
                        // the frontend switches to composite (global-child-index + node-id) keys for this BLAS.
                        for (uint32_t j = 0; j < i; ++j)
                        {
                            if (!child_ptrs[j].IsInvalid() && child_ptrs[j].GetRawPointer() == child_ptrs[i].GetRawPointer())
                            {
                                has_node_packing_ = true;
                                break;
                            }
                        }
                    }
                }
            }
            else if (node_ptr.IsTriangleNode())
            {
                ++leaf_node_count_;
            }
            else if (node_ptr.IsInstanceNode())
            {
                // Cluster BLAS leaves are hardware instance nodes referencing CLASes.
                ++leaf_node_count_;
            }
        }
    }

    bool EncodedRtIp31BottomLevelBvh::HasNodePacking() const
    {
        return has_node_packing_;
    }

    void EncodedRtIp31BottomLevelBvh::ComputeSurfaceAreaHeuristic()
    {
        size_t num_box_nodes = header_->GetInteriorNodeCount();
        if (num_box_nodes == 0)
        {
            return;
        }

        std::deque<dxr::amd::NodePointer> traversal_stack;

        std::vector<uint8_t>& interior_nodes = GetInteriorNodesData();

        // Top level node doesn't exist in the data so needs to be created. Assumed to be a BVH8.
        dxr::amd::NodePointer root_ptr = dxr::amd::NodePointer(dxr::amd::NodeType::kAmdNodeBoxFp32, dxr::amd::kAccelerationStructureHeaderSize);
        traversal_stack.push_back(root_ptr);

        const auto& header_offsets = header_->GetBufferOffsets();
        while (!traversal_stack.empty())
        {
            dxr::amd::NodePointer node_ptr = traversal_stack.back();
            traversal_stack.pop_back();

            // Get the byte offset relative to the internal node buffer.
            auto byte_offset = node_ptr.GetByteOffset() - header_offsets.interior_nodes;
            if (node_ptr.IsFp32BoxNode())
            {
                // The quantized BVH8 node uses the same enum value as Fp32 Box node.
                const auto            node = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[byte_offset]);
                dxr::amd::NodePointer child_ptrs[8]{};
                node->DecodeChildrenOffsets((uint32_t*)child_ptrs);
                float out_surface_area = 0.0f;
                float total_child_area = 0.0f;

                for (uint32_t i = 0; i < node->ValidChildCount(); ++i)
                {
                    if (!child_ptrs[i].IsInvalid())
                    {
                        traversal_stack.push_back(child_ptrs[i]);

                        if (RraBlasGetSurfaceAreaImpl(this, child_ptrs[i].GetRawPointer(), 0, 0, &out_surface_area) ==
                            kRraOk)  // Can pass 0 here since it's RtIp3.
                        {
                            total_child_area += out_surface_area;
                        }
                    }
                }

                // Take that as ratio of the current node.
                float sah{};
                out_surface_area = 0.0;
                if (RraBlasGetSurfaceAreaImpl(this, node_ptr.GetRawPointer(), 0, 0, &out_surface_area) == kRraOk)  // Can pass 0 here since it's RtIp3.
                {
                    sah = 0.25f * (total_child_area / out_surface_area);
                }

                if (out_surface_area == 0.0)
                {
                    sah = 0.0f;
                }

                SetInteriorNodeSurfaceAreaHeuristic(node_ptr.GetRawPointer(), sah);
            }
            else if (node_ptr.IsTriangleNode())
            {
                BoundingVolumeExtents extent{};
                RraErrorCode error_code = RraBlasGetBoundingVolumeExtents(id_, node_ptr.GetRawPointer(), 0, 0, &extent);  // Pass zero since it's RtIp3.
                RRA_ASSERT(error_code == kRraOk);
                float obb_total_surface_area{};
                error_code = RraBvhGetBoundingVolumeSurfaceArea(&extent, &obb_total_surface_area);
                RRA_ASSERT(error_code == kRraOk);
                float triangle_surface_area{};
                error_code = RraBlasGetSurfaceArea(id_, node_ptr.GetRawPointer(), 0, 0, &triangle_surface_area);  // Pass zero since it's RtIp3.
                RRA_ASSERT(error_code == kRraOk);
                RRA_UNUSED(error_code);
                float sah = 0.0f;

                if (obb_total_surface_area >= triangle_surface_area && obb_total_surface_area > FLT_MIN)
                {
                    // Multiply triangle area by 2, to account for probability of ray going through front or back face.
                    sah = (2.0f * triangle_surface_area) / obb_total_surface_area;
                }

                // Mathematically SAH should not ever be greater than 1.0, but with really problematic triangles (extremely long and thin)
                // floating point errors can push it over. I've seen as high as 1.454 in the Deathloop trace.
                if (!std::isnan(sah))
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

                SetLeafNodeSurfaceAreaHeuristic(node_ptr.GetRawPointer(), sah);
            }
        }
    }

    uint32_t EncodedRtIp31BottomLevelBvh::GetParentNode(uint32_t node_id, uint32_t global_child_index) const
    {
        RRA_UNUSED(global_child_index);
        dxr::amd::NodePointer node_ptr(node_id);
        if (node_ptr.IsLeafNode())
        {
            // With RtIp3.1, triangle node parents are not stored explicitly so we store them in this map during traversal.
            return triangle_node_parents_.at(node_ptr.GetRawPointer());
        }
        else
        {
            const auto& interior_nodes = GetInteriorNodesData();
            const auto& header_offsets = GetHeader().GetBufferOffsets();
            uint32_t    byte_offset    = node_ptr.GetByteOffset() - header_offsets.interior_nodes;
            const auto  node           = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[byte_offset]);
            return node->parentPointer;
        }
    }

    void EncodedRtIp31BottomLevelBvh::PreprocessParents()
    {
        size_t num_box_nodes = header_->GetInteriorNodeCount();
        if (num_box_nodes == 0)
        {
            return;
        }

        std::deque<dxr::amd::NodePointer> traversal_stack;

        std::vector<uint8_t>& interior_nodes = GetInteriorNodesData();

        // Top level node doesn't exist in the data so needs to be created. Assumed to be a BVH8.
        dxr::amd::NodePointer root_ptr = dxr::amd::NodePointer(dxr::amd::NodeType::kAmdNodeBoxFp32, dxr::amd::kAccelerationStructureHeaderSize);
        traversal_stack.push_back(root_ptr);

        const auto& header_offsets = header_->GetBufferOffsets();
        while (!traversal_stack.empty())
        {
            dxr::amd::NodePointer node_ptr = traversal_stack.back();
            traversal_stack.pop_back();

            // Get the byte offset relative to the internal node buffer.
            auto byte_offset = node_ptr.GetByteOffset() - header_offsets.interior_nodes;
            if (node_ptr.IsFp32BoxNode())
            {
                // The quantized BVH8 node uses the same enum value as Fp32 Box node.
                const auto            node = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[byte_offset]);
                dxr::amd::NodePointer child_ptrs[8]{};
                node->DecodeChildrenOffsets((uint32_t*)child_ptrs);

                for (uint32_t i = 0; i < node->ValidChildCount(); ++i)
                {
                    if (!child_ptrs[i].IsInvalid())
                    {
                        // Record parents for all leaves (triangles and Cluster-BLAS instance leaves); box-node
                        // children are pushed for further traversal.
                        if (child_ptrs[i].IsLeafNode())
                        {
                            triangle_node_parents_[child_ptrs[i].GetRawPointer()] = node_ptr.GetRawPointer();
                        }
                        else
                        {
                            traversal_stack.push_back(child_ptrs[i]);
                        }
                    }
                }
            }
        }
    }

    float EncodedRtIp31BottomLevelBvh::GetLeafNodeSurfaceAreaHeuristic(uint32_t node_id, uint32_t global_child_index) const
    {
        RRA_UNUSED(global_child_index);
        dxr::amd::NodePointer* node_ptr = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);

        const uint32_t byte_offset = node_ptr->GetByteOffset();
        const uint32_t leaf_nodes  = GetHeader().GetBufferOffsets().interior_nodes;
        if (byte_offset < leaf_nodes)
        {
            // Bad address for a triangle.
            return std::numeric_limits<float>::quiet_NaN();
        }
        const uint32_t index = (byte_offset - leaf_nodes) / sizeof(dxr::amd::TriangleNode);
        // A CBLAS cluster-reference leaf is a hardware instance node, not a triangle node, so it has no
        // surface-area-heuristic entry. Return NaN rather than throwing from at().
        const auto it = triangle_surface_area_heuristic_.find(index);
        if (it == triangle_surface_area_heuristic_.end())
        {
            return std::numeric_limits<float>::quiet_NaN();
        }
        return it->second;
    }

    void EncodedRtIp31BottomLevelBvh::SetLeafNodeSurfaceAreaHeuristic(uint32_t node_ptr, float surface_area_heuristic)
    {
        dxr::amd::NodePointer node        = (dxr::amd::NodePointer)node_ptr;
        const uint32_t        byte_offset = node.GetByteOffset();
        const uint32_t        leaf_nodes  = GetHeader().GetBufferOffsets().interior_nodes;
        if (byte_offset < leaf_nodes)
        {
            // Bad address for a triangle.
            return;
        }
        const uint32_t index                    = (byte_offset - leaf_nodes) / sizeof(dxr::amd::TriangleNode);
        triangle_surface_area_heuristic_[index] = surface_area_heuristic;
    }

    bool EncodedRtIp31BottomLevelBvh::IsClusterBlas() const
    {
        return header_->GetRawHeader().IsClusterBlas();
    }

    bool EncodedRtIp31BottomLevelBvh::IsCluster() const
    {
        return header_->GetRawHeader().IsCluster();
    }

    uint32_t EncodedRtIp31BottomLevelBvh::GetTriangleCount() const
    {
        // A CLAS (ClusterLevel) stores its triangles directly in cluster leaf nodes and leaves the
        // geometry-info primitive count at 0, so fall back to the header's active primitive count for
        // vertex-buffer sizing / unique-triangle count. Ordinary BLASes use the geometry-info sum unchanged.
        if (header_->GetRawHeader().IsCluster())
        {
            return header_->GetActivePrimitiveCount();
        }
        return EncodedBottomLevelBvh::GetTriangleCount();
    }

    bool EncodedRtIp31BottomLevelBvh::IsClusterRefNode(uint32_t node_id) const
    {
        if (!IsClusterBlas())
        {
            return false;
        }
        return dxr::amd::NodePointer(node_id).IsInstanceNode();
    }

    const HwInstanceNodeRRA* EncodedRtIp31BottomLevelBvh::GetClusterRefInstanceNode(uint32_t node_id) const
    {
        dxr::amd::NodePointer node_ptr(node_id);
        if (!node_ptr.IsInstanceNode())
        {
            return nullptr;
        }

        const auto&    header_offsets = header_->GetBufferOffsets();
        const uint32_t byte_offset    = node_ptr.GetByteOffset() - header_offsets.interior_nodes;
        if (byte_offset >= interior_nodes_.size())
        {
            return nullptr;
        }

        return reinterpret_cast<const HwInstanceNodeRRA*>(&interior_nodes_[byte_offset]);
    }

    RraErrorCode EncodedRtIp31BottomLevelBvh::GetClasIndexFromClusterRefNode(uint32_t node_id, uint64_t* out_clas_blas_index) const
    {
        if (out_clas_blas_index == nullptr)
        {
            return kRraErrorInvalidPointer;
        }

        const HwInstanceNodeRRA* instance_node = GetClusterRefInstanceNode(node_id);
        if (instance_node == nullptr)
        {
            return kRraErrorInvalidChildNode;
        }

        // The cluster-reference leaf encodes the referenced CLAS header address (the same aligned_addr_64b<<6
        // decode used by TLAS instances). blas_map_ registers that address as a key during load, so this is
        // an exact lookup.
        NodePointer64 temp_ptr{};
        temp_ptr.u64                = instance_node->data.childBasePtr;
        uint64_t   clas_address     = (temp_ptr.aligned_addr_64b << 6);
        const auto it               = blas_map_.find(clas_address);
        if (it == blas_map_.end())
        {
            return kRraErrorIndexOutOfRange;
        }
        *out_clas_blas_index = it->second;
        return kRraOk;
    }

    RraErrorCode EncodedRtIp31BottomLevelBvh::GetClusterRefNodeTransform(uint32_t node_id, float* out_transform) const
    {
        if (out_transform == nullptr)
        {
            return kRraErrorInvalidPointer;
        }

        const HwInstanceNodeRRA* instance_node = GetClusterRefInstanceNode(node_id);
        if (instance_node == nullptr)
        {
            return kRraErrorInvalidChildNode;
        }

        memcpy(out_transform, instance_node->data.worldToObject, dxr::kMatrix3x4Size);
        return kRraOk;
    }

    RraErrorCode EncodedRtIp31BottomLevelBvh::GetClusterRefNodeId(uint32_t node_id, uint32_t* out_id) const
    {
        if (out_id == nullptr)
        {
            return kRraErrorInvalidPointer;
        }

        const HwInstanceNodeRRA* instance_node = GetClusterRefInstanceNode(node_id);
        if (instance_node == nullptr)
        {
            return kRraErrorInvalidChildNode;
        }

        // Low 24 bits hold the instance contribution / CLAS id; the top 8 bits are the instance mask.
        *out_id = instance_node->data.userDataAndInstanceMask & 0x00FFFFFF;
        return kRraOk;
    }

    RraErrorCode EncodedRtIp31BottomLevelBvh::GetClusterRefNodeMask(uint32_t node_id, uint32_t* out_mask) const
    {
        if (out_mask == nullptr)
        {
            return kRraErrorInvalidPointer;
        }

        const HwInstanceNodeRRA* instance_node = GetClusterRefInstanceNode(node_id);
        if (instance_node == nullptr)
        {
            return kRraErrorInvalidChildNode;
        }

        // The top 8 bits hold the instance mask; the low 24 bits are the instance contribution / CLAS id.
        *out_mask = (instance_node->data.userDataAndInstanceMask >> 24) & 0xFF;
        return kRraOk;
    }

    void EncodedRtIp31BottomLevelBvh::ConvertBlasAddressesToIndices(const std::unordered_map<GpuVirtualAddress, uint64_t>& blas_map)
    {
        // Only a Cluster BLAS needs the map, to resolve its instance-leaves to their referenced CLAS index.
        if (IsClusterBlas())
        {
            blas_map_ = blas_map;
        }
    }

}  // namespace rta

