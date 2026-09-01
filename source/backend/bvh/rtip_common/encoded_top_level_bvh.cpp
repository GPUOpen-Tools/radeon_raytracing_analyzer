//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 1.1 (Navi2x) specific top level acceleration structure
/// implementation.
//=============================================================================

#include "bvh/rtip_common/encoded_top_level_bvh.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <deque>
#include <iostream>
#include <unordered_set>
#include <vector>

#include "public/rra_assert.h"
#include "public/rra_blas.h"
#include "public/rra_error.h"

#include "bvh/rtip11/rt_ip_11_header.h"
#include "bvh/rtip_common/i_acceleration_structure_header.h"

namespace rta
{
    EncodedTopLevelBvh::~EncodedTopLevelBvh()
    {
    }

    std::int32_t EncodedTopLevelBvh::GetInstanceNodeSize() const
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

    bool EncodedTopLevelBvh::HasBvhReferences() const
    {
        return true;
    }

    uint64_t EncodedTopLevelBvh::GetBlasCount(bool empty_placeholder) const
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

    uint64_t EncodedTopLevelBvh::GetInstanceCount(uint64_t index) const
    {
        auto iter = instance_list_.find(index);
        if (iter != instance_list_.end())
        {
            return iter->second.size();
        }
        return 0;
    }

    uint32_t EncodedTopLevelBvh::GetInstanceNode(uint64_t blas_index, uint64_t instance_index) const
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

    uint64_t EncodedTopLevelBvh::GetTotalProceduralNodeCount() const
    {
        uint64_t procedural_count = 0;
        for (const auto& it : instance_list_)
        {
            uint32_t     procedural_nodes = 0;
            RraErrorCode status           = RraBlasGetProceduralNodeCount(it.first, &procedural_nodes);
            RRA_ASSERT(status == kRraOk);
            if (status == kRraOk)
            {
                procedural_count += static_cast<uint64_t>(procedural_nodes);
            }
        }
        return procedural_count;
    }

    float EncodedTopLevelBvh::GetLeafNodeSurfaceAreaHeuristic(uint32_t node_id, uint32_t global_child_index) const
    {
        RRA_UNUSED(global_child_index);
        const int32_t index = GetInstanceIndex(node_id);
        assert(index != -1);
        assert(index < static_cast<int32_t>(instance_surface_area_heuristic_.size()));
        return instance_surface_area_heuristic_[index];
    }

    void EncodedTopLevelBvh::SetLeafNodeSurfaceAreaHeuristic(uint32_t node_id, float surface_area_heuristic)
    {
        const int32_t index = GetInstanceIndex(node_id);
        assert(index != -1);
        assert(index < static_cast<int32_t>(instance_surface_area_heuristic_.size()));
        instance_surface_area_heuristic_[index] = surface_area_heuristic;
    }

    bool EncodedTopLevelBvh::IsPartitioned() const
    {
        return is_partitioned_;
    }

    uint32_t EncodedTopLevelBvh::GetPartitionCount() const
    {
        return partition_count_;
    }

    uint32_t EncodedTopLevelBvh::GetMaxPartitionInstances() const
    {
        return max_partition_instances_;
    }

    uint32_t EncodedTopLevelBvh::GetMaxGlobalInstances() const
    {
        return max_global_instances_;
    }

    const EncodedTopLevelBvh::PartitionInfo* EncodedTopLevelBvh::GetPartitionInfo(uint32_t partition_index) const
    {
        if (partition_index < partition_infos_.size())
        {
            return &partition_infos_[partition_index];
        }
        return nullptr;
    }

    bool EncodedTopLevelBvh::GetInstancePartitionIndex(uint32_t instance_index, uint32_t* out_partition_index, bool* out_active) const
    {
        if (instance_index >= instance_partition_indices_.size())
        {
            return false;
        }
        if (out_partition_index != nullptr)
        {
            *out_partition_index = instance_partition_indices_[instance_index];
        }
        if (out_active != nullptr)
        {
            *out_active = instance_partition_active_[instance_index] != 0;
        }
        return true;
    }

    void EncodedTopLevelBvh::ParsePartitionData()
    {
        is_partitioned_          = false;
        partition_count_         = 0;
        max_partition_instances_ = 0;
        max_global_instances_    = 0;
        partition_infos_.clear();
        instance_partition_indices_.clear();
        instance_partition_active_.clear();

        if (header_ == nullptr)
        {
            return;
        }

        const AccelStructHeader& raw = header_->GetRawHeader();
        if (!raw.IsPartitioned())
        {
            return;
        }

        // Struct sizes within the retained sideband region (which starts at the geometry_info offset).
        constexpr size_t kSidebandSize      = 64;  // Per-instance hardware instance sideband.
        constexpr size_t kInstanceInfoSize  = 32;  // GPURT InstanceInfo.
        constexpr size_t kPartitionInfoSize = 32;  // GPURT PartitionInfo (24B payload, padded).

        const uint32_t num_descs      = header_->GetGeometryDescriptionCount();
        const uint32_t num_partitions = raw.GetNumPartitions();

        const size_t instance_info_offset  = static_cast<size_t>(num_descs) * kSidebandSize;
        const size_t partition_info_offset = instance_info_offset + static_cast<size_t>(num_descs) * kInstanceInfoSize;
        const size_t required_size         = partition_info_offset + (static_cast<size_t>(num_partitions) + 1) * kPartitionInfoSize;

        if (sideband_data_.size() < required_size)
        {
            // The partition arrays were not retained; treat as non-partitioned rather than reading OOB.
            return;
        }

        is_partitioned_          = true;
        partition_count_         = num_partitions;
        max_partition_instances_ = raw.MaxNumPartitionInstances();
        max_global_instances_    = raw.MaxNumGlobalInstances();

        // Parse PartitionInfo[] (num_partitions + 1 entries; the last entry is the global partition).
        partition_infos_.resize(num_partitions + 1);
        for (uint32_t p = 0; p <= num_partitions; ++p)
        {
            const uint8_t* ptr  = sideband_data_.data() + partition_info_offset + static_cast<size_t>(p) * kPartitionInfoSize;
            PartitionInfo& info = partition_infos_[p];
            memcpy(&info.internal_node_count, ptr + 0, sizeof(uint32_t));
            memcpy(&info.instance_count, ptr + 4, sizeof(uint32_t));
            memcpy(&info.root_node_index, ptr + 8, sizeof(uint32_t));
            memcpy(&info.leaf_index_offset, ptr + 12, sizeof(uint32_t));
            memcpy(&info.fat_leaf_count, ptr + 16, sizeof(uint32_t));
            memcpy(info.translation, ptr + 20, 3 * sizeof(float));
        }

        // Parse InstanceInfo[] (num_descs entries) for per-instance partition index and bounds.
        instance_partition_indices_.assign(num_descs, kInvalidPartition);
        instance_partition_active_.assign(num_descs, 0);
        for (uint32_t i = 0; i < num_descs; ++i)
        {
            const uint8_t* ptr = sideband_data_.data() + instance_info_offset + static_cast<size_t>(i) * kInstanceInfoSize;

            float    bounds_min[3];
            float    bounds_max[3];
            uint32_t partition_index_raw;
            memcpy(bounds_min, ptr + 0, 3 * sizeof(float));
            memcpy(bounds_max, ptr + 12, 3 * sizeof(float));
            memcpy(&partition_index_raw, ptr + 24, sizeof(uint32_t));

            const bool     active          = (partition_index_raw & 0x80000000u) == 0;
            const uint32_t partition_index = partition_index_raw & 0x7FFFFFFFu;

            instance_partition_active_[i]  = active ? 1 : 0;
            instance_partition_indices_[i] = partition_index;

            // Aggregate a per-partition AABB from member instance bounds (global slot included).
            if (partition_index <= num_partitions)
            {
                PartitionInfo& info = partition_infos_[partition_index];
                if (!info.bounds_valid)
                {
                    info.bounds_valid = true;
                    for (int c = 0; c < 3; ++c)
                    {
                        info.bounds_min[c] = bounds_min[c];
                        info.bounds_max[c] = bounds_max[c];
                    }
                }
                else
                {
                    for (int c = 0; c < 3; ++c)
                    {
                        info.bounds_min[c] = std::min(info.bounds_min[c], bounds_min[c]);
                        info.bounds_max[c] = std::max(info.bounds_max[c], bounds_max[c]);
                    }
                }
            }
        }
    }

}  // namespace rta

