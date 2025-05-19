//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
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
        uint64_t    depth_sum      = 0;
        uint32_t    leaf_count     = 0;
        while (!traversal_stack.empty())
        {
            const auto& index_to_level = traversal_stack.front();
            auto        node_ptr       = index_to_level.first;
            auto        level          = index_to_level.second;

            max_tree_depth_ = std::max(max_tree_depth_, level + 1);

            traversal_stack.pop_front();

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

    float IBvh::GetInteriorNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr) const
    {
        const uint32_t index = (node_ptr.GetByteOffset() - GetHeader().GetBufferOffsets().interior_nodes) / sizeof(dxr::amd::Float32BoxNode);
        RRA_ASSERT(index < box_surface_area_heuristic_.size());
        return box_surface_area_heuristic_[index];
    }

    dxr::amd::AxisAlignedBoundingBox IBvh::ComputeRootNodeBoundingBox(const dxr::amd::Float32BoxNode* box_node) const
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

    dxr::amd::AxisAlignedBoundingBox IBvh::ComputeRootNodeBoundingBox(const QuantizedBVH8BoxNode* box_node) const
    {
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

    uint32_t IBvh::GetNodeObbIndex(const dxr::amd::NodePointer node_ptr) const
    {
        assert(node_ptr.IsFp32BoxNode());

        if (interior_nodes_.empty() || !RraRtipInfoGetOBBSupported())
        {
            return ObbDisabled;
        }

        auto                 byte_offset = node_ptr.GetByteOffset() - header_->GetBufferOffsets().interior_nodes;
        QuantizedBVH8BoxNode node        = *reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes_[byte_offset]);

        return node.OBBMatrixIndex();
    }

    static glm::mat3 DecodeRotationMatrix(uint32_t id)
    {
        constexpr uint32_t NUM_OBB_TRANSFORMS{104};
        assert(id < NUM_OBB_TRANSFORMS);

        // The stage 1 lookup table containing the indices of the floats constituting the rotation
        // matrices.
        // Each value is 6-bits, with the lower 5 bits indicating the float and highest bit
        // indicating the sign of the float.
        static const uint32_t s1_lut[NUM_OBB_TRANSFORMS][9] = {
            {25, 0, 0, 0, 22, 43, 0, 11, 22},     {25, 0, 0, 0, 22, 11, 0, 43, 22},     {25, 0, 0, 0, 17, 49, 0, 17, 17},
            {25, 0, 0, 0, 17, 17, 0, 49, 17},     {25, 0, 0, 0, 11, 54, 0, 22, 11},     {25, 0, 0, 0, 11, 22, 0, 54, 11},
            {25, 0, 0, 0, 0, 57, 0, 25, 0},       {25, 0, 0, 0, 0, 25, 0, 57, 0},       {22, 0, 11, 0, 25, 0, 43, 0, 22},
            {22, 0, 43, 0, 25, 0, 11, 0, 22},     {17, 0, 17, 0, 25, 0, 49, 0, 17},     {17, 0, 49, 0, 25, 0, 17, 0, 17},
            {11, 0, 22, 0, 25, 0, 54, 0, 11},     {11, 0, 54, 0, 25, 0, 22, 0, 11},     {0, 0, 25, 0, 25, 0, 57, 0, 0},
            {0, 0, 57, 0, 25, 0, 25, 0, 0},       {22, 43, 0, 11, 22, 0, 0, 0, 25},     {22, 11, 0, 43, 22, 0, 0, 0, 25},
            {17, 49, 0, 17, 17, 0, 0, 0, 25},     {17, 17, 0, 49, 17, 0, 0, 0, 25},     {11, 54, 0, 22, 11, 0, 0, 0, 25},
            {11, 22, 0, 54, 11, 0, 0, 0, 25},     {0, 57, 0, 25, 0, 0, 0, 0, 25},       {0, 25, 0, 57, 0, 0, 0, 0, 25},
            {22, 38, 6, 6, 24, 1, 38, 1, 24},     {22, 6, 38, 38, 24, 1, 6, 1, 24},     {17, 44, 12, 12, 20, 2, 44, 2, 20},
            {17, 12, 44, 44, 20, 2, 12, 2, 20},   {11, 47, 15, 15, 16, 7, 47, 7, 16},   {11, 15, 47, 47, 16, 7, 15, 7, 16},
            {0, 49, 17, 17, 12, 12, 49, 12, 12},  {0, 17, 49, 49, 12, 12, 17, 12, 12},  {22, 38, 38, 6, 24, 33, 6, 33, 24},
            {22, 6, 6, 38, 24, 33, 38, 33, 24},   {17, 44, 44, 12, 20, 34, 12, 34, 20}, {17, 12, 12, 44, 20, 34, 44, 34, 20},
            {11, 47, 47, 15, 16, 39, 15, 39, 16}, {11, 15, 15, 47, 16, 39, 47, 39, 16}, {0, 49, 49, 17, 12, 44, 17, 44, 12},
            {0, 17, 17, 49, 12, 44, 49, 44, 12},  {24, 38, 1, 6, 22, 38, 1, 6, 24},     {24, 6, 1, 38, 22, 6, 1, 38, 24},
            {20, 44, 2, 12, 17, 44, 2, 12, 20},   {20, 12, 2, 44, 17, 12, 2, 44, 20},   {16, 47, 7, 15, 11, 47, 7, 15, 16},
            {16, 15, 7, 47, 11, 15, 7, 47, 16},   {12, 49, 12, 17, 0, 49, 12, 17, 12},  {12, 17, 12, 49, 0, 17, 12, 49, 12},
            {24, 6, 33, 38, 22, 38, 33, 6, 24},   {24, 38, 33, 6, 22, 6, 33, 38, 24},   {20, 12, 34, 44, 17, 44, 34, 12, 20},
            {20, 44, 34, 12, 17, 12, 34, 44, 20}, {16, 15, 39, 47, 11, 47, 39, 15, 16}, {16, 47, 39, 15, 11, 15, 39, 47, 16},
            {12, 17, 44, 49, 0, 49, 44, 17, 12},  {12, 49, 44, 17, 0, 17, 44, 49, 12},  {24, 1, 6, 1, 24, 38, 38, 6, 22},
            {24, 1, 38, 1, 24, 6, 6, 38, 22},     {20, 2, 12, 2, 20, 44, 44, 12, 17},   {20, 2, 44, 2, 20, 12, 12, 44, 17},
            {16, 7, 15, 7, 16, 47, 47, 15, 11},   {16, 7, 47, 7, 16, 15, 15, 47, 11},   {12, 12, 17, 12, 12, 49, 49, 17, 0},
            {12, 12, 49, 12, 12, 17, 17, 49, 0},  {24, 33, 6, 33, 24, 6, 38, 38, 22},   {24, 33, 38, 33, 24, 38, 6, 6, 22},
            {20, 34, 12, 34, 20, 12, 44, 44, 17}, {20, 34, 44, 34, 20, 44, 12, 12, 17}, {16, 39, 15, 39, 16, 15, 47, 47, 11},
            {16, 39, 47, 39, 16, 47, 15, 15, 11}, {12, 44, 17, 44, 12, 17, 49, 49, 0},  {12, 44, 49, 44, 12, 49, 17, 17, 0},
            {23, 35, 5, 5, 23, 35, 35, 5, 23},    {23, 5, 35, 35, 23, 5, 5, 35, 23},    {19, 40, 13, 13, 19, 40, 40, 13, 19},
            {19, 13, 40, 40, 19, 13, 13, 40, 19}, {14, 41, 18, 18, 14, 41, 41, 18, 14}, {14, 18, 41, 41, 14, 18, 18, 41, 14},
            {10, 36, 21, 21, 10, 36, 36, 21, 10}, {10, 21, 36, 36, 10, 21, 21, 36, 10}, {23, 37, 3, 3, 23, 5, 37, 35, 23},
            {23, 3, 37, 37, 23, 35, 3, 5, 23},    {19, 45, 8, 8, 19, 13, 45, 40, 19},   {19, 8, 45, 45, 19, 40, 8, 13, 19},
            {14, 50, 9, 9, 14, 18, 50, 41, 14},   {14, 9, 50, 50, 14, 41, 9, 18, 14},   {10, 53, 4, 4, 10, 21, 53, 36, 10},
            {10, 4, 53, 53, 10, 36, 4, 21, 10},   {23, 37, 35, 3, 23, 37, 5, 3, 23},    {23, 3, 5, 37, 23, 3, 35, 37, 23},
            {19, 45, 40, 8, 19, 45, 13, 8, 19},   {19, 8, 13, 45, 19, 8, 40, 45, 19},   {14, 50, 41, 9, 14, 50, 18, 9, 14},
            {14, 9, 18, 50, 14, 9, 41, 50, 14},   {10, 53, 36, 4, 10, 53, 21, 4, 10},   {10, 4, 21, 53, 10, 4, 36, 53, 10},
            {23, 35, 37, 5, 23, 3, 3, 37, 23},    {23, 5, 3, 35, 23, 37, 37, 3, 23},    {19, 40, 45, 13, 19, 8, 8, 45, 19},
            {19, 13, 8, 40, 19, 45, 45, 8, 19},   {14, 41, 50, 18, 14, 9, 9, 50, 14},   {14, 18, 9, 41, 14, 50, 50, 9, 14},
            {10, 36, 53, 21, 10, 4, 4, 53, 10},   {10, 21, 4, 36, 10, 53, 53, 4, 10}};
        // The stage 2 lookup table containing the floating point data.
        // Each value is 30-bits as the sign bit and highest exponent bit are unused.
        static const uint32_t s2_lut[] = {
            0x00000000, 0x3d1be50c, 0x3e15f61a, 0x3e484336, 0x3e79df93, 0x3e7c3a3a, 0x3e8a8bd4, 0x3e9e0875, 0x3e9f0938,
            0x3ea7bf1b, 0x3eaaaaab, 0x3ec3ef15, 0x3f000000, 0x3f01814f, 0x3f16a507, 0x3f273d75, 0x3f30fbc5, 0x3f3504f3,
            0x3f3d3a87, 0x3f4e034d, 0x3f5a827a, 0x3f692290, 0x3f6c835e, 0x3f73023f, 0x3f7641af, 0x3f800000,
        };

        glm::mat3 out = {};
        uint32_t  float_id;

        // Loop over all 9 values in the relevant S1 LUT entry, using the lower 5-bits to index
        // the float from the S2 LUT and the highest bit to insert the sign bit.
        for (uint32_t j = 0; j < 3; ++j)
        {
            for (uint32_t i = 0; i < 3; ++i)
            {
                float_id   = s1_lut[id][(j * 3) + i];
                uint32_t u = s2_lut[float_id & 0x1f] | ((float_id >> 5) ? 0x80000000 : 0);
                std::memcpy(&out[j][i], &u, sizeof(u));
            }
        }

        return out;
    }

    glm::mat3 IBvh::GetNodeBoundingVolumeOrientation(const dxr::amd::NodePointer node_ptr) const
    {
        assert(node_ptr.IsFp32BoxNode());

        // Empty BLAS will be given identity rotation.
        if (interior_nodes_.empty() || !RraRtipInfoGetOBBSupported())
        {
            return glm::mat3(1.0f);
        }

        auto                 byte_offset = node_ptr.GetByteOffset() - header_->GetBufferOffsets().interior_nodes;
        QuantizedBVH8BoxNode node        = *reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes_[byte_offset]);

        uint32_t obb_index = node.OBBMatrixIndex();
        return obb_index >= ObbDisabled ? glm::mat3(1.0f) : DecodeRotationMatrix(obb_index);
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

    void IBvh::SetInteriorNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr, float surface_area_heuristic)
    {
        const uint32_t byte_offset   = node_ptr.GetByteOffset();
        const uint32_t header_offset = GetHeader().GetBufferOffsets().interior_nodes;
        const uint32_t index         = (byte_offset - header_offset) / sizeof(dxr::amd::Float32BoxNode);
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

