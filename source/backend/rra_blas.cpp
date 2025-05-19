//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
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
#include "bvh/rtip31/primitive_node.h"
#include "bvh/rtip_common/ray_tracing_defs.h"
#include "rra_bvh_impl.h"
#include "rra_data_set.h"
#include "surface_area_heuristic.h"

// External reference to the global dataset.
extern RraDataSet data_set_;

/// @brief Private function to get the length of a vector, given 2 3D points.
///
/// @param vert_1 The first vertex.
/// @param vert_2 The second vertex.
///
/// @return The vector length.
static float GetLength(const dxr::amd::Float3& vert_1, const dxr::amd::Float3& vert_2)
{
    float delta_x   = vert_1.x - vert_2.x;
    float x_squared = delta_x * delta_x;
    float delta_y   = vert_1.y - vert_2.y;
    float y_squared = delta_y * delta_y;
    float delta_z   = vert_1.z - vert_2.z;
    float z_squared = delta_z * delta_z;

    return sqrt(x_squared + y_squared + z_squared);
}

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

bool RraBlasIsTriangleNode(uint64_t blas_index, uint32_t node_ptr)
{
    rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return false;
    }
    bool out_is_tri{};
    RraBvhIsTriangleNode(blas, node_ptr, &out_is_tri);
    return out_is_tri;
}

RraErrorCode RraBlasGetNodeBaseAddress(uint64_t blas_index, uint32_t node_ptr, uint64_t* out_address)
{
    const rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const auto base_addr = blas->GetVirtualAddress();

    const dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    *out_address = base_addr + blas->GetHeader().GetMetaDataSize() + node->GetGpuVirtualAddress();
    return kRraOk;
}

RraErrorCode RraBlasGetNodeParent(uint64_t blas_index, uint32_t node_ptr, uint32_t* out_parent_node_ptr)
{
    rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    const auto& interior_nodes = blas->GetInteriorNodesData();
    if (interior_nodes.size() == 0)
    {
        return kRraErrorInvalidPointer;
    }

    dxr::amd::NodePointer parent_node = blas->GetParentNode(node);
    *out_parent_node_ptr              = *reinterpret_cast<uint32_t*>(&parent_node);

    return kRraOk;
}

RraErrorCode RraBlasGetSurfaceArea(uint64_t blas_index, uint32_t node_ptr, float* out_surface_area)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    return RraBlasGetSurfaceAreaImpl(blas, reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr), out_surface_area);
}

RraErrorCode RraBlasGetSurfaceAreaHeuristic(uint64_t blas_index, uint32_t node_ptr, float* out_surface_area_heuristic)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);
    RraErrorCode                 error_code   = RraBvhGetSurfaceAreaHeuristic(blas, *current_node, out_surface_area_heuristic);

    if (!isnan(*out_surface_area_heuristic) && *out_surface_area_heuristic > 1.0f)
    {
        *out_surface_area_heuristic = 1.0f;
    }

    return error_code;
}

/// @brief Calculate the surface area of a triangle.
/// @param v0 Vertex 0 of the triangle.
/// @param v1 Vertex 1 of the triangle.
/// @param v2 Vertex 2 of the triangle.
/// @return Surface area of the triangle.
float TriangleSurfaceArea(const dxr::amd::Float3& v0, const dxr::amd::Float3& v1, const dxr::amd::Float3& v2)
{
    // Calculate the surface area of the triangle using Heron's Formula.
    float length_a       = GetLength(v1, v0);
    float length_b       = GetLength(v2, v1);
    float length_c       = GetLength(v0, v2);
    float semi_perimeter = (length_a + length_b + length_c) * 0.5f;
    return sqrt(semi_perimeter * (semi_perimeter - length_a) * (semi_perimeter - length_b) * (semi_perimeter - length_c));
}

float RraBlasGetTriangleSurfaceArea(const dxr::amd::TriangleNode& triangle_node, uint32_t tri_count)
{
    auto  tri0         = triangle_node.GetTriangle(dxr::amd::NodeType::kAmdNodeTriangle0);
    float surface_area = TriangleSurfaceArea(tri0.v0, tri0.v1, tri0.v2);

    if (tri_count == 2)
    {
        auto tri1 = triangle_node.GetTriangle(dxr::amd::NodeType::kAmdNodeTriangle1);
        surface_area += TriangleSurfaceArea(tri1.v0, tri1.v1, tri1.v2);
    }

    return surface_area;
}

dxr::amd::Float3 Vec3ToFloat3(const glm::vec3& v)
{
    return {v.x, v.y, v.z};
}

RraErrorCode RraBlasGetSurfaceAreaImpl(const rta::EncodedBottomLevelBvh* blas, const dxr::amd::NodePointer* node_ptr, float* out_surface_area)
{
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
                surface_area += TriangleSurfaceArea(Vec3ToFloat3(tri0.v0), Vec3ToFloat3(tri0.v1), Vec3ToFloat3(tri0.v2));

                if (tri_pair_idx.first->ReadTrianglePairDesc(tri_pair_idx.second).Tri1Valid())
                {
                    TriangleData tri1 = tri_pair_idx.first->UnpackTriangleVertices(tri_pair_idx.second, 1);
                    surface_area += TriangleSurfaceArea(Vec3ToFloat3(tri1.v0), Vec3ToFloat3(tri1.v1), Vec3ToFloat3(tri1.v2));
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

            uint32_t tri_count{};
            RraBlasGetNodeTriangleCount(blas->GetID(), node_ptr->GetRawPointer(), &tri_count);

            *out_surface_area = RraBlasGetTriangleSurfaceArea(*triangle_node, tri_count);
        }

        return kRraOk;
    }
    else if (node_ptr->IsBoxNode())
    {
        return RraBvhGetBoundingVolumeSurfaceArea(blas, node_ptr, out_surface_area);
    }
    return kRraOk;
}

RraErrorCode RraBlasGetMinimumSurfaceAreaHeuristic(uint64_t blas_index, uint32_t node_ptr, bool tri_only, float* out_min_surface_area_heuristic)
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

    const dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    *out_min_surface_area_heuristic = rra::GetMinimumSurfaceAreaHeuristic(blas, *current_node, tri_only);
    return kRraOk;
}

RraErrorCode RraBlasGetAverageSurfaceAreaHeuristic(uint64_t blas_index, uint32_t node_ptr, bool tri_only, float* out_avg_surface_area_heuristic)
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

    const dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    *out_avg_surface_area_heuristic = rra::GetAverageSurfaceAreaHeuristic(blas, *current_node, tri_only);
    return kRraOk;
}

RraErrorCode RraBlasGetTriangleSurfaceAreaHeuristic(uint64_t blas_index, uint32_t node_ptr, float* out_tri_surface_area_heuristic)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    *out_tri_surface_area_heuristic = rra::GetAverageSurfaceAreaHeuristic(blas, *current_node, true);
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
        uint32_t total_triangle_count = 0;

        const auto& geometry_infos = blas->GetGeometryInfos();
        for (auto geom_iter = geometry_infos.begin(); geom_iter != geometry_infos.end(); ++geom_iter)
        {
            total_triangle_count += geom_iter->GetPrimitiveCount();
        }

        *out_triangle_count = total_triangle_count;
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

    if (blas->GetHeader().GetGeometryType() == rta::BottomLevelBvhGeometryType::kTriangle)
    {
        if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
        {
            rta::EncodedRtIp31BottomLevelBvh* blas_rtip31 = (rta::EncodedRtIp31BottomLevelBvh*)blas;
            *out_triangle_count                           = blas_rtip31->GetNodeCount(rta::BvhNodeFlags::kIsLeafNode);
        }
        else
        {
            rta::EncodedRtIp11BottomLevelBvh* blas_rtip11 = (rta::EncodedRtIp11BottomLevelBvh*)blas;
            *out_triangle_count                           = blas_rtip11->GetNodeCount(rta::BvhNodeFlags::kIsLeafNode);
        }
    }
    else
    {
        *out_triangle_count = 0;
    }

    return kRraOk;
}

RraErrorCode RraBlasGetProceduralNodeCount(uint64_t blas_index, uint32_t* out_procedural_Node_count)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    if (blas->GetHeader().GetGeometryType() == rta::BottomLevelBvhGeometryType::kAABB)
    {
        *out_procedural_Node_count = blas->GetHeader().GetLeafNodeCount();
    }
    else
    {
        *out_procedural_Node_count = 0;
    }

    return kRraOk;
}

RraErrorCode RraBlasGetNodeName(uint64_t blas_index, uint32_t node_ptr, const char** out_name)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    switch ((uint32_t)node->GetType())
    {
    case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle0:
        if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
        {
            *out_name = blas->IsProcedural() ? "Procedural" : "Triangles";
            break;
        }
        *out_name = blas->IsProcedural() ? "Procedural" : "Triangle";
        break;

    case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle1:
    case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle2:
    case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle3:
    case NODE_TYPE_TRIANGLE_4:
    case NODE_TYPE_TRIANGLE_5:
    case NODE_TYPE_TRIANGLE_6:
    case NODE_TYPE_TRIANGLE_7:
        *out_name = blas->IsProcedural() ? "Procedural" : "Triangles";
        break;

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

    case (uint32_t)dxr::amd::NodeType::kAmdNodeProcedural:
        *out_name = "Procedural";
        break;

    default:
        *out_name = "Unknown";
        break;
    }

    return kRraOk;
}

RraErrorCode RraBlasGetNodeNameToolTip(uint64_t blas_index, uint32_t node_ptr, const char** out_tooltip)
{
    static const char* proc_string = "A node containing procedural geometry data";
    static const char* tri_string  = "A node containing triangle geometry data";

    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
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
        *out_tooltip = blas->IsProcedural() ? proc_string : tri_string;
        break;

    case (uint32_t)dxr::amd::NodeType::kAmdNodeProcedural:
        *out_tooltip = proc_string;
        break;

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
        *out_tooltip = "A node containing an instance of a BLAS";
        break;

    default:
        *out_tooltip = "";
        break;
    }

    return kRraOk;
}

RraErrorCode RraBlasGetGeometryIndex(uint64_t blas_index, uint32_t node_ptr, uint32_t* out_geometry_index)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);
    if (current_node->IsTriangleNode())
    {
        if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
        {
            rta::EncodedRtIp31BottomLevelBvh* blas_rtip31 = (rta::EncodedRtIp31BottomLevelBvh*)blas;
            uint32_t                          pair_indices_count{};
            auto                              pair_indices = blas_rtip31->GetTrianglePairIndices(node_ptr, &pair_indices_count);

            auto pair_index = uint32_t(current_node->GetType());
            RRA_UNUSED(pair_index);

            *out_geometry_index = (pair_indices_count == 0) ? 0 : pair_indices.front().first->UnpackGeometryIndex(pair_indices.front().second, 0);
        }
        else
        {
            const auto& header_offsets = blas->GetHeader().GetBufferOffsets();
            if (current_node->GetByteOffset() < header_offsets.leaf_nodes)
            {
                *out_geometry_index = 0;
                return kRraErrorInvalidPointer;
            }

            const rta::EncodedRtIp11BottomLevelBvh* blas_rtip11   = (rta::EncodedRtIp11BottomLevelBvh*)blas;
            const dxr::amd::TriangleNode*           triangle_node = blas_rtip11->GetTriangleNode(*current_node);

#ifdef _DEBUG
            const uint32_t index = (current_node->GetByteOffset() - header_offsets.leaf_nodes) / sizeof(dxr::amd::TriangleNode);

            const auto* triangle_nodes = reinterpret_cast<const dxr::amd::TriangleNode*>(blas_rtip11->GetLeafNodesData().data());
            const auto& tri_node       = triangle_nodes[index];

            RRA_ASSERT(&tri_node == triangle_node);
#endif  // DEBUG

            *out_geometry_index = triangle_node->GetGeometryIndex();
        }
    }
    else
    {
        // The given node Id doesn't refer to a triangle node.
        // Don't output any geometry Id, and return an invalid child error code.
        return kRraErrorInvalidChildNode;
    }

    return kRraOk;
}

RraErrorCode RraBlasGetGeometryPrimitiveCount(uint64_t blas_index, uint32_t geometry_index, uint32_t* out_primitive_count)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    if (geometry_index > blas->GetGeometryInfos().size())
    {
        return kRraErrorIndexOutOfRange;
    }

    *out_primitive_count = blas->GetGeometryInfos()[geometry_index].GetPrimitiveCount();

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

RraErrorCode RraBlasGetPrimitiveIndex(uint64_t blas_index, uint32_t node_ptr, uint32_t local_primitive_index, uint32_t* out_primitive_index)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);
    if (current_node->IsTriangleNode())
    {
        if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
        {
            rta::EncodedRtIp31BottomLevelBvh* blas_rtip31 = (rta::EncodedRtIp31BottomLevelBvh*)blas;
            uint32_t                          pair_indices_count{};
            auto                              pair_indices = blas_rtip31->GetTrianglePairIndices(node_ptr, &pair_indices_count);

            *out_primitive_index =
                (pair_indices_count == 0) ? 0 : pair_indices.front().first->UnpackPrimitiveIndex(pair_indices.front().second, local_primitive_index);
        }
        else
        {
            const rta::EncodedRtIp11BottomLevelBvh* blas_rtip11    = (rta::EncodedRtIp11BottomLevelBvh*)blas;
            const dxr::amd::TriangleNode*           triangle_node  = blas_rtip11->GetTriangleNode(*current_node);
            const auto&                             header_offsets = blas->GetHeader().GetBufferOffsets();
            if (current_node->GetByteOffset() < header_offsets.leaf_nodes)
            {
                *out_primitive_index = 0;
                return kRraErrorInvalidPointer;
            }

#ifdef _DEBUG
            const uint32_t node_index = (current_node->GetByteOffset() - header_offsets.leaf_nodes) / sizeof(dxr::amd::TriangleNode);

            // Get the primitive index for this node index.
            const auto* triangle_nodes = reinterpret_cast<const dxr::amd::TriangleNode*>(blas_rtip11->GetLeafNodesData().data());
            const auto& tri_node       = triangle_nodes[node_index];

            RRA_ASSERT(&tri_node == triangle_node);
#endif  // DEBUG

            // No way to exctract more than 2 primitive indexes as of yet.
            RRA_ASSERT(local_primitive_index < 2);

            *out_primitive_index =
                triangle_node->GetPrimitiveIndex(local_primitive_index == 0 ? dxr::amd::NodeType::kAmdNodeTriangle0 : dxr::amd::NodeType::kAmdNodeTriangle1);
        }
    }
    else
    {
        // The given node Id doesn't refer to a triangle node.
        // Don't output any geometry Id, and return an invalid child error code.
        return kRraErrorInvalidChildNode;
    }

    return kRraOk;
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

RraErrorCode RraBlasGetIsInactive(uint64_t blas_index, uint32_t node_ptr, bool* out_is_inactive)
{
    const auto&                       bottom_level_bvhs = data_set_.bvh_bundle->GetBottomLevelBvhs();
    const rta::EncodedBottomLevelBvh* blas              = dynamic_cast<rta::EncodedBottomLevelBvh*>(&(*bottom_level_bvhs[blas_index]));
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    if (current_node == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    if (current_node->IsTriangleNode())
    {
        if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
        {
            rta::EncodedRtIp31BottomLevelBvh* blas_rtip31 = (rta::EncodedRtIp31BottomLevelBvh*)blas;
            uint32_t                          pair_indices_count{};
            auto                              pair_indices = blas_rtip31->GetTrianglePairIndices(node_ptr, &pair_indices_count);

            *out_is_inactive =
                (pair_indices_count == 0) ? true : std::isnan(pair_indices.front().first->UnpackTriangleVertices(pair_indices.front().second, 0).v0.x);
        }
        else
        {
            const rta::EncodedRtIp11BottomLevelBvh* blas_rtip11   = (rta::EncodedRtIp11BottomLevelBvh*)blas;
            const dxr::amd::TriangleNode*           triangle_node = blas_rtip11->GetTriangleNode(*current_node);
            if (triangle_node == nullptr)
            {
                return kRraErrorInvalidPointer;
            }
            *out_is_inactive = triangle_node->IsInactive(current_node->GetType());
        }
        return kRraOk;
    }
    else if (blas->IsProcedural())
    {
        if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
        {
            rta::EncodedRtIp31BottomLevelBvh* blas_rtip31 = (rta::EncodedRtIp31BottomLevelBvh*)blas;
            uint32_t                          pair_indices_count{};
            auto                              pair_indices = blas_rtip31->GetTrianglePairIndices(node_ptr, &pair_indices_count);
            if ((pair_indices_count == 0) || pair_indices.front().first == nullptr || !pair_indices.front().first->IsProcedural(pair_indices.front().second, 0))
            {
                return kRraErrorInvalidPointer;
            }
            *out_is_inactive = std::isnan(pair_indices.front().first->UnpackTriangleVertices(pair_indices.front().second, 0).v0.x);
        }
        else
        {
            const rta::EncodedRtIp11BottomLevelBvh* blas_rtip11     = (rta::EncodedRtIp11BottomLevelBvh*)blas;
            const dxr::amd::ProceduralNode*         procedural_node = blas_rtip11->GetProceduralNode(*current_node);
            if (procedural_node == nullptr)
            {
                return kRraErrorInvalidPointer;
            }
            *out_is_inactive = procedural_node->IsInactive();
        }
        return kRraOk;
    }

    return kRraErrorInvalidChildNode;
}

RraErrorCode RraBlasGetNodeTriangleCount(uint64_t blas_index, uint32_t node_ptr, uint32_t* out_triangle_count)
{
    const auto&                       bottom_level_bvhs = data_set_.bvh_bundle->GetBottomLevelBvhs();
    const rta::EncodedBottomLevelBvh* blas              = dynamic_cast<rta::EncodedBottomLevelBvh*>(&(*bottom_level_bvhs[blas_index]));
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    if (RraBlasIsEmpty(blas_index))
    {
        *out_triangle_count = 0;
        return kRraOk;
    }

    dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    if (current_node == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    if (!current_node->IsTriangleNode())
    {
        *out_triangle_count = 0;
        return kRraOk;
    }

    if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
    {
        rta::EncodedRtIp31BottomLevelBvh* blas_rtip31 = (rta::EncodedRtIp31BottomLevelBvh*)blas;
        uint32_t                          pair_indices_count{};
        auto                              pair_indices = blas_rtip31->GetTrianglePairIndices(node_ptr, &pair_indices_count);

        uint32_t tri_count{};
        for (uint32_t i = 0; i < pair_indices_count; ++i)
        {
            tri_count += 1 + pair_indices[i].first->ReadTrianglePairDesc(pair_indices[i].second).Tri1Valid();
        }
        *out_triangle_count = tri_count;

        return kRraOk;
    }
    // The incoming node id should be a triangle node. If it's not, we can't extract triangle data.
    if (current_node->GetType() == dxr::amd::NodeType::kAmdNodeTriangle0)
    {
        *out_triangle_count = 1;
    }
    else if (current_node->GetType() == dxr::amd::NodeType::kAmdNodeTriangle1)
    {
        *out_triangle_count = 2;
    }
    else if (current_node->GetType() == dxr::amd::NodeType::kAmdNodeTriangle2)
    {
        *out_triangle_count = 3;
    }
    else if (current_node->GetType() == dxr::amd::NodeType::kAmdNodeTriangle3)
    {
        *out_triangle_count = 4;
    }
    else if ((int)current_node->GetType() == NODE_TYPE_TRIANGLE_4)
    {
        *out_triangle_count = 5;
    }
    else if ((int)current_node->GetType() == NODE_TYPE_TRIANGLE_5)
    {
        *out_triangle_count = 6;
    }
    else if ((int)current_node->GetType() == NODE_TYPE_TRIANGLE_6)
    {
        *out_triangle_count = 7;
    }
    else if ((int)current_node->GetType() == NODE_TYPE_TRIANGLE_7)
    {
        *out_triangle_count = 8;
    }
    else
    {
        *out_triangle_count = 0;
    }
    return kRraOk;
}

RraErrorCode RraBlasGetNodeTriangles(uint64_t blas_index, uint32_t node_ptr, TriangleVertices* out_triangles)
{
    const auto&                       bottom_level_bvhs = data_set_.bvh_bundle->GetBottomLevelBvhs();
    const rta::EncodedBottomLevelBvh* blas              = dynamic_cast<rta::EncodedBottomLevelBvh*>(&(*bottom_level_bvhs[blas_index]));
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    if (current_node == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    // The incoming node id should be a triangle node. If it's not, we can't extract triangle data.
    if (current_node->IsTriangleNode())
    {
        if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
        {
            rta::EncodedRtIp31BottomLevelBvh* blas_rtip31 = (rta::EncodedRtIp31BottomLevelBvh*)blas;
            uint32_t                          pair_indices_count{};
            auto                              pair_indices = blas_rtip31->GetTrianglePairIndices(node_ptr, &pair_indices_count);

            uint32_t triangle_idx = 0;

            for (uint32_t i = 0; i < pair_indices_count; ++i)
            {
                auto& pair_index{pair_indices[i]};
                auto  pair_descriptor = pair_index.first->ReadTrianglePairDesc(pair_index.second);

                auto tri0_data                  = pair_index.first->UnpackTriangleVertices(pair_index.second, 0);
                out_triangles[triangle_idx].a.x = tri0_data.v0.x;
                out_triangles[triangle_idx].a.y = tri0_data.v0.y;
                out_triangles[triangle_idx].a.z = tri0_data.v0.z;
                out_triangles[triangle_idx].b.x = tri0_data.v1.x;
                out_triangles[triangle_idx].b.y = tri0_data.v1.y;
                out_triangles[triangle_idx].b.z = tri0_data.v1.z;
                out_triangles[triangle_idx].c.x = tri0_data.v2.x;
                out_triangles[triangle_idx].c.y = tri0_data.v2.y;
                out_triangles[triangle_idx].c.z = tri0_data.v2.z;
                triangle_idx++;

                if (pair_descriptor.Tri1Valid())
                {
                    auto tri1_data                  = pair_index.first->UnpackTriangleVertices(pair_index.second, 1);
                    out_triangles[triangle_idx].a.x = tri1_data.v0.x;
                    out_triangles[triangle_idx].a.y = tri1_data.v0.y;
                    out_triangles[triangle_idx].a.z = tri1_data.v0.z;
                    out_triangles[triangle_idx].b.x = tri1_data.v1.x;
                    out_triangles[triangle_idx].b.y = tri1_data.v1.y;
                    out_triangles[triangle_idx].b.z = tri1_data.v1.z;
                    out_triangles[triangle_idx].c.x = tri1_data.v2.x;
                    out_triangles[triangle_idx].c.y = tri1_data.v2.y;
                    out_triangles[triangle_idx].c.z = tri1_data.v2.z;
                    triangle_idx++;
                }
            }
        }
        else
        {
            const rta::EncodedRtIp11BottomLevelBvh* blas_rtip11    = (rta::EncodedRtIp11BottomLevelBvh*)blas;
            const dxr::amd::TriangleNode*           triangle_node  = blas_rtip11->GetTriangleNode(*current_node);
            const auto&                             header_offsets = blas->GetHeader().GetBufferOffsets();

            if (current_node->GetByteOffset() < header_offsets.leaf_nodes)
            {
                return kRraErrorInvalidPointer;
            }

#ifdef _DEBUG
            const uint32_t node_index = (current_node->GetByteOffset() - header_offsets.leaf_nodes) / sizeof(dxr::amd::TriangleNode);

            const auto* triangle_nodes = reinterpret_cast<const dxr::amd::TriangleNode*>(blas_rtip11->GetLeafNodesData().data());
            const auto& tri_node       = triangle_nodes[node_index];

            RRA_ASSERT(&tri_node == triangle_node);
#endif  // DEBUG

            const auto& verts = triangle_node->GetVertices();

            // Copy the triangle vertices to the output pointer.
            const size_t vertex_size = sizeof(VertexPosition);
            memcpy(&out_triangles->a, &verts[0], vertex_size);
            memcpy(&out_triangles->b, &verts[1], vertex_size);
            memcpy(&out_triangles->c, &verts[2], vertex_size);

            if (current_node->GetType() == dxr::amd::NodeType::kAmdNodeTriangle1)
            {
                out_triangles++;
                memcpy(&out_triangles->a, &verts[2], vertex_size);
                memcpy(&out_triangles->b, &verts[1], vertex_size);
                memcpy(&out_triangles->c, &verts[3], vertex_size);
            }
        }
    }

    return kRraOk;
}

RraErrorCode RraBlasGetNodeVertexCount(uint64_t blas_index, uint32_t node_ptr, uint32_t* out_count)
{
    const auto&                       bottom_level_bvhs = data_set_.bvh_bundle->GetBottomLevelBvhs();
    const rta::EncodedBottomLevelBvh* blas              = dynamic_cast<rta::EncodedBottomLevelBvh*>(&(*bottom_level_bvhs[blas_index]));
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    if (current_node == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    // The incoming node id should be a triangle node. If it's not, we can't extract triangle data.
    if (current_node->IsTriangleNode())
    {
        if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
        {
            rta::EncodedRtIp31BottomLevelBvh* blas_rtip31 = (rta::EncodedRtIp31BottomLevelBvh*)blas;
            uint32_t                          pair_indices_count{};
            auto                              pair_indices = blas_rtip31->GetTrianglePairIndices(node_ptr, &pair_indices_count);

            // Leftmost 32 bits are pair_indices_idx and rightmost are vertex index. Since vertices may index into separate PrimitiveStructures.
            std::unordered_set<uint64_t> vertex_index_set{};
            uint32_t                     pair_indices_idx{0};
            for (uint32_t i = 0; i < pair_indices_count; ++i)
            {
                TrianglePairDesc desc{pair_indices[i].first->ReadTrianglePairDesc(pair_indices[i].second)};
                vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri0V0());
                vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri0V1());
                vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri0V2());

                if (desc.Tri1Valid())
                {
                    vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri1V0());
                    vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri1V1());
                    vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri1V2());
                }
                ++pair_indices_idx;
            }

            *out_count = (uint32_t)vertex_index_set.size();
        }
        else
        {
            uint32_t triangle_count{};
            RraBlasGetNodeTriangleCount(blas_index, node_ptr, &triangle_count);
            *out_count = (triangle_count == 1 ? 3 : 4);
        }
    }
    return kRraOk;
}

RraErrorCode RraBlasGetNodeVertices(uint64_t blas_index, uint32_t node_ptr, struct VertexPosition* out_vertices)
{
    const auto&                       bottom_level_bvhs = data_set_.bvh_bundle->GetBottomLevelBvhs();
    const rta::EncodedBottomLevelBvh* blas              = dynamic_cast<rta::EncodedBottomLevelBvh*>(&(*bottom_level_bvhs[blas_index]));
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    if (current_node == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    // The incoming node id should be a triangle node. If it's not, we can't extract triangle data.
    if (current_node->IsTriangleNode())
    {
        const auto& header_offsets = blas->GetHeader().GetBufferOffsets();

        if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
        {
            rta::EncodedRtIp31BottomLevelBvh* blas_rtip31 = (rta::EncodedRtIp31BottomLevelBvh*)blas;
            uint32_t                          pair_indices_count{};
            auto                              pair_indices = blas_rtip31->GetTrianglePairIndices(node_ptr, &pair_indices_count);

            // Leftmost 32 bits are pair_indices_idx and rightmost are vertex index. Since vertices may index into separate PrimitiveStructures.
            std::unordered_set<uint64_t> vertex_index_set{};
            {
                uint32_t pair_indices_idx{0};
                for (uint32_t i = 0; i < pair_indices_count; ++i)
                {
                    TrianglePairDesc desc{pair_indices[i].first->ReadTrianglePairDesc(pair_indices[i].second)};
                    vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri0V0());
                    vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri0V1());
                    vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri0V2());

                    if (desc.Tri1Valid())
                    {
                        vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri1V0());
                        vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri1V1());
                        vertex_index_set.insert(((uint64_t)pair_indices_idx << 32) | desc.Tri1V2());
                    }
                    ++pair_indices_idx;
                }
            }

            uint32_t vertex_idx{0};
            for (uint64_t vertex_index_pair : vertex_index_set)
            {
                uint32_t  pair_indices_idx = (uint32_t)(vertex_index_pair >> 32);
                uint32_t  vertex_index     = (uint32_t)vertex_index_pair;
                glm::vec3 v                = pair_indices[pair_indices_idx].first->ReadVertex(vertex_index, false);

                out_vertices[vertex_idx++] = {v.x, v.y, v.z};
            }
        }
        else
        {
            const uint32_t                          node_index  = (current_node->GetByteOffset() - header_offsets.leaf_nodes) / sizeof(dxr::amd::TriangleNode);
            const rta::EncodedRtIp11BottomLevelBvh* blas_rtip11 = (rta::EncodedRtIp11BottomLevelBvh*)blas;
            // Get the triangle vertices for this node index.
            const auto* triangle_nodes = reinterpret_cast<const dxr::amd::TriangleNode*>(blas_rtip11->GetLeafNodesData().data());
            const auto& tri_node       = triangle_nodes[node_index];
            const auto& verts          = tri_node.GetVertices();

            // Copy the triangle vertices to the output pointer.
            const size_t vertex_size = sizeof(VertexPosition);
            memcpy(&out_vertices[0], &verts[0], vertex_size);
            memcpy(&out_vertices[1], &verts[1], vertex_size);
            memcpy(&out_vertices[2], &verts[2], vertex_size);

            if (current_node->GetType() == dxr::amd::NodeType::kAmdNodeTriangle1)
            {
                memcpy(&out_vertices[3], &verts[3], vertex_size);
            }
        }
    }
    return kRraOk;
}

RraErrorCode RraBlasGetBoundingVolumeExtents(uint64_t blas_index, uint32_t node_ptr, BoundingVolumeExtents* out_extents)
{
    const rta::IBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    dxr::amd::NodePointer*           current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);
    dxr::amd::AxisAlignedBoundingBox bounding_box;

    RraErrorCode error_code = RraBvhGetNodeBoundingVolume(blas, current_node, bounding_box);
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

RraErrorCode RraBlasGetNodeObbIndex(uint64_t blas_index, uint32_t node_ptr, uint32_t* obb_index)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    glm::mat3    rotation{};
    RraErrorCode error_code = RraBvhGetNodeObbIndex(blas, node, obb_index);
    return error_code;
}

RraErrorCode RraBlasGetNodeBoundingVolumeOrientation(uint64_t blas_index, uint32_t node_ptr, float* out_rotation)
{
    const rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    glm::mat3    rotation{};
    RraErrorCode error_code = RraBvhGetNodeBoundingVolumeOrientation(blas, node, rotation);

    if (error_code != kRraOk)
    {
        return error_code;
    }

    std::memcpy(out_rotation, &rotation, sizeof(glm::mat3));
    return kRraOk;
}

