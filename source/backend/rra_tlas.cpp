//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the TLAS interface.
///
/// Contains public functions specific to the TLAS.
//=============================================================================

#include "rra_tlas_impl.h"

#include <limits.h>

#include "public/rra_assert.h"
#include "public/rra_rtip_info.h"

#include "bvh/inode.h"
#include "bvh/rtip31/encoded_rt_ip_31_top_level_bvh.h"
#include "math_util.h"
#include "rra_data_set.h"
#include "surface_area_heuristic.h"

// External reference to the global dataset.
extern RraDataSet data_set_;

static RraErrorCode RraTlasGetBoundingVolumeExtentsImpl(const rta::IBvh* tlas, uint32_t node_id, uint32_t child_index, BoundingVolumeExtents* out_extents)
{
    dxr::amd::AxisAlignedBoundingBox bounding_box;

    RraErrorCode error_code = RraBvhGetNodeBoundingVolume(tlas, node_id, child_index, 0, bounding_box);  // Pass 0 since it's unused for TLAS.
    if (error_code != kRraOk)
    {
        return error_code;
    }

    (*out_extents).min_x = bounding_box.min.x;
    (*out_extents).min_y = bounding_box.min.y;
    (*out_extents).min_z = bounding_box.min.z;
    (*out_extents).max_x = bounding_box.max.x;
    (*out_extents).max_y = bounding_box.max.y;
    (*out_extents).max_z = bounding_box.max.z;

    return kRraOk;
}

rta::EncodedTopLevelBvh* RraTlasGetTlasFromTlasIndex(uint64_t tlas_index)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    const auto& top_level_bvhs = data_set_.bvh_bundle->GetTopLevelBvhs();
    if (tlas_index >= top_level_bvhs.size())
    {
        return nullptr;
    }

    auto tlas_ptr = top_level_bvhs[tlas_index].get();
    return dynamic_cast<rta::EncodedTopLevelBvh*>(tlas_ptr);
}

RraErrorCode RraTlasGetBaseAddress(uint64_t tlas_index, uint64_t* out_address)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    *out_address = tlas->GetVirtualAddress();

    return kRraOk;
}

RraErrorCode RraTlasGetAPIAddress(uint64_t tlas_index, uint64_t* out_address)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const auto base_addr = tlas->GetVirtualAddress();

    *out_address = base_addr + tlas->GetHeader().GetMetaDataSize();

    return kRraOk;
}

bool RraTlasIsEmpty(uint64_t tlas_index)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return true;
    }

    return tlas->IsEmpty();
}

RraErrorCode RraTlasGetTotalNodeCount(uint64_t tlas_index, uint64_t* out_node_count)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const auto& top_level_bvhs = data_set_.bvh_bundle->GetTopLevelBvhs();
    if (tlas_index >= top_level_bvhs.size())
    {
        return kRraErrorInvalidPointer;
    }

    const auto& tlas = top_level_bvhs[tlas_index];
    *out_node_count  = tlas->GetNodeCount(rta::BvhNodeFlags::kNone);

    return kRraOk;
}

RraErrorCode RraTlasGetChildNodeCount(uint64_t tlas_index, uint32_t parent_node, uint32_t* out_child_count)
{
    RRA_ASSERT(out_child_count != nullptr);
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    const auto& top_level_bvhs = data_set_.bvh_bundle->GetTopLevelBvhs();
    if (tlas_index >= top_level_bvhs.size())
    {
        return kRraErrorInvalidPointer;
    }

    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    return RraBvhGetChildNodeCount(tlas, parent_node, out_child_count);
}

RraErrorCode RraTlasGetChildNodes(uint64_t tlas_index, uint32_t parent_node, uint32_t* out_child_nodes)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    const auto& top_level_bvhs = data_set_.bvh_bundle->GetTopLevelBvhs();
    if (tlas_index >= top_level_bvhs.size())
    {
        return kRraErrorInvalidPointer;
    }

    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    return RraBvhGetChildNodes(tlas, parent_node, out_child_nodes);
}

RraErrorCode RraTlasGetChildIndices(uint64_t tlas_index, uint32_t parent_node, uint32_t* out_child_nodes)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    const auto& top_level_bvhs = data_set_.bvh_bundle->GetTopLevelBvhs();
    if (tlas_index >= top_level_bvhs.size())
    {
        return kRraErrorInvalidPointer;
    }

    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);

    return RraBvhGetChildIndices(tlas, parent_node, out_child_nodes);
}

RraErrorCode RraTlasGetChildNodePtr(uint64_t tlas_index, uint32_t parent_node, uint32_t child_index, uint32_t* out_node_ptr)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    return RraBvhGetChildNodePtr(tlas, parent_node, child_index, out_node_ptr);
}

RraErrorCode RraTlasGetNodeName(uint64_t tlas_index, uint32_t node_id, const char** out_name)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    return tlas->GetNodeName(node_id, out_name);
}

RraErrorCode RraTlasGetNodeNameToolTip(uint64_t tlas_index, uint32_t node_id, const char** out_tooltip)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    return tlas->GetNodeNameToolTip(node_id, out_tooltip);
}

RraErrorCode RraTlasGetNodeBaseAddress(uint64_t tlas_index, uint32_t node_id, uint64_t* out_address)
{
    if (out_address == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetBaseAddress(node_id, tlas, out_address);
}

RraErrorCode RraTlasGetNodeParent(uint64_t tlas_index, uint32_t node_ptr, uint32_t* out_parent_node_id)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const auto& interior_nodes = tlas->GetInteriorNodesData();
    if (interior_nodes.size() == 0)
    {
        return kRraErrorInvalidPointer;
    }

    *out_parent_node_id = tlas->GetParentNode(node_ptr, 0);  // Pass 0 since it's unused for TLAS.
    return kRraOk;
}

RraErrorCode RraTlasGetInstanceNodeInfo(uint64_t tlas_index, uint32_t node_id, uint64_t* out_blas_address, uint64_t* out_instance_count, bool* out_is_empty)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);

    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const auto& bottom_level_bvhs = data_set_.bvh_bundle->GetBottomLevelBvhs();
    if (tlas_index >= bottom_level_bvhs.size())
    {
        return kRraErrorInvalidPointer;
    }
    uint64_t     blas_index{};
    RraErrorCode result = tlas->GetBlasIndex(node_id, &blas_index);
    if (result != kRraOk)
    {
        return result;
    }

    const rta::IBvh* instance_blas = dynamic_cast<rta::IBvh*>(&(*bottom_level_bvhs[blas_index]));
    if (instance_blas == nullptr)
    {
        return kRraErrorIndexOutOfRange;
    }

    *out_blas_address   = instance_blas->GetVirtualAddress();
    *out_instance_count = tlas->GetInstanceCount(blas_index);
    *out_is_empty       = instance_blas->IsEmpty();

    return kRraOk;
}

RraErrorCode RraTlasGetInstanceCount(uint64_t tlas_index, uint64_t blas_index, uint64_t* out_instance_count)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_instance_count = tlas->GetInstanceCount(blas_index);
    return kRraOk;
}

RraErrorCode RraTlasGetBlasCount(uint64_t tlas_index, uint64_t* out_blas_count)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    *out_blas_count = tlas->GetBlasCount(data_set_.bvh_bundle->ContainsEmptyPlaceholder());
    return kRraOk;
}

RraErrorCode RraTlasGetBoxNodeCount(uint64_t tlas_index, uint64_t* out_node_count)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const auto& tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_node_count = tlas->GetNodeCount(rta::BvhNodeFlags::kIsInteriorNode);

    return kRraOk;
}

RraErrorCode RraTlasGetBox16NodeCount(uint64_t tlas_index, uint32_t* out_node_count)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const auto& tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_node_count = tlas->GetHeader().GetInteriorFp16NodeCount();

    return kRraOk;
}

RraErrorCode RraTlasGetBox32NodeCount(uint64_t tlas_index, uint32_t* out_node_count)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const auto& tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_node_count = tlas->GetHeader().GetInteriorFp32NodeCount();

    return kRraOk;
}

RraErrorCode RraTlasGetInstanceNodeCount(uint64_t tlas_index, uint64_t* out_instance_count)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const auto& tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_instance_count = tlas->GetNodeCount(rta::BvhNodeFlags::kIsLeafNode);

    return kRraOk;
}

RraErrorCode RraTlasGetInstanceNode(uint64_t tlas_index, uint64_t blas_index, uint64_t instance_index, uint32_t* out_node_id)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    if (instance_index >= tlas->GetInstanceCount(blas_index))
    {
        return kRraErrorIndexOutOfRange;
    }

    uint32_t node_id = tlas->GetInstanceNode(blas_index, instance_index);

    *out_node_id = node_id;
    return kRraOk;
}

RraErrorCode RraTlasGetInstanceNodeTransform(uint64_t tlas_index, uint32_t node_id, float* transform)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    return tlas->GetInstanceNodeTransform(node_id, transform);
}

RraErrorCode RraTlasGetOriginalInstanceNodeTransform(uint64_t tlas_index, uint32_t node_id, float* transform)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    return tlas->GetOriginalInstanceNodeTransform(node_id, transform);
}

RraErrorCode RraTlasGetBlasIndexFromInstanceNode(uint64_t tlas_index, uint32_t node_id, uint64_t* out_blas_index)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    return tlas->GetBlasIndex(node_id, out_blas_index);
}

RraErrorCode RraTlasGetBlasFromInstanceNode(const rta::EncodedRtIp11TopLevelBvh* tlas, uint32_t node_id, const rta::EncodedRtIp11BottomLevelBvh** out_blas)
{
    uint64_t     blas_index = 0;
    RraErrorCode error_code = tlas->GetBlasIndex(node_id, &blas_index);
    if (error_code != kRraOk)
    {
        return error_code;
    }

    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    const auto& bottom_level_bvhs = data_set_.bvh_bundle->GetBottomLevelBvhs();
    if (blas_index >= bottom_level_bvhs.size())
    {
        return kRraErrorInvalidPointer;
    }

    const rta::EncodedRtIp11BottomLevelBvh* blas = dynamic_cast<rta::EncodedRtIp11BottomLevelBvh*>(&(*bottom_level_bvhs[blas_index]));
    if (blas != nullptr)
    {
        *out_blas = blas;
        return kRraOk;
    }
    return kRraErrorInvalidPointer;
}

RraErrorCode RraTlasGetBoundingVolumeExtents(uint64_t tlas_index, uint32_t node_ptr, uint32_t child_index, BoundingVolumeExtents* out_extents)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    return RraTlasGetBoundingVolumeExtentsImpl(tlas, node_ptr, child_index, out_extents);
}

RraErrorCode RraTlasGetSurfaceAreaHeuristic(uint64_t tlas_index, uint32_t node_id, float* out_surface_area_heuristic)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    return RraBvhGetSurfaceAreaHeuristic(tlas, node_id, 0, out_surface_area_heuristic);
}

RraErrorCode RraTlasGetSurfaceAreaImpl(const rta::EncodedRtIp11TopLevelBvh* tlas, uint32_t node_id, uint32_t child_index, float* out_surface_area)
{
    BoundingVolumeExtents extents    = {};
    RraErrorCode          error_code = RraTlasGetBoundingVolumeExtentsImpl(tlas, node_id, child_index, &extents);
    if (error_code == kRraOk)
    {
        error_code = RraBvhGetBoundingVolumeSurfaceArea(&extents, out_surface_area);
    }
    return error_code;
}

RraErrorCode RraTlasGetMinimumSurfaceAreaHeuristic(uint64_t tlas_index, uint32_t node_id, float* out_max_surface_area_heuristic)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_max_surface_area_heuristic = rra::GetMinimumSurfaceAreaHeuristic(tlas, node_id, 0, false);
    return kRraOk;
}

RraErrorCode RraTlasGetAverageSurfaceAreaHeuristic(uint64_t tlas_index, uint32_t node_ptr, float* out_avg_surface_area_heuristic)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_avg_surface_area_heuristic = rra::GetAverageSurfaceAreaHeuristic(tlas, node_ptr, 0, false);
    return kRraOk;
}

RraErrorCode RraTlasGetNodeTransformedSurfaceArea(const rta::EncodedRtIp11TopLevelBvh* tlas,
                                                  uint32_t                             node_id,
                                                  const rta::IBvh*                     volume_bvh,
                                                  float*                               out_surface_area)
{
    dxr::amd::AxisAlignedBoundingBox bounding_box;

    uint32_t     root_node = 0;
    RraErrorCode result    = RraBvhGetRootNodePtr(&root_node);
    if (result != kRraOk)
    {
        return result;
    }

    result = RraBvhGetNodeBoundingVolume(volume_bvh, node_id, 0, 0, bounding_box);  // Pass 0 since it's unused by TLAS.
    if (result != kRraOk)
    {
        return result;
    }

    const dxr::amd::InstanceNode* instance_node = nullptr;
    result                                      = tlas->GetInstanceNodeFromInstancePointer(node_id, &instance_node);
    if (result != kRraOk)
    {
        return result;
    }

    const dxr::Matrix3x4& transform = instance_node->GetExtraData().GetOriginalInstanceTransform();

    BoundingVolumeExtents extents = rra::math_util::TransformAABB(bounding_box, transform);

    return RraBvhGetBoundingVolumeSurfaceArea(&extents, out_surface_area);
}

RraErrorCode RraTlasGetInstanceIndexFromInstanceNode(uint64_t tlas_index, uint32_t node_id, uint32_t* out_instance_index)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    return tlas->GetInstanceIndex(node_id, out_instance_index);
}

RraErrorCode RraTlasGetUniqueInstanceIndexFromInstanceNode(uint64_t tlas_index, uint32_t node_id, uint32_t* out_instance_index)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    uint32_t instance_index = tlas->GetInstanceIndex(node_id);
    if (instance_index == UINT_MAX)
    {
        return kRraErrorIndexOutOfRange;
    }

    *out_instance_index = instance_index;
    return kRraOk;
}

RraErrorCode RraTlasGetInstanceNodeMask(uint64_t tlas_index, uint32_t node_id, uint32_t* out_mask)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    return tlas->GetInstanceNodeMask(node_id, out_mask);
}

RraErrorCode RraTlasGetInstanceNodeID(uint64_t tlas_index, uint32_t node_id, uint32_t* out_id)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    return tlas->GetInstanceNodeID(node_id, out_id);
}

RraErrorCode RraTlasGetInstanceNodeHitGroup(uint64_t tlas_index, uint32_t node_id, uint32_t* out_hit_group)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    return tlas->GetInstanceNodeHitGroup(node_id, out_hit_group);
}

RraErrorCode RraTlasGetSizeInBytes(uint64_t tlas_index, uint32_t* out_size_in_bytes)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_size_in_bytes = tlas->GetHeader().GetFileSize();
    return kRraOk;
}

RraErrorCode RraTlasGetEffectiveSizeInBytes(uint64_t tlas_index, uint64_t* out_size_in_bytes)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    // Get the memory size of the TLAS.
    *out_size_in_bytes = tlas->GetHeader().GetFileSize();

    // Add the memory for all referenced BLASes in the TLAS.
    *out_size_in_bytes += tlas->GetReferencedBlasMemorySize();

    return kRraOk;
}

RraErrorCode RraTlasGetTotalTriangleCount(uint64_t tlas_index, uint64_t* triangle_count)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *triangle_count = tlas->GetTotalTriangleCount();

    return kRraOk;
}

RraErrorCode RraTlasGetUniqueTriangleCount(uint64_t tlas_index, uint64_t* triangle_count)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *triangle_count = tlas->GetUniqueTriangleCount();

    return kRraOk;
}

RraErrorCode RraTlasGetTotalProceduralNodeCount(uint64_t tlas_index, uint64_t* out_count)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_count = tlas->GetTotalProceduralNodeCount();

    return kRraOk;
}

RraErrorCode RraTlasGetInactiveInstancesCount(uint64_t tlas_index, uint64_t* inactive_count)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *inactive_count = tlas->GetInactiveInstanceCount();

    return kRraOk;
}

RraErrorCode RraTlasGetBuildFlags(uint64_t tlas_index, VkBuildAccelerationStructureFlagBitsKHR* out_flags)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const rta::IRtIpCommonAccelerationStructureHeader& header = tlas->GetHeader();

    // Internally, the build flags have the same values as those in the vulkan enum passed in, so currently just
    // need to do a simple cast. If the internal structure changes, then the internal flags will need mapping
    // to the vulkan API enum.
    *out_flags = static_cast<VkBuildAccelerationStructureFlagBitsKHR>(header.GetPostBuildInfo().GetBuildFlags());
    return kRraOk;
}

RraErrorCode RraTlasGetRebraidingEnabled(uint64_t tlas_index, bool* out_enabled)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const rta::IRtIpCommonAccelerationStructureHeader& header = tlas->GetHeader();

    *out_enabled = header.GetPostBuildInfo().GetRebraiding();
    return kRraOk;
}

RraErrorCode RraTlasGetInstanceFlags(uint64_t tlas_index, uint32_t node_id, uint32_t* out_flags)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    return tlas->GetInstanceFlags(node_id, out_flags);
}

RraErrorCode RraTlasGetFusedInstancesEnabled(uint64_t tlas_index, bool* out_enabled)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const rta::IRtIpCommonAccelerationStructureHeader& header = tlas->GetHeader();

    *out_enabled = header.GetPostBuildInfo().GetFusedInstances();
    return kRraOk;
}

RraErrorCode RraTlasGetNodeObbIndex(uint64_t tlas_index, uint32_t node_id, uint32_t* obb_index)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    glm::mat3    rotation{};
    RraErrorCode error_code = RraBvhGetNodeObbIndex(tlas, node_id, obb_index);
    return error_code;
}

RraErrorCode RraTlasGetNodeBoundingVolumeOrientation(uint64_t tlas_index, uint32_t node_id, float* out_rotation)
{
    if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() != rta::RayTracingIpLevel::RtIp3_1)
    {
        glm::mat3 identity{glm::mat3(1.0f)};
        std::memcpy(out_rotation, &identity, sizeof(identity));
        return kRraOk;
    }

    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    glm::mat3    rotation{};
    RraErrorCode error_code = RraBvhGetNodeBoundingVolumeOrientation(tlas, node_id, rotation);

    if (error_code != kRraOk)
    {
        return error_code;
    }

    std::memcpy(out_rotation, &rotation, sizeof(glm::mat3));
    return kRraOk;
}

RraErrorCode RraTlasGetMetaDataSize(uint64_t tlas_index, uint32_t* out_byte_size)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_byte_size = tlas->GetMetaData().GetByteSize();
    return kRraOk;
}

bool RraTlasIsBoxNode(uint64_t tlas_index, uint32_t node_id)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return false;
    }
    return RraBvhIsBoxNode(tlas, node_id);
}

bool RraTlasIsBox16Node(uint64_t tlas_index, uint32_t node_id)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return false;
    }
    return RraBvhIsBox16Node(tlas, node_id);
}

bool RraTlasIsBox32Node(uint64_t tlas_index, uint32_t node_id)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return false;
    }
    return RraBvhIsBox32Node(tlas, node_id);
}

bool RraTlasHasChildren(uint64_t tlas_index, uint32_t node_id)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return false;
    }
    return RraBvhHasChildren(tlas, node_id);
}

bool RraTlasIsInstanceNode(uint64_t tlas_index, uint32_t node_id)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return false;
    }

    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return false;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetIsInstanceNode(node_id, tlas);
}

bool RraTlasIsPartitioned(uint64_t tlas_index)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return false;
    }
    return tlas->IsPartitioned();
}

bool RraTlasHasNodePacking(uint64_t tlas_index)
{
    const auto* tlas = dynamic_cast<const rta::EncodedRtIp31TopLevelBvh*>(RraTlasGetTlasFromTlasIndex(tlas_index));
    if (tlas == nullptr)
    {
        return false;
    }
    return tlas->HasNodePacking();
}

RraErrorCode RraTlasGetPartitionCount(uint64_t tlas_index, uint32_t* out_count)
{
    RRA_ASSERT(out_count != nullptr);
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr || out_count == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    *out_count = tlas->GetPartitionCount();
    return kRraOk;
}

RraErrorCode RraTlasGetMaxPartitionInstances(uint64_t tlas_index, uint32_t* out_max)
{
    RRA_ASSERT(out_max != nullptr);
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr || out_max == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    *out_max = tlas->GetMaxPartitionInstances();
    return kRraOk;
}

RraErrorCode RraTlasGetMaxGlobalInstances(uint64_t tlas_index, uint32_t* out_max)
{
    RRA_ASSERT(out_max != nullptr);
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr || out_max == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    *out_max = tlas->GetMaxGlobalInstances();
    return kRraOk;
}

RraErrorCode RraTlasGetPartitionInfo(uint64_t tlas_index, uint32_t partition_index, RraPartitionInfo* out_info)
{
    RRA_ASSERT(out_info != nullptr);
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr || out_info == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const rta::EncodedTopLevelBvh::PartitionInfo* info = tlas->GetPartitionInfo(partition_index);
    if (info == nullptr)
    {
        return kRraErrorIndexOutOfRange;
    }

    out_info->partition_index     = partition_index;
    out_info->instance_count      = info->instance_count;
    out_info->internal_node_count = info->internal_node_count;
    out_info->fat_leaf_count      = info->fat_leaf_count;
    out_info->bounds_valid        = info->bounds_valid;
    out_info->is_global           = (partition_index == tlas->GetPartitionCount());
    for (int c = 0; c < 3; ++c)
    {
        out_info->translation[c] = info->translation[c];
        out_info->bounds_min[c]  = info->bounds_min[c];
        out_info->bounds_max[c]  = info->bounds_max[c];
    }

    return kRraOk;
}

RraErrorCode RraTlasGetInstancePartitionIndex(uint64_t tlas_index, uint32_t instance_index, uint32_t* out_partition_index, bool* out_active)
{
    RRA_ASSERT(out_partition_index != nullptr);
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr || out_partition_index == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    if (!tlas->GetInstancePartitionIndex(instance_index, out_partition_index, out_active))
    {
        return kRraErrorIndexOutOfRange;
    }
    return kRraOk;
}

