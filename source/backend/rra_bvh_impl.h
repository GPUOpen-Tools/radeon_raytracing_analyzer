//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the BVH interface.
///
/// Contains functions common to all acceleration structures that are not
/// exposed to the public interface.
//=============================================================================

#ifndef RRA_BACKEND_RRA_BVH_IMPL_H_
#define RRA_BACKEND_RRA_BVH_IMPL_H_

#include "glm/glm/glm.hpp"

#include "public/rra_bvh.h"

#include "bvh/dxr_definitions.h"
#include "bvh/ibvh.h"

/// @brief Check if the given node is a box node.
///
/// @param [in] bvh       The acceleration structure containing the node of interest.
/// @param [in] node_id   The encoded node pointer.
///
/// @return True if the given node is a box node, and false if it's not.
bool RraBvhIsBoxNode(const rta::IBvh* bvh, uint32_t node_id);

/// @brief Check if the given node is a box 16 node.
///
/// @param [in] bvh       The acceleration structure containing the node of interest.
/// @param [in] node_id   The encoded node pointer.
///
/// @return True if the given node is a box 16 node, and false if it's not.
bool RraBvhIsBox16Node(const rta::IBvh* bvh, uint32_t node_id);

/// @brief Check if the given node is a box 32 node.
///
/// @param [in] bvh       The acceleration structure containing the node of interest.
/// @param [in] node_id   The encoded node pointer.
///
/// @return True if the given node is a box 32 node, and false if it's not.
bool RraBvhIsBox32Node(const rta::IBvh* bvh, uint32_t node_id);

/// @brief Check if the given node has child nodes.
///
/// @param [in] bvh       The acceleration structure containing the node of interest.
/// @param [in] node_id   The encoded node pointer.
///
/// @return True if the given node has children, and false if not.
bool RraBvhHasChildren(const rta::IBvh* bvh, uint32_t node_id);

/// @brief Get the child node count for a given node.
///
/// @param [in]  bvh                The acceleration structure containing the node of interest.
/// @param [in]  parent_node        The parent to get count for.
/// @param [out] out_child_count    A pointer to the child node count.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBvhGetChildNodeCount(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_count);

/// @brief Get the child nodes for a given node.
///
/// @param [in]  bvh                The acceleration structure containing the node of interest.
/// @param [in]  parent_node        The parent node to get child nodes for.
/// @param [out] out_child_nodes    A pointer to a list allocated with the count of child nodes.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBvhGetChildNodes(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_nodes);

/// @brief Get the child indices for a given node.
///
/// @param [in]  bvh                The acceleration structure containing the node of interest.
/// @param [in]  parent_node        The parent node to get child nodes for.
/// @param [out] out_child_indices   A pointer to a list allocated with the count of child nodes.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBvhGetChildIndices(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_indices);

/// @brief Get the child node pointer for a given node.
///
/// @param [in]  bvh            The acceleration structure containing the node of interest.
/// @param [in]  parent_node    The parent of the child node to find.
/// @param [in]  child_index    The index of the child node held in the parent node.
/// @param [out] out_node_id    The child node pointer.
///
/// @return kRraOk if successful otherwise there are three possible error codes:
///         kRraErrorInvalidPointer if either the TLAS is invalid.
///         kRraErrorInvalidChildNode if the child node is invalid. (Meaning there may be more child nodes)
///         kRraErrorIndexOutOfRange if the child node index is out of range.
RraErrorCode RraBvhGetChildNodePtr(const rta::IBvh* bvh, uint32_t parent_node, uint32_t child_index, uint32_t* out_node_id);

/// @brief Get the bounding volume for a provided node.
///
/// @param [in]  bvh                The acceleration structure containing the node of interest.
/// @param [in]  node_id            The node of interest.
/// @param [in]  child_index        The node's child index.
/// @param [in]  global_child_index The node's global child index.
/// @param [out] out_bounding_box   The calculated bounding volume.
///
/// @return RraOk if successful, an error code if not.
RraErrorCode RraBvhGetNodeBoundingVolume(const rta::IBvh*                  bvh,
                                         uint32_t                          node_id,
                                         uint32_t                          child_index,
                                         uint32_t                          global_child_index,
                                         dxr::amd::AxisAlignedBoundingBox& out_bounding_box);

/// @brief Get the index of the node's OBB matrix.
///
/// @param [in]  bvh              The acceleration structure containing the node of interest.
/// @param [in]  node_id          The node of interest.
/// @param [out] out_rotation     The orientation of the bounding volume.
///
/// @return RraOk if successful, an error code if not.
RraErrorCode RraBvhGetNodeObbIndex(const rta::IBvh* bvh, uint32_t node_id, uint32_t* obb_index);

/// @brief Get the bounding volume orientation for a provided node.
///
/// @param [in]  bvh              The acceleration structure containing the node of interest.
/// @param [in]  node_id          The node of interest.
/// @param [out] out_rotation     The orientation of the bounding volume.
///
/// @return RraOk if successful, an error code if not.
RraErrorCode RraBvhGetNodeBoundingVolumeOrientation(const rta::IBvh* bvh, uint32_t node_id, glm::mat3& out_rotation);

/// @brief Get the surface area of the bounding volume for a provided node.
///
/// @param [in]  bvh                The acceleration structure containing the node of interest.
/// @param [in]  node_id            The node of interest.
/// @param [in]  child_index        The node's child index.
/// @param [in]  global_child_index The node's global child index.
/// @param [out] out_surface_area   The calculated surface area.
///
/// @return RraOk if successful, an error code if not.
RraErrorCode RraBvhGetBoundingVolumeSurfaceArea(const rta::IBvh* bvh,
                                                uint32_t         node_id,
                                                uint32_t         child_index,
                                                uint32_t         global_child_index,
                                                float*           out_surface_area);

/// @brief Get the surface area heuristic for a provided node.
///
/// @param [in]  bvh                The acceleration structure containing the node of interest.
/// @param [in]  node_ptr           The node of interest.
/// @param [in]  global_child_index The node of interest's global child ID.
/// @param [out] out_surface_area   The calculated surface area heuristic.
///
/// @return RraOk if successful, an error code if not.
RraErrorCode RraBvhGetSurfaceAreaHeuristic(const rta::IBvh* bvh, uint32_t node_id, uint32_t global_child_index, float* out_surface_area_heuristic);

/// @brief Get a reference to the array of child nodes for a particular node.
///
/// Assumes the parent node is an internal/box node.
///
/// @param [in] bvh            The acceleration structure to use.
/// @param [in] root_id        The parent node.
/// @param [in] node_offset    The offset into the interior nodes array.
///
/// @return A reference to the array of child nodes.
std::array<uint32_t, MAX_CHILD_NODES> RraBvhGetChildNodeArray(const rta::IBvh* bvh, uint32_t root_id, uint32_t node_offset);

#endif  // RRA_BACKEND_RRA_BVH_IMPL_H_

