//=============================================================================
// Copyright (c) 2021-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  BVH base class implementations.
//=============================================================================

#include "bvh/ibvh.h"

#include <float.h>
#include <deque>

#include "public/rra_assert.h"
#include "public/rra_print.h"
#include "public/rra_rtip_info.h"

#include "bvh/dxr_type_conversion.h"
#include "bvh/flags_util.h"
#include "bvh/rtip31/child_info.h"
#include "bvh/rtip31/internal_node.h"
#include "bvh/rtip31/primitive_node.h"

namespace rta
{
    IBvh::IBvh()
    {
    }

    IBvh::~IBvh()
    {
    }

    void IBvh::ConvertBlasAddressesToIndices(const std::unordered_map<GpuVirtualAddress, std::uint64_t>& blas_map)
    {
        RRA_UNUSED(blas_map);
    }

    void IBvh::SetID(const std::uint64_t index_or_address)
    {
        id_ = index_or_address;
        meta_data_.SetGpuVa(index_or_address);
    }

    std::uint64_t IBvh::GetID() const
    {
        return id_;
    }

    BvhFormat IBvh::GetFormat() const
    {
        BvhFormat format = {};
        format.encoding  = RayTracingIpLevel::RtIp1_1;
        return format;
    }

    std::uint32_t IBvh::GetNodeCount(const BvhNodeFlags flag)
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

    std::uint64_t IBvh::GetBufferByteSize() const
    {
        return GetBufferByteSizeImpl(ExportOption::kDefault);
    }

    void IBvh::SetVirtualAddress(const std::uint64_t address)
    {
        gpu_virtual_address_ = address;
    }

    std::uint64_t IBvh::GetVirtualAddress() const
    {
        return gpu_virtual_address_;
    }

    void IBvh::SetRelativeReferences(const std::unordered_map<GpuVirtualAddress, std::uint64_t>& reference_map,
                                     bool                                                        map_self,
                                     std::unordered_set<GpuVirtualAddress>&                      missing_set)
    {
        RRA_UNUSED(map_self);
        RRA_UNUSED(missing_set);

        // Fix up the metadata address:
        GpuVirtualAddress address = this->GetVirtualAddress() + this->GetHeaderOffset();
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

    const IRtIpCommonAccelerationStructureHeader& IBvh::GetHeader() const
    {
        return *header_;
    }

    // Compute the node buffer sizes based on compaction and the header entries, such as triangle compression.
    static std::tuple<std::uint64_t, std::uint64_t> ComputeNodeBufferSizesFromBvhHeader(IRtIpCommonAccelerationStructureHeader& header,
                                                                                        const bool                              is_compacted,
                                                                                        const bool                              output_warnings = true)
    {
        RRA_UNUSED(is_compacted);
        RRA_UNUSED(output_warnings);

        const auto actual_interior_node_buffer_size =
            static_cast<std::uint64_t>(header.GetBufferOffsets().leaf_nodes - header.GetBufferOffsets().interior_nodes);
        const auto actual_leaf_node_buffer_size = header.CalculateActualLeafNodeBufferSize();

        return {actual_interior_node_buffer_size, actual_leaf_node_buffer_size};
    }

    void IBvh::LoadBaseDataFromFile(rdf::Stream&              metadata_stream,
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
            //assert(worst_case_interior_node_buffer_size == actual_interior_node_buffer_size);
            RRA_UNUSED(worst_case_interior_node_buffer_size);
        }
#endif

        rta::RayTracingIpLevel rtip_level{(rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel()};
        bool                   rtip_1_or_2{rtip_level == rta::RayTracingIpLevel::RtIp1_0 || rtip_level == rta::RayTracingIpLevel::RtIp1_1 ||
                         rtip_level == rta::RayTracingIpLevel::RtIp2_0 || rtip_level == rta::RayTracingIpLevel::RtIpNone};

        if (rtip_1_or_2 && meta_data_.GetByteSize() > 0)
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
                    // Put cursor at beginning of parent data. Parent data is at the end of metadata
                    // and has a size of leaf size + internal node size.
                    metadata_stream.Seek(metadata_stream.GetSize() - parent_data_size);
                }
            }
            if (parent_data_size > 0)
            {
                metadata_stream.Read(parent_data_.GetSizeInBytes(), parent_data);
            }
            const size_t num_box_nodes = header_->GetInteriorNodeCount();
            box_surface_area_heuristic_.resize(num_box_nodes, 0);
        }
        else
        {
            std::uint32_t struct_size = sizeof(dxr::amd::Float32BoxNode);
            box_surface_area_heuristic_.resize(interior_node_buffer_size / struct_size, 0);
        }

        interior_nodes_ = std::vector<std::uint8_t>(interior_node_buffer_size);
        bvh_stream.Read(interior_node_buffer_size, interior_nodes_.data());
    }

    uint32_t IBvh::ScanTreeDepth()
    {
        uint64_t depth_sum  = 0;
        uint32_t leaf_count = 0;

        size_t num_box_nodes = header_->GetInteriorNodeCount();
        if (num_box_nodes == 0)
        {
            return 0;
        }

        std::deque<std::pair<dxr::amd::NodePointer, std::uint32_t>> traversal_stack;

        const auto& interior_nodes = GetInteriorNodesData();

        // Top level node doesn't exist in the data so needs to be created. Assumed to be a Box32.
        dxr::amd::NodePointer root_ptr = dxr::amd::NodePointer(dxr::amd::NodeType::kAmdNodeBoxFp32, dxr::amd::kAccelerationStructureHeaderSize);

        // Assume there's a single root node.
        auto box_nodes_per_interior_node = 1;

        traversal_stack.push_back(std::make_pair(root_ptr, 0));

        const auto& header_offsets = header_->GetBufferOffsets();
        while (!traversal_stack.empty())
        {
            const auto& index_to_level = traversal_stack.back();
            auto        node_ptr       = index_to_level.first;
            auto        level          = index_to_level.second;

            max_tree_depth_ = std::max(max_tree_depth_, level + 1);

            traversal_stack.pop_back();

            for (auto count = 0; count < box_nodes_per_interior_node; count++)
            {
                // Get the byte offset relative to the internal node buffer.
                auto byte_offset = node_ptr.GetByteOffset() - header_offsets.interior_nodes;
                if (node_ptr.IsFp32BoxNode())
                {
                    if ((RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == RayTracingIpLevel::RtIp3_1)
                    {
                        // The quantized BVH8 node uses the same enum value as Fp32 Box node.
                        const auto            node = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[byte_offset]);
                        dxr::amd::NodePointer child_ptrs[8]{};
                        node->DecodeChildrenOffsets((uint32_t*)child_ptrs);

                        for (const auto& ptr : child_ptrs)
                        {
                            if (!ptr.IsInvalid())
                            {
                                traversal_stack.push_back(std::make_pair(ptr, level + 1));
                            }
                        }
                    }
                    else
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
        return leaf_count;
    }

    const std::vector<std::uint8_t>& IBvh::GetInteriorNodesData() const
    {
        return interior_nodes_;
    }

    std::vector<std::uint8_t>& IBvh::GetInteriorNodesData()
    {
        return interior_nodes_;
    }

    float IBvh::GetBoxSurfaceAreaHeuristic(uint32_t index) const
    {
        RRA_ASSERT(index < box_surface_area_heuristic_.size());
        return box_surface_area_heuristic_[index];
    }

    float IBvh::GetInteriorNodeSurfaceAreaHeuristic(uint32_t node_id) const
    {
        dxr::amd::NodePointer* node  = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);
        const uint32_t         index = (node->GetByteOffset() - GetHeader().GetBufferOffsets().interior_nodes) / sizeof(dxr::amd::Float32BoxNode);
        return GetBoxSurfaceAreaHeuristic(index);
    }

    bool IBvh::IsCompacted() const
    {
        return IsCompactedImpl();
    }

    bool IBvh::IsEmpty() const
    {
        return IsEmptyImpl();
    }

    uint64_t IBvh::GetInactiveInstanceCount() const
    {
        return GetInactiveInstanceCountImpl();
    }

    uint32_t IBvh::GetMaxTreeDepth() const
    {
        return max_tree_depth_;
    }

    uint32_t IBvh::GetAvgTreeDepth() const
    {
        return avg_tree_depth_;
    }

    void IBvh::SetInteriorNodeSurfaceAreaHeuristic(uint32_t node_id, float surface_area_heuristic)
    {
        dxr::amd::NodePointer* node          = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);
        const uint32_t         byte_offset   = node->GetByteOffset();
        const uint32_t         header_offset = GetHeader().GetBufferOffsets().interior_nodes;
        const uint32_t         index         = (byte_offset - header_offset) / sizeof(dxr::amd::Float32BoxNode);
        RRA_ASSERT(index < box_surface_area_heuristic_.size());
        box_surface_area_heuristic_[index] = surface_area_heuristic;
    }

    const dxr::amd::MetaDataV1& IBvh::GetMetaData() const
    {
        return meta_data_;
    }

    const dxr::amd::NodePointer* IBvh::GetPrimitiveNodePointer(int32_t index) const
    {
        return &primitive_node_ptrs_[index];
    }

    uint64_t IBvh::GetHeaderOffset() const
    {
        return header_offset_;
    }

    void IBvh::PreprocessParents()
    {
        // No-op by default.
    }

    bool IBvh::IsCompactedImpl() const
    {
        return is_compacted_;
    }

    bool IBvh::IsEmptyImpl() const
    {
        return header_->GetInteriorNodeCount() == 0;
    }

    uint64_t IBvh::GetInactiveInstanceCountImpl() const
    {
        return 0;
    }

}  // namespace rta

