//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 1.1 (Navi2x) specific top level acceleration structure
/// implementation.
//=============================================================================

#include "bvh/rtip_common/encoded_top_level_bvh.h"

#include <iostream>
#include <vector>
#include <cassert>
#include <deque>
#include <unordered_set>

#include "public/rra_assert.h"
#include "public/rra_blas.h"
#include "public/rra_error.h"
#include "bvh/rtip_common/i_acceleration_structure_header.h"
#include "bvh/rtip11/rt_ip_11_header.h"

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

    dxr::amd::NodePointer EncodedTopLevelBvh::GetInstanceNode(uint64_t blas_index, uint64_t instance_index) const
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

    float EncodedTopLevelBvh::GetLeafNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr) const
    {
        const int32_t index = GetInstanceIndex(&node_ptr);
        assert(index != -1);
        assert(index < static_cast<int32_t>(instance_surface_area_heuristic_.size()));
        return instance_surface_area_heuristic_[index];
    }

    void EncodedTopLevelBvh::SetLeafNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr, float surface_area_heuristic)
    {
        const int32_t index = GetInstanceIndex(&node_ptr);
        assert(index != -1);
        assert(index < static_cast<int32_t>(instance_surface_area_heuristic_.size()));
        instance_surface_area_heuristic_[index] = surface_area_heuristic;
    }

}  // namespace rta
