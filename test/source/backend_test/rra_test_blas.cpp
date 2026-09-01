//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test BLAS implementation.
//=============================================================================

#include "rra_test_blas.h"

#include <math.h>  // --> isnan, isinf
#include <deque>
#include <vector>

#include "public/rra_blas.h"
#include "public/rra_bvh.h"
#include "public/rra_macro.h"

#include "rra_test_config.h"

namespace backend_test
{
    RRATestBlas::RRATestBlas(const std::string& test_name)
        : RRATestBase(test_name)
    {
    }

    RRATestBlas::~RRATestBlas()
    {
    }

    bool RRATestBlas::RunTests(const RRATestConfig& config)
    {
        AddTestStartMessage(config.log);

        bool result = true;

        // Iterate through all BLASes.
        uint64_t blas_count = 0;
        if (RraBvhGetTotalBlasCount(&blas_count) != kRraOk)
        {
            config.log.Write(" ERROR: Unable to get BLAS count");
            return false;
        }

        for (uint64_t loop = 0; loop < blas_count; loop++)
        {
            if (TestBLASTraversal(loop, config.log) == false)
            {
                result = false;
            }

            if (TestBLASGeometryInfo(loop, config.log) == false)
            {
                result = false;
            }
        }

        AddTestResultMessage(result, config.log);
        return result;
    }

    bool RRATestBlas::TestBLASTraversal(uint64_t blas_index, const Log& log)
    {
        uint32_t total_triangle_count = 0;

        uint32_t root_node = UINT32_MAX;
        RRA_BUBBLE_ON_ERROR(RraBvhGetRootNodePtr(&root_node));

        std::deque<std::pair<uint32_t, uint32_t>> traversal_stack;  // Pairs of (node_addr, child_index).
        traversal_stack.push_back({root_node, 0});
        uint32_t global_child_index = UINT32_MAX;

        // Assume traversal test will be OK.
        bool traversal_result = true;

        // Traverse the tree and add all triangle nodes to the table.
        while (!traversal_stack.empty())
        {
            const auto pair        = traversal_stack.back();
            uint32_t   node_addr   = pair.first;
            uint32_t   child_index = pair.second;
            traversal_stack.pop_back();
            ++global_child_index;

            bool is_internal_node = RraBlasHasChildren(blas_index, node_addr);
            if (is_internal_node)
            {
                // For each item on the stack, add the children if valid.
                uint32_t child_node_count = 0;
                RRA_BUBBLE_ON_ERROR(RraBlasGetChildNodeCount(blas_index, node_addr, &child_node_count));
                std::vector<uint32_t> child_nodes(child_node_count);
                RRA_BUBBLE_ON_ERROR(RraBlasGetChildNodes(blas_index, node_addr, child_nodes.data()));

                for (uint32_t i = 0; i < child_node_count; i++)
                {
                    uint32_t child_node = child_nodes[i];
                    traversal_stack.push_back({child_node, i});
                }
            }
            else if (RraBlasIsTriangleNode(blas_index, node_addr))
            {
                float surface_area = 0.0f;
                if (RraBlasGetSurfaceArea(blas_index, node_addr, child_index, global_child_index, &surface_area) == kRraOk)
                {
                    if (surface_area <= 0.0f)
                    {
                        log.Write(" WARNING: invalid surface area value (%f) for node 0x%x in blas[%llu]", surface_area, node_addr, blas_index);
                    }
                }

                float surface_area_heuristic = 0.0f;
                if (RraBlasGetSurfaceAreaHeuristic(blas_index, node_addr, global_child_index, &surface_area_heuristic) == kRraOk)
                {
                    if (surface_area_heuristic < 0.0f || surface_area_heuristic > 1.0 || isnan(surface_area_heuristic))
                    {
                        log.Write(
                            " WARNING: invalid surface area heuristic value (%f) for node 0x%x in blas[%llu]", surface_area_heuristic, node_addr, blas_index);
                    }
                }

                // Show SAH max and average.
                if (RraBlasGetMinimumSurfaceAreaHeuristic(blas_index, node_addr, global_child_index, false, &surface_area_heuristic) == kRraOk)
                {
                    if (surface_area_heuristic < 0.0f || surface_area_heuristic > 1.0 || isnan(surface_area_heuristic))
                    {
                        log.Write(" WARNING: invalid minimum surface area heuristic value (%f) for node 0x%x in blas[%llu]",
                                  surface_area_heuristic,
                                  node_addr,
                                  blas_index);
                    }
                }

                if (RraBlasGetAverageSurfaceAreaHeuristic(blas_index, node_addr, global_child_index, false, &surface_area_heuristic) == kRraOk)
                {
                    if (surface_area_heuristic < 0.0f || surface_area_heuristic > 1.0 || isnan(surface_area_heuristic))
                    {
                        log.Write(" WARNING: invalid average surface area heuristic value (%f) for node 0x%x in blas[%llu]",
                                  surface_area_heuristic,
                                  node_addr,
                                  blas_index);
                    }
                }

                uint32_t triangle_count;
                if (RraBlasGetNodeTriangleCount(blas_index, node_addr, child_index, global_child_index, &triangle_count) == kRraOk)
                {
                    total_triangle_count += triangle_count;
                }

                // Make sure geometry index reported by a triangle node is within the geometry info struct range.
                uint32_t geometry_index = 0;
                if (RraBlasGetGeometryIndex(blas_index, node_addr, child_index, global_child_index, &geometry_index) != kRraOk)
                {
                    log.Write(" WARNING: can't get geometry index for triangle node 0x%x in blas[%llu]", node_addr, blas_index);
                }
                else
                {
                    uint32_t     geometry_flags{};
                    RraErrorCode flags_result = RraBlasGetGeometryFlags(blas_index, geometry_index, &geometry_flags);
                    if (flags_result != kRraOk)
                    {
                        if (flags_result == kRraErrorIndexOutOfRange)
                        {
                            uint32_t geometry_count = 0;
                            RraBlasGetGeometryCount(blas_index, &geometry_count);
                            log.Write(" ERROR: geometry index is out of range for triangle node 0x%x in blas[%llu] : expected max of %u, found %u",
                                      node_addr,
                                      blas_index,
                                      geometry_count,
                                      geometry_index);
                            traversal_result = false;
                        }
                        else
                        {
                            log.Write(" WARNING: can't get geometry flags for triangle node 0x%x in blas[%llu]", node_addr, blas_index);
                        }
                    }
                }
            }
        }
        if (traversal_result == false)
        {
            return false;
        }

        uint32_t unique_triangle_count = 0;
        if (RraBlasGetUniqueTriangleCount(blas_index, &unique_triangle_count) != kRraOk)
        {
            log.Write(" ERROR: Unable to get BLAS unique triangle count for blas[%llu]", blas_index);
            return false;
        }

        uint32_t active_primitive_count = 0;
        if (RraBlasGetActivePrimitiveCount(blas_index, &active_primitive_count) != kRraOk)
        {
            log.Write(" ERROR: Unable to get BLAS active triangle count for blas[%llu]", blas_index);
            return false;
        }

        return true;
    }

    bool RRATestBlas::TestBLASGeometryInfo(uint64_t blas_index, const Log& log)
    {
        uint32_t geometry_count = 0;
        if (RraBlasGetGeometryCount(blas_index, &geometry_count) != kRraOk)
        {
            log.Write(" ERROR: Unable to get BLAS geometry count for blas[%llu]", blas_index);
            return false;
        }

        uint64_t total_primitive_count = 0;
        for (uint32_t geometry_index = 0; geometry_index < geometry_count; ++geometry_index)
        {
            uint32_t geometry_flags = 0;
            if (RraBlasGetGeometryFlags(blas_index, geometry_index, &geometry_flags) != kRraOk)
            {
                log.Write(" ERROR: Unable to get BLAS geometry flags for blas[%llu]", blas_index);
                return false;
            }

            uint32_t mask = static_cast<uint32_t>(~(VK_GEOMETRY_OPAQUE_BIT_KHR | VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR));
            if ((mask & geometry_flags) != 0)
            {
                log.Write(" ERROR: Illegal geometry flags (0x%x) for blas[%llu]", geometry_flags, blas_index);
                return false;
            }

            uint32_t primitive_count = 0;
            if (RraBlasGetGeometryPrimitiveCount(blas_index, geometry_index, &primitive_count) != kRraOk)
            {
                log.Write(" ERROR: Unable to get BLAS geometry primitive count for blas[%llu]", blas_index);
                return false;
            }
            total_primitive_count += primitive_count;
        }

        uint32_t triangle_count = 0;
        if (RraBlasGetTriangleNodeCount(blas_index, &triangle_count) != kRraOk)
        {
            log.Write(" ERROR: Unable to get BLAS header triangle node count for blas[%llu]", blas_index);
            return false;
        }

        uint32_t procedural_node_count = 0;
        if (RraBlasGetProceduralNodeCount(blas_index, &procedural_node_count) != kRraOk)
        {
            log.Write(" ERROR: Unable to get BLAS header procedural node count for blas[%llu]", blas_index);
            return false;
        }

        return true;
    }

}  // namespace backend_test

