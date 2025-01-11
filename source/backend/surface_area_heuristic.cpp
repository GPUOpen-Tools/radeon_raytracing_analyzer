//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the Surface area heuristic calculations.
//=============================================================================

#include "surface_area_heuristic.h"

#include <float.h>
#include <math.h>
#include <numeric>
#include <execution>
#include <algorithm>

#include "bvh/rtip11/encoded_rt_ip_11_bottom_level_bvh.h"
#include "bvh/rtip11/encoded_rt_ip_11_top_level_bvh.h"
#include "bvh/dxr_definitions.h"
#include "public/rra_assert.h"
#include "public/rra_error.h"
#include "public/rra_rtip_info.h"
#include "rra_bvh_impl.h"
#include "rra_blas_impl.h"
#include "rra_data_set.h"
#include "rra_tlas_impl.h"
#include "bvh/rtip31/internal_node.h"
#include "bvh/rtip31/encoded_rt_ip_31_bottom_level_bvh.h"
#include "bvh/rtip31/encoded_rt_ip_31_top_level_bvh.h"
#include "bvh/rtip31/primitive_node.h"

// External reference to the global dataset.
extern RraDataSet data_set_;

namespace rra
{
    /// @brief Find the minimum value of 3 provided values.
    ///
    /// @param [in] value1 The first value.
    /// @param [in] value2 The second value.
    /// @param [in] value3 The third value.
    ///
    /// @return The minimum value.
    static float Min(float value1, float value2, float value3)
    {
        float min1 = std::min(value1, value2);
        return std::min(min1, value3);
    }

    /// @brief Find the minimum value of 4 provided values.
    ///
    /// @param [in] value1 The first value.
    /// @param [in] value2 The second value.
    /// @param [in] value3 The third value.
    /// @param [in] value4 The fourth value.
    ///
    /// @return The minimum value.
    static float Min(float value1, float value2, float value3, float value4)
    {
        return std::min(Min(value1, value2, value3), value4);
    }

    /// @brief Find the maximum value of 3 provided values.
    ///
    /// @param [in] value1 The first value.
    /// @param [in] value2 The second value.
    /// @param [in] value3 The third value.
    ///
    /// @return The maximum value.
    static float Max(float value1, float value2, float value3)
    {
        float min1 = std::max(value1, value2);
        return std::max(min1, value3);
    }

    /// @brief Find the maximum value of 3 provided values.
    ///
    /// @param [in] value1 The first value.
    /// @param [in] value2 The second value.
    /// @param [in] value3 The third value.
    ///
    /// @return The maximum value.
    static float Max(float value1, float value2, float value3, float value4)
    {
        return std::max(Max(value1, value2, value3), value4);
    }

    /// @brief Calculate the surface area of a bounding volume around a triangle.
    ///
    /// Uses the triangle coordinates to construct a bounding volume around the
    /// triangle.
    ///
    /// @param [in] triangle The triangle node structure describing the triangle.
    /// @param [in] tri_count The number of triangles in the node.
    ///
    /// @return The triangle node's bounding volume surface area.
    static float CalculateTriangleAABBSurfaceArea(const dxr::amd::TriangleNode& triangle, uint32_t tri_count)
    {
        // Calculate the triangle AABB.
        const auto& verts = triangle.GetVertices();

        BoundingVolumeExtents bounding_volume;
        if (tri_count == 1)
        {
            bounding_volume.min_x = Min(verts[0].x, verts[1].x, verts[2].x);
            bounding_volume.min_y = Min(verts[0].y, verts[1].y, verts[2].y);
            bounding_volume.min_z = Min(verts[0].z, verts[1].z, verts[2].z);
            bounding_volume.max_x = Max(verts[0].x, verts[1].x, verts[2].x);
            bounding_volume.max_y = Max(verts[0].y, verts[1].y, verts[2].y);
            bounding_volume.max_z = Max(verts[0].z, verts[1].z, verts[2].z);
        }
        else if (tri_count == 2)
        {
            bounding_volume.min_x = Min(verts[0].x, verts[1].x, verts[2].x, verts[3].x);
            bounding_volume.min_y = Min(verts[0].y, verts[1].y, verts[2].y, verts[3].y);
            bounding_volume.min_z = Min(verts[0].z, verts[1].z, verts[2].z, verts[3].z);
            bounding_volume.max_x = Max(verts[0].x, verts[1].x, verts[2].x, verts[3].x);
            bounding_volume.max_y = Max(verts[0].y, verts[1].y, verts[2].y, verts[3].y);
            bounding_volume.max_z = Max(verts[0].z, verts[1].z, verts[2].z, verts[3].z);
        }

        // Calculate the surface area.
        float aabb_surface_area = 0.0f;
        if (RraBvhGetBoundingVolumeSurfaceArea(&bounding_volume, &aabb_surface_area) == kRraOk)
        {
            return aabb_surface_area;
        }

        return 1.0f;
    }

    /// @brief Get a reference to the array of child nodes for a particular node.
    ///
    /// Assumes the parent node is a box16 or box32 node.
    ///
    /// @param [in] root_node      The parent node.
    /// @param [in] interior_nodes The array of interior nodes.
    /// @param [in] node_offset    The offset into the interior nodes array.
    ///
    /// @return A reference to the array of child nodes.
    static std::array<dxr::amd::NodePointer, 8> GetChildNodeArray(const dxr::amd::NodePointer root_node,
                                                                  const std::vector<uint8_t>& interior_nodes,
                                                                  uint32_t                    node_offset)
    {
        RRA_ASSERT(root_node.IsBoxNode());
        if (root_node.IsFp32BoxNode())
        {
            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
            {
                const auto                           node = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[node_offset]);
                std::array<dxr::amd::NodePointer, 8> children{};
                node->DecodeChildrenOffsets(reinterpret_cast<uint32_t*>(children.data()));
                return children;
            }
            else
            {
                const dxr::amd::Float32BoxNode*      box_node = reinterpret_cast<const dxr::amd::Float32BoxNode*>(&interior_nodes[node_offset]);
                std::array<dxr::amd::NodePointer, 8> children_padded{};
                const auto&                          children = box_node->GetChildren();
                std::copy(children.begin(), children.end(), children_padded.begin());

                return children_padded;
            }
        }
        else
        {
            const dxr::amd::Float16BoxNode*      box_node = reinterpret_cast<const dxr::amd::Float16BoxNode*>(&interior_nodes[node_offset]);
            std::array<dxr::amd::NodePointer, 8> children_padded{};
            const auto&                          children = box_node->GetChildren();
            std::copy(children.begin(), children.end(), children_padded.begin());

            return children_padded;
        }
    }

    /// @brief Recursive function to calculate the surface area heuristic for a given BLAS.
    ///
    /// The BLAS will be traversed starting at the provided node given and the surface area heuristic will be
    /// calculated for each child node.
    ///
    /// @param [in] blas      The bottom level acceleration structure to use.
    /// @param [in] root_node The root node of the BLAS to start from.
    ///
    /// @return The surface area heuristic for the node passed in.
    static float CalculateSAHForBlasNode(rta::EncodedBottomLevelBvh* blas, const dxr::amd::NodePointer root_node)
    {
        float sah          = 0.0f;
        float sub_tree_sah = 0.0f;

        if (root_node.IsBoxNode())
        {
            float total_child_area = 0.0f;

            const auto  node_offset    = root_node.GetByteOffset() - blas->GetHeader().GetBufferOffsets().interior_nodes;
            const auto& interior_nodes = blas->GetInteriorNodesData();

            if (interior_nodes.size() == 0)
            {
                return 1.0f;
            }
            const auto& child_array      = GetChildNodeArray(root_node, interior_nodes, node_offset);
            float       out_surface_area = 0.0f;
            for (const auto& child_node : child_array)
            {
                // find SAH for child nodes.
                sub_tree_sah += CalculateSAHForBlasNode(blas, child_node);
                if (RraBlasGetSurfaceAreaImpl(blas, &child_node, &out_surface_area) == kRraOk)
                {
                    total_child_area += out_surface_area;
                }
            }

            // Take that as ratio of the current node.
            out_surface_area = 0.0;
            if (RraBlasGetSurfaceAreaImpl(blas, &root_node, &out_surface_area) == kRraOk)
            {
                sah = 0.25f * (total_child_area / out_surface_area);
            }

            if (out_surface_area == 0.0)
            {
                sah = 0.0f;
            }

            blas->SetInteriorNodeSurfaceAreaHeuristic(root_node, sah);
        }
        else if (root_node.IsTriangleNode())
        {
            // Get SAH from BLAS since it's already been computed for triangle nodes.
            sah = blas->GetLeafNodeSurfaceAreaHeuristic(root_node);
        }

        return sah + sub_tree_sah;
    }

    /// @brief Recursive function to calculate the surface area heuristic for a given TLAS.
    ///
    /// The TLAS will be traversed starting at the provided node given and the surface area heuristic will be
    /// calculated for each child node.
    ///
    /// @param [in] tlas      The top level acceleration structure to use.
    /// @param [in] root_node The root node of the BLAS to start from.
    ///
    /// @return The surface area heuristic for the node passed in.
    static float CalculateSAHForTlasNode(rta::EncodedRtIp11TopLevelBvh* tlas, const dxr::amd::NodePointer root_node)
    {
        float sah          = 0.0f;
        float sub_tree_sah = 0.0f;

        if (root_node.IsBoxNode())
        {
            float total_child_area = 0.0f;

            const auto  node_offset    = root_node.GetByteOffset() - tlas->GetHeader().GetBufferOffsets().interior_nodes;
            const auto& interior_nodes = tlas->GetInteriorNodesData();

            if (interior_nodes.size() == 0)
            {
                return 1.0f;
            }

            const auto& child_array      = GetChildNodeArray(root_node, interior_nodes, node_offset);
            float       out_surface_area = 0.0f;
            for (const auto& child_node : child_array)
            {
                // Find SAH for child nodes.
                sub_tree_sah += CalculateSAHForTlasNode(tlas, child_node);
                if (RraTlasGetSurfaceAreaImpl(tlas, &child_node, &out_surface_area) == kRraOk)
                {
                    total_child_area += static_cast<float>(out_surface_area);
                }
            }

            // Take that as ratio of the current node.
            out_surface_area = 0.0;
            if (RraTlasGetSurfaceAreaImpl(tlas, &root_node, &out_surface_area) == kRraOk)
            {
                if (out_surface_area > 0.0f)
                {
                    sah = std::min(1.0f, (total_child_area / (static_cast<float>(out_surface_area))) / 4.0f);
                }
                else
                {
                    sah = 1.0f;
                }
            }

            tlas->SetInteriorNodeSurfaceAreaHeuristic(root_node, sah);
        }
        else if (root_node.IsInstanceNode())
        {
            // Get SAH from BLAS since it's already been computed for triangle nodes.
            const rta::EncodedRtIp11BottomLevelBvh* blas = nullptr;
            if (RraTlasGetBlasFromInstanceNode(tlas, &root_node, &blas) == kRraOk)
            {
                sub_tree_sah     = blas->GetSurfaceAreaHeuristic();
                float child_area = 0.0f;

                if (blas->IsEmpty())
                {
                    sah = 0.0f;
                }
                else if (RraTlasGetNodeTransformedSurfaceArea(tlas, &root_node, blas, &child_area) == kRraOk)
                {
                    sah                     = 1.0f;
                    float tlas_surface_area = 0.0f;
                    if (RraTlasGetSurfaceAreaImpl(tlas, &root_node, &tlas_surface_area) == kRraOk)
                    {
                        // Account for rounding errors.
                        if (tlas_surface_area < child_area)
                        {
                            tlas_surface_area = child_area;
                        }

                        // Account for invalid surface area.
                        if (tlas_surface_area > 0)
                        {
                            sah = child_area / tlas_surface_area;
                        }
                    }
                }
                else
                {
                    sah = std::numeric_limits<float>::quiet_NaN();
                }
                tlas->SetLeafNodeSurfaceAreaHeuristic(root_node, sah);
            }
            else
            {
                RRA_ASSERT_FAIL("Can't calculate SAH from instance node.");
            }
        }

        return sah + sub_tree_sah;
    }

    /// @brief Get all the triangle NodePointers from the BLAS
    ///
    /// @param [in] blas_index Index of BLAS to get the triangle NodePointers from.
    /// @param [out] triangle_nodes The triangle nodes are written into this vector.
    ///
    /// @returns The error code.
    RraErrorCode GetBlasTriangleNodes(uint64_t blas_index, std::vector<dxr::amd::NodePointer>& triangle_nodes)
    {
        uint32_t root_node = UINT32_MAX;
        RRA_BUBBLE_ON_ERROR(RraBvhGetRootNodePtr(&root_node));

        uint32_t child_node_count;
        uint32_t triangle_count;

        uint32_t triangle_node_count{};
        RraBlasGetTriangleNodeCount(blas_index, &triangle_node_count);
        triangle_nodes.reserve(triangle_node_count);

        std::vector<uint32_t> traversal_stack{};
        traversal_stack.reserve(64);  // It is rare for the traversal stack to get deeper than ~28 so this should be sufficient memory to reserve.
        traversal_stack.push_back(root_node);

        // Memoized traversal of the tree.
        while (!traversal_stack.empty())
        {
            uint32_t current_node{traversal_stack.back()};
            traversal_stack.pop_back();

            // Add the triangles to the list.
            RRA_BUBBLE_ON_ERROR(RraBlasGetChildNodeCount(blas_index, current_node, &child_node_count));
            std::array<uint32_t, 8> child_nodes{};
            RRA_BUBBLE_ON_ERROR(RraBlasGetChildNodes(blas_index, current_node, child_nodes.data()));
            traversal_stack.insert(traversal_stack.end(), child_nodes.data(), child_nodes.data() + child_node_count);

            // Get the triangle nodes. If this is not a triangle the triangle count is 0.
            RRA_BUBBLE_ON_ERROR(RraBlasGetNodeTriangleCount(blas_index, current_node, &triangle_count));

            // Continue with processing the node if it's a triangle node with 1 or more triangles within.
            if (triangle_count > 0)
            {
                triangle_nodes.push_back(dxr::amd::NodePointer(current_node));
            }
        }
        return kRraOk;
    }

    /// @brief Calculate the surface area heuristic for a given RtIp1.1 BLAS.
    ///
    /// @param [in] blas The bottom level acceleration structure index.
    ///
    /// @returns Error code.
    static RraErrorCode CalcBlasSAH(rta::EncodedRtIp11BottomLevelBvh* blas)
    {
        RRA_ASSERT(blas != nullptr);
        if (!blas)
        {
            return kRraErrorInvalidPointer;
        }

        if (blas->IsEmpty())
        {
            blas->SetSurfaceAreaHeuristic(0.0f);
            return kRraOk;
        }

        // For each triangle node, calculate the SAH.
        const auto* triangle_nodes = reinterpret_cast<const dxr::amd::TriangleNode*>(blas->GetLeafNodesData().data());
        const auto& header_offsets = blas->GetHeader().GetBufferOffsets();

        std::vector<dxr::amd::NodePointer> tri_node_pointers;
        RRA_BUBBLE_ON_ERROR(GetBlasTriangleNodes(blas->GetID(), tri_node_pointers));

        for (const auto& node_ptr : tri_node_pointers)
        {
            if (node_ptr.GetByteOffset() < header_offsets.leaf_nodes)
            {
                // Bad address for a triangle.
                continue;
            }

            const uint32_t node_index = (node_ptr.GetByteOffset() - header_offsets.leaf_nodes) / sizeof(dxr::amd::TriangleNode);

            uint32_t tri_count{};
            RraBlasGetNodeTriangleCount(blas->GetID(), node_ptr.GetID(), &tri_count);

            if (node_ptr.GetType() == dxr::amd::NodeType::kAmdNodeTriangle0)
            {
                tri_count = 1;
            }
            else if (node_ptr.GetType() == dxr::amd::NodeType::kAmdNodeTriangle1)
            {
                tri_count = 2;
            }

            float aabb_surface_area         = CalculateTriangleAABBSurfaceArea(triangle_nodes[node_index], tri_count);
            float triangle_surface_area     = RraBlasGetTriangleSurfaceArea(triangle_nodes[node_index], tri_count);
            float triangle_avg_surface_area = triangle_surface_area / tri_count;
            float sah                       = 0.0f;

            // Make sure the surface area of the triangle bounding volume is larger than the triangle surface area.
            if (aabb_surface_area >= triangle_surface_area && aabb_surface_area > FLT_MIN)
            {
                // Multiply triangle area by 2, to account for probability of ray going through front or back face.
                sah = (2.0f * triangle_avg_surface_area) / aabb_surface_area;

                // SAH is currently in the range [0.0, 0.5] since a triangle can occupy at most half the space of its bounding volume.
                // So multiply by 2.0 to normalize the SAH to a range [0.0, 1.0].
                sah *= 2.0f;
            }

            // Mathematically SAH should not ever be greater than 1.0, but with really problematic triangles (extremely long and thin)
            // floating point errors can push it over. I've seen as high as 1.454 in the Deathloop trace.
            if (!isnan(sah))
            {
                if (sah > 1.01f)
                {
                    // SAH has passed threshold, so assume this triangle is problematic and mark it as 0.
                    sah = 0.0f;
                }
                else
                {
                    // Otherwise it's only a small floating point error so clamp it to a valid value.
                    sah = std::min(sah, 1.0f);
                }
            }

            // Store the SAH back to the BLAS.
            blas->SetLeafNodeSurfaceAreaHeuristic(node_ptr.GetRawPointer(), sah);
        }

        // Iterate over the box nodes and calculate their SAH values.
        // Top level node doesn't exist in the data so needs to be created. Assumed to be a Box32.
        dxr::amd::NodePointer root_node = dxr::amd::NodePointer(dxr::amd::NodeType::kAmdNodeBoxFp32, dxr::amd::kAccelerationStructureHeaderSize);
        float                 sah       = CalculateSAHForBlasNode(blas, root_node);

        blas->SetSurfaceAreaHeuristic(sah);

        return kRraOk;
    }

    /// @brief Calculate the surface area heuristic for a given RtIp3.1 BLAS.
    ///
    /// @param [in] blas The bottom level acceleration structure index.
    ///
    /// @returns Error code.
    static RraErrorCode CalcBlasSAH(rta::EncodedRtIp31BottomLevelBvh* blas)
    {
        blas->ComputeSurfaceAreaHeuristic();
        return kRraOk;
    }

    /// @brief Calculate the surface area heuristic for a given TLAS.
    ///
    /// @param [in] tlas The top level acceleration structure.
    static void CalcTlasSAH(rta::EncodedTopLevelBvh* tlas)
    {
        // Iterate over the box nodes and calculate their SAH values.
        // Top level node doesn't exist in the data so needs to be created. Assumed to be a Box32.
        dxr::amd::NodePointer root_node = dxr::amd::NodePointer(dxr::amd::NodeType::kAmdNodeBoxFp32, dxr::amd::kAccelerationStructureHeaderSize);
        if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
        {
            // TODO: Implement this.
        }
        else
        {
            float sah = CalculateSAHForTlasNode((rta::EncodedRtIp11TopLevelBvh*)tlas, root_node);
            RRA_UNUSED(sah);
        }
    }

    /// @brief Recursive function to calculate the maximum surface area heuristic value for a given acceleration structure.
    ///
    /// The acceleration structure will be traversed starting at the provided node given and the maximum surface area heuristic will be
    /// updated if necessary for each child node.
    ///
    /// @param [in]      bvh       The acceleration structure to use.
    /// @param [in]      root_node The root node of the acceleration to start from.
    /// @param [in]      tri_only  All non-triangle nodes will be ignored if this is true.
    /// @param [in, out] min_sah   The minimum surface area heuristic found.
    static void GetMinimumSurfaceAreaHeuristicImpl(const rta::IBvh* bvh, const dxr::amd::NodePointer root_node, bool tri_only, float* min_sah)
    {
        if (root_node.IsTriangleNode() || !tri_only)
        {
            float sah = 0.0f;
            if (RraBvhGetSurfaceAreaHeuristic(bvh, root_node, &sah) != kRraOk)
            {
                return;
            }

            *min_sah = std::min(*min_sah, sah);
        }

        if (root_node.IsBoxNode())
        {
            const auto  node_offset    = root_node.GetByteOffset() - bvh->GetHeader().GetBufferOffsets().interior_nodes;
            const auto& interior_nodes = bvh->GetInteriorNodesData();
            uint32_t    child_count{};
            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
            {
                const auto node = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[node_offset]);
                child_count     = node->ValidChildCount();
            }
            else
            {
                const dxr::amd::Float32BoxNode* box_node = reinterpret_cast<const dxr::amd::Float32BoxNode*>(&interior_nodes[node_offset]);
                child_count                              = box_node->GetValidChildCount();
            }

            const auto& child_array = GetChildNodeArray(root_node, interior_nodes, node_offset);
            for (uint32_t child_index = 0; child_index < child_count; child_index++)
            {
                // Find SAH for child nodes.
                const auto& child_node = child_array[child_index];
                GetMinimumSurfaceAreaHeuristicImpl(bvh, child_node, tri_only, min_sah);
            }
        }
    }

    /// @brief Recursive function to calculate the total surface area heuristic value for a given acceleration structure.
    ///
    /// The acceleration structure will be traversed starting at the provided node given and the surface area heuristic values will
    /// be summed. This will be used to calculate an average value.
    ///
    /// @param [in]      bvh        The acceleration structure to use.
    /// @param [in]      root_node  The root node of the acceleration to start from.
    /// @param [in]      tri_only   All non-triangle nodes will be ignored if this is true.
    /// @param [in, out] total_sah  The total (summed) surface area heuristic value.
    /// @param [in, out] node_count The number of nodes processed.
    static void GetTotalSurfaceAreaHeuristicImpl(const rta::IBvh*            bvh,
                                                 const dxr::amd::NodePointer root_node,
                                                 bool                        tri_only,
                                                 float*                      total_sah,
                                                 int32_t*                    node_count)
    {
        if (root_node.IsTriangleNode() || !tri_only)
        {
            float sah = 0.0f;
            if (RraBvhGetSurfaceAreaHeuristic(bvh, root_node, &sah) != kRraOk)
            {
                return;
            }

            (*node_count)++;
            *total_sah += sah;
        }

        if (root_node.IsBoxNode())
        {
            const auto  node_offset    = root_node.GetByteOffset() - bvh->GetHeader().GetBufferOffsets().interior_nodes;
            const auto& interior_nodes = bvh->GetInteriorNodesData();
            assert(node_offset < interior_nodes.size());

            uint32_t child_count{};
            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
            {
                const auto node = reinterpret_cast<const QuantizedBVH8BoxNode*>(&interior_nodes[node_offset]);
                child_count     = node->ValidChildCount();
            }
            else
            {
                const dxr::amd::Float32BoxNode* box_node = reinterpret_cast<const dxr::amd::Float32BoxNode*>(&interior_nodes[node_offset]);
                child_count                              = box_node->GetValidChildCount();
            }

            const auto& child_array = GetChildNodeArray(root_node, interior_nodes, node_offset);
            for (uint32_t child_index = 0; child_index < child_count; child_index++)
            {
                // Find SAH for child nodes.
                const auto& child_node = child_array[child_index];
                GetTotalSurfaceAreaHeuristicImpl(bvh, child_node, tri_only, total_sah, node_count);
            }
        }
    }

    RraErrorCode CalculateSurfaceAreaHeuristics(RraDataSet& data_set)
    {
        // Calculate BLAS SAH.
        const auto&           bottom_level_bvhs = data_set.bvh_bundle->GetBottomLevelBvhs();
        std::vector<uint32_t> blas_indices(bottom_level_bvhs.size());
        std::iota(blas_indices.begin(), blas_indices.end(), 0);

        std::for_each(std::execution::par, blas_indices.begin(), blas_indices.end(), [&](uint32_t blas_index) {
            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
            {
                rta::EncodedRtIp31BottomLevelBvh* blas = (rta::EncodedRtIp31BottomLevelBvh*)bottom_level_bvhs[blas_index].get();
                if (blas == nullptr)
                {
                    return;
                }
                CalcBlasSAH(blas);
            }
            else
            {
                rta::EncodedRtIp11BottomLevelBvh* blas = (rta::EncodedRtIp11BottomLevelBvh*)bottom_level_bvhs[blas_index].get();
                if (blas == nullptr)
                {
                    return;
                }
                CalcBlasSAH(blas);
            }
        });

        // Calculate the SAH for each TLAS.
        // The leaf nodes here will be an instance node/BLAS.
        const auto&           top_level_bvhs = data_set.bvh_bundle->GetTopLevelBvhs();
        std::vector<uint32_t> tlas_indices(top_level_bvhs.size());
        std::iota(blas_indices.begin(), blas_indices.end(), 0);

        std::for_each(std::execution::par, tlas_indices.begin(), tlas_indices.end(), [&](uint32_t tlas_index) {
            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
            {
                rta::EncodedRtIp31TopLevelBvh* tlas = (rta::EncodedRtIp31TopLevelBvh*)top_level_bvhs[tlas_index].get();
                if (tlas == nullptr)
                {
                    return;
                }
                CalcTlasSAH(tlas);
            }
            else
            {
                rta::EncodedRtIp11TopLevelBvh* tlas = (rta::EncodedRtIp11TopLevelBvh*)top_level_bvhs[tlas_index].get();
                if (tlas == nullptr)
                {
                    return;
                }
                CalcTlasSAH(tlas);
            }
        });

        return kRraOk;
    }

    float GetMinimumSurfaceAreaHeuristic(const rta::IBvh* bvh, const dxr::amd::NodePointer node_ptr, bool tri_only)
    {
        float min_sah = 1.0f;
        GetMinimumSurfaceAreaHeuristicImpl(bvh, node_ptr, tri_only, &min_sah);

        return min_sah;
    }

    float GetAverageSurfaceAreaHeuristic(const rta::IBvh* bvh, const dxr::amd::NodePointer node_ptr, bool tri_only)
    {
        float   total      = 0.0f;
        int32_t node_count = 0;
        GetTotalSurfaceAreaHeuristicImpl(bvh, node_ptr, tri_only, &total, &node_count);

        if (node_count <= 0)
        {
            return 0.0f;
        }

        float avg = total / static_cast<float>(node_count);

        if (avg > 1.0f)
        {
            return 1.0f;
        }

        return avg;
    }

}  // namespace rra
