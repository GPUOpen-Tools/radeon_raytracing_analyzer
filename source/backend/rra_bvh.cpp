//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the BVH interface.
///
/// Contains public functions common to all acceleration structures.
//=============================================================================

#include "rra_bvh_impl.h"

#include <float.h>

#include "bvh/ibvh.h"
#include "bvh/rtip_common/encoded_bottom_level_bvh.h"
#include "bvh/dxr_definitions.h"
#include "public/rra_assert.h"
#include "rra_data_set.h"
#include "bvh/gpu_def.h"
#include "public/rra_rtip_info.h"
#include "bvh/rtip31/internal_node.h"
#include "bvh/rtip31/child_info.h"
#include "bvh/rtip31/primitive_node.h"

// External reference to the global dataset.
extern RraDataSet data_set_;

RraErrorCode RraBvhGetNodeBoundingVolume(const rta::IBvh* bvh, const dxr::amd::NodePointer* node_ptr, dxr::amd::AxisAlignedBoundingBox& out_bounding_box)
{
    if (node_ptr->IsInvalid())
    {
        return kRraErrorInvalidPointer;
    }

    const auto& interior_nodes = bvh->GetInteriorNodesData();
    if (interior_nodes.size() == 0)
    {
        return kRraErrorInvalidPointer;
    }

    if (bvh->IsEmpty())
    {
        return kRraErrorInvalidPointer;
    }

    dxr::amd::NodePointer parent_node = bvh->GetParentNode(node_ptr);

    // Need to get the node parent, and look for the node in the children of the parent, since that's where
    // the bounding box info is stored.
    if (parent_node.IsInvalid())
    {
        if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
        {
            // TODO: Need to calculate the OBB, this function only returns AABB. Needs some reworking.
            const auto node  = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[0]);
            out_bounding_box = bvh->ComputeRootNodeBoundingBox(node);
            return kRraOk;
        }
        else
        {
            const auto node  = reinterpret_cast<const dxr::amd::Float32BoxNode*>(&interior_nodes[0]);
            out_bounding_box = bvh->ComputeRootNodeBoundingBox(node);
            return kRraOk;
        }
    }
    else
    {
        uint64_t parent_index;
        if (parent_node.IsBoxNode())
        {
            parent_index = parent_node.GetByteOffset() - bvh->GetHeader().GetBufferOffsets().interior_nodes;
        }
        else
        {
            parent_index = parent_node.GetByteOffset() - bvh->GetHeader().GetBufferOffsets().leaf_nodes;
        }

        if (parent_node.IsFp16BoxNode())
        {
            const dxr::amd::Float16BoxNode* box_node    = reinterpret_cast<const dxr::amd::Float16BoxNode*>(&interior_nodes[parent_index]);
            const auto&                     child_array = box_node->GetChildren();
            const auto&                     bbox_array  = box_node->GetBoundingBoxes();
            for (auto child_index = 0; child_index < 4; child_index++)
            {
                if (child_array[child_index].GetRawPointer() == node_ptr->GetRawPointer())
                {
                    out_bounding_box = bbox_array[child_index];
                    return kRraOk;
                }
            }
        }
        else if (parent_node.IsFp32BoxNode())
        {
            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
            {
                QuantizedBVH8BoxNode node = *reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[parent_index]);
                uint32_t             child_nodes[8]{};
                node.DecodeChildrenOffsets(child_nodes);

                for (uint32_t i{0}; i < node.ValidChildCount(); ++i)
                {
                    if (child_nodes[i] == node_ptr->GetRawPointer())
                    {
                        auto bbox = node.childInfos[i].DecodeBounds(node.Origin(), node.Exponents());

                        out_bounding_box.min.x = bbox.min.x;
                        out_bounding_box.min.y = bbox.min.y;
                        out_bounding_box.min.z = bbox.min.z;

                        out_bounding_box.max.x = bbox.max.x;
                        out_bounding_box.max.y = bbox.max.y;
                        out_bounding_box.max.z = bbox.max.z;
                        return kRraOk;
                    }
                }
            }
            else
            {
                const auto  box_node    = reinterpret_cast<const dxr::amd::Float32BoxNode*>(&interior_nodes[parent_index]);
                const auto& child_array = box_node->GetChildren();
                const auto& bbox_array  = box_node->GetBoundingBoxes();
                for (auto child_index = 0; child_index < 4; child_index++)
                {
                    if (child_array[child_index].GetRawPointer() == node_ptr->GetRawPointer())
                    {
                        out_bounding_box = bbox_array[child_index];
                        return kRraOk;
                    }
                }
            }
        }
    }
    return kRraErrorInvalidPointer;
}

RraErrorCode RraBvhGetNodeObbIndex(const rta::IBvh* bvh, const dxr::amd::NodePointer* node_ptr, uint32_t* obb_index)
{
    if (node_ptr->IsInvalid())
    {
        return kRraErrorInvalidPointer;
    }

    *obb_index = bvh->GetNodeObbIndex(*node_ptr);
    return kRraOk;
}

RraErrorCode RraBvhGetNodeBoundingVolumeOrientation(const rta::IBvh* bvh, const dxr::amd::NodePointer* node_ptr, glm::mat3& out_rotation)
{
    if (node_ptr->IsInvalid())
    {
        return kRraErrorInvalidPointer;
    }

    out_rotation = bvh->GetNodeBoundingVolumeOrientation(*node_ptr);
    return kRraOk;
}

RraErrorCode RraBvhGetRootNodePtr(uint32_t* out_node_ptr)
{
    dxr::amd::NodePointer root_ptr = dxr::amd::NodePointer(dxr::amd::NodeType::kAmdNodeBoxFp32, dxr::amd::kAccelerationStructureHeaderSize);
    *out_node_ptr                  = root_ptr.GetRawPointer();

    return kRraOk;
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

RraErrorCode RraBvhGetNodeOffset(uint32_t node_ptr, uint64_t* out_offset)
{
    dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    *out_offset = node->GetGpuVirtualAddress();
    return kRraOk;
}

bool RraBvhIsBoxNode(uint32_t node_ptr)
{
    dxr::amd::NodeType node_type = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr)->GetType();

    return node_type == dxr::amd::NodeType::kAmdNodeBoxFp32 || node_type == dxr::amd::NodeType::kAmdNodeBoxFp16;
}

bool RraBvhIsBox16Node(uint32_t node_ptr)
{
    dxr::amd::NodeType node_type = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr)->GetType();

    return node_type == dxr::amd::NodeType::kAmdNodeBoxFp16;
}

bool RraBvhIsBox32Node(uint32_t node_ptr)
{
    dxr::amd::NodeType node_type = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr)->GetType();

    return node_type == dxr::amd::NodeType::kAmdNodeBoxFp32;
}

bool RraBvhIsInstanceNode(uint32_t node_ptr)
{
    dxr::amd::NodeType node_type = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr)->GetType();

    return node_type == dxr::amd::NodeType::kAmdNodeInstance;
}

bool RraBvhIsProceduralNode(uint32_t node_ptr)
{
    dxr::amd::NodeType node_type = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr)->GetType();

    return node_type == dxr::amd::NodeType::kAmdNodeProcedural;
}

RraErrorCode RraBvhGetBoundingVolumeSurfaceArea(const rta::IBvh* bvh, const dxr::amd::NodePointer* node_ptr, float* out_surface_area)
{
    dxr::amd::AxisAlignedBoundingBox bounding_box;
    RraErrorCode                     result = RraBvhGetNodeBoundingVolume(bvh, node_ptr, bounding_box);
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

RraErrorCode RraBvhGetSurfaceAreaHeuristic(const rta::IBvh* bvh, const dxr::amd::NodePointer node_ptr, float* out_surface_area_heuristic)
{
    auto blas = dynamic_cast<const rta::EncodedBottomLevelBvh*>(bvh);
    if (blas && blas->IsProcedural())
    {
        *out_surface_area_heuristic = 1.0f;
        return kRraOk;
    }

    if (node_ptr.IsInvalid())
    {
        return kRraErrorInvalidPointer;
    }

    if (bvh->IsEmpty())
    {
        *out_surface_area_heuristic = 0.0f;
        return kRraOk;
    }

    if (node_ptr.IsBoxNode())
    {
        *out_surface_area_heuristic = bvh->GetInteriorNodeSurfaceAreaHeuristic(node_ptr);
        return kRraOk;
    }
    else
    {
        *out_surface_area_heuristic = bvh->GetLeafNodeSurfaceAreaHeuristic(node_ptr);
        return kRraOk;
    }
}

RraErrorCode RraBvhIsTriangleNode(rta::IBvh* bvh, uint32_t node_ptr, bool* out_is_triangle)
{
    rta::EncodedBottomLevelBvh* blas = dynamic_cast<rta::EncodedBottomLevelBvh*>(bvh);
    if (blas && blas->IsProcedural())
    {
        *out_is_triangle = false;
        return kRraOk;
    }

    dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    switch ((uint32_t)node->GetType())
    {
    case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle0:
    case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle1:
    case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle2:
    case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle3:
    case NODE_TYPE_TRIANGLE_4:
    case NODE_TYPE_TRIANGLE_5:
    case NODE_TYPE_TRIANGLE_6:
    case NODE_TYPE_TRIANGLE_7:
        // Return true if the node is any of the valid triangle node types.
        *out_is_triangle = true;
        break;
    default:
        // If it's not a triangle node type, don't do anything.
        break;
    }

    return kRraOk;
}

RraErrorCode RraBvhGetChildNodeCount(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_count)
{
    const auto&            header_offsets = bvh->GetHeader().GetBufferOffsets();
    dxr::amd::NodePointer* node_ptr       = reinterpret_cast<dxr::amd::NodePointer*>(&parent_node);
    auto                   byte_offset    = node_ptr->GetByteOffset() - header_offsets.interior_nodes;
    const auto&            interior_nodes = bvh->GetInteriorNodesData();

    if (interior_nodes.size() > byte_offset)
    {
        if (node_ptr->IsFp32BoxNode())
        {
            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
            {
                const auto node  = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[byte_offset]);
                *out_child_count = node->ValidChildCount();
            }
            else
            {
                const auto node  = reinterpret_cast<const dxr::amd::Float32BoxNode*>(&interior_nodes[byte_offset]);
                *out_child_count = node->GetValidChildCount();
            }
        }
        else if (node_ptr->IsFp16BoxNode())
        {
            const auto node  = reinterpret_cast<const dxr::amd::Float16BoxNode*>(&interior_nodes[byte_offset]);
            *out_child_count = node->GetValidChildCount();
        }
        else
        {
            *out_child_count = 0;
        }
    }
    else
    {
        *out_child_count = 0;
    }
    return kRraOk;
}

RraErrorCode RraBvhGetChildNodes(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_nodes)
{
    const auto&            header_offsets = bvh->GetHeader().GetBufferOffsets();
    dxr::amd::NodePointer* node_ptr       = reinterpret_cast<dxr::amd::NodePointer*>(&parent_node);
    auto                   byte_offset    = node_ptr->GetByteOffset() - header_offsets.interior_nodes;
    const auto&            interior_nodes = bvh->GetInteriorNodesData();

    if (interior_nodes.size() > byte_offset)
    {
        if (node_ptr->IsFp32BoxNode())
        {
            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
            {
                const auto node = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[byte_offset]);
                uint32_t   child_nodes[8]{};
                node->DecodeChildrenOffsets(child_nodes);

                // DecodeChildrenOffsets writes to all 8 children slots, but out_child_nodes only has allocated number of valid children.
                for (uint32_t i{0}; i < node->ValidChildCount(); ++i)
                {
                    if (!dxr::amd::NodePointer(child_nodes[i]).IsInvalid())
                    {
                        out_child_nodes[i] = child_nodes[i];
                    }
                }
            }
            else
            {
                const auto node     = reinterpret_cast<const dxr::amd::Float32BoxNode*>(&interior_nodes[byte_offset]);
                const auto children = node->GetChildren();
                for (size_t i = 0; i < children.size(); i++)
                {
                    if (!children[i].IsInvalid())
                    {
                        *out_child_nodes = children[i].GetRawPointer();
                        out_child_nodes++;
                    }
                }
            }
        }
        else if (node_ptr->IsFp16BoxNode())
        {
            const auto node     = reinterpret_cast<const dxr::amd::Float16BoxNode*>(&interior_nodes[byte_offset]);
            const auto children = node->GetChildren();
            for (size_t i = 0; i < children.size(); i++)
            {
                if (!children[i].IsInvalid())
                {
                    *out_child_nodes = children[i].GetRawPointer();
                    out_child_nodes++;
                }
            }
        }
    }
    return kRraOk;
}

RraErrorCode RraBvhGetChildNodePtr(const rta::IBvh* bvh, uint32_t parent_node, uint32_t child_index, uint32_t* out_node_ptr)
{
    const auto&            header_offsets = bvh->GetHeader().GetBufferOffsets();
    dxr::amd::NodePointer* node_ptr       = reinterpret_cast<dxr::amd::NodePointer*>(&parent_node);
    auto                   byte_offset    = node_ptr->GetByteOffset() - header_offsets.interior_nodes;
    const auto&            interior_nodes = bvh->GetInteriorNodesData();

    if (interior_nodes.size() > byte_offset)
    {
        if (node_ptr->IsFp32BoxNode())
        {
            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
            {
                const auto node = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[byte_offset]);
                if (node->ValidChildCount() <= child_index)
                {
                    return kRraErrorIndexOutOfRange;
                }

                uint32_t child_pointers[8]{};
                node->DecodeChildrenOffsets(child_pointers);
                *out_node_ptr = child_pointers[child_index];
                return kRraOk;
            }
            else
            {
                const auto node = reinterpret_cast<const dxr::amd::Float32BoxNode*>(&interior_nodes[byte_offset]);
                if (node->GetChildren().size() <= child_index)
                {
                    return kRraErrorIndexOutOfRange;
                }

                const auto& ptr = node->GetChildren()[child_index];
                if (!ptr.IsInvalid())
                {
                    *out_node_ptr = ptr.GetRawPointer();
                    return kRraOk;
                }
                else
                {
                    return kRraErrorInvalidChildNode;
                }
            }
        }
        else if (node_ptr->IsFp16BoxNode())
        {
            const auto node = reinterpret_cast<const dxr::amd::Float16BoxNode*>(&interior_nodes[byte_offset]);
            if (node->GetChildren().size() <= child_index)
            {
                return kRraErrorIndexOutOfRange;
            }

            const auto& ptr = node->GetChildren()[child_index];
            if (!ptr.IsInvalid())
            {
                *out_node_ptr = ptr.GetRawPointer();
                return kRraOk;
            }
            else
            {
                return kRraErrorInvalidChildNode;
            }
        }
    }
    return kRraErrorIndexOutOfRange;
}

RraErrorCode RraBvhGetTlasCount(uint64_t* out_count)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
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
            const rta::IBvh* tlas = dynamic_cast<rta::IBvh*>(&(*top_level_bvhs[tlas_index]));
            if (tlas != nullptr)
            {
                *out_size_in_bytes += tlas->GetHeader().GetFileSize();
            }
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
            const rta::IBvh* blas = dynamic_cast<rta::IBvh*>(&(*bottom_level_bvhs[blas_index]));
            if (blas != nullptr)
            {
                *out_size_in_bytes += blas->GetHeader().GetFileSize();
            }
        }
    }
    return kRraOk;
}

RraErrorCode RraBvhGetTotalTraceSizeInBytes(uint64_t* out_size_in_bytes)
{
    uint64_t tlas_size{};
    RraBvhGetTotalTlasSizeInBytes(&tlas_size);

    uint64_t blas_size{};
    RraBvhGetTotalBlasSizeInBytes(&blas_size);

    *out_size_in_bytes = tlas_size + blas_size;
    return kRraOk;
}
