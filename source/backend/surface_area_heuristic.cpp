//=============================================================================
// Copyright (c) 2021-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the Surface area heuristic calculations.
//=============================================================================

#include "surface_area_heuristic.h"

#include <float.h>
#include <math.h>
#include <algorithm>
#include <execution>
#include <numeric>

#include "public/rra_assert.h"
#include "public/rra_error.h"
#include "public/rra_rtip_info.h"

#include "bvh/dxr_definitions.h"
#include "bvh/rtip11/encoded_rt_ip_11_bottom_level_bvh.h"
#include "bvh/rtip11/encoded_rt_ip_11_top_level_bvh.h"
#include "bvh/rtip31/encoded_rt_ip_31_bottom_level_bvh.h"
#include "bvh/rtip31/encoded_rt_ip_31_top_level_bvh.h"
#include "bvh/rtip31/internal_node.h"
#include "bvh/rtip31/primitive_node.h"
#include "rra_blas_impl.h"
#include "rra_bvh_impl.h"
#include "rra_data_set.h"
#include "rra_tlas_impl.h"

// External reference to the global dataset.
extern RraDataSet data_set_;

namespace rra
{
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

            const auto& child_array      = RraBvhGetChildNodeArray(tlas, root_node.GetRawPointer(), node_offset);
            float       out_surface_area = 0.0f;
            for (uint32_t child_idx = 0; child_idx < (uint32_t)child_array.size(); ++child_idx)
            {
                // Find SAH for child nodes.
                sub_tree_sah += CalculateSAHForTlasNode(tlas, child_array[child_idx]);
                if (RraTlasGetSurfaceAreaImpl(tlas, child_array[child_idx], child_idx, &out_surface_area) == kRraOk)
                {
                    total_child_area += static_cast<float>(out_surface_area);
                }
            }

            // Take that as ratio of the current node.
            out_surface_area = 0.0;
            if (RraTlasGetSurfaceAreaImpl(tlas, root_node.GetRawPointer(), 0, &out_surface_area) == kRraOk)  // Pass 0 here since it's RtIp11, it's ignored.
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

            tlas->SetInteriorNodeSurfaceAreaHeuristic(root_node.GetRawPointer(), sah);
        }
        else if (root_node.IsInstanceNode())
        {
            // Get SAH from BLAS since it's already been computed for triangle nodes.
            const rta::EncodedRtIp11BottomLevelBvh* blas = nullptr;
            if (RraTlasGetBlasFromInstanceNode(tlas, root_node.GetRawPointer(), &blas) == kRraOk)
            {
                sub_tree_sah     = blas->GetSurfaceAreaHeuristic();
                float child_area = 0.0f;

                if (blas->IsEmpty())
                {
                    sah = 0.0f;
                }
                else if (RraTlasGetNodeTransformedSurfaceArea(tlas, root_node.GetRawPointer(), blas, &child_area) == kRraOk)
                {
                    sah                     = 1.0f;
                    float tlas_surface_area = 0.0f;
                    if (RraTlasGetSurfaceAreaImpl(tlas, root_node.GetRawPointer(), 0, &tlas_surface_area) ==
                        kRraOk)  // Pass 0 here since it's RtIp11, it's ignored.
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
                tlas->SetLeafNodeSurfaceAreaHeuristic(root_node.GetRawPointer(), sah);
            }
            else
            {
                RRA_ASSERT_FAIL("Can't calculate SAH from instance node.");
            }
        }

        return sah + sub_tree_sah;
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
    /// @param [in]      bvh             The acceleration structure to use.
    /// @param [in]      root_node_id    The root node of the acceleration to start from.
    /// @param [in]      global_child_id The global child index.
    /// @param [in]      tri_only        All non-triangle nodes will be ignored if this is true.
    /// @param [in, out] min_sah         The minimum surface area heuristic found.
    static void GetMinimumSurfaceAreaHeuristicImpl(const rta::IBvh* bvh, uint32_t root_node_id, uint32_t& global_child_id, bool tri_only, float* min_sah)
    {
        const auto& interior_nodes = bvh->GetInteriorNodesData();
        dxr::amd::NodePointer root_node(root_node_id);
        if (root_node.IsTriangleNode() || !tri_only)
        {
            float sah = 0.0f;
            if (RraBvhGetSurfaceAreaHeuristic(bvh, root_node_id, global_child_id, &sah) != kRraOk)
            {
                return;
            }

            *min_sah = std::min(*min_sah, sah);
        }

        if (root_node.IsBoxNode())
        {
            const auto node_offset = root_node.GetByteOffset() - bvh->GetHeader().GetBufferOffsets().interior_nodes;
            RRA_ASSERT(node_offset < interior_nodes.size());
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

            const auto& child_array = RraBvhGetChildNodeArray(bvh, root_node_id, node_offset);
            for (uint32_t child_index = 0; child_index < child_count; child_index++)
            {
                // Find SAH for child nodes.
                const auto& child_node = child_array[child_index];
                GetMinimumSurfaceAreaHeuristicImpl(bvh, child_node, global_child_id, tri_only, min_sah);
            }
        }
    }

    /// @brief Recursive function to calculate the total surface area heuristic value for a given acceleration structure.
    ///
    /// The acceleration structure will be traversed starting at the provided node given and the surface area heuristic values will
    /// be summed. This will be used to calculate an average value.
    ///
    /// @param [in]      bvh                The acceleration structure to use.
    /// @param [in]      root_node_id       The root node of the acceleration to start from.
    /// @param [in]      global_child_index The global child index.
    /// @param [in]      tri_only           All non-triangle nodes will be ignored if this is true.
    /// @param [in, out] total_sah          The total (summed) surface area heuristic value.
    /// @param [in, out] node_count         The number of nodes processed.
    static void GetTotalSurfaceAreaHeuristicImpl(const rta::IBvh* bvh,
                                                 uint32_t         root_node_id,
                                                 uint32_t         global_child_index,
                                                 bool             tri_only,
                                                 float*           total_sah,
                                                 int32_t*         node_count)
    {
        const auto& interior_nodes = bvh->GetInteriorNodesData();

        std::deque<uint32_t> traversal_stack;
        traversal_stack.push_back(root_node_id);
        --global_child_index;  // Start one index before since we increment in the loop.

        while (!traversal_stack.empty())
        {
            uint32_t node_id{traversal_stack.back()};
            traversal_stack.pop_back();
            ++global_child_index;

            dxr::amd::NodePointer root_node(node_id);
            if (root_node.IsTriangleNode() || !tri_only)
            {
                float sah = 0.0f;
                if (RraBvhGetSurfaceAreaHeuristic(bvh, node_id, global_child_index, &sah) != kRraOk)
                {
                    return;
                }

                (*node_count)++;
                *total_sah += sah;
            }

            if (root_node.IsBoxNode())
            {
                const auto node_offset = root_node.GetByteOffset() - bvh->GetHeader().GetBufferOffsets().interior_nodes;
                RRA_ASSERT(node_offset < interior_nodes.size());
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

                const auto& child_array = RraBvhGetChildNodeArray(bvh, node_id, node_offset);
                for (uint32_t child_index = 0; child_index < child_count; child_index++)
                {
                    // Find SAH for child nodes.
                    const auto& child_node = child_array[child_index];
                    traversal_stack.push_back(child_node);
                }
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
            rta::EncodedBottomLevelBvh* bvh = (rta::EncodedBottomLevelBvh*)bottom_level_bvhs[blas_index].get();
            bvh->ComputeSurfaceAreaHeuristic();
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

    float GetMinimumSurfaceAreaHeuristic(const rta::IBvh* bvh, uint32_t node_id, uint32_t global_child_id, bool tri_only)
    {
        float    min_sah                  = 1.0f;
        uint32_t starting_global_child_id = global_child_id - 1;  // Subtract one since we increment in the function call.
        GetMinimumSurfaceAreaHeuristicImpl(bvh, node_id, starting_global_child_id, tri_only, &min_sah);

        return min_sah;
    }

    float GetAverageSurfaceAreaHeuristic(const rta::IBvh* bvh, uint32_t node_id, uint32_t global_child_id, bool tri_only)
    {
        float   total      = 0.0f;
        int32_t node_count = 0;
        GetTotalSurfaceAreaHeuristicImpl(bvh, node_id, global_child_id, tri_only, &total, &node_count);

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
