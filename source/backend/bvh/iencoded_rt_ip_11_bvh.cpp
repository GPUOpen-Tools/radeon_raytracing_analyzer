//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 1.1 (Navi2x) specific base class implementation.
//=============================================================================

#include "bvh/iencoded_rt_ip_11_bvh.h"

#include <iostream>
#include <vector>
#include <cassert>
#include <deque>
#include <unordered_set>
#include <float.h>

#include "public/rra_assert.h"

#include "bvh/dxr_type_conversion.h"
#include "bvh/flags_util.h"
#include "bvh/irt_ip_11_acceleration_structure_header.h"

#include "bvh/encoded_rt_ip_11_bottom_level_bvh.h"
#include "bvh/encoded_rt_ip_11_top_level_bvh.h"

namespace rta
{
    IEncodedRtIp11Bvh::IEncodedRtIp11Bvh()
        : header_(CreateRtIp11AccelerationStructureHeader())
    {
    }

    IEncodedRtIp11Bvh::~IEncodedRtIp11Bvh()
    {
    }

    void IEncodedRtIp11Bvh::SetID(const std::uint64_t id)
    {
        id_ = id;
        meta_data_.SetGpuVa(id);
    }

    std::uint64_t IEncodedRtIp11Bvh::GetID() const
    {
        return id_;
    }

    void IEncodedRtIp11Bvh::SetVirtualAddress(const std::uint64_t address)
    {
        gpu_virtual_address_ = address;
    }

    std::uint64_t IEncodedRtIp11Bvh::GetVirtualAddress() const
    {
        return gpu_virtual_address_;
    }

    const dxr::amd::MetaDataV1& IEncodedRtIp11Bvh::GetMetaData() const
    {
        return meta_data_;
    }

    const IRtIp11AccelerationStructureHeader& IEncodedRtIp11Bvh::GetHeader() const
    {
        return *header_;
    }

    const dxr::amd::ParentBlock& IEncodedRtIp11Bvh::GetParentData() const
    {
        return parent_data_;
    }

    const std::vector<std::uint8_t>& IEncodedRtIp11Bvh::GetInteriorNodesData() const
    {
        return interior_nodes_;
    }

    std::vector<std::uint8_t>& IEncodedRtIp11Bvh::GetInteriorNodesData()
    {
        return interior_nodes_;
    }

    bool IEncodedRtIp11Bvh::IsCompacted() const
    {
        return is_compacted_;
    }

    std::uint64_t IEncodedRtIp11Bvh::GetBufferByteSize() const
    {
        return GetBufferByteSizeImpl(ExportOption::kDefault);
    }

    BvhFormat IEncodedRtIp11Bvh::GetFormat() const
    {
        BvhFormat format = {};
        format.encoding  = BvhEncoding::kAmdRtIp_1_1;
        return format;
    }

    const dxr::amd::Float32BoxNode* IEncodedRtIp11Bvh::GetFloat32Box(const dxr::amd::NodePointer node_pointer, const int offset) const
    {
        assert(node_pointer.IsFp32BoxNode());
        return reinterpret_cast<const dxr::amd::Float32BoxNode*>(
            &interior_nodes_[node_pointer.GetByteOffset() - (size_t)header_->GetBufferOffsets().interior_nodes + offset * dxr::amd::kFp32BoxNodeSize]);
    }

    const dxr::amd::Float16BoxNode* IEncodedRtIp11Bvh::GetFloat16Box(const dxr::amd::NodePointer node_pointer, const int offset) const
    {
        assert(node_pointer.IsFp16BoxNode());
        return reinterpret_cast<const dxr::amd::Float16BoxNode*>(
            &interior_nodes_[node_pointer.GetByteOffset() - (size_t)header_->GetBufferOffsets().interior_nodes + offset * dxr::amd::kFp16BoxNodeSize]);
    }

    const dxr::amd::InstanceNode* IEncodedRtIp11Bvh::GetInstanceNode(const dxr::amd::NodePointer node_pointer, const int offset) const
    {
        assert(node_pointer.IsInstanceNode());
        if (header_->GetPostBuildInfo().IsBottomLevel())
        {
            // Bottom BHV cannot have instance nodes
            assert(false);
            return nullptr;
        }
        else
        {
            RRA_UNUSED(offset);
            RRA_ASSERT(offset != 0);
            auto bvh = static_cast<const EncodedRtIp11TopLevelBvh*>(this);
            return bvh->GetInstanceNode(&node_pointer);
        }
    }

    const dxr::amd::TriangleNode* IEncodedRtIp11Bvh::GetTriangleNode(const dxr::amd::NodePointer node_pointer, const int offset) const
    {
        assert(node_pointer.IsTriangleNode());
        if (header_->GetPostBuildInfo().IsBottomLevel() && node_pointer.GetByteOffset() >= header_->GetBufferOffsets().leaf_nodes)
        {
            auto bvh = static_cast<const EncodedRtIp11BottomLevelBvh*>(this);
            return reinterpret_cast<const dxr::amd::TriangleNode*>(
                &bvh->GetLeafNodesData()[node_pointer.GetByteOffset() + (size_t)offset * dxr::amd::kLeafNodeSize - header_->GetBufferOffsets().leaf_nodes]);
        }
        else
        {
            // Bottom BHV cannot have instance nodes
            return nullptr;
        }
    }

    const dxr::amd::ProceduralNode* IEncodedRtIp11Bvh::GetProceduralNode(const dxr::amd::NodePointer node_pointer, int offset) const
    {
        assert(node_pointer.IsProceduralNode());
        if (header_->GetPostBuildInfo().IsBottomLevel())
        {
            auto bvh = static_cast<const EncodedRtIp11BottomLevelBvh*>(this);
            return reinterpret_cast<const dxr::amd::ProceduralNode*>(
                &bvh->GetLeafNodesData()[node_pointer.GetByteOffset() + (size_t)offset * dxr::amd::kLeafNodeSize - header_->GetBufferOffsets().leaf_nodes]);
        }
        else
        {
            // Bottom BHV cannot have instance nodes
            assert(false);
            return nullptr;
        }
    }

    const dxr::amd::NodePointer* IEncodedRtIp11Bvh::GetPrimitiveNodePointer(int32_t index) const
    {
        return &primitive_node_ptrs_[index];
    }

    bool IEncodedRtIp11Bvh::IsCompactedImpl() const
    {
        return is_compacted_;
    }

    bool IEncodedRtIp11Bvh::IsEmptyImpl() const
    {
        return header_->GetInteriorNodeCount() == 0;
    }

    uint64_t IEncodedRtIp11Bvh::GetInactiveInstanceCountImpl() const
    {
        return 0;
    }

    // Compute the node buffer sizes based on compaction and the header entries, such as triangle compression.
    static std::tuple<std::uint64_t, std::uint64_t> ComputeNodeBufferSizesFromBvhHeader(IRtIp11AccelerationStructureHeader& header,
                                                                                        const bool                          is_compacted,
                                                                                        const bool                          output_warnings = true)
    {
        const auto actual_interior_node_buffer_size = header.GetBufferOffsets().leaf_nodes - header.GetBufferOffsets().interior_nodes;

        const auto compression_mode    = ToDxrTriangleCompressionMode(header.GetPostBuildInfo().GetTriangleCompressionMode());
        const auto compression_enabled = (compression_mode != dxr::amd::TriangleCompressionMode::kAmdNoTriangleCompression);

        auto interior_node_buffer_size = (is_compacted) ? header.CalculateInteriorNodeBufferSize() : header.CalculateWorstCaseInteriorNodeBufferSize();

        // Use the fallback and use the actual one, print out a warning
        if (interior_node_buffer_size != actual_interior_node_buffer_size)
        {
            RRA_UNUSED(output_warnings);
#ifdef _DEBUG
            if (output_warnings)
            {
                std::cout << "Warning: interior node buffer byte size could not be reconstructed "
                             "to estimate the parent data: expected "
                          << actual_interior_node_buffer_size << " but computed " << interior_node_buffer_size << "\n ";
            }
#endif
            interior_node_buffer_size = actual_interior_node_buffer_size;
        }

        auto leaf_node_buffer_size = (is_compacted && !compression_enabled)
                                         ? header.CalculateLeafNodeBufferSize()
                                         : header.CalculateCompressionModeLeafNodeBufferSize(ToBvhTriangleCompressionMode(compression_mode));

        auto actual_leaf_node_buffer_size = header.CalculateActualLeafNodeBufferSize();

        if (actual_leaf_node_buffer_size != leaf_node_buffer_size)
        {
#ifdef _DEBUG
            if (output_warnings)
            {
                std::cout << "Warning: leaf node buffer byte size could not be reconstructed "
                             "to estimate the parent data: expected "
                          << actual_leaf_node_buffer_size << " but computed " << leaf_node_buffer_size << "\n ";
            }
#endif
            leaf_node_buffer_size = actual_leaf_node_buffer_size;
        }

        return {actual_interior_node_buffer_size, actual_leaf_node_buffer_size};
    }

    std::uint32_t IEncodedRtIp11Bvh::GetNodeCount(const BvhNodeFlags flag) const
    {
        if (flag == BvhNodeFlags::kIsInteriorNode)
        {
            return header_->GetInteriorNodeCount();
        }
        else if (flag == BvhNodeFlags::kIsLeafNode)
        {
            return header_->GetLeafNodeCount();
        }
        else
        {
            return header_->GetInteriorNodeCount() + header_->GetLeafNodeCount();
        }
    }

    void IEncodedRtIp11Bvh::LoadBaseDataFromFile(rdf::Stream&              metadata_stream,
                                                 rdf::Stream&              bvh_stream,
                                                 const std::uint32_t       interior_node_buffer_size,
                                                 const BvhBundleReadOption import_option)
    {
        RRA_UNUSED(import_option);
        const auto compression_mode = ToDxrTriangleCompressionMode(header_->GetPostBuildInfo().GetTriangleCompressionMode());

        // Test current interior node buffer size against expected buffer size to determine if this BVH is
        // in compacted state.
        const auto actual_interior_node_buffer_size = header_->GetBufferOffsets().leaf_nodes - header_->GetBufferOffsets().interior_nodes;

        // Check if the interior node data has actually been compacted
        is_compacted_ = actual_interior_node_buffer_size == header_->CalculateInteriorNodeBufferSize();

#ifdef _DEBUG
        // Check if compaction is enabled if we want to load the entire data
        const bool compaction_allowed = IsFlagSet(header_->GetPostBuildInfo().GetBuildFlags(), BvhBuildFlags::kAllowCompaction);

        if (compaction_allowed && !is_compacted_)
        {
            const auto worst_case_interior_node_buffer_size = header_->CalculateWorstCaseInteriorNodeBufferSize();
            assert(worst_case_interior_node_buffer_size == actual_interior_node_buffer_size);
        }
#endif

        if (meta_data_.GetByteSize() > 0)
        {
            const auto     result                 = ComputeNodeBufferSizesFromBvhHeader(*header_, is_compacted_);
            const uint64_t interior_node_buf_size = std::get<0>(result);
            const uint64_t leaf_node_buf_size     = std::get<1>(result);

            parent_data_ = dxr::amd::ParentBlock(static_cast<uint32_t>(interior_node_buf_size), static_cast<uint32_t>(leaf_node_buf_size), compression_mode);
            auto* parent_data = parent_data_.GetLinkData().data();

            // If the streams are different, then it's the new format.
            // The parent data is at the end of the metadata.
            uint32_t parent_data_size = parent_data_.GetSizeInBytes();
            if (&metadata_stream != &bvh_stream)
            {
                if (parent_data_size > 0)
                {
                    metadata_stream.Seek(metadata_stream.GetSize() - parent_data_size);
                }
            }
            if (parent_data_size > 0)
            {
                metadata_stream.Read(parent_data_.GetSizeInBytes(), parent_data);
            }
        }

        interior_nodes_ = std::vector<std::uint8_t>(interior_node_buffer_size);
        bvh_stream.Read(interior_node_buffer_size, interior_nodes_.data());

        const size_t num_box_nodes = header_->GetInteriorNodeCount();

        box_surface_area_heuristic_.resize(num_box_nodes, 0);
    }

    void IEncodedRtIp11Bvh::SetRelativeReferences(const std::unordered_map<GpuVirtualAddress, std::uint64_t>& reference_map,
                                                  bool                                                        map_self,
                                                  std::unordered_set<GpuVirtualAddress>&                      missing_set)
    {
        RRA_UNUSED(map_self);
        RRA_UNUSED(missing_set);

        // Fix up the metadata address:
        GpuVirtualAddress address = this->GetVirtualAddress();
        const auto&       it      = reference_map.find(address);
        if (it != reference_map.end())
        {
            SetID(it->second);
        }
        else
        {
            RRA_ASSERT_MESSAGE(false, "Can't find address to index mapping");
        }
    }

    dxr::amd::AxisAlignedBoundingBox IEncodedRtIp11Bvh::ComputeRootNodeBoundingBox(const dxr::amd::Float32BoxNode* box_node) const
    {
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

    dxr::amd::NodePointer IEncodedRtIp11Bvh::GetParentNode(const dxr::amd::NodePointer* node_ptr) const
    {
        assert(!node_ptr->IsInvalid());

        const auto& parent_data       = GetParentData();
        const auto& parent_links      = parent_data.GetLinkData();
        const auto  compression_mode  = ToDxrTriangleCompressionMode(GetHeader().GetPostBuildInfo().GetTriangleCompressionMode());
        const auto  parent_link_index = node_ptr->CalculateParentLinkIndex(parent_data.GetSizeInBytes(), compression_mode);

        if (parent_link_index >= parent_data.GetLinkCount())
        {
            return {};
        }

        dxr::amd::NodePointer parent_node = parent_links[parent_link_index];

        return parent_node;
    }

    void IEncodedRtIp11Bvh::ScanTreeDepth()
    {
        size_t num_box_nodes = header_->GetInteriorNodeCount();
        if (num_box_nodes == 0)
        {
            return;
        }

        std::deque<std::pair<dxr::amd::NodePointer, std::uint32_t>> traversal_stack;

        const auto& interior_nodes = GetInteriorNodesData();

        // Top level node doesn't exist in the data so needs to be created. Assumed to be a Box32.
        dxr::amd::NodePointer root_ptr = dxr::amd::NodePointer(dxr::amd::NodeType::kAmdNodeBoxFp32, dxr::amd::kAccelerationStructureHeaderSize);

        // Assume there's a single root node.
        auto box_nodes_per_interior_node = 1;

        traversal_stack.push_back(std::make_pair(root_ptr, 0));

        const auto& header_offsets = header_->GetBufferOffsets();
        uint64_t    depth_sum      = 0;
        uint32_t    leaf_count     = 0;
        while (!traversal_stack.empty())
        {
            auto index_to_level = traversal_stack.front();
            auto node_ptr       = index_to_level.first;
            auto level          = index_to_level.second;

            max_tree_depth_ = std::max(max_tree_depth_, level + 1);

            traversal_stack.pop_front();

            for (auto count = 0; count < box_nodes_per_interior_node; count++)
            {
                auto byte_offset = node_ptr.GetByteOffset() - header_offsets.interior_nodes;
                if (node_ptr.IsFp32BoxNode())
                {
                    const auto node = reinterpret_cast<const dxr::amd::Float32BoxNode*>(&interior_nodes[byte_offset]);
                    for (const auto& ptr : node->GetChildren())
                    {
                        if (!ptr.IsInvalid())
                        {
                            traversal_stack.push_back(std::make_pair(ptr, level + 1));
                        }
                    }
                }
                else if (node_ptr.IsFp16BoxNode())
                {
                    const auto node = reinterpret_cast<const dxr::amd::Float16BoxNode*>(&interior_nodes[byte_offset]);
                    for (const auto& ptr : node->GetChildren())
                    {
                        if (!ptr.IsInvalid())
                        {
                            traversal_stack.push_back(std::make_pair(ptr, level + 1));
                        }
                    }
                }
                else if (node_ptr.IsTriangleNode())
                {
                    leaf_count++;
                    depth_sum += static_cast<uint64_t>(level) + 1;
                }
            }
        }
        if (leaf_count > 0)
        {
            depth_sum /= leaf_count;
        }
        avg_tree_depth_ = static_cast<uint32_t>(depth_sum);
    }

    float IEncodedRtIp11Bvh::GetInteriorNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr) const
    {
        const uint32_t index = (node_ptr.GetByteOffset() - GetHeader().GetBufferOffsets().interior_nodes) / sizeof(dxr::amd::Float32BoxNode);
        RRA_ASSERT(index < box_surface_area_heuristic_.size());
        return box_surface_area_heuristic_[index];
    }

    void IEncodedRtIp11Bvh::SetInteriorNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr, float surface_area_heuristic)
    {
        const uint32_t byte_offset   = node_ptr.GetByteOffset();
        const uint32_t header_offset = GetHeader().GetBufferOffsets().interior_nodes;
        const uint32_t index         = (byte_offset - header_offset) / sizeof(dxr::amd::Float32BoxNode);
        RRA_ASSERT(index < box_surface_area_heuristic_.size());
        box_surface_area_heuristic_[index] = surface_area_heuristic;
    }

    uint32_t IEncodedRtIp11Bvh::GetMaxTreeDepth() const
    {
        return max_tree_depth_;
    }

    uint32_t IEncodedRtIp11Bvh::GetAvgTreeDepth() const
    {
        return avg_tree_depth_;
    }

}  // namespace rta
