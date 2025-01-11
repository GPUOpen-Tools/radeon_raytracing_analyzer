//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 3.1 (Navi4x) specific bottom level acceleration structure
/// implementation.
//=============================================================================

#include "bvh/rtip31/encoded_rt_ip_31_bottom_level_bvh.h"
#include "bvh/rtip31/rt_ip_31_acceleration_structure_header.h"

#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>  // --> isnan, isinf, ceil
#include <deque>
#include <limits>
#include "bvh/dxr_definitions.h"
#include "surface_area_heuristic.h"
#include "public/rra_blas.h"
#include "rra_blas_impl.h"

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
        const bool skip_meta_data = static_cast<std::uint8_t>(import_option) & static_cast<std::uint8_t>(BvhBundleReadOption::kNoMetaData);

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

        const auto prim_node_ptrs_size = header_->GetPrimitiveCount() * sizeof(dxr::amd::NodePointer);
        primitive_node_ptrs_           = std::vector<dxr::amd::NodePointer>(header_->GetPrimitiveCount());
        buffer_stream.Read(prim_node_ptrs_size, primitive_node_ptrs_.data());

        buffer_stream.Close();

        // Set the root node offset.
        header_offset_ = static_cast<uint64_t>(chunk_header.header_offset);

#ifdef RRA_INTERNAL_COMMENTS
        //        return Validate();
#endif

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

    std::array<std::pair<PrimitiveStructure*, uint32_t>, 8> EncodedRtIp31BottomLevelBvh::GetTrianglePairIndices(dxr::amd::NodePointer node_ptr,
                                                                                                                uint32_t*             out_count)
    {
        auto                  byte_offset    = node_ptr.GetByteOffset() - header_->GetBufferOffsets().interior_nodes;
        std::vector<uint8_t>& interior_nodes = GetInteriorNodesData();

        // Get full range of tri pairs.
        std::array<std::pair<PrimitiveStructure*, uint32_t>, 8> tri_pair_indices{};
        PrimitiveStructure*                                     prim_structure = reinterpret_cast<PrimitiveStructure*>(&interior_nodes[byte_offset]);
        uint32_t                                                pair_index     = node_ptr.GetTrianglePairIndex();

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
                prim_structure = reinterpret_cast<PrimitiveStructure*>(&interior_nodes[byte_offset]);
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
        triangle_surface_area_heuristic_.resize((uint32_t)interior_nodes_.size(), 0);
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
            dxr::amd::NodePointer node_ptr = traversal_stack.front();
            traversal_stack.pop_front();

            // Get the byte offset relative to the internal node buffer.
            auto byte_offset = node_ptr.GetByteOffset() - header_offsets.interior_nodes;
            if (node_ptr.IsFp32BoxNode())
            {
                ++interior_node_count_;

                // The quantized BVH8 node uses the same enum value as Fp32 Box node.
                const auto            node = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes_[byte_offset]);
                dxr::amd::NodePointer child_ptrs[8]{};
                node->DecodeChildrenOffsets((uint32_t*)child_ptrs);

                for (uint32_t i = 0; i < node->ValidChildCount(); ++i)
                {
                    if (!child_ptrs[i].IsInvalid())
                    {
                        traversal_stack.push_back(child_ptrs[i]);
                    }
                }
            }
            else if (node_ptr.IsTriangleNode())
            {
                ++leaf_node_count_;
            }
        }
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
            dxr::amd::NodePointer node_ptr = traversal_stack.front();
            traversal_stack.pop_front();

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

                        if (RraBlasGetSurfaceAreaImpl(this, &child_ptrs[i], &out_surface_area) == kRraOk)
                        {
                            total_child_area += out_surface_area;
                        }
                    }
                }

                // Take that as ratio of the current node.
                float sah{};
                out_surface_area = 0.0;
                if (RraBlasGetSurfaceAreaImpl(this, &node_ptr, &out_surface_area) == kRraOk)
                {
                    sah = 0.25f * (total_child_area / out_surface_area);
                }

                if (out_surface_area == 0.0)
                {
                    sah = 0.0f;
                }

                SetInteriorNodeSurfaceAreaHeuristic(node_ptr, sah);
            }
            else if (node_ptr.IsTriangleNode())
            {
                BoundingVolumeExtents extent{};
                RraBlasGetBoundingVolumeExtents(id_, node_ptr.GetRawPointer(), &extent);
                float obb_total_surface_area{};
                RraBvhGetBoundingVolumeSurfaceArea(&extent, &obb_total_surface_area);
                float triangle_surface_area{};
                RraBlasGetSurfaceArea(id_, node_ptr.GetRawPointer(), &triangle_surface_area);
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

    dxr::amd::NodePointer EncodedRtIp31BottomLevelBvh::GetParentNode(const dxr::amd::NodePointer* node_ptr) const
    {
        if (node_ptr->IsLeafNode())
        {
            // With RtIp3.1, triangle node parents are not stored explicitly so we store them in this map during traversal.
            return dxr::amd::NodePointer(triangle_node_parents_.at(node_ptr->GetRawPointer()));
        }
        else
        {
            const auto& interior_nodes = GetInteriorNodesData();
            const auto& header_offsets = GetHeader().GetBufferOffsets();
            uint32_t    byte_offset    = node_ptr->GetByteOffset() - header_offsets.interior_nodes;
            const auto  node           = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[byte_offset]);
            return dxr::amd::NodePointer(node->parentPointer);
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
            dxr::amd::NodePointer node_ptr = traversal_stack.front();
            traversal_stack.pop_front();

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
                        if (child_ptrs[i].IsTriangleNode())
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

    float EncodedRtIp31BottomLevelBvh::GetLeafNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr) const
    {
        const uint32_t byte_offset = node_ptr.GetByteOffset();
        const uint32_t leaf_nodes  = GetHeader().GetBufferOffsets().interior_nodes;
        if (byte_offset < leaf_nodes)
        {
            // Bad address for a triangle.
            return std::numeric_limits<float>::quiet_NaN();
        }
        const uint32_t index = (byte_offset - leaf_nodes) / sizeof(dxr::amd::TriangleNode);
        assert(index < triangle_surface_area_heuristic_.size());
        return triangle_surface_area_heuristic_.at(index);
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
        const uint32_t index = (byte_offset - leaf_nodes) / sizeof(dxr::amd::TriangleNode);
        assert(index < triangle_surface_area_heuristic_.size());
        triangle_surface_area_heuristic_[index] = surface_area_heuristic;
    }

}  // namespace rta
