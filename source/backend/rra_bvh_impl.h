//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the BVH interface.
///
/// Contains all functions common to all acceleration structures.
//=============================================================================

#ifndef RRA_BACKEND_RRA_BVH_IMPL_H_
#define RRA_BACKEND_RRA_BVH_IMPL_H_

#include "bvh/dxr_definitions.h"
#include "bvh/rtip11/iencoded_rt_ip_11_bvh.h"
#include "public/rra_bvh.h"

/// @brief Get the child node count for a given node.
///
/// @param [in]  bvh                The acceleration structure containing the node of interest.
/// @param [in]  parent_node        The parent to get count for.
/// @param [out] out_child_count    A pointer to to the child node count.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBvhGetChildNodeCount(const rta::IEncodedRtIp11Bvh* bvh, uint32_t parent_node, uint32_t* out_child_count);

/// @brief Get the child nodes for a given node.
///
/// @param [in]  bvh                The acceleration structure containing the node of interest.
/// @param [in]  parent_node        The parent node to get child nodes for.
/// @param [out] out_child_nodes    A pointer to a list allocated with the count of child nodes.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBvhGetChildNodes(const rta::IEncodedRtIp11Bvh* bvh, uint32_t parent_node, uint32_t* out_child_nodes);

/// @brief Get the child node pointer for a given node.
///
/// @param [in]  bvh            The acceleration structure containing the node of interest.
/// @param [in]  parent_node    The parent of the child node to find.
/// @param [in]  child_index    The index of the child node held in the parent node.
/// @param [out] out_node_ptr   The child node pointer.
///
/// @return kRraOk if successful otherwise there are three possible error codes:
///         kRraErrorInvalidPointer if either the TLAS is invalid.
///         kRraErrorInvalidChildNode if the child node is invalid. (Meaning there may be more child nodes)
///         kRraErrorIndexOutOfRange if the child node index is out of range.
RraErrorCode RraBvhGetChildNodePtr(const rta::IEncodedRtIp11Bvh* bvh, uint32_t parent_node, uint32_t child_index, uint32_t* out_node_ptr);

/// @brief Get the bounding volume for a provided node.
///
/// @param [in]  bvh              The acceleration structure containing the node of interest.
/// @param [in]  node_ptr         The node of interest.
/// @param [out] out_bounding_box The calculated bounding volume.
///
/// @return RraOk if successful, an error code if not.
RraErrorCode RraBvhGetNodeBoundingVolume(const rta::IEncodedRtIp11Bvh*     bvh,
                                         const dxr::amd::NodePointer*      node_ptr,
                                         dxr::amd::AxisAlignedBoundingBox& out_bounding_box);

/// @brief Get the surface area of the bounding volume for a provided node.
///
/// @param [in]  bvh              The acceleration structure containing the node of interest.
/// @param [in]  node_ptr         The node of interest.
/// @param [out] out_surface_area The calculated surface area.
///
/// @return RraOk if successful, an error code if not.
RraErrorCode RraBvhGetBoundingVolumeSurfaceArea(const rta::IEncodedRtIp11Bvh* bvh, const dxr::amd::NodePointer* node_ptr, float* out_surface_area);

/// @brief Get the surface area heuristic for a provided node.
///
/// @param [in]  bvh              The acceleration structure containing the node of interest.
/// @param [in]  node_ptr         The node of interest.
/// @param [out] out_surface_area The calculated surface area heuristic.
///
/// @return RraOk if successful, an error code if not.
RraErrorCode RraBvhGetSurfaceAreaHeuristic(const rta::IEncodedRtIp11Bvh* bvh, const dxr::amd::NodePointer node_ptr, float* out_surface_area_heuristic);

#endif  // RRA_BACKEND_RRA_BVH_IMPL_H_
