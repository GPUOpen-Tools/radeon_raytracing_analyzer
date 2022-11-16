//=============================================================================
// Copyright (c) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 1.1 (Navi2x) specific top level acceleration structure
/// implementation.
//=============================================================================

#include "bvh/encoded_rt_ip_11_top_level_bvh.h"

#include <iostream>
#include <vector>
#include <cassert>
#include <deque>
#include <unordered_set>

#include "public/rra_assert.h"
#include "public/rra_blas.h"
#include "public/rra_error.h"

namespace rta
{

    EncodedRtIp11TopLevelBvh::~EncodedRtIp11TopLevelBvh()
    {
    }

    const std::vector<dxr::amd::InstanceNode>& EncodedRtIp11TopLevelBvh::GetInstanceNodes() const
    {
        return instance_nodes_;
    }

    bool EncodedRtIp11TopLevelBvh::HasBvhReferences() const
    {
        return true;
    }

    std::uint64_t EncodedRtIp11TopLevelBvh::GetBufferByteSizeImpl(const ExportOption export_option) const
    {
        auto file_size = header_->GetFileSize();
        if (export_option == ExportOption::kNoMetaData)
        {
            file_size -= meta_data_.GetByteSize();
        }
        auto min_file_size = kMinimumFileSize;
        return std::max(file_size, min_file_size);
    }

    bool EncodedRtIp11TopLevelBvh::LoadRawAccelStrucFromFile(rdf::ChunkFile&                     chunk_file,
                                                             const std::uint64_t                 chunk_index,
                                                             const RawAccelStructRdfChunkHeader& chunk_header,
                                                             const char*                         chunk_identifier,
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

        auto rt_ip11_header = CreateRtIp11AccelerationStructureHeader();
        rt_ip11_header->LoadFromBuffer(dxr::amd::kAccelerationStructureHeaderSize, buffer.data() + chunk_header.header_offset);

        instance_nodes_ = std::vector<dxr::amd::InstanceNode>(rt_ip11_header->GetPrimitiveCount());
        buffer_stream.Read(dxr::amd::kInstanceNodeSize * instance_nodes_.size(), instance_nodes_.data());

        const auto prim_node_ptr_size = instance_nodes_.size() * sizeof(dxr::amd::NodePointer);
        primitive_node_ptrs_.resize(instance_nodes_.size());
        buffer_stream.Read(prim_node_ptr_size, primitive_node_ptrs_.data());

        buffer_stream.Close();

        return true;
    }

    bool EncodedRtIp11TopLevelBvh::PostLoad()
    {
        bool result = BuildInstanceList();
        ScanTreeDepth();
        instance_surface_area_heuristic_.resize(instance_nodes_.size(), 0);
        return result;
    }

    void EncodedRtIp11TopLevelBvh::SetRelativeReferences(const std::unordered_map<GpuVirtualAddress, std::uint64_t>& reference_map,
                                                         bool                                                        map_self,
                                                         std::unordered_set<GpuVirtualAddress>&                      missing_set)
    {
        if (map_self)
        {
            IEncodedRtIp11Bvh::SetRelativeReferences(reference_map, map_self, missing_set);
        }
        else
        {
            // Fix up the instance node addresses.
            for (auto& instance_node : instance_nodes_)
            {
                if (instance_node.IsInactive())
                {
                    continue;
                }

                uint64_t                address        = instance_node.GetDesc().GetBottomLevelBvhGpuVa(dxr::InstanceDescType::kRaw);
                uint32_t                meta_data_size = instance_node.GetExtraData().GetBottomLevelBvhMetaDataSize();
                const GpuVirtualAddress old_reference  = address - meta_data_size;

                const auto it = reference_map.find(old_reference);
                if (it != reference_map.end())
                {
                    const auto new_relative_reference = it->second;
                    instance_node.GetDesc().SetBottomLevelBvhGpuVa(new_relative_reference << 3, dxr::InstanceDescType::kRaw);
                }
                else
                {
                    missing_set.insert(old_reference);
                    const auto new_relative_reference = 0;
                    instance_node.GetDesc().SetBottomLevelBvhGpuVa(new_relative_reference << 3, dxr::InstanceDescType::kRaw);
                }
            }
        }
    }

    bool EncodedRtIp11TopLevelBvh::BuildInstanceList()
    {
        if (IsEmpty())
        {
            // An empty TLAS should be OK; it just won't be shown in the UI.
            return true;
        }

        std::deque<std::pair<dxr::amd::NodePointer, std::uint32_t>> traversal_stack;

        const auto& interior_nodes = GetInteriorNodesData();

        // Top level node doesn't exist in the data so needs to be created. Assumed to be a Box32.
        dxr::amd::NodePointer root_ptr                 = dxr::amd::NodePointer(dxr::amd::NodeType::kAmdNodeBoxFp32, dxr::amd::kAccelerationStructureHeaderSize);
        uint64_t              num_traversal_node_count = 0;

        // Assume there's a single root node.
        auto box_nodes_per_interior_node = 1;

        traversal_stack.push_back(std::make_pair(root_ptr, 0));

        const auto& header_offsets = header_->GetBufferOffsets();
        while (!traversal_stack.empty())
        {
            auto front    = traversal_stack.front();
            auto node_ptr = front.first;
            auto level    = front.second;

            traversal_stack.pop_front();

            if (node_ptr.IsInstanceNode())
            {
                auto byte_offset = node_ptr.GetByteOffset() - header_offsets.leaf_nodes;

                uint32_t instance_node_size = sizeof(dxr::amd::InstanceNode);
                uint32_t instance_index     = byte_offset / instance_node_size;

                const auto& instance_nodes = GetInstanceNodes();
                if (instance_index < instance_nodes.size())
                {
                    const dxr::amd::InstanceNode* instance_node = &instance_nodes[instance_index];
                    const auto&                   desc          = instance_node->GetDesc();

                    uint64_t              blas_index = desc.GetBottomLevelBvhGpuVa(dxr::InstanceDescType::kRaw) >> 3;
                    uint32_t              address    = (instance_index * sizeof(dxr::amd::InstanceNode)) + header_offsets.leaf_nodes;
                    dxr::amd::NodePointer new_node   = dxr::amd::NodePointer(dxr::amd::NodeType::kAmdNodeInstance, address);

                    if (instance_list_.find(blas_index) == instance_list_.end())
                    {
                        std::vector<dxr::amd::NodePointer> node_list = {new_node};
                        instance_list_.insert(std::make_pair(blas_index, node_list));
                    }
                    else
                    {
                        instance_list_[blas_index].push_back(new_node);
                    }
                    num_traversal_node_count++;
                }
                else
                {
                    RRA_ASSERT_MESSAGE(false, "Instance pointer out of range");
                }
            }

            for (auto count = 0; count < box_nodes_per_interior_node; count++)
            {
                if (node_ptr.IsBoxNode())
                {
                    num_traversal_node_count++;
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
                }
            }
        }

        if (num_traversal_node_count > GetNodeCount(BvhNodeFlags::kNone))
        {
            return false;
        }
        return true;
    }

    uint64_t EncodedRtIp11TopLevelBvh::GetInactiveInstanceCountImpl() const
    {
        uint64_t inactive_count{0};
        for (auto& instance_node : instance_nodes_)
        {
            if (instance_node.IsInactive())
            {
                ++inactive_count;
            }
        }
        return inactive_count;
    }

    uint64_t EncodedRtIp11TopLevelBvh::GetBlasCount(bool empty_placeholder) const
    {
        auto size = instance_list_.size();
        if (empty_placeholder && size)
        {
            // If there are instances referencing the missing blas index, ignore it as a valid BLAS.
            uint64_t missing_blas_index = 0;
            auto     iter               = instance_list_.find(missing_blas_index);
            if (iter != instance_list_.end())
            {
                return size - 1;
            }
        }
        return size;
    }

    uint64_t EncodedRtIp11TopLevelBvh::GetReferencedBlasMemorySize() const
    {
        uint64_t total_memory = 0;

        for (const auto& it : instance_list_)
        {
            uint32_t     blas_memory = 0;
            RraErrorCode status      = RraBlasGetSizeInBytes(it.first, &blas_memory);
            RRA_ASSERT(status == kRraOk);
            if (status == kRraOk)
            {
                total_memory += blas_memory;
            }
        }
        return total_memory;
    }

    uint64_t EncodedRtIp11TopLevelBvh::GetTotalTriangleCount() const
    {
        uint64_t triangle_count = 0;
        for (const auto& it : instance_list_)
        {
            uint32_t     blas_triangles = 0;
            RraErrorCode status         = RraBlasGetUniqueTriangleCount(it.first, &blas_triangles);
            RRA_ASSERT(status == kRraOk);
            if (status == kRraOk)
            {
                triangle_count += static_cast<uint64_t>(blas_triangles) * it.second.size();
            }
        }
        return triangle_count;
    }

    uint64_t EncodedRtIp11TopLevelBvh::GetUniqueTriangleCount() const
    {
        uint64_t triangle_count = 0;
        for (const auto& it : instance_list_)
        {
            uint32_t     blas_triangles = 0;
            RraErrorCode status         = RraBlasGetUniqueTriangleCount(it.first, &blas_triangles);
            RRA_ASSERT(status == kRraOk);
            if (status == kRraOk)
            {
                triangle_count += static_cast<uint64_t>(blas_triangles);
            }
        }
        return triangle_count;
    }

    uint64_t EncodedRtIp11TopLevelBvh::GetInstanceCount(uint64_t index) const
    {
        auto iter = instance_list_.find(index);
        if (iter != instance_list_.end())
        {
            return iter->second.size();
        }
        return 0;
    }

    dxr::amd::NodePointer EncodedRtIp11TopLevelBvh::GetInstanceNode(uint64_t blas_index, uint64_t instance_index) const
    {
        size_t num_instances = 0;
        auto   iter          = instance_list_.find(blas_index);
        if (iter != instance_list_.end())
        {
            num_instances = iter->second.size();
            if (instance_index < num_instances)
            {
                return iter->second[instance_index];
            }
        }
        return dxr::amd::kInvalidNode;
    }

    float EncodedRtIp11TopLevelBvh::GetLeafNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr) const
    {
        const uint32_t byte_offset = node_ptr.GetByteOffset();
        const uint32_t leaf_nodes  = GetHeader().GetBufferOffsets().leaf_nodes;
        const uint32_t index       = (byte_offset - leaf_nodes) / sizeof(dxr::amd::InstanceNode);
        assert(index < instance_nodes_.size());
        assert(index < instance_surface_area_heuristic_.size());
        return instance_surface_area_heuristic_[index];
    }

    void EncodedRtIp11TopLevelBvh::SetLeafNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr, float surface_area_heuristic)
    {
        const uint32_t byte_offset = node_ptr.GetByteOffset();
        const uint32_t leaf_nodes  = GetHeader().GetBufferOffsets().leaf_nodes;
        const uint32_t index       = (byte_offset - leaf_nodes) / sizeof(dxr::amd::InstanceNode);
        assert(index < instance_nodes_.size());
        assert(index < instance_surface_area_heuristic_.size());
        instance_surface_area_heuristic_[index] = surface_area_heuristic;
    }

}  // namespace rta
