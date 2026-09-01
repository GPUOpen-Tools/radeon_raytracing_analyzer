//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the BVH interface.
///
/// Contains public functions common to all acceleration structures.
//=============================================================================

#include "rra_bvh_impl.h"

#include <float.h>
#include <limits>

#include "public/rra_assert.h"
#include "public/rra_rtip_info.h"

#include "bvh/dxr_definitions.h"
#include "bvh/gpu_def.h"
#include "bvh/ibvh.h"
#include "bvh/inode.h"
#include "bvh/rtip_common/encoded_bottom_level_bvh.h"
#include "rra_data_set.h"

// External reference to the global dataset.
extern RraDataSet data_set_;

RraErrorCode RraBvhGetNodeBoundingVolume(const rta::IBvh*                  bvh,
                                         uint32_t                          node_id,
                                         uint32_t                          child_index,
                                         uint32_t                          global_child_index,
                                         dxr::amd::AxisAlignedBoundingBox& out_bounding_box)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetNodeBoundingVolume(bvh, node_id, child_index, global_child_index, out_bounding_box);
}

RraErrorCode RraBvhGetNodeObbIndex(const rta::IBvh* bvh, uint32_t node_id, uint32_t* obb_index)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();

    *obb_index = bvh_node.GetNodeObbIndex(node_id, bvh);
    return kRraOk;
}

RraErrorCode RraBvhGetNodeBoundingVolumeOrientation(const rta::IBvh* bvh, uint32_t node_id, glm::mat3& out_rotation)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();

    out_rotation = bvh_node.GetNodeBoundingVolumeOrientation(node_id, bvh);
    return kRraOk;
}

RraErrorCode RraBvhGetRootNodePtr(uint32_t* out_node_ptr)
{
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetRootNodePtr(out_node_ptr);
}

RraErrorCode RraBvhGetBoundingVolumeSurfaceArea(const BoundingVolumeExtents* extents, float* out_surface_area)
{
    const float dx = extents->max_x - extents->min_x;
    const float dy = extents->max_y - extents->min_y;
    const float dz = extents->max_z - extents->min_z;

    float width  = std::max(FLT_MIN, dx);
    float height = std::max(FLT_MIN, dy);
    float depth  = std::max(FLT_MIN, dz);

    const float result = 2.0f * ((width * height) + (width * depth) + (height * depth));
    *out_surface_area  = result;

    return kRraOk;
}

RraErrorCode RraBvhGetNodeOffset(uint32_t node_id, uint64_t* out_offset)
{
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetNodeOffset(node_id, out_offset);
}

bool RraBvhIsBoxNode(const rta::IBvh* bvh, uint32_t node_id)
{
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return false;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetIsBoxNode(node_id, bvh);
}

bool RraBvhIsBox16Node(const rta::IBvh* bvh, uint32_t node_id)
{
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return false;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetIsBox16Node(node_id, bvh);
}

bool RraBvhIsBox32Node(const rta::IBvh* bvh, uint32_t node_id)
{
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return false;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetIsBox32Node(node_id, bvh);
}

bool RraBvhHasChildren(const rta::IBvh* bvh, uint32_t node_id)
{
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return false;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetHasChildren(node_id, bvh);
}

RraErrorCode RraBvhGetBoundingVolumeSurfaceArea(const rta::IBvh* bvh,
                                                uint32_t         node_id,
                                                uint32_t         child_index,
                                                uint32_t         global_child_index,
                                                float*           out_surface_area)
{
    dxr::amd::AxisAlignedBoundingBox bounding_box;
    RraErrorCode                     result = RraBvhGetNodeBoundingVolume(bvh, node_id, child_index, global_child_index, bounding_box);
    if (result != kRraOk)
    {
        return result;
    }

    BoundingVolumeExtents bounding_volume_extents;

    bounding_volume_extents.min_x = bounding_box.min.x;
    bounding_volume_extents.min_y = bounding_box.min.y;
    bounding_volume_extents.min_z = bounding_box.min.z;
    bounding_volume_extents.max_x = bounding_box.max.x;
    bounding_volume_extents.max_y = bounding_box.max.y;
    bounding_volume_extents.max_z = bounding_box.max.z;

    return RraBvhGetBoundingVolumeSurfaceArea(&bounding_volume_extents, out_surface_area);
}

RraErrorCode RraBvhGetSurfaceAreaHeuristic(const rta::IBvh* bvh, uint32_t node_id, uint32_t global_child_index, float* out_surface_area_heuristic)
{
    auto blas = dynamic_cast<const rta::EncodedBottomLevelBvh*>(bvh);
    if (blas && blas->IsProcedural())
    {
        *out_surface_area_heuristic = 1.0f;
        return kRraOk;
    }

    if (bvh && bvh->IsEmpty())
    {
        *out_surface_area_heuristic = 0.0f;
        return kRraOk;
    }

    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetSurfaceAreaHeuristic(bvh, node_id, global_child_index, out_surface_area_heuristic);
}

std::array<uint32_t, MAX_CHILD_NODES> RraBvhGetChildNodeArray(const rta::IBvh* bvh, uint32_t root_id, uint32_t node_offset)
{
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return {};
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetChildNodeArray(bvh, root_id, node_offset);
}

RraErrorCode RraBvhGetChildNodeCount(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_count)
{
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetChildNodeCount(bvh, parent_node, out_child_count);
}

RraErrorCode RraBvhGetChildNodes(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_nodes)
{
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetChildNodes(bvh, parent_node, out_child_nodes);
}

RraErrorCode RraBvhGetChildIndices(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_indices)
{
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetChildIndices(bvh, parent_node, out_child_indices);
}

RraErrorCode RraBvhGetChildNodePtr(const rta::IBvh* bvh, uint32_t parent_node, uint32_t child_index, uint32_t* out_node_id)
{
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetChildNodePtr(bvh, parent_node, child_index, out_node_id);
}

RraErrorCode RraBvhGetTlasCount(uint64_t* out_count)
{
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const auto& top_level_bvhs = data_set_.bvh_bundle->GetTopLevelBvhs();
    *out_count                 = top_level_bvhs.size();

    return kRraOk;
}

RraErrorCode RraBvhGetBlasCount(uint64_t* out_count)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_count = data_set_.bvh_bundle->GetBlasCount();
    return kRraOk;
}

RraErrorCode RraBvhGetTotalBlasCount(uint64_t* out_count)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_count = data_set_.bvh_bundle->GetTotalBlasCount();
    return kRraOk;
}

RraErrorCode RraBvhGetMissingBlasCount(uint64_t* out_count)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_count = data_set_.bvh_bundle->GetMissingBlasCount();
    return kRraOk;
}

RraErrorCode RraBvhGetInactiveInstancesCount(uint64_t* out_count)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_count = data_set_.bvh_bundle->GetInactiveInstanceCount();
    return kRraOk;
}

RraErrorCode RraBvhGetEmptyBlasCount(uint64_t* out_count)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_count = data_set_.bvh_bundle->GetEmptyBlasCount();
    return kRraOk;
}

RraErrorCode RraBvhGetTotalTlasSizeInBytes(uint64_t* out_size_in_bytes)
{
    uint64_t tlas_count = 0;
    if (RraBvhGetTlasCount(&tlas_count) == kRraOk)
    {
        RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
        const auto& top_level_bvhs = data_set_.bvh_bundle->GetTopLevelBvhs();
        for (uint64_t tlas_index = 0; tlas_index < tlas_count; tlas_index++)
        {
            const rta::IBvh* tlas = top_level_bvhs[tlas_index].get();
            RRA_ASSERT(tlas != nullptr);
            *out_size_in_bytes += tlas->GetHeader().GetFileSize();
        }
    }
    return kRraOk;
}

RraErrorCode RraBvhGetTotalBlasSizeInBytes(uint64_t* out_size_in_bytes)
{
    uint64_t blas_count = 0;
    if (RraBvhGetBlasCount(&blas_count) == kRraOk)
    {
        RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
        const auto& bottom_level_bvhs = data_set_.bvh_bundle->GetBottomLevelBvhs();
        uint64_t    offset            = 0;
        if (data_set_.bvh_bundle->ContainsEmptyPlaceholder())
        {
            offset = 1;
        }
        for (uint64_t blas_index = offset; blas_index < (blas_count + offset); blas_index++)
        {
            const rta::IBvh* blas = bottom_level_bvhs[blas_index].get();
            RRA_ASSERT(blas != nullptr);
            *out_size_in_bytes += blas->GetHeader().GetFileSize();
        }
    }
    return kRraOk;
}

RraErrorCode RraBvhGetTotalTraceSizeInBytes(uint64_t* out_size_in_bytes)
{
    uint64_t     tlas_size{};
    RraErrorCode error_code = RraBvhGetTotalTlasSizeInBytes(&tlas_size);
    if (error_code != kRraOk)
    {
        return error_code;
    }

    uint64_t blas_size{};
    error_code = RraBvhGetTotalBlasSizeInBytes(&blas_size);
    if (error_code != kRraOk)
    {
        return error_code;
    }

    *out_size_in_bytes = tlas_size + blas_size;
    return kRraOk;
}

uint32_t RraBvhGetMaxChildCount()
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return 0;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetMaxChildCount();
}

