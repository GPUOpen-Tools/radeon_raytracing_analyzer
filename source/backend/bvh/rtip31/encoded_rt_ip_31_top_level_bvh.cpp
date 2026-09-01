//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 3.1 (Navi4x) specific top level acceleration structure
/// implementation.
//=============================================================================

#include "bvh/rtip31/encoded_rt_ip_31_top_level_bvh.h"

#include <cassert>
#include <deque>
#include <iostream>
#include <unordered_set>
#include <vector>

#include "public/rra_assert.h"
#include "public/rra_blas.h"
#include "public/rra_error.h"

#include "bvh/rtip31/common.h"
#include "bvh/rtip31/internal_node.h"
#include "bvh/rtip31/primitive_node.h"
#include "bvh/rtip31/rt_ip_31_acceleration_structure_header.h"

namespace rta
{
    EncodedRtIp31TopLevelBvh::EncodedRtIp31TopLevelBvh()
    {
        header_ = std::make_unique<DxrRtIp31AccelerationStructureHeader>();
    }

    EncodedRtIp31TopLevelBvh::~EncodedRtIp31TopLevelBvh()
    {
    }

    static bool InstanceIsInactive(const InstanceNodeDataRRA* instance_node)
    {
        return instance_node->hw_instance_node.data.childBasePtr == 0 || instance_node->sideband.blasMetadataSize == 0 ||
               (instance_node->hw_instance_node.data.userDataAndInstanceMask >> 24) == 0;
    }

    void EncodedRtIp31TopLevelBvh::SetRelativeReferences(const std::unordered_map<GpuVirtualAddress, std::uint64_t>& reference_map,
                                                         bool                                                        map_self,
                                                         std::unordered_set<GpuVirtualAddress>&                      missing_set)
    {
        if (map_self)
        {
            IBvh::SetRelativeReferences(reference_map, map_self, missing_set);
        }
    }

    RraErrorCode EncodedRtIp31TopLevelBvh::BlasAddressToIndex(GpuVirtualAddress address, uint64_t* out_index) const
    {
        if (out_index == nullptr)
        {
            return kRraErrorInvalidPointer;
        }

        const auto it = blas_map_.find(address);
        if (it == blas_map_.end())
        {
            return kRraErrorIndexOutOfRange;
        }
        *out_index = it->second;

        return kRraOk;
    }

    RraErrorCode EncodedRtIp31TopLevelBvh::ResolveInstanceBlasAddress(GpuVirtualAddress blas_address, uint64_t blas_metadata_size, uint64_t* out_index) const
    {
        // Each BLAS is registered in blas_map_ under two keys spaced by its metadata size: its base
        // (GetVirtualAddress) and its traversal root (GetVirtualAddressForTraversal == base + metadata); see
        // bvh_bundle.cpp ~330-331. An instance's childBasePtr points at (or near) the traversal root, but the exact
        // offset is GPURT-version-dependent: older captures point straight at a registered key, newer ones sit one
        // or two metadata pages higher. A bare exact match is therefore unsafe: because BLASes are packed
        // contiguously, an instance's offset childBasePtr can numerically equal an *adjacent* BLAS's registered base
        // key, silently resolving to the wrong neighbour (observed on a packing capture: a blas-32 instance whose
        // childBasePtr == base32 + 2*meta32 collided with blas-33's base key and mis-loaded blas-33's geometry).
        //
        // Disambiguate with the instance's own metadata size: the correct BLAS is the one whose {base, base+meta}
        // pair is spaced by exactly this instance's metadata size. A collided neighbour has a different metadata
        // size, so its pair does not line up and it is rejected. Probe the instance address and successively lower
        // metadata-page offsets, accepting the first candidate whose pair validates.
        if (blas_metadata_size != 0)
        {
            const auto pair_matches = [&](GpuVirtualAddress key, uint64_t index) -> bool {
                // key is correct if it is one half of index's {base, base+meta} pair, spaced by this metadata size.
                const auto upper = blas_map_.find(key + blas_metadata_size);
                if (upper != blas_map_.end() && upper->second == index)
                {
                    return true;  // key is the base; base+meta is the traversal key of the same BLAS.
                }
                const auto lower = blas_map_.find(key - blas_metadata_size);
                return lower != blas_map_.end() && lower->second == index;  // key is the traversal key; key-meta is the base.
            };

            for (uint64_t pages = 0; pages <= 2; ++pages)
            {
                const GpuVirtualAddress key = blas_address - pages * blas_metadata_size;
                const auto              it  = blas_map_.find(key);
                if (it != blas_map_.end() && pair_matches(key, it->second))
                {
                    *out_index = it->second;
                    return kRraOk;
                }
            }
        }

        // Fallback: original version-tolerant behaviour (exact, then one page lower). Preserves resolution for
        // captures whose BLASes are not registered as validated {base, base+meta} pairs (e.g. the single-key null
        // BLAS at index 0), so the disambiguation above only ever *adds* precision and cannot regress traces that
        // already resolved.
        if (BlasAddressToIndex(blas_address, out_index) == kRraOk)
        {
            return kRraOk;
        }
        if (blas_metadata_size != 0)
        {
            return BlasAddressToIndex(blas_address - blas_metadata_size, out_index);
        }
        return kRraErrorIndexOutOfRange;
    }

    uint32_t EncodedRtIp31TopLevelBvh::GetParentNode(uint32_t node_id, uint32_t global_child_index) const
    {
        RRA_UNUSED(global_child_index);
        dxr::amd::NodePointer node_ptr(node_id);
        if (node_ptr.IsLeafNode())
        {
            std::optional<InstanceNodeDataRRA> hw_instance_node = GetHwInstanceNode(&node_ptr);

            if (hw_instance_node.has_value())
            {
                return hw_instance_node.value().hw_instance_node.data.childRootNodeOrParentPtr;
            }
        }
        else
        {
            const auto& interior_nodes = GetInteriorNodesData();
            const auto& header_offsets = GetHeader().GetBufferOffsets();
            uint32_t    byte_offset    = node_ptr.GetByteOffset() - header_offsets.interior_nodes;
            const auto  node           = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[byte_offset]);
            return node->parentPointer;
        }

        assert(false);
        return {};
    }

    std::int32_t EncodedRtIp31TopLevelBvh::GetInstanceNodeSize() const
    {
        if (GetHeader().GetPostBuildInfo().GetFusedInstances() == true)
        {
            return dxr::amd::kFusedInstanceNodeSize;
        }
        else
        {
            return dxr::amd::kInstanceNodeSize;
        }
    }

    std::optional<InstanceNodeDataRRA> EncodedRtIp31TopLevelBvh::GetHwInstanceNode(const dxr::amd::NodePointer* node_ptr) const
    {
        const auto& header_offsets  = GetHeader().GetBufferOffsets();
        uint32_t    instance_offset = node_ptr->GetByteOffset();
        instance_offset -= header_offsets.interior_nodes;

        // Validate the whole node (128 bytes), not just its first byte: the new-layout path below reads dwords at
        // inline offsets and both paths construct a full HwInstanceNodeRRA, so a truncated capture must be rejected
        // before the reinterpret. Written as a subtraction to avoid instance_offset + sizeof overflow.
        if (interior_nodes_.size() < sizeof(HwInstanceNodeRRA) || instance_offset > interior_nodes_.size() - sizeof(HwInstanceNodeRRA))
        {
            return std::nullopt;
        }

        HwInstanceNodeRRA hw_instance_node = *reinterpret_cast<const HwInstanceNodeRRA*>(&interior_nodes_[instance_offset]);

        if (GetHeader().GetRawHeader().UsesLegacySidebandLayout())
        {
            uint32_t sideband_offset =
                ComputeInstanceSidebandOffset(header_offsets.interior_nodes + (uint32_t)instance_offset, header_offsets.leaf_nodes, header_offsets.geometry_info);
            sideband_offset -= header_offsets.geometry_info;  // Make relative to sideband_data_.

            if (sideband_offset >= sideband_data_.size())
            {
                return std::nullopt;
            }

            InstanceSidebandData sideband = *reinterpret_cast<const InstanceSidebandData*>(&sideband_data_[sideband_offset]);
            return InstanceNodeDataRRA{hw_instance_node, sideband};
        }

        // New layout (v16.12+): instance index, id, flags and BLAS metadata size live inline in the node; the
        // sideband is indexed by instance index and only holds the transform, translation and partition index.
        // Translate both into the canonical RRA InstanceSidebandData so downstream consumers are layout-agnostic.
        const uint8_t* node_bytes = &interior_nodes_[instance_offset];
        uint32_t       instance_id_and_flags;
        uint32_t       index_and_metadata_page;
        memcpy(&instance_id_and_flags, node_bytes + RTIP3_1_INSTANCE_NODE_INSTANCE_ID_AND_FLAGS_OFFSET, sizeof(uint32_t));
        memcpy(&index_and_metadata_page, node_bytes + RTIP3_1_INSTANCE_NODE_INDEX_AND_METADATA_PAGE_OFFSET, sizeof(uint32_t));

        const uint32_t instance_index = index_and_metadata_page & 0x00FFFFFFu;

        uint32_t sideband_offset = ComputeInstanceSidebandOffsetFromIndex(instance_index, header_offsets.geometry_info);
        sideband_offset -= header_offsets.geometry_info;  // Make relative to sideband_data_.

        if (sideband_offset + sizeof(InstanceSidebandDataNew) > sideband_data_.size())
        {
            return std::nullopt;
        }

        const InstanceSidebandDataNew* new_sideband = reinterpret_cast<const InstanceSidebandDataNew*>(&sideband_data_[sideband_offset]);

        InstanceSidebandData sideband{};
        sideband.instanceIndex      = instance_index;
        sideband.instanceIdAndFlags = instance_id_and_flags;
        sideband.blasMetadataSize   = (index_and_metadata_page >> 24) << RTIP3_1_INSTANCE_NODE_METADATA_PAGE_SIZE_SHIFT;
        sideband.padding0           = new_sideband->partitionIndex;
        memcpy(sideband.objectToWorld, new_sideband->objectToWorld, dxr::kMatrix3x4Size);

        return InstanceNodeDataRRA{hw_instance_node, sideband};
    }

    int32_t EncodedRtIp31TopLevelBvh::GetInstanceIndex(uint32_t node_id) const
    {
        dxr::amd::NodePointer node_ptr(node_id);
        const auto&           header_offsets = GetHeader().GetBufferOffsets();
        uint32_t              byte_offset    = node_ptr.GetByteOffset();
        byte_offset -= header_offsets.interior_nodes;

        if (byte_offset >= interior_nodes_.size())
        {
            return -1;
        }

        uint32_t instance_node_size = GetInstanceNodeSize();
        return byte_offset / instance_node_size;
    }

    bool EncodedRtIp31TopLevelBvh::HasBvhReferences() const
    {
        return true;
    }

    std::uint64_t EncodedRtIp31TopLevelBvh::GetBufferByteSizeImpl(const ExportOption export_option) const
    {
        auto file_size = header_->GetFileSize();
        if (export_option == ExportOption::kNoMetaData)
        {
            file_size -= meta_data_.GetByteSize();
        }
        auto min_file_size = kMinimumFileSize;
        return std::max(file_size, min_file_size);
    }

    bool EncodedRtIp31TopLevelBvh::LoadRawAccelStrucFromFile(rdf::ChunkFile&                     chunk_file,
                                                             const std::uint64_t                 chunk_index,
                                                             const RawAccelStructRdfChunkHeader& chunk_header,
                                                             const char*                         chunk_identifier,
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

        header_->LoadFromBuffer(dxr::amd::kAccelerationStructureHeaderSize, buffer.data() + chunk_header.header_offset);

        if (!header_->IsValid())
        {
            return false;
        }

        uint64_t address =
            ((static_cast<std::uint64_t>(chunk_header.accel_struct_base_va_hi) << 32) | chunk_header.accel_struct_base_va_lo) + chunk_header.header_offset;
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

        uint32_t sideband_data_size = header_->GetBufferOffsets().prim_node_ptrs - header_->GetBufferOffsets().geometry_info;
        sideband_data_              = std::vector<std::uint8_t>(sideband_data_size);
        buffer_stream.Read(sideband_data_size, sideband_data_.data());

        auto rtip_header = std::make_unique<DxrRtIp31AccelerationStructureHeader>();
        rtip_header->LoadFromBuffer(dxr::amd::kAccelerationStructureHeaderSize, buffer.data() + chunk_header.header_offset);

        int32_t num_instance_nodes = rtip_header->GetPrimitiveCount();

        const auto prim_node_ptr_size = num_instance_nodes * sizeof(dxr::amd::NodePointer);
        primitive_node_ptrs_.resize(num_instance_nodes);
        buffer_stream.Read(prim_node_ptr_size, primitive_node_ptrs_.data());

        buffer_stream.Close();

        // Set the root node offset.
        header_offset_ = static_cast<uint64_t>(chunk_header.header_offset);

        return true;
    }

    bool EncodedRtIp31TopLevelBvh::PostLoad()
    {
        bool result = BuildInstanceList();
        ScanTreeDepth();
        instance_surface_area_heuristic_.resize(header_->GetPrimitiveCount(), 0);
        ParsePartitionData();
        return result;
    }

    void EncodedRtIp31TopLevelBvh::ConvertBlasAddressesToIndices(const std::unordered_map<GpuVirtualAddress, uint64_t>& blas_map)
    {
        blas_map_               = blas_map;
        auto instance_list_copy = instance_list_;

        for (const auto& pair : instance_list_copy)
        {
            auto     extracted  = instance_list_.extract(pair.first);
            uint64_t blas_index = 0;
            // An instance encodes the BLAS root-node address; blas_map registers that address as a key
            // (see bvh_bundle.cpp). Resolve with the version-tolerant fallback: exact first, then one metadata
            // page lower for newer GPURT captures (see ResolveInstanceBlasAddress()).
            uint64_t   blas_metadata_size = 0;
            const auto meta_it            = instance_blas_metadata_size_.find(pair.first);
            if (meta_it != instance_blas_metadata_size_.end())
            {
                blas_metadata_size = meta_it->second;
            }
            if (ResolveInstanceBlasAddress(pair.first, blas_metadata_size, &blas_index) == kRraOk)
            {
                extracted.key() = blas_index;
                instance_list_.insert(std::move(extracted));
            }
        }
    }

    bool EncodedRtIp31TopLevelBvh::BuildInstanceList()
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

        // Node packing (RTIP3.1): several sibling box slots may decode to the same child pointer (a slot with
        // NodeRangeLength()==0 does not advance the running offset), so the same subtree would otherwise be walked
        // once per referencing slot. That double-adds the shared instances and inflates num_traversal_node_count past
        // the header's unique node count (tripping the consistency guard below). Track visited node pointers and walk
        // each shared subtree exactly once.
        std::unordered_set<uint32_t> visited_nodes;

        traversal_stack.push_back(std::make_pair(root_ptr, 0));

        const auto& header_offsets = header_->GetBufferOffsets();
        while (!traversal_stack.empty())
        {
            auto front    = traversal_stack.back();
            auto node_ptr = front.first;
            auto level    = front.second;

            traversal_stack.pop_back();

            // Node packing: this node pointer was already reached through an earlier (packed) sibling slot; its subtree
            // and instances were already accounted for, so don't re-walk it.
            if (!visited_nodes.insert(node_ptr.GetRawPointer()).second)
            {
                continue;
            }

            if (node_ptr.IsInstanceNode())
            {
                auto byte_offset = node_ptr.GetByteOffset() - header_offsets.interior_nodes;
                if (byte_offset < interior_nodes_.size())
                {
                    HwInstanceNodeRRA* instance_node = reinterpret_cast<HwInstanceNodeRRA*>(&interior_nodes_[byte_offset]);

                    NodePointer64 temp_ptr{};
                    temp_ptr.u64 = instance_node->data.childBasePtr;
                    // also shifted by 6 because it is aligned to 64.
                    const uint64_t blas_address = (temp_ptr.aligned_addr_64b << 6);

                    // Record the instance's BLAS-metadata page size (available for both sideband layouts) so the
                    // raw address can be resolved with the version-tolerant fallback in ConvertBlasAddressesToIndices().
                    // The address is stored raw (unnormalized); the fallback subtracts a metadata page only if the
                    // exact lookup misses, so older captures that point straight at the traversal root are unaffected.
                    const std::optional<InstanceNodeDataRRA> instance_data = GetHwInstanceNode(&node_ptr);
                    if (instance_data.has_value())
                    {
                        instance_blas_metadata_size_[blas_address] = instance_data->sideband.blasMetadataSize;
                    }

                    uint32_t              address  = byte_offset + header_offsets.interior_nodes;
                    dxr::amd::NodePointer new_node = dxr::amd::NodePointer(dxr::amd::NodeType::kAmdNodeInstance, address);

                    if (instance_list_.find(blas_address) == instance_list_.end())
                    {
                        std::vector<uint32_t> node_list = {new_node.GetRawPointer()};
                        instance_list_.insert(std::make_pair(blas_address, node_list));
                    }
                    else
                    {
                        instance_list_[blas_address].push_back(new_node.GetRawPointer());
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
                        // The quantized BVH8 node uses the same enum value as Fp32 Box node.
                        const auto            node = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[byte_offset]);
                        dxr::amd::NodePointer child_ptrs[8]{};
                        node->DecodeChildrenOffsets((uint32_t*)child_ptrs);

                        for (uint32_t i = 0; i < node->ValidChildCount(); ++i)
                        {
                            if (!child_ptrs[i].IsInvalid())
                            {
                                // Node packing: flag when an earlier sibling slot already decoded to this same child
                                // pointer, so the frontend switches to composite (global-child-index + node-id) keys.
                                for (uint32_t j = 0; j < i; ++j)
                                {
                                    if (!child_ptrs[j].IsInvalid() && child_ptrs[j].GetRawPointer() == child_ptrs[i].GetRawPointer())
                                    {
                                        has_node_packing_ = true;
                                        break;
                                    }
                                }

                                traversal_stack.push_back(std::make_pair(child_ptrs[i], level + 1));
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

        // A partitioned TLAS (PTLAS) carries partition-level internal nodes above the base level, and the
        // global partition's instances hang directly off the root, so the stack walk legitimately visits more
        // nodes than the header's flat interior+leaf count. Only apply this tree-consistency guard (which
        // catches degenerate/cyclic single-level TLASes) when the capture is not partitioned.
        if (!header_->GetRawHeader().IsPartitioned() && num_traversal_node_count > GetNodeCount(BvhNodeFlags::kNone))
        {
            return false;
        }
        return true;
    }

    bool EncodedRtIp31TopLevelBvh::HasNodePacking() const
    {
        return has_node_packing_;
    }

    uint64_t EncodedRtIp31TopLevelBvh::GetInactiveInstanceCountImpl() const
    {
        uint64_t    inactive_count{0};
        size_t      byte_offset       = 0;
        const auto& header_offsets    = GetHeader().GetBufferOffsets();
        const auto  instance_node_size = GetInstanceNodeSize();
        while (byte_offset < interior_nodes_.size())
        {
            // Route through GetHwInstanceNode so both the legacy per-leaf-slot and the new per-instance-index
            // sideband layouts are canonicalized (blasMetadataSize is available for both) before the check.
            const uint32_t        address = header_offsets.interior_nodes + (uint32_t)byte_offset;
            dxr::amd::NodePointer node_ptr(dxr::amd::NodeType::kAmdNodeInstance, address);

            std::optional<InstanceNodeDataRRA> instance_node_data = GetHwInstanceNode(&node_ptr);
            if (!instance_node_data.has_value())
            {
                break;
            }

            if (InstanceIsInactive(&instance_node_data.value()))
            {
                ++inactive_count;
            }

            byte_offset += instance_node_size;
        }
        return inactive_count;
    }

    uint64_t EncodedRtIp31TopLevelBvh::GetBlasCount(bool empty_placeholder) const
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

    uint64_t EncodedRtIp31TopLevelBvh::GetReferencedBlasMemorySize() const
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

    uint64_t EncodedRtIp31TopLevelBvh::GetTotalTriangleCount() const
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

    uint64_t EncodedRtIp31TopLevelBvh::GetUniqueTriangleCount() const
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

    RraErrorCode EncodedRtIp31TopLevelBvh::GetNodeName(uint32_t node_id, const char** out_name) const
    {
        dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);

        switch ((uint32_t)node->GetType())
        {
        case (uint32_t)dxr::amd::NodeType::kAmdNodeBoxFp16:
            *out_name = "Box16";
            break;

        case (uint32_t)dxr::amd::NodeType::kAmdNodeBoxFp32:
            *out_name = "Bvh8";
            break;

        case (uint32_t)dxr::amd::NodeType::kAmdNodeInstance:
            *out_name = "Instance";
            break;

        default:
            *out_name = "Unknown";
            break;
        }
        return kRraOk;
    }

    RraErrorCode EncodedRtIp31TopLevelBvh::GetNodeNameToolTip(uint32_t node_id, const char** out_tooltip) const
    {
        dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);

        switch ((uint32_t)node->GetType())
        {
        case (uint32_t)dxr::amd::NodeType::kAmdNodeBoxFp16:
            *out_tooltip = "A 16-bit floating point bounding volume node with up to 4 child nodes";
            break;

        case (uint32_t)dxr::amd::NodeType::kAmdNodeBoxFp32:
            *out_tooltip = "A compressed bounding volume node with up to 8 child nodes";
            break;

        case (uint32_t)dxr::amd::NodeType::kAmdNodeInstance:
            *out_tooltip = "A node containing an instance of a BLAS. Double-click to view this instance in the BLAS viewer";
            break;

        default:
            *out_tooltip = "";
            break;
        }

        return kRraOk;
    }

    RraErrorCode EncodedRtIp31TopLevelBvh::GetBlasIndex(uint32_t node_id, uint64_t* out_blas_index) const
    {
        InstanceNodeDataRRA hw_instance_node{};
        RraErrorCode        result = GetInstanceNodeFromInstancePointer(node_id, &hw_instance_node);
        if (result != kRraOk)
        {
            return result;
        }

        NodePointer64 temp_ptr{};
        temp_ptr.u64 = hw_instance_node.hw_instance_node.data.childBasePtr;
        // also shifted by 6 because it is aligned to 64.
        const uint64_t blas_address = (temp_ptr.aligned_addr_64b << 6);

        result = ResolveInstanceBlasAddress(blas_address, hw_instance_node.sideband.blasMetadataSize, out_blas_index);

        return result;
    }

    RraErrorCode EncodedRtIp31TopLevelBvh::GetInstanceIndex(uint32_t node_id, uint32_t* out_instance_index) const
    {
        InstanceNodeDataRRA instance_node{};
        RraErrorCode        result = GetInstanceNodeFromInstancePointer(node_id, &instance_node);
        if (result != kRraOk)
        {
            return result;
        }

        *out_instance_index = instance_node.sideband.instanceIndex;
        return result;
    }

    RraErrorCode EncodedRtIp31TopLevelBvh::GetInstanceNodeTransform(uint32_t node_id, float* out_transform) const
    {
        InstanceNodeDataRRA instance_node{};
        RraErrorCode        result = GetInstanceNodeFromInstancePointer(node_id, &instance_node);

        if (result != kRraOk)
        {
            return result;
        }

        memcpy(out_transform, instance_node.hw_instance_node.data.worldToObject, dxr::kMatrix3x4Size);
        return result;
    }

    RraErrorCode EncodedRtIp31TopLevelBvh::GetOriginalInstanceNodeTransform(uint32_t node_id, float* out_transform) const
    {
        InstanceNodeDataRRA instance_node{};
        RraErrorCode        result = GetInstanceNodeFromInstancePointer(node_id, &instance_node);

        if (result != kRraOk)
        {
            return result;
        }

        memcpy(out_transform, instance_node.sideband.objectToWorld, dxr::kMatrix3x4Size);
        return result;
    }

    RraErrorCode EncodedRtIp31TopLevelBvh::GetInstanceNodeMask(uint32_t node_id, uint32_t* out_mask) const
    {
        InstanceNodeDataRRA instance_node{};
        RraErrorCode        result = GetInstanceNodeFromInstancePointer(node_id, &instance_node);

        if (result != kRraOk)
        {
            return result;
        }

        *out_mask = instance_node.hw_instance_node.data.userDataAndInstanceMask >> 24;
        return result;
    }

    RraErrorCode EncodedRtIp31TopLevelBvh::GetInstanceNodeID(uint32_t node_id, uint32_t* out_id) const
    {
        InstanceNodeDataRRA instance_node{};
        RraErrorCode        result = GetInstanceNodeFromInstancePointer(node_id, &instance_node);

        if (result != kRraOk)
        {
            return result;
        }

        constexpr uint32_t instance_id_bit_mask{0xFFFFFF};
        *out_id = instance_node.sideband.instanceIdAndFlags & instance_id_bit_mask;
        return result;
    }

    RraErrorCode EncodedRtIp31TopLevelBvh::GetInstanceNodeHitGroup(uint32_t node_id, uint32_t* out_hit_group) const
    {
        InstanceNodeDataRRA instance_node{};
        RraErrorCode        result = GetInstanceNodeFromInstancePointer(node_id, &instance_node);

        if (result != kRraOk)
        {
            return result;
        }

        constexpr uint32_t user_data_bit_mask{0xFFFFFF};
        *out_hit_group = instance_node.hw_instance_node.data.userDataAndInstanceMask & user_data_bit_mask;
        return result;
    }

    RraErrorCode EncodedRtIp31TopLevelBvh::GetInstanceFlags(uint32_t node_id, uint32_t* out_flags) const
    {
        InstanceNodeDataRRA instance_node{};
        RraErrorCode        result = GetInstanceNodeFromInstancePointer(node_id, &instance_node);

        if (result != kRraOk)
        {
            return result;
        }

        *out_flags = instance_node.sideband.instanceIdAndFlags >> 24;
        return result;
    }

    uint64_t EncodedRtIp31TopLevelBvh::GetInstanceCount(uint64_t index) const
    {
        auto iter = instance_list_.find(index);
        if (iter != instance_list_.end())
        {
            return iter->second.size();
        }
        return 0;
    }

    dxr::amd::NodePointer EncodedRtIp31TopLevelBvh::GetInstanceNode(uint64_t blas_index, uint64_t instance_index) const
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

    float EncodedRtIp31TopLevelBvh::GetLeafNodeSurfaceAreaHeuristic(uint32_t node_id, uint32_t global_child_index) const
    {
        RRA_UNUSED(global_child_index);
        const int32_t index = GetInstanceIndex(node_id);
        assert(index != -1);
        assert(index < static_cast<int32_t>(instance_surface_area_heuristic_.size()));
        return instance_surface_area_heuristic_[index];
    }

    void EncodedRtIp31TopLevelBvh::SetLeafNodeSurfaceAreaHeuristic(uint32_t node_id, float surface_area_heuristic)
    {
        const int32_t index = GetInstanceIndex(node_id);
        assert(index != -1);
        assert(index < static_cast<int32_t>(instance_surface_area_heuristic_.size()));
        instance_surface_area_heuristic_[index] = surface_area_heuristic;
    }

    RraErrorCode EncodedRtIp31TopLevelBvh::GetInstanceNodeFromInstancePointer(uint32_t node_id, InstanceNodeDataRRA* out_instance_node) const
    {
        dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);
        if (!node->IsInstanceNode())
        {
            return kRraErrorInvalidPointer;
        }

        auto instance_node = GetHwInstanceNode(node);
        if (!instance_node)
        {
            return kRraErrorIndexOutOfRange;
        }
        *out_instance_node = instance_node.value();

        return kRraOk;
    }

}  // namespace rta

