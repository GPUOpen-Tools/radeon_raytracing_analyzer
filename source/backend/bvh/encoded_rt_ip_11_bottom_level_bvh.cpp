//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 1.1 (Navi2x) specific bottom level acceleration structure
/// implementation.
//=============================================================================

#include "bvh/encoded_rt_ip_11_bottom_level_bvh.h"

#include <iostream>
#include <vector>
#include <cassert>
#include <deque>

namespace rta
{

    EncodedRtIp11BottomLevelBvh::~EncodedRtIp11BottomLevelBvh()
    {
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
            auto primitive_index = 0;

            if (header_->GetGeometryType() == BottomLevelBvhGeometryType::kTriangle)
            {
                const auto* triangle_nodes = reinterpret_cast<const dxr::amd::TriangleNode*>(leaf_nodes_.data());
                const auto& triangle_node  = triangle_nodes[i];
                geometry_index             = triangle_node.GetGeometryIndex();
                primitive_index            = triangle_node.GetPrimitiveIndex(dxr::amd::NodeType::kAmdNodeTriangle0);
                node_ptr                   = dxr::amd::NodePointer(dxr::amd::NodeType::kAmdNodeTriangle0, byte_offset);
            }
            else
            {
                const auto* procedural_nodes = reinterpret_cast<const dxr::amd::ProceduralNode*>(leaf_nodes_.data());
                const auto& procedural_node  = procedural_nodes[i];
                geometry_index               = procedural_node.GetGeometryIndex();
                primitive_index              = procedural_node.GetPrimitiveIndex();
                node_ptr                     = dxr::amd::NodePointer(dxr::amd::NodeType::kAmdNodeProcedural, byte_offset);
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
        const bool skip_meta_data = static_cast<std::uint8_t>(import_option) & static_cast<std::uint8_t>(BvhBundleReadOption::kNoMetaData);

        std::vector<std::uint8_t> buffer(data_size);
        if (data_size > 0)
        {
            chunk_file.ReadChunkDataToBuffer(identifier, static_cast<uint32_t>(chunk_index), buffer.data());
        }

        if (!skip_meta_data)
        {
            memcpy(&meta_data_, buffer.data() + chunk_header.meta_header_offset, chunk_header.meta_header_size);
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

        return true;
    }

    bool EncodedRtIp11BottomLevelBvh::Validate()
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

    bool EncodedRtIp11BottomLevelBvh::PostLoad()
    {
        size_t num_leaf_nodes = leaf_nodes_.size() / sizeof(dxr::amd::TriangleNode);
        ScanTreeDepth();
        triangle_surface_area_heuristic_.resize(num_leaf_nodes, 0);
        return true;
    }

    float EncodedRtIp11BottomLevelBvh::GetLeafNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr) const
    {
        const uint32_t byte_offset = node_ptr.GetByteOffset();
        const uint32_t leaf_nodes  = GetHeader().GetBufferOffsets().leaf_nodes;
        const uint32_t index       = (byte_offset - leaf_nodes) / sizeof(dxr::amd::TriangleNode);
        assert(index < (leaf_nodes_.size() / sizeof(dxr::amd::TriangleNode)));
        assert(index < triangle_surface_area_heuristic_.size());
        return triangle_surface_area_heuristic_[index];
    }

    void EncodedRtIp11BottomLevelBvh::SetLeafNodeSurfaceAreaHeuristic(uint64_t leaf_index, float surface_area_heuristic)
    {
        assert(leaf_index < (leaf_nodes_.size() / sizeof(dxr::amd::TriangleNode)));
        assert(leaf_index < triangle_surface_area_heuristic_.size());
        triangle_surface_area_heuristic_[leaf_index] = surface_area_heuristic;
    }

    float EncodedRtIp11BottomLevelBvh::GetSurfaceAreaHeuristic() const
    {
        return surface_area_heuristic_;
    }

    void EncodedRtIp11BottomLevelBvh::SetSurfaceAreaHeuristic(float surface_area_heuristic)
    {
        surface_area_heuristic_ = surface_area_heuristic;
    }

}  // namespace rta
