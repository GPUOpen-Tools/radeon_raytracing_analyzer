//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of the Surface area heuristic calculator.
//=============================================================================

#ifndef RRA_BACKEND_SURFACE_AREA_HEURISTIC_H_
#define RRA_BACKEND_SURFACE_AREA_HEURISTIC_H_

#include "rra_data_set.h"

// Surface area heuristic calculator functions. Used only by the backend; no public interface.

struct PrimitiveStructure;

namespace rra
{
    /// @brief Calculate the surface area heuristic values for all nodes in the TLASes and BLASes.
    ///
    /// @param [in] data_set The data set containing the loaded trace data.
    ///
    /// @return RraOk if successful, an error code if not.
    RraErrorCode CalculateSurfaceAreaHeuristics(RraDataSet& data_set);

    /// @brief Get the minimum surface area heuristic for a given node and its children.
    ///
    /// @param [in] bvh      The acceleration structure where the node is located.
    /// @param [in] node_ptr The node of interest.
    /// @param [in] tri_only All non-triangle nodes will be ignored if this is true.
    ///
    /// @return The minimum surface area heuristic.
    float GetMinimumSurfaceAreaHeuristic(const rta::IBvh* bvh, const dxr::amd::NodePointer node_ptr, bool tri_only);

    /// @brief Get the average (mean) surface area heuristic for a given node and its children.
    ///
    /// @param [in] bvh      The acceleration structure where the node is located.
    /// @param [in] node_ptr The node of interest.
    /// @param [in] tri_only All non-triangle nodes will be ignored if this is true.
    ///
    /// @return The average surface area heuristic.
    float GetAverageSurfaceAreaHeuristic(const rta::IBvh* bvh, const dxr::amd::NodePointer node_ptr, bool tri_only);
}  // namespace rra

#endif  // RRA_BACKEND_SURFACE_AREA_HEURISTIC_H_

