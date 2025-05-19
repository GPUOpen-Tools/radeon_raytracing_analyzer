//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include "bvh/rtip11/encoded_rt_ip_11_bottom_level_bvh.h"
#include "bvh/rtip11/encoded_rt_ip_11_top_level_bvh.h"
#include "bvh/rtip31/encoded_rt_ip_31_top_level_bvh.h"
#include "math_util.h"
#include "rra_blas_impl.h"
#include "rra_bvh_impl.h"
#include "rra_data_set.h"
#include "surface_area_heuristic.h"

// External reference to the global dataset.
extern RraDataSet data_set_;

static RraErrorCode GetInstanceNodeFromInstancePointer(const rta::EncodedRtIp11TopLevelBvh* tlas,
                                                       const dxr::amd::NodePointer*         node,
                                                       const dxr::amd::InstanceNode**       out_instance_node)
{
    if (!node->IsInstanceNode())
    {
        return kRraErrorInvalidPointer;
    }

    const dxr::amd::InstanceNode* instance_node = tlas->GetInstanceNode(node);
    if (instance_node == nullptr)
    {
        return kRraErrorIndexOutOfRange;
    }
    *out_instance_node = instance_node;

    return kRraOk;
}

static RraErrorCode GetHwInstanceNodeFromInstancePointer(const rta::EncodedRtIp31TopLevelBvh* tlas,
                                                         const dxr::amd::NodePointer*         node,
                                                         InstanceNodeDataRRA*                 out_instance_node)
{
    if (!node->IsInstanceNode())
    {
        return kRraErrorInvalidPointer;
    }

    auto instance_node = tlas->GetHwInstanceNode(node);
    if (!instance_node)
    {
        return kRraErrorIndexOutOfRange;
    }
    *out_instance_node = instance_node.value();

    return kRraOk;
}

static RraErrorCode GetBlasIndexFromInstanceNodeImpl(const rta::EncodedTopLevelBvh* tlas, const dxr::amd::NodePointer* node, uint64_t* out_blas_index)
{
    if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
    {
        InstanceNodeDataRRA instance_node{};
        auto                result = GetHwInstanceNodeFromInstancePointer((rta::EncodedRtIp31TopLevelBvh*)tlas, node, &instance_node);
        if (result != kRraOk)
        {
            return result;
        }

        NodePointer64 temp_ptr;
        temp_ptr.u64 = instance_node.hw_instance_node.data.childBasePtr;
        // also shifted by 6 because it is aligned to 64.
        auto blas_address = (temp_ptr.aligned_addr_64b << 6);
        auto opt_index    = data_set_.bvh_bundle->GetBlasIndexFromVirtualAddress(blas_address);
        if (!opt_index)
        {
            return kRraErrorInvalidPointer;
        }

        *out_blas_index = opt_index.value();
    }
    else
    {
        const dxr::amd::InstanceNode* instance_node = nullptr;
        auto                          result        = GetInstanceNodeFromInstancePointer((rta::EncodedRtIp11TopLevelBvh*)tlas, node, &instance_node);
        if (result != kRraOk)
        {
            return result;
        }

        const auto& desc = instance_node->GetDesc();
        *out_blas_index  = desc.GetBottomLevelBvhGpuVa(dxr::InstanceDescType::kRaw) >> 3;
    }

    return kRraOk;
}

static RraErrorCode RraTlasGetBoundingVolumeExtentsImpl(const rta::IBvh* tlas, const dxr::amd::NodePointer* node_ptr, BoundingVolumeExtents* out_extents)
{
    dxr::amd::AxisAlignedBoundingBox bounding_box;

    RraErrorCode error_code = RraBvhGetNodeBoundingVolume(tlas, node_ptr, bounding_box);
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

    return reinterpret_cast<rta::EncodedTopLevelBvh*>(tlas_ptr);
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

RraErrorCode RraTlasGetChildNodePtr(uint64_t tlas_index, uint32_t parent_node, uint32_t child_index, uint32_t* out_node_ptr)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    return RraBvhGetChildNodePtr(tlas, parent_node, child_index, out_node_ptr);
}

RraErrorCode RraTlasGetNodeName(uint32_t node_ptr, const char** out_name)
{
    dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    switch ((uint32_t)node->GetType())
    {
    case (uint32_t)dxr::amd::NodeType::kAmdNodeBoxFp16:
        *out_name = "Box16";
        break;

    case (uint32_t)dxr::amd::NodeType::kAmdNodeBoxFp32:
        if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
        {
            *out_name = "Bvh8";
            break;
        }
        else
        {
            *out_name = "Box32";
            break;
        }

    case (uint32_t)dxr::amd::NodeType::kAmdNodeInstance:
        *out_name = "Instance";
        break;

    default:
        *out_name = "Unknown";
        break;
    }

    return kRraOk;
}

RraErrorCode RraTlasGetNodeNameToolTip(uint32_t node_ptr, const char** out_tooltip)
{
    dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    switch ((uint32_t)node->GetType())
    {
    case (uint32_t)dxr::amd::NodeType::kAmdNodeBoxFp16:
        *out_tooltip = "A 16-bit floating point bounding volume node with up to 4 child nodes";
        break;

    case (uint32_t)dxr::amd::NodeType::kAmdNodeBoxFp32:
        if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
        {
            *out_tooltip = "A compressed bounding volume node with up to 8 child nodes";
            break;
        }
        else
        {
            *out_tooltip = "A 32-bit floating point bounding volume node with up to 4 child nodes";
            break;
        }

    case (uint32_t)dxr::amd::NodeType::kAmdNodeInstance:
        *out_tooltip = "A node containing an instance of a BLAS. Double-click to view this instance in the BLAS viewer";
        break;

    default:
        *out_tooltip = "";
        break;
    }

    return kRraOk;
}

RraErrorCode RraTlasGetNodeBaseAddress(uint64_t tlas_index, uint32_t node_ptr, uint64_t* out_address)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const auto                   base_addr = tlas->GetVirtualAddress();
    const dxr::amd::NodePointer* node      = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    *out_address = base_addr + tlas->GetHeader().GetMetaDataSize() + node->GetGpuVirtualAddress();

    return kRraOk;
}

RraErrorCode RraTlasGetNodeParent(uint64_t tlas_index, uint32_t node_ptr, uint32_t* out_parent_node_ptr)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    const auto& interior_nodes = tlas->GetInteriorNodesData();
    if (interior_nodes.size() == 0)
    {
        return kRraErrorInvalidPointer;
    }

    dxr::amd::NodePointer parent_node = tlas->GetParentNode(node);
    *out_parent_node_ptr              = *reinterpret_cast<uint32_t*>(&parent_node);

    return kRraOk;
}

RraErrorCode RraTlasGetInstanceNodeInfo(uint64_t tlas_index, uint32_t node_ptr, uint64_t* out_blas_address, uint64_t* out_instance_count, bool* out_is_empty)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);

    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    const auto& bottom_level_bvhs = data_set_.bvh_bundle->GetBottomLevelBvhs();
    if (tlas_index >= bottom_level_bvhs.size())
    {
        return kRraErrorInvalidPointer;
    }
    uint64_t blas_index{};

    if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
    {
        rta::EncodedRtIp31TopLevelBvh* rtip3_tlas = (rta::EncodedRtIp31TopLevelBvh*)tlas;
        InstanceNodeDataRRA            hw_instance_node{};
        RraErrorCode                   result = GetHwInstanceNodeFromInstancePointer(rtip3_tlas, node, &hw_instance_node);
        RRA_ASSERT(result == kRraOk);

        NodePointer64 temp_ptr{};
        temp_ptr.u64 = hw_instance_node.hw_instance_node.data.childBasePtr;
        // also shifted by 6 because it is aligned to 64.
        uint64_t blas_address = (temp_ptr.aligned_addr_64b << 6);

        blas_index = rtip3_tlas->BlasAddressToIndex(blas_address);
    }
    else
    {
        const dxr::amd::InstanceNode* instance_node = nullptr;
        RraErrorCode                  result        = GetInstanceNodeFromInstancePointer((rta::EncodedRtIp11TopLevelBvh*)tlas, node, &instance_node);

        if (result != kRraOk)
        {
            return result;
        }

        const auto& desc = instance_node->GetDesc();
        blas_index       = desc.GetBottomLevelBvhGpuVa(dxr::InstanceDescType::kRaw) >> 3;
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

RraErrorCode RraTlasGetInstanceNode(uint64_t tlas_index, uint64_t blas_index, uint64_t instance_index, uint32_t* out_node_ptr)
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

    dxr::amd::NodePointer ptr = tlas->GetInstanceNode(blas_index, instance_index);
    if (ptr.IsInvalid())
    {
        return kRraErrorInvalidPointer;
    }

    *out_node_ptr = ptr.GetRawPointer();
    return kRraOk;
}

RraErrorCode RraTlasGetInstanceNodeTransform(uint64_t tlas_index, uint32_t node_ptr, float* transform)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
    {
        InstanceNodeDataRRA instance_node{};
        RraErrorCode        result = GetHwInstanceNodeFromInstancePointer((rta::EncodedRtIp31TopLevelBvh*)tlas, node, &instance_node);

        if (result != kRraOk)
        {
            return result;
        }

        memcpy(transform, instance_node.hw_instance_node.data.worldToObject, dxr::kMatrix3x4Size);
    }
    else
    {
        const dxr::amd::InstanceNode* instance_node = nullptr;
        auto                          result        = GetInstanceNodeFromInstancePointer((rta::EncodedRtIp11TopLevelBvh*)tlas, node, &instance_node);
        if (result != kRraOk)
        {
            return result;
        }

        const auto&    desc          = instance_node->GetDesc();
        dxr::Matrix3x4 dxr_transform = desc.GetTransform();
        memcpy(transform, dxr_transform.data(), dxr::kMatrix3x4Size);
    }

    return kRraOk;
}

RraErrorCode RraTlasGetOriginalInstanceNodeTransform(uint64_t tlas_index, uint32_t node_ptr, float* transform)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
    {
        InstanceNodeDataRRA instance_node{};
        RraErrorCode        result = GetHwInstanceNodeFromInstancePointer((rta::EncodedRtIp31TopLevelBvh*)tlas, node, &instance_node);

        if (result != kRraOk)
        {
            return result;
        }

        memcpy(transform, instance_node.sideband.objectToWorld, dxr::kMatrix3x4Size);
    }
    else
    {
        const dxr::amd::InstanceNode* instance_node = nullptr;
        auto                          result        = GetInstanceNodeFromInstancePointer((rta::EncodedRtIp11TopLevelBvh*)tlas, node, &instance_node);
        if (result != kRraOk)
        {
            return result;
        }

        dxr::Matrix3x4 original_transform = instance_node->GetExtraData().GetOriginalInstanceTransform();
        memcpy(transform, original_transform.data(), dxr::kMatrix3x4Size);
    }
    return kRraOk;
}

RraErrorCode RraTlasGetBlasIndexFromInstanceNode(uint64_t tlas_index, uint32_t node_ptr, uint64_t* out_blas_index)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    return GetBlasIndexFromInstanceNodeImpl(tlas, node, out_blas_index);
}

RraErrorCode RraTlasGetBlasFromInstanceNode(const rta::EncodedRtIp11TopLevelBvh*     tlas,
                                            const dxr::amd::NodePointer*             node_ptr,
                                            const rta::EncodedRtIp11BottomLevelBvh** out_blas)
{
    uint64_t     blas_index = 0;
    RraErrorCode error_code = GetBlasIndexFromInstanceNodeImpl(tlas, node_ptr, &blas_index);
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

RraErrorCode RraTlasGetBoundingVolumeExtents(uint64_t tlas_index, uint32_t node_ptr, BoundingVolumeExtents* out_extents)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    return RraTlasGetBoundingVolumeExtentsImpl(tlas, current_node, out_extents);
}

RraErrorCode RraTlasGetSurfaceAreaHeuristic(uint64_t tlas_index, uint32_t node_ptr, float* out_surface_area_heuristic)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);
    return RraBvhGetSurfaceAreaHeuristic(tlas, *current_node, out_surface_area_heuristic);
}

RraErrorCode RraTlasGetSurfaceAreaImpl(const rta::EncodedRtIp11TopLevelBvh* tlas, const dxr::amd::NodePointer* node_ptr, float* out_surface_area)
{
    BoundingVolumeExtents extents    = {};
    RraErrorCode          error_code = RraTlasGetBoundingVolumeExtentsImpl(tlas, node_ptr, &extents);
    if (error_code == kRraOk)
    {
        error_code = RraBvhGetBoundingVolumeSurfaceArea(&extents, out_surface_area);
    }
    return error_code;
}

RraErrorCode RraTlasGetMinimumSurfaceAreaHeuristic(uint64_t tlas_index, uint32_t node_ptr, float* out_max_surface_area_heuristic)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    *out_max_surface_area_heuristic = rra::GetMinimumSurfaceAreaHeuristic(tlas, *current_node, false);
    return kRraOk;
}

RraErrorCode RraTlasGetAverageSurfaceAreaHeuristic(uint64_t tlas_index, uint32_t node_ptr, float* out_avg_surface_area_heuristic)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    *out_avg_surface_area_heuristic = rra::GetAverageSurfaceAreaHeuristic(tlas, *current_node, false);
    return kRraOk;
}

RraErrorCode RraTlasGetNodeTransformedSurfaceArea(const rta::EncodedRtIp11TopLevelBvh* tlas,
                                                  const dxr::amd::NodePointer*         node_ptr,
                                                  const rta::IBvh*                     volume_bvh,
                                                  float*                               out_surface_area)
{
    if (!node_ptr->IsInstanceNode())
    {
        return kRraErrorInvalidPointer;
    }

    dxr::amd::AxisAlignedBoundingBox bounding_box;

    uint32_t     root_node = 0;
    RraErrorCode result    = RraBvhGetRootNodePtr(&root_node);
    if (result != kRraOk)
    {
        return result;
    }

    result = RraBvhGetNodeBoundingVolume(volume_bvh, reinterpret_cast<dxr::amd::NodePointer*>(&root_node), bounding_box);
    if (result != kRraOk)
    {
        return result;
    }

    const dxr::amd::InstanceNode* instance_node = nullptr;
    result                                      = GetInstanceNodeFromInstancePointer(tlas, node_ptr, &instance_node);
    if (result != kRraOk)
    {
        return result;
    }

    const dxr::Matrix3x4& transform = instance_node->GetExtraData().GetOriginalInstanceTransform();

    BoundingVolumeExtents extents = rra::math_util::TransformAABB(bounding_box, transform);

    return RraBvhGetBoundingVolumeSurfaceArea(&extents, out_surface_area);
}

RraErrorCode RraTlasGetInstanceIndexFromInstanceNode(uint64_t tlas_index, uint32_t node_ptr, uint32_t* out_instance_index)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    if (!node->IsInstanceNode())
    {
        return kRraErrorInvalidPointer;
    }

    if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
    {
        InstanceNodeDataRRA instance_node{};
        RraErrorCode        result = GetHwInstanceNodeFromInstancePointer((rta::EncodedRtIp31TopLevelBvh*)tlas, node, &instance_node);

        if (result != kRraOk)
        {
            return result;
        }

        *out_instance_index = instance_node.sideband.instanceIndex;
    }
    else
    {
        const dxr::amd::InstanceNode* instance_node = nullptr;
        RraErrorCode                  result        = GetInstanceNodeFromInstancePointer((rta::EncodedRtIp11TopLevelBvh*)tlas, node, &instance_node);
        if (result != kRraOk)
        {
            return result;
        }

        *out_instance_index = instance_node->GetExtraData().GetInstanceIndex();
    }
    return kRraOk;
}

RraErrorCode RraTlasGetUniqueInstanceIndexFromInstanceNode(uint64_t tlas_index, uint32_t node_ptr, uint32_t* out_instance_index)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    uint32_t instance_index = tlas->GetInstanceIndex(node);
    if (instance_index == UINT_MAX)
    {
        return kRraErrorIndexOutOfRange;
    }

    *out_instance_index = instance_index;
    return kRraOk;
}

RraErrorCode RraTlasGetInstanceNodeMask(uint64_t tlas_index, uint32_t node_ptr, uint32_t* out_mask)
{
    dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
    {
        InstanceNodeDataRRA instance_node{};
        RraErrorCode        result = GetHwInstanceNodeFromInstancePointer((rta::EncodedRtIp31TopLevelBvh*)tlas, node, &instance_node);

        if (result != kRraOk)
        {
            return result;
        }

        *out_mask = instance_node.hw_instance_node.data.userDataAndInstanceMask >> 24;
    }
    else
    {
        const dxr::amd::InstanceNode* instance_node{};
        RraErrorCode                  error_code = GetInstanceNodeFromInstancePointer((rta::EncodedRtIp11TopLevelBvh*)tlas, node, &instance_node);
        if (error_code != kRraOk)
        {
            return error_code;
        }

        *out_mask = instance_node->GetDesc().GetMask();
    }

    return kRraOk;
}

RraErrorCode RraTlasGetInstanceNodeID(uint64_t tlas_index, uint32_t node_ptr, uint32_t* out_id)
{
    dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
    {
        InstanceNodeDataRRA instance_node{};
        RraErrorCode        result = GetHwInstanceNodeFromInstancePointer((rta::EncodedRtIp31TopLevelBvh*)tlas, node, &instance_node);

        if (result != kRraOk)
        {
            return result;
        }

        constexpr uint32_t instance_id_bit_mask{0xFFFFFF};
        *out_id = instance_node.sideband.instanceIdAndFlags & instance_id_bit_mask;
    }
    else
    {
        const dxr::amd::InstanceNode* instance_node{};
        RraErrorCode                  error_code = GetInstanceNodeFromInstancePointer((rta::EncodedRtIp11TopLevelBvh*)tlas, node, &instance_node);
        if (error_code != kRraOk)
        {
            return error_code;
        }

        *out_id = instance_node->GetDesc().GetInstanceID();
    }

    return kRraOk;
}

RraErrorCode RraTlasGetInstanceNodeHitGroup(uint64_t tlas_index, uint32_t node_ptr, uint32_t* out_hit_group)
{
    dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
    {
        InstanceNodeDataRRA instance_node{};
        RraErrorCode        result = GetHwInstanceNodeFromInstancePointer((rta::EncodedRtIp31TopLevelBvh*)tlas, node, &instance_node);

        if (result != kRraOk)
        {
            return result;
        }

        constexpr uint32_t user_data_bit_mask{0xFFFFFF};
        *out_hit_group = instance_node.hw_instance_node.data.userDataAndInstanceMask & user_data_bit_mask;
    }
    else
    {
        const dxr::amd::InstanceNode* instance_node{};
        RraErrorCode                  error_code = GetInstanceNodeFromInstancePointer((rta::EncodedRtIp11TopLevelBvh*)tlas, node, &instance_node);
        if (error_code != kRraOk)
        {
            return error_code;
        }

        *out_hit_group = instance_node->GetDesc().GetHitGroup();
    }

    return kRraOk;
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

RraErrorCode RraTlasGetInstanceFlags(uint64_t tlas_index, uint32_t node_ptr, uint32_t* out_flags)
{
    dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
    {
        InstanceNodeDataRRA instance_node{};
        RraErrorCode        result = GetHwInstanceNodeFromInstancePointer((rta::EncodedRtIp31TopLevelBvh*)tlas, node, &instance_node);

        if (result != kRraOk)
        {
            return result;
        }

        constexpr uint32_t user_data_bit_mask{0xFFFFFF};
        RRA_UNUSED(user_data_bit_mask);
        *out_flags = instance_node.sideband.instanceIdAndFlags >> 24;
    }
    else
    {
        const dxr::amd::InstanceNode* instance_node{};
        RraErrorCode                  error_code = GetInstanceNodeFromInstancePointer((rta::EncodedRtIp11TopLevelBvh*)tlas, node, &instance_node);
        if (error_code != kRraOk)
        {
            return error_code;
        }

        *out_flags = static_cast<uint32_t>(instance_node->GetDesc().GetInstanceFlags());
    }

    return kRraOk;
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

RraErrorCode RraTlasGetNodeObbIndex(uint64_t tlas_index, uint32_t node_ptr, uint32_t* obb_index)
{
    const rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    glm::mat3    rotation{};
    RraErrorCode error_code = RraBvhGetNodeObbIndex(tlas, node, obb_index);
    return error_code;
}

RraErrorCode RraTlasGetNodeBoundingVolumeOrientation(uint64_t tlas_index, uint32_t node_ptr, float* out_rotation)
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

    const dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    glm::mat3    rotation{};
    RraErrorCode error_code = RraBvhGetNodeBoundingVolumeOrientation(tlas, node, rotation);

    if (error_code != kRraOk)
    {
        return error_code;
    }

    std::memcpy(out_rotation, &rotation, sizeof(glm::mat3));
    return kRraOk;
}

