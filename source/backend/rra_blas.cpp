//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the BLAS interface.
///
/// Contains public functions specific to the BLAS.
//=============================================================================

#include "rra_blas_impl.h"

#include <math.h>  // for sqrt

#include "bvh/encoded_rt_ip_11_bottom_level_bvh.h"
#include "bvh/flags_util.h"
#include "public/rra_assert.h"
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

rta::EncodedRtIp11BottomLevelBvh* RraBlasGetBlasFromBlasIndex(uint64_t blas_index)
{
    RRA_ASSERT(data_set_.bvh_bundle.get() != nullptr);
    const auto& bottom_level_bvhs = data_set_.bvh_bundle->GetBottomLevelBvhs();
    if (blas_index >= bottom_level_bvhs.size())
    {
        return nullptr;
    }
    return dynamic_cast<rta::EncodedRtIp11BottomLevelBvh*>(&(*bottom_level_bvhs[blas_index]));
}

RraErrorCode RraBlasGetBaseAddress(uint64_t blas_index, uint64_t* out_address)
{
    const rta::IEncodedRtIp11Bvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    *out_address = blas->GetVirtualAddress();

    return kRraOk;
}

bool RraBlasIsEmpty(uint64_t blas_index)
{
    const rta::IEncodedRtIp11Bvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
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
    *out_node_count  = blas->GetNodeCount(rta::BvhNodeFlags::kNone);

    return kRraOk;
}

RraErrorCode RraBlasGetChildNodeCount(uint64_t blas_index, uint32_t parent_node, uint32_t* out_child_count)
{
    const rta::IEncodedRtIp11Bvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    return RraBvhGetChildNodeCount(blas, parent_node, out_child_count);
}

RraErrorCode RraBlasGetChildNodes(uint64_t blas_index, uint32_t parent_node, uint32_t* out_child_nodes)
{
    const rta::IEncodedRtIp11Bvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
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
    const rta::IEncodedRtIp11Bvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    return RraBvhGetChildNodePtr(blas, parent_node, child_index, out_node_ptr);
}

RraErrorCode RraBlasGetNodeBaseAddress(uint64_t blas_index, uint32_t node_ptr, uint64_t* out_address)
{
    const rta::IEncodedRtIp11Bvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const auto base_addr = blas->GetVirtualAddress();

    const dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);

    *out_address = base_addr + blas->GetHeader().GetMetaDataSize() + node->GetGpuVirtualAddress();
    return kRraOk;
}

RraErrorCode RraBlasGetNodeParentBaseAddress(uint64_t blas_index, uint32_t node_ptr, uint64_t* out_address)
{
    const rta::IEncodedRtIp11Bvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }
    const auto                   base_addr   = blas->GetVirtualAddress();
    const dxr::amd::NodePointer* node        = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);
    const dxr::amd::NodePointer  parent_node = blas->GetParentNode(node);

    *out_address = base_addr + blas->GetHeader().GetMetaDataSize() + parent_node.GetGpuVirtualAddress();

    return kRraOk;
}

RraErrorCode RraBlasGetSurfaceArea(uint64_t blas_index, uint32_t node_ptr, float* out_surface_area)
{
    const rta::EncodedRtIp11BottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    return RraBlasGetSurfaceAreaImpl(blas, reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr), out_surface_area);
}

RraErrorCode RraBlasGetSurfaceAreaHeuristic(uint64_t blas_index, uint32_t node_ptr, float* out_surface_area_heuristic)
{
    uint32_t tri_count{};
    RraBlasGetNodeTriangleCount(blas_index, node_ptr, &tri_count);

    const rta::EncodedRtIp11BottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
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

RraErrorCode RraBlasGetSurfaceAreaImpl(const rta::EncodedRtIp11BottomLevelBvh* blas, const dxr::amd::NodePointer* node_ptr, float* out_surface_area)
{
    if (node_ptr->IsTriangleNode())
    {
        const auto& header_offsets = blas->GetHeader().GetBufferOffsets();

        if (node_ptr->GetByteOffset() < header_offsets.leaf_nodes)
        {
            *out_surface_area = std::numeric_limits<float>::quiet_NaN();
            return kRraOk;
        }

        const dxr::amd::TriangleNode* triangle_node = blas->GetTriangleNode(*node_ptr);

#ifdef _DEBUG
        const uint32_t node_index = (node_ptr->GetByteOffset() - header_offsets.leaf_nodes) / sizeof(dxr::amd::TriangleNode);

        // Get the triangle vertices for this node index.
        const auto* triangle_nodes = reinterpret_cast<const dxr::amd::TriangleNode*>(blas->GetLeafNodesData().data());
        const auto& tri_node       = triangle_nodes[node_index];

        RRA_ASSERT(&tri_node == triangle_node);
#endif  // DEBUG

        uint32_t tri_count{};
        RraBlasGetNodeTriangleCount(blas->GetID(), node_ptr->GetRawPointer(), &tri_count);

        *out_surface_area = RraBlasGetTriangleSurfaceArea(*triangle_node, tri_count);
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
    const rta::EncodedRtIp11BottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
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
    const rta::EncodedRtIp11BottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
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
    const rta::EncodedRtIp11BottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
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
    const rta::EncodedRtIp11BottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

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

RraErrorCode RraBlasGetTriangleNodeCount(uint64_t blas_index, uint32_t* out_triangle_count)
{
    const rta::EncodedRtIp11BottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    if (blas->GetHeader().GetGeometryType() == rta::BottomLevelBvhGeometryType::kTriangle)
    {
        *out_triangle_count = blas->GetHeader().GetLeafNodeCount();
    }
    else
    {
        *out_triangle_count = 0;
    }

    return kRraOk;
}

RraErrorCode RraBlasGetProceduralNodeCount(uint64_t blas_index, uint32_t* out_procedural_Node_count)
{
    const rta::EncodedRtIp11BottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

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

RraErrorCode RraBlasGetGeometryIndex(uint64_t blas_index, uint32_t node_ptr, uint32_t* out_geometry_index)
{
    const rta::EncodedRtIp11BottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);
    if (current_node->IsTriangleNode())
    {
        const auto& header_offsets = blas->GetHeader().GetBufferOffsets();
        if (current_node->GetByteOffset() < header_offsets.leaf_nodes)
        {
            *out_geometry_index = 0;
            return kRraErrorInvalidPointer;
        }

        const dxr::amd::TriangleNode* triangle_node = blas->GetTriangleNode(*current_node);

#ifdef _DEBUG
        const uint32_t index = (current_node->GetByteOffset() - header_offsets.leaf_nodes) / sizeof(dxr::amd::TriangleNode);

        const auto* triangle_nodes = reinterpret_cast<const dxr::amd::TriangleNode*>(blas->GetLeafNodesData().data());
        const auto& tri_node       = triangle_nodes[index];

        RRA_ASSERT(&tri_node == triangle_node);
#endif  // DEBUG

        *out_geometry_index = triangle_node->GetGeometryIndex();
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
    const rta::EncodedRtIp11BottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

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
    const rta::EncodedRtIp11BottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_geometry_count = (uint32_t)blas->GetGeometryInfos().size();

    return kRraOk;
}

RraErrorCode RraBlasGetPrimitiveIndex(uint64_t blas_index, uint32_t node_ptr, uint32_t local_primitive_index, uint32_t* out_primitive_index)
{
    const rta::EncodedRtIp11BottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const dxr::amd::NodePointer* current_node = reinterpret_cast<dxr::amd::NodePointer*>(&node_ptr);
    if (current_node->IsTriangleNode())
    {
        const dxr::amd::TriangleNode* triangle_node  = blas->GetTriangleNode(*current_node);
        const auto&                   header_offsets = blas->GetHeader().GetBufferOffsets();
        if (current_node->GetByteOffset() < header_offsets.leaf_nodes)
        {
            *out_primitive_index = 0;
            return kRraErrorInvalidPointer;
        }

#ifdef _DEBUG
        const uint32_t node_index = (current_node->GetByteOffset() - header_offsets.leaf_nodes) / sizeof(dxr::amd::TriangleNode);

        // Get the primitive index for this node index.
        const auto* triangle_nodes = reinterpret_cast<const dxr::amd::TriangleNode*>(blas->GetLeafNodesData().data());
        const auto& tri_node       = triangle_nodes[node_index];

        RRA_ASSERT(&tri_node == triangle_node);
#endif  // DEBUG

        // No way to exctract more than 2 primitive indexes as of yet.
        RRA_ASSERT(local_primitive_index < 2);

        *out_primitive_index =
            triangle_node->GetPrimitiveIndex(local_primitive_index == 0 ? dxr::amd::NodeType::kAmdNodeTriangle0 : dxr::amd::NodeType::kAmdNodeTriangle1);
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
    const rta::EncodedRtIp11BottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);

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
    const auto&                             bottom_level_bvhs = data_set_.bvh_bundle->GetBottomLevelBvhs();
    const rta::EncodedRtIp11BottomLevelBvh* blas              = dynamic_cast<rta::EncodedRtIp11BottomLevelBvh*>(&(*bottom_level_bvhs[blas_index]));
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
        const dxr::amd::TriangleNode* triangle_node = blas->GetTriangleNode(*current_node);
        if (triangle_node == nullptr)
        {
            return kRraErrorInvalidPointer;
        }
        *out_is_inactive = triangle_node->IsInactive(current_node->GetType());
        return kRraOk;
    }
    else if (current_node->IsProceduralNode())
    {
        const dxr::amd::ProceduralNode* procedural_node = blas->GetProceduralNode(*current_node);
        if (procedural_node == nullptr)
        {
            return kRraErrorInvalidPointer;
        }
        *out_is_inactive = procedural_node->IsInactive();
        return kRraOk;
    }

    return kRraErrorInvalidChildNode;
}

RraErrorCode RraBlasGetNodeTriangleCount(uint64_t blas_index, uint32_t node_ptr, uint32_t* out_triangle_count)
{
    const auto&                             bottom_level_bvhs = data_set_.bvh_bundle->GetBottomLevelBvhs();
    const rta::EncodedRtIp11BottomLevelBvh* blas              = dynamic_cast<rta::EncodedRtIp11BottomLevelBvh*>(&(*bottom_level_bvhs[blas_index]));
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

    // The incoming node id should be a triangle node. If it's not, we can't extract triangle data.
    if (current_node->GetType() == dxr::amd::NodeType::kAmdNodeTriangle0)
    {
        *out_triangle_count = 1;
    }
    else if (current_node->GetType() == dxr::amd::NodeType::kAmdNodeTriangle1)
    {
        *out_triangle_count = 2;
    }
    else
    {
        *out_triangle_count = 0;
    }
    return kRraOk;
}

RraErrorCode RraBlasGetNodeTriangles(uint64_t blas_index, uint32_t node_ptr, TriangleVertices* out_triangles)
{
    const auto&                             bottom_level_bvhs = data_set_.bvh_bundle->GetBottomLevelBvhs();
    const rta::EncodedRtIp11BottomLevelBvh* blas              = dynamic_cast<rta::EncodedRtIp11BottomLevelBvh*>(&(*bottom_level_bvhs[blas_index]));
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
        const dxr::amd::TriangleNode* triangle_node  = blas->GetTriangleNode(*current_node);
        const auto&                   header_offsets = blas->GetHeader().GetBufferOffsets();

        if (current_node->GetByteOffset() < header_offsets.leaf_nodes)
        {
            return kRraErrorInvalidPointer;
        }

#ifdef _DEBUG
        const uint32_t node_index = (current_node->GetByteOffset() - header_offsets.leaf_nodes) / sizeof(dxr::amd::TriangleNode);

        // Get the triangle vertices for this node index.
        const auto* triangle_nodes = reinterpret_cast<const dxr::amd::TriangleNode*>(blas->GetLeafNodesData().data());
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

    return kRraOk;
}

RraErrorCode RraBlasGetNodeVertices(uint64_t blas_index, uint32_t node_ptr, struct VertexPosition* out_vertices)
{
    const auto&                             bottom_level_bvhs = data_set_.bvh_bundle->GetBottomLevelBvhs();
    const rta::EncodedRtIp11BottomLevelBvh* blas              = dynamic_cast<rta::EncodedRtIp11BottomLevelBvh*>(&(*bottom_level_bvhs[blas_index]));
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
        const auto&    header_offsets = blas->GetHeader().GetBufferOffsets();
        const uint32_t node_index     = (current_node->GetByteOffset() - header_offsets.leaf_nodes) / sizeof(dxr::amd::TriangleNode);

        // Get the triangle vertices for this node index.
        const auto* triangle_nodes = reinterpret_cast<const dxr::amd::TriangleNode*>(blas->GetLeafNodesData().data());
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

    return kRraOk;
}

RraErrorCode RraBlasGetBoundingVolumeExtents(uint64_t blas_index, uint32_t node_ptr, BoundingVolumeExtents* out_extents)
{
    const rta::IEncodedRtIp11Bvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
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
    const rta::IEncodedRtIp11Bvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    const rta::IRtIp11AccelerationStructureHeader& header = blas->GetHeader();

    // Internally, the build flags have the same values as those in the vulkan enum passed in, so currently just
    // need to do a simple cast. If the internal structure changes, then the internal flags will need mapping
    // to the vulkan API enum.
    *out_flags = static_cast<VkBuildAccelerationStructureFlagBitsKHR>(header.GetPostBuildInfo().GetBuildFlags());
    return kRraOk;
}

RraErrorCode RraBlasGetSizeInBytes(uint64_t blas_index, uint32_t* out_size_in_bytes)
{
    const rta::IEncodedRtIp11Bvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_size_in_bytes = blas->GetHeader().GetFileSize();
    return kRraOk;
}
