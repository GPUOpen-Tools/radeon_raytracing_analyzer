//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the BLAS interface.
///
/// Contains public functions specific to the BLAS.
//=============================================================================

#include "rra_blas_impl.h"

#include <math.h>  // for sqrt
#include <unordered_set>

#include "glm/glm/glm.hpp"

#include "public/rra_assert.h"
#include "public/rra_rtip_info.h"

#include "bvh/flags_util.h"
#include "bvh/rtip11/encoded_rt_ip_11_bottom_level_bvh.h"
#include "bvh/rtip31/encoded_rt_ip_31_bottom_level_bvh.h"
#include "bvh/rtip_common/ray_tracing_defs.h"
#include "rra_bvh_impl.h"
#include "rra_data_set.h"
#include "surface_area_heuristic.h"

// External reference to the global dataset.
extern RraDataSet data_set_;

rta::EncodedBottomLevelBvh* RraBlasGetBlasFromBlasIndex(uint64_t blas_index)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    const auto& bottom_level_bvhs = data_set_.bvh_bundle->GetBottomLevelBvhs();
    if (blas_index >= bottom_level_bvhs.size())
    {
        return nullptr;
    }
    return dynamic_cast<rta::EncodedBottomLevelBvh*>(&(*bottom_level_bvhs[blas_index]));
}

RraErrorCode RraBlasGetBaseAddress(uint64_t blas_index, uint64_t* out_address)
{
    const rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    *out_address = blas->GetVirtualAddress();

    return kRraOk;
}

bool RraBlasIsEmpty(uint64_t blas_index)
{
    const rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return true;
    }

    return blas->IsEmpty();
}

RraErrorCode RraBlasGetTotalNodeCount(uint64_t blas_index, uint64_t* out_node_count)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const auto& blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_node_count = blas->GetNodeCount(rta::BvhNodeFlags::kNone);

    return kRraOk;
}

RraErrorCode RraBlasGetChildNodeCount(uint64_t blas_index, uint32_t parent_node, uint32_t* out_child_count)
{
    const rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    return RraBvhGetChildNodeCount(blas, parent_node, out_child_count);
}

RraErrorCode RraBlasGetChildNodes(uint64_t blas_index, uint32_t parent_node, uint32_t* out_child_nodes)
{
    const rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    return RraBvhGetChildNodes(blas, parent_node, out_child_nodes);
}

RraErrorCode RraBlasGetChildIndices(uint64_t blas_index, uint32_t parent_node, uint32_t* out_child_nodes)
{
    const rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    return RraBvhGetChildIndices(blas, parent_node, out_child_nodes);
}

RraErrorCode RraBlasGetBoxNodeCount(uint64_t blas_index, uint64_t* out_node_count)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const auto& blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_node_count = blas->GetNodeCount(rta::BvhNodeFlags::kIsInteriorNode);

    return kRraOk;
}

RraErrorCode RraBlasGetBox16NodeCount(uint64_t blas_index, uint32_t* out_node_count)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const auto& blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_node_count = blas->GetHeader().GetInteriorFp16NodeCount();

    return kRraOk;
}

RraErrorCode RraBlasGetBox32NodeCount(uint64_t blas_index, uint32_t* out_node_count)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const auto& blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_node_count = blas->GetHeader().GetInteriorFp32NodeCount();

    return kRraOk;
}

RraErrorCode RraBlasGetMaxTreeDepth(uint64_t blas_index, uint32_t* out_tree_depth)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const auto& blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_tree_depth = blas->GetMaxTreeDepth();

    return kRraOk;
}

RraErrorCode RraBlasGetAvgTreeDepth(uint64_t blas_index, uint32_t* out_tree_depth)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const auto& blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_tree_depth = blas->GetAvgTreeDepth();

    return kRraOk;
}

RraErrorCode RraBlasGetChildNodePtr(uint64_t blas_index, uint32_t parent_node, uint32_t child_index, uint32_t* out_node_ptr)
{
    const rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    return RraBvhGetChildNodePtr(blas, parent_node, child_index, out_node_ptr);
}

bool RraBlasIsBoxNode(uint64_t blas_index, uint32_t node_id)
{
    rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return false;
    }
    return RraBvhIsBoxNode(blas, node_id);
}

bool RraBlasIsBox16Node(uint64_t blas_index, uint32_t node_id)
{
    rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return false;
    }
    return RraBvhIsBox16Node(blas, node_id);
}

bool RraBlasIsBox32Node(uint64_t blas_index, uint32_t node_id)
{
    rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return false;
    }
    return RraBvhIsBox32Node(blas, node_id);
}

bool RraBlasHasChildren(uint64_t blas_index, uint32_t node_id)
{
    rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return false;
    }
    return RraBvhHasChildren(blas, node_id);
}

bool RraBlasIsTriangleNode(uint64_t blas_index, uint32_t node_id)
{
    rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return false;
    }

    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return false;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetIsTriangleNode(node_id, blas);
}

bool RraBlasIsProceduralNode(uint64_t blas_index, uint32_t node_id)
{
    rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return false;
    }

    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return false;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetIsProceduralNode(node_id, blas);
}

RraErrorCode RraBlasGetNodeBaseAddress(uint64_t blas_index, uint32_t node_id, uint64_t* out_address)
{
    if (out_address == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    if (data_set_.bvh_bundle.get() == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const auto& bvh_node = data_set_.bvh_bundle->GetBvhNode();
    return bvh_node.GetBaseAddress(node_id, blas, out_address);
}

RraErrorCode RraBlasGetNodeParent(uint64_t blas_index, uint32_t node_id, uint32_t* out_parent_node_ptr)
{
    rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);

    const auto& interior_nodes = blas->GetInteriorNodesData();
    if (interior_nodes.size() == 0)
    {
        return kRraErrorInvalidPointer;
    }

    dxr::amd::NodePointer parent_node =
        blas->GetParentNode(node->GetRawPointer(), 0);  // This function uses node pointers and so is only for older RtIp levels.
    *out_parent_node_ptr = *reinterpret_cast<uint32_t*>(&parent_node);

    return kRraOk;
}

RraErrorCode RraBlasGetSurfaceArea(uint64_t blas_index, uint32_t node_id, uint32_t child_index, uint32_t global_child_index, float* out_surface_area)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    return RraBlasGetSurfaceAreaImpl(blas, node_id, child_index, global_child_index, out_surface_area);
}

RraErrorCode RraBlasGetSurfaceAreaHeuristic(uint64_t blas_index, uint32_t node_id, uint32_t global_child_index, float* out_surface_area_heuristic)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    RraErrorCode error_code = RraBvhGetSurfaceAreaHeuristic(blas, node_id, global_child_index, out_surface_area_heuristic);

    if (!isnan(*out_surface_area_heuristic) && *out_surface_area_heuristic > 1.0f)
    {
        *out_surface_area_heuristic = 1.0f;
    }

    return error_code;
}

static dxr::amd::Float3 Vec3ToFloat3(const glm::vec3& v)
{
    return {v.x, v.y, v.z};
}

RraErrorCode RraBlasGetSurfaceAreaImpl(const rta::EncodedBottomLevelBvh* blas,
                                       uint32_t                          node_id,
                                       uint32_t                          child_index,
                                       uint32_t                          global_child_index,
                                       float*                            out_surface_area)
{
    {
        dxr::amd::NodePointer* node_ptr = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);

        if (node_ptr->IsTriangleNode())
        {
            const auto& header_offsets = blas->GetHeader().GetBufferOffsets();

            if (node_ptr->GetByteOffset() < header_offsets.interior_nodes)
            {
                *out_surface_area = std::numeric_limits<float>::quiet_NaN();
                return kRraOk;
            }

            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
            {
                rta::EncodedRtIp31BottomLevelBvh* blas_rtip31 = (rta::EncodedRtIp31BottomLevelBvh*)blas;
                uint32_t                          pair_indices_count{};
                auto                              triangle_pair_indices = blas_rtip31->GetTrianglePairIndices(*node_ptr, &pair_indices_count);

                float surface_area{0.0f};

                for (uint32_t i = 0; i < pair_indices_count; ++i)
                {
                    auto&        tri_pair_idx = triangle_pair_indices[i];
                    TriangleData tri0         = tri_pair_idx.first->UnpackTriangleVertices(tri_pair_idx.second, 0);
                    surface_area += blas->TriangleSurfaceArea(Vec3ToFloat3(tri0.v0), Vec3ToFloat3(tri0.v1), Vec3ToFloat3(tri0.v2));

                    if (tri_pair_idx.first->ReadTrianglePairDesc(tri_pair_idx.second).Tri1Valid())
                    {
                        TriangleData tri1 = tri_pair_idx.first->UnpackTriangleVertices(tri_pair_idx.second, 1);
                        surface_area += blas->TriangleSurfaceArea(Vec3ToFloat3(tri1.v0), Vec3ToFloat3(tri1.v1), Vec3ToFloat3(tri1.v2));
                    }
                }

                *out_surface_area = surface_area;
            }
            else
            {
                if (node_ptr->GetByteOffset() < header_offsets.leaf_nodes)
                {
                    *out_surface_area = std::numeric_limits<float>::quiet_NaN();
                    return kRraOk;
                }

                const rta::EncodedRtIp11BottomLevelBvh* blas_rtip11   = (rta::EncodedRtIp11BottomLevelBvh*)blas;
                const dxr::amd::TriangleNode*           triangle_node = blas_rtip11->GetTriangleNode(*node_ptr);
                if (triangle_node == nullptr)
                {
                    return kRraErrorInvalidPointer;
                }

                uint32_t     tri_count{};
                RraErrorCode error_code = RraBlasGetNodeTriangleCount(blas->GetID(), node_ptr->GetRawPointer(), 0, global_child_index, &tri_count);
                if (error_code != kRraOk)
                {
                    return error_code;
                }

                *out_surface_area = blas->GetTriangleSurfaceArea(*triangle_node, tri_count);
            }

            return kRraOk;
        }
        else if (node_ptr->IsBoxNode())
        {
            return RraBvhGetBoundingVolumeSurfaceArea(blas, node_id, child_index, global_child_index, out_surface_area);
        }
    }

    return kRraOk;
}

RraErrorCode RraBlasGetMinimumSurfaceAreaHeuristic(uint64_t blas_index,
                                                   uint32_t node_id,
                                                   uint32_t global_child_index,
                                                   bool     tri_only,
                                                   float*   out_min_surface_area_heuristic)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    if (RraBlasIsEmpty(blas_index))
    {
        *out_min_surface_area_heuristic = 0.0f;
        return kRraOk;
    }

    *out_min_surface_area_heuristic = rra::GetMinimumSurfaceAreaHeuristic(blas, node_id, global_child_index, tri_only);
    return kRraOk;
}

RraErrorCode RraBlasGetAverageSurfaceAreaHeuristic(uint64_t blas_index,
                                                   uint32_t node_id,
                                                   uint32_t global_node_id,
                                                   bool     tri_only,
                                                   float*   out_avg_surface_area_heuristic)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    if (RraBlasIsEmpty(blas_index))
    {
        *out_avg_surface_area_heuristic = 0.0f;
        return kRraOk;
    }

    *out_avg_surface_area_heuristic = rra::GetAverageSurfaceAreaHeuristic(blas, node_id, global_node_id, tri_only);
    return kRraOk;
}

RraErrorCode RraBlasGetTriangleSurfaceAreaHeuristic(uint64_t blas_index, uint32_t node_id, uint32_t global_node_id, float* out_tri_surface_area_heuristic)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_tri_surface_area_heuristic = rra::GetAverageSurfaceAreaHeuristic(blas, node_id, global_node_id, true);
    return kRraOk;
}

RraErrorCode RraBlasGetUniqueTriangleCount(uint64_t blas_index, uint32_t* out_triangle_count)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    if (blas->GetHeader().GetGeometryType() == rta::BottomLevelBvhGeometryType::kTriangle)
    {
        *out_triangle_count = blas->GetTriangleCount();
    }
    else
    {
        *out_triangle_count = 0;
    }

    return kRraOk;
}

RraErrorCode RraBlasGetActivePrimitiveCount(uint64_t blas_index, uint32_t* out_triangle_count)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_triangle_count = blas->GetHeader().GetActivePrimitiveCount();
    return kRraOk;
}

RraErrorCode RraBlasGetTriangleNodeCount(uint64_t blas_index, uint32_t* out_triangle_count)
{
    rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    return blas->GetTriangleNodeCount(out_triangle_count);
}

RraErrorCode RraBlasGetProceduralNodeCount(uint64_t blas_index, uint32_t* out_procedural_Node_count)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    return blas->GetProceduralNodeCount(out_procedural_Node_count);
}

RraErrorCode RraBlasGetNodeName(uint64_t blas_index, uint32_t node_id, const char** out_name)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    RraErrorCode result = blas->GetNodeName(node_id, out_name);
    return result;
}

RraErrorCode RraBlasGetNodeNameToolTip(uint64_t blas_index, uint32_t node_id, const char** out_tooltip)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    RraErrorCode result = blas->GetNodeNameToolTip(node_id, out_tooltip);
    return result;
}

RraErrorCode RraBlasGetGeometryIndex(uint64_t blas_index, uint32_t node_id, uint32_t child_index, uint32_t global_child_index, uint32_t* out_geometry_index)
{
    rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    RraErrorCode result = blas->GetGeometryIndex(node_id, child_index, global_child_index, out_geometry_index);
    return result;
}

RraErrorCode RraBlasGetGeometryPrimitiveCount(uint64_t blas_index, uint32_t geometry_index, uint32_t* out_primitive_count)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    if (geometry_index >= blas->GetGeometryInfos().size())
    {
        return kRraErrorIndexOutOfRange;
    }

    *out_primitive_count = blas->GetGeometryTriangleCount(geometry_index);

    return kRraOk;
}

RraErrorCode RraBlasGetGeometryCount(uint64_t blas_index, uint32_t* out_geometry_count)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_geometry_count = (uint32_t)blas->GetGeometryInfos().size();

    return kRraOk;
}

RraErrorCode RraBlasGetPrimitiveIndex(uint64_t  blas_index,
                                      uint32_t  node_id,
                                      uint32_t  child_index,
                                      uint32_t  global_child_index,
                                      uint32_t  local_primitive_index,
                                      uint32_t* out_primitive_index)
{
    rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    RraErrorCode result = blas->GetPrimitiveIndex(node_id, child_index, global_child_index, local_primitive_index, out_primitive_index);
    return result;
}

RraErrorCode RraBlasGetGeometryFlags(uint64_t blas_index, uint32_t geometry_index, uint32_t* out_geometry_flags)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    if (out_geometry_flags == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    // Retrieve a reference to the geometry info using the provided index.
    const auto& geometry_infos = blas->GetGeometryInfos();
    if (geometry_index < geometry_infos.size())
    {
        const auto& info = geometry_infos[geometry_index];

        // Check if the opaque flag is set in the geometry flags.
        const auto flags       = info.GetGeometryFlags();
        uint32_t   opaque_flag = static_cast<uint32_t>(dxr::GeometryFlags::kAmdFlagOpaque);
        *out_geometry_flags    = (static_cast<uint32_t>(flags) & opaque_flag) == opaque_flag;
    }
    else
    {
        return kRraErrorIndexOutOfRange;
    }

    return kRraOk;
}

RraErrorCode RraBlasGetIsInactive(uint64_t blas_index, uint32_t node_id, uint32_t child_index, uint32_t global_child_index, bool* out_is_inactive)
{
    rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    RraErrorCode result = blas->GetIsInactive(node_id, child_index, global_child_index, out_is_inactive);
    return result;
}

RraErrorCode RraBlasGetNodeTriangleCount(uint64_t blas_index, uint32_t node_id, uint32_t child_index, uint32_t global_child_index, uint32_t* out_triangle_count)
{
    if (RraBlasIsEmpty(blas_index))
    {
        *out_triangle_count = 0;
        return kRraOk;
    }

    rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    RraErrorCode result = blas->GetNodeTriangleCount(node_id, child_index, global_child_index, out_triangle_count);
    return result;
}

RraErrorCode RraBlasGetNodeTriangles(uint64_t blas_index, uint32_t node_id, uint32_t child_index, uint32_t global_child_index, TriangleVertices* out_triangles)
{
    rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    RraErrorCode result = blas->GetNodeTriangles(node_id, child_index, global_child_index, out_triangles);
    return result;
}

RraErrorCode RraBlasGetNodeVertexCount(uint64_t blas_index, uint32_t node_id, uint32_t child_index, uint32_t global_child_index, uint32_t* out_count)
{
    rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    RraErrorCode result = blas->GetNodeVertexCount(node_id, child_index, global_child_index, out_count);
    return result;
}

RraErrorCode RraBlasGetNodeVertices(uint64_t               blas_index,
                                    uint32_t               node_id,
                                    uint32_t               child_index,
                                    uint32_t               global_child_index,
                                    struct VertexPosition* out_vertices)
{
    rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    RraErrorCode result = blas->GetNodeVertices(node_id, child_index, global_child_index, out_vertices);
    return result;
}

RraErrorCode RraBlasGetBoundingVolumeExtents(uint64_t               blas_index,
                                             uint32_t               node_id,
                                             uint32_t               child_index,
                                             uint32_t               global_child_index,
                                             BoundingVolumeExtents* out_extents)
{
    const rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    dxr::amd::AxisAlignedBoundingBox bounding_box;
    RraErrorCode                     error_code = RraBvhGetNodeBoundingVolume(blas, node_id, child_index, global_child_index, bounding_box);
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

RraErrorCode RraBlasGetBuildFlags(uint64_t blas_index, VkBuildAccelerationStructureFlagBitsKHR* out_flags)
{
    const rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const rta::IRtIpCommonAccelerationStructureHeader& header = blas->GetHeader();

    // Internally, the build flags have the same values as those in the vulkan enum passed in, so currently just
    // need to do a simple cast. If the internal structure changes, then the internal flags will need mapping
    // to the vulkan API enum.
    *out_flags = static_cast<VkBuildAccelerationStructureFlagBitsKHR>(header.GetPostBuildInfo().GetBuildFlags());
    return kRraOk;
}

RraErrorCode RraBlasGetSizeInBytes(uint64_t blas_index, uint32_t* out_size_in_bytes)
{
    const rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_size_in_bytes = blas->GetHeader().GetFileSize();
    return kRraOk;
}

RraErrorCode RraBlasGetNodeObbIndex(uint64_t blas_index, uint32_t node_id, uint32_t* obb_index)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    glm::mat3    rotation{};
    RraErrorCode error_code = RraBvhGetNodeObbIndex(blas, node_id, obb_index);
    return error_code;
}

RraErrorCode RraBlasGetNodeBoundingVolumeOrientation(uint64_t blas_index, uint32_t node_id, float* out_rotation)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    glm::mat3    rotation{};
    RraErrorCode error_code = RraBvhGetNodeBoundingVolumeOrientation(blas, node_id, rotation);

    if (error_code != kRraOk)
    {
        return error_code;
    }

    std::memcpy(out_rotation, &rotation, sizeof(glm::mat3));
    return kRraOk;
}

RraErrorCode RraBlasGetMetaDataSize(uint64_t blas_index, uint32_t* out_byte_size)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_byte_size = blas->GetMetaData().GetByteSize();
    return kRraOk;
}

bool RraBlasIsClusterBlas(uint64_t blas_index)
{
    const auto* blas = dynamic_cast<const rta::EncodedRtIp31BottomLevelBvh*>(RraBlasGetBlasFromBlasIndex(blas_index));
    if (blas == nullptr)
    {
        return false;
    }
    return blas->IsClusterBlas();
}

bool RraBlasIsCluster(uint64_t blas_index)
{
    const auto* blas = dynamic_cast<const rta::EncodedRtIp31BottomLevelBvh*>(RraBlasGetBlasFromBlasIndex(blas_index));
    if (blas == nullptr)
    {
        return false;
    }
    return blas->IsCluster();
}

bool RraBlasIsClusterRefNode(uint64_t blas_index, uint32_t node_id)
{
    const auto* blas = dynamic_cast<const rta::EncodedRtIp31BottomLevelBvh*>(RraBlasGetBlasFromBlasIndex(blas_index));
    if (blas == nullptr)
    {
        return false;
    }
    return blas->IsClusterRefNode(node_id);
}

bool RraBlasHasNodePacking(uint64_t blas_index)
{
    const auto* blas = dynamic_cast<const rta::EncodedRtIp31BottomLevelBvh*>(RraBlasGetBlasFromBlasIndex(blas_index));
    if (blas == nullptr)
    {
        return false;
    }
    return blas->HasNodePacking();
}

RraErrorCode RraBlasGetClasIndexFromClusterRefNode(uint64_t blas_index, uint32_t node_id, uint64_t* out_clas_blas_index)
{
    const auto* blas = dynamic_cast<const rta::EncodedRtIp31BottomLevelBvh*>(RraBlasGetBlasFromBlasIndex(blas_index));
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    return blas->GetClasIndexFromClusterRefNode(node_id, out_clas_blas_index);
}

RraErrorCode RraBlasGetClusterRefNodeTransform(uint64_t blas_index, uint32_t node_id, float* out_transform)
{
    const auto* blas = dynamic_cast<const rta::EncodedRtIp31BottomLevelBvh*>(RraBlasGetBlasFromBlasIndex(blas_index));
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    return blas->GetClusterRefNodeTransform(node_id, out_transform);
}

RraErrorCode RraBlasGetClusterRefNodeId(uint64_t blas_index, uint32_t node_id, uint32_t* out_id)
{
    const auto* blas = dynamic_cast<const rta::EncodedRtIp31BottomLevelBvh*>(RraBlasGetBlasFromBlasIndex(blas_index));
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    return blas->GetClusterRefNodeId(node_id, out_id);
}

RraErrorCode RraBlasGetClusterRefNodeMask(uint64_t blas_index, uint32_t node_id, uint32_t* out_mask)
{
    const auto* blas = dynamic_cast<const rta::EncodedRtIp31BottomLevelBvh*>(RraBlasGetBlasFromBlasIndex(blas_index));
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    return blas->GetClusterRefNodeMask(node_id, out_mask);
}

