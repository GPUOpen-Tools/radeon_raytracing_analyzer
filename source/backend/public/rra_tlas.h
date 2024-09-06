//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the public TLAS interface.
///
/// Contains public functions specific to the TLAS.
//=============================================================================

#ifndef RRA_BACKEND_PUBLIC_RRA_TLAS_H_
#define RRA_BACKEND_PUBLIC_RRA_TLAS_H_

#include <stdbool.h>

#include "rra_bvh.h"
#include "rra_error.h"

#include "vulkan/include/vulkan/vulkan_core.h"

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

/// @brief Get the base address for the tlas_index given.
///
/// @param [in]  tlas_index  The index of the TLAS to use.
/// @param [out] out_address A pointer to receive the base address of the TLAS.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetBaseAddress(uint64_t tlas_index, uint64_t* out_address);

/// @brief Get the api (user visible) address for the tlas_index given.
///
/// @param [in]  tlas_index  The index of the TLAS to use.
/// @param [out] out_address A pointer to receive the api address of the TLAS.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetAPIAddress(uint64_t tlas_index, uint64_t* out_address);

/// @brief Is the TLAS empty.
///
/// @param [in]  tlas_index  The index of the TLAS to use.
///
/// @return true if empty, false if not.
bool RraTlasIsEmpty(uint64_t tlas_index);

/// @brief Get the total number of nodes for the tlas_index given.
///
/// The total number of nodes is the sum of the internal nodes and leaf nodes.
///
/// @param [in]  tlas_index     The index of the TLAS to use.
/// @param [out] out_node_count A pointer to receive the total node count of the TLAS.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetTotalNodeCount(uint64_t tlas_index, uint64_t* out_node_count);

/// @brief Get the child node count for a given node.
///
/// @param [in]  tlas_index         The index of the TLAS to use.
/// @param [in]  parent_node        The parent to get count for.
/// @param [out] out_child_count    A pointer to to the child node count.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetChildNodeCount(uint64_t tlas_index, uint32_t parent_node, uint32_t* out_child_count);

/// @brief Get the child nodes for a given node.
///
/// @param [in]  tlas_index         The index of the TLAS to use.
/// @param [in]  parent_node        The parent node to get child nodes for.
/// @param [out] out_child_nodes    A pointer to a list allocated with the count of child nodes.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetChildNodes(uint64_t tlas_index, uint32_t parent_node, uint32_t* out_child_nodes);

/// @brief Get the child node pointer for a given node.
///
/// @param [in]  tlas_index     The index of the TLAS to use.
/// @param [in]  parent_node    The parent of the child node to find.
/// @param [in]  child_index    The index of the child node held in the parent node.
/// @param [out] out_node_ptr   The child node pointer.
///
/// @return kRraOk if successful otherwise there are three possible error codes:
///         kRraErrorInvalidPointer if either the TLAS is invalid.
///         kRraErrorInvalidChildNode if the child node is invalid. (Meaning there may be more child nodes)
///         kRraErrorIndexOutOfRange if the child node index is out of range.
RraErrorCode RraTlasGetChildNodePtr(uint64_t tlas_index, uint32_t parent_node, uint32_t child_index, uint32_t* out_node_ptr);

/// @brief Get the base address for a given node.
///
/// @param [in]  tlas_index     The index of the TLAS to use.
/// @param [in]  node_ptr       The node whose base address is to be found.
/// @param [out] out_address    The base address of the node.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetNodeBaseAddress(uint64_t tlas_index, uint32_t node_ptr, uint64_t* out_address);

/// @brief Get the node_ptr to a given node's parent.
///
/// @param [in]  tlas_index          The index of the TLAS to use.
/// @param [in]  node_ptr            The node whose parent base address is to be found.
/// @param [out] out_parent_node_ptr The node pointer to the parent.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetNodeParent(uint64_t tlas_index, uint32_t node_ptr, uint32_t* out_parent_node_ptr);

/// @brief Get the instance information for an instance node.
///
/// @param [in]  tlas_index          The index of the TLAS to use.
/// @param [in]  node_ptr            The instance node pointer.
/// @param [out] out_blas_address    A pointer to receive the blas address.
/// @param [out] out_instance_count  A pointer to receive the instance count.
/// @param [out] out_is_empty        A pointer to receive whether the instance is empty.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetInstanceNodeInfo(uint64_t tlas_index, uint32_t node_ptr, uint64_t* out_blas_address, uint64_t* out_instance_count, bool* out_is_empty);

/// @brief Get the instance count for a given BLAS in a TLAS.
///
/// @param [in]  tlas_index          The index of the TLAS where the BLAS instance is.
/// @param [in]  blas_index          The index of the BLAS whose instance count is needed.
/// @param [out] out_instance_count  A pointer to a variable to hold the instance count returned.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetInstanceCount(uint64_t tlas_index, uint64_t blas_index, uint64_t* out_instance_count);

/// @brief Get the total number of box nodes for the tlas_index given.
///
/// The total number of box nodes is the number of internal nodes.
///
/// @param [in]  tlas_index     The index of the TLAS to use.
/// @param [out] out_node_count A pointer to receive the total box node count of the BLAS.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetBoxNodeCount(uint64_t tlas_index, uint64_t* out_node_count);

/// @brief Get the number of box-16 nodes for the tlas_index given.
///
/// @param [in]  tlas_index     The index of the TLAS to use.
/// @param [out] out_node_count A pointer to receive the box-16 node count of the BLAS.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetBox16NodeCount(uint64_t tlas_index, uint32_t* out_node_count);

/// @brief Get the number of box-32 nodes for the tlas_index given.
///
/// @param [in]  tlas_index     The index of the TLAS to use.
/// @param [out] out_node_count A pointer to receive the box-32 node count of the BLAS.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetBox32NodeCount(uint64_t tlas_index, uint32_t* out_node_count);

/// @brief Get the instance node count for a TLAS.
///
/// @param [in]  tlas_index          The index of the TLAS to use.
/// @param [out] out_instance_count  The number of instance nodes in the given TLAS.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetInstanceNodeCount(uint64_t tlas_index, uint64_t* out_instance_count);

/// @brief Get the number of unique BLASes in a TLAS.
///
/// @param [in]  tlas_index      The index of the TLAS whose BLAS count is needed.
/// @param [out] out_blas_count  A pointer to a variable to hold the BLAS count returned.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetBlasCount(uint64_t tlas_index, uint64_t* out_blas_count);

/// @brief Get the instance node for an instance of a BLAS.
///
/// @param [in]  tlas_index          The index of the TLAS to use.
/// @param [in]  blas_index          The index of the BLAS to use.
/// @param [in]  instance_index      The index of the instance in the TLAS.
/// @param [out] out_node_ptr        The node pointer of the instance.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetInstanceNode(uint64_t tlas_index, uint64_t blas_index, uint64_t instance_index, uint32_t* out_node_ptr);

/// @brief Get the instance transformation for an instance node.
///
/// @param [in]  tlas_index          The index of the TLAS to use.
/// @param [in]  node_ptr            The instance node pointer.
/// @param [out] transform           A pointer to receive the transform data, 12 floating points of allocation is needed.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetInstanceNodeTransform(uint64_t tlas_index, uint32_t node_ptr, float* transform);

/// @brief Get the original (not inverse) instance transformation for an instance node.
///
/// @param [in]  tlas_index          The index of the TLAS to use.
/// @param [in]  node_ptr            The instance node pointer.
/// @param [out] transform           A pointer to receive the transform data, 12 floating points of allocation is needed.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetOriginalInstanceNodeTransform(uint64_t tlas_index, uint32_t node_ptr, float* transform);

/// @brief Get the BLAS index from a TLAS instance node.
///
/// @param [in]  tlas_index          The index of the TLAS to use.
/// @param [in]  node_ptr            The instance node pointer.
/// @param [out] out_blas_index      A pointer to receive the blas index.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetBlasIndexFromInstanceNode(uint64_t tlas_index, uint32_t node_ptr, uint64_t* out_blas_index);

/// @brief Get the Instance index, given a TLAS index and an instance node pointer.
///
/// These instances are shared among instances generated for rebraiding.
///
/// @param [in]  tlas_index          The index of the TLAS to use.
/// @param [in]  node_ptr            The node pointer of the instance node.
/// @param [out] out_instance_index  A pointer to receive the instance index.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetInstanceIndexFromInstanceNode(uint64_t tlas_index, uint32_t node_ptr, uint32_t* out_instance_index);

/// @brief Get the unique Instance index, given a TLAS index and an instance node pointer.
///
/// This index is unique even among instances generated from rebraiding.
///
/// @param [in]  tlas_index          The index of the TLAS to use.
/// @param [in]  node_ptr            The node pointer of the instance node.
/// @param [out] out_instance_index  A pointer to receive the unique instance index.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetUniqueInstanceIndexFromInstanceNode(uint64_t tlas_index, uint32_t node_ptr, uint32_t* out_instance_index);

/// @brief Get the bounding volume extents of a given node.
///
/// @param [in]  tlas_index          The index of the TLAS to use.
/// @param [in]  node_ptr            The node pointer whose bounding volume is to be found.
/// @param [out] out_extents         The bounding volume extents structure.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetBoundingVolumeExtents(uint64_t tlas_index, uint32_t node_ptr, struct BoundingVolumeExtents* out_extents);

/// @brief Get the surface area heuristic of a given node.
///
/// @param [in]  tlas_index                 The index of the TLAS to use.
/// @param [in]  node_ptr                   The node pointer whose SAH is to be found.
/// @param [out] out_surface_area_heuristic A pointer to receive the surface area heuristic.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetSurfaceAreaHeuristic(uint64_t tlas_index, uint32_t node_ptr, float* out_surface_area_heuristic);

/// @brief Get the minimum surface area heuristic of a given node and its children.
///
/// @param [in]  tlas_index                     The index of the TLAS to use.
/// @param [in]  node_ptr                       The node pointer whose SAH is to be found.
/// @param [out] out_min_surface_area_heuristic A pointer to receive the minimum surface area heuristic.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetMinimumSurfaceAreaHeuristic(uint64_t tlas_index, uint32_t node_ptr, float* out_min_surface_area_heuristic);

/// @brief Get the average surface area heuristic of a given node and its children.
///
/// @param [in]  tlas_index                     The index of the TLAS to use.
/// @param [in]  node_ptr                       The node pointer whose SAH is to be found.
/// @param [out] out_avg_surface_area_heuristic A pointer to receive the average surface area heuristic.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetAverageSurfaceAreaHeuristic(uint64_t tlas_index, uint32_t node_ptr, float* out_avg_surface_area_heuristic);

/// @brief Get the instance mask as specified through the API.
///
/// @param tlas_index    The index of the TLAS to use.
/// @param node_ptr      The node pointer containing the instance.
/// @param out_mask      The mask of this instance.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetInstanceNodeMask(uint64_t tlas_index, uint32_t node_ptr, uint32_t* out_mask);

/// @brief Get the instance ID as specified through the API.
///
/// @param tlas_index    The index of the TLAS to use.
/// @param node_ptr      The node pointer containing the instance.
/// @param out_id        The ID of this instance.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetInstanceNodeID(uint64_t tlas_index, uint32_t node_ptr, uint32_t* out_id);

/// @brief Get the instance hit group as specified through the API.
///
/// @param tlas_index     The index of the TLAS to use.
/// @param node_ptr       The node pointer containing the instance.
/// @param out_hit_group  The hit group of this instance.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetInstanceNodeHitGroup(uint64_t tlas_index, uint32_t node_ptr, uint32_t* out_hit_group);

/// @brief Get the size of the TLAS, in bytes.
///
/// @param [in] tlas_index         The index of the TLAS to use.
/// @param [out] out_size_in_bytes The size.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetSizeInBytes(uint64_t tlas_index, uint32_t* out_size_in_bytes);

/// @brief Get the size of the TLAS and unique BLASes, in bytes.
///
/// @param [in] tlas_index         The index of the TLAS to use.
/// @param [out] out_size_in_bytes The size of the TLAS and all BLASes referenced.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetEffectiveSizeInBytes(uint64_t tlas_index, uint64_t* out_size_in_bytes);

/// @brief Get the total number of triangles referenced by the instance nodes in this TLAS.
///
/// @param [in] tlas_index       The index of the TLAS to use.
/// @param [out] triangle_count  The total triangle count.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetTotalTriangleCount(uint64_t tlas_index, uint64_t* triangle_count);

/// @brief Get the sum of all the triangles in each BLAS referenced by the TLAS.
///
/// @param [in] tlas_index The index of the TLAS.
/// @param [out] out_count The number of unique triangles is written here.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetUniqueTriangleCount(uint64_t tlas_index, uint64_t* out_count);

/// @brief Get the total number of procedural nodes referenced by the instance nodes in this TLAS.
///
/// @param tlas_index The index of the TLAS.
/// @param out_count  The total procedural node count.
/// 
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetTotalProceduralNodeCount(uint64_t tlas_index, uint64_t* out_count);

/// @brief Get the number of inactive instances referenced by this TLAS.
///
/// @param [in] tlas_index      The index of the TLAS.
/// @param [out] inactive_count The number of inactive instances in the TLAS.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetInactiveInstancesCount(uint64_t tlas_index, uint64_t* inactive_count);

/// @brief Retrieve the build flags used to build this TLAS.
///
/// @param [in] tlas_index The index of the TLAS to use.
/// @param [out] out_flags The build flags.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetBuildFlags(uint64_t tlas_index, VkBuildAccelerationStructureFlagBitsKHR* out_flags);

/// @brief Query whether or not rebraiding was used to build this TLAS.
///
/// @param [in] tlas_index The index of the TLAS to use.
/// @param [out] out_enabled True if enabled, false otherwise.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetRebraidingEnabled(uint64_t tlas_index, bool* out_enabled);

/// @brief Retrieve the instance flags.
///
/// @param [in] tlas_index	The index of the TLAS to use.
/// @param [in] node_ptr	The node pointer containing the instance.
/// @param [out] out_flags	The instance flags.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetInstanceFlags(uint64_t tlas_index, uint32_t node_ptr, uint32_t* out_flags);

/// @brief Query whether or not fused instances was used to build this TLAS.
///
/// @param [in] tlas_index The index of the TLAS to use.
/// @param [out] out_enabled True if enabled, false otherwise.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetFusedInstancesEnabled(uint64_t tlas_index, bool* out_enabled);

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus
#endif  // RRA_BACKEND_PUBLIC_RRA_TLAS_H_
