//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the public BLAS interface.
///
/// Contains public functions specific to the BLAS.
//=============================================================================

#ifndef RRA_BACKEND_PUBLIC_RRA_BLAS_H_
#define RRA_BACKEND_PUBLIC_RRA_BLAS_H_

#include "rra_bvh.h"
#include "rra_error.h"

#include "vulkan/include/vulkan/vulkan_core.h"

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

/// @brief Get the base address for the blas_index given.
///
/// @param [in]  blas_index  The index of the BLAS to use.
/// @param [out] out_address A pointer to receive the base address of the BLAS.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetBaseAddress(uint64_t blas_index, uint64_t* out_address);

/// @brief Is the BLAS empty.
///
/// @param [in]  blas_index  The index of the BLAS to use.
///
/// @return true if empty, false if not.
bool RraBlasIsEmpty(uint64_t blas_index);

/// @brief Get the total number of nodes for the blas_index given.
///
/// The total number of nodes is the sum of the internal nodes and leaf nodes.
///
/// @param [in]  blas_index     The index of the BLAS to use.
/// @param [out] out_node_count A pointer to receive the total node count of the BLAS.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetTotalNodeCount(uint64_t blas_index, uint64_t* out_node_count);

/// @brief Get the child node count for a given node.
///
/// @param [in]  blas_index     The index of the BLAS to use.
/// @param [in]  parent_node    The parent to get count for.
/// @param [out] out_child_count   The child node count pointer.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetChildNodeCount(uint64_t blas_index, uint32_t parent_node, uint32_t* out_child_count);

/// @brief Get the child nodes for a given node.
///
/// @param [in]  blas_index     The index of the BLAS to use.
/// @param [in]  parent_node    The parent to get count for.
/// @param [out] out_child_nodes   A pointer to a list allocated with the count of child nodes.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetChildNodes(uint64_t blas_index, uint32_t parent_node, uint32_t* out_child_nodes);

/// @brief Get the total number of box nodes for the blas_index given.
///
/// The total number of box nodes is the number of internal nodes.
///
/// @param [in]  blas_index     The index of the BLAS to use.
/// @param [out] out_node_count A pointer to receive the total box node count of the BLAS.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetBoxNodeCount(uint64_t blas_index, uint64_t* out_node_count);

/// @brief Get the number of box-16 nodes for the blas_index given.
///
/// @param [in]  blas_index     The index of the BLAS to use.
/// @param [out] out_node_count A pointer to receive the box-16 node count of the BLAS.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetBox16NodeCount(uint64_t blas_index, uint32_t* out_node_count);

/// @brief Get the number of box-32 nodes for the blas_index given.
///
/// @param [in]  blas_index     The index of the BLAS to use.
/// @param [out] out_node_count A pointer to receive the box-32 node count of the BLAS.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetBox32NodeCount(uint64_t blas_index, uint32_t* out_node_count);

/// @brief Get the maximum tree depth for the blas_index given.
///
/// @param [in]  blas_index     The index of the BLAS to use.
/// @param [out] out_tree_depth A pointer to receive the maximum tree depth of the BLAS.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetMaxTreeDepth(uint64_t blas_index, uint32_t* out_tree_depth);

/// @brief Get the average tree depth of a triangle node for the blas_index given.
///
/// @param [in]  blas_index     The index of the BLAS to use.
/// @param [out] out_tree_depth A pointer to receive the maximum tree depth of the BLAS.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetAvgTreeDepth(uint64_t blas_index, uint32_t* out_tree_depth);

/// @brief Get the child node pointer for a given node.
///
/// @param [in]  blas_index     The index of the BLAS to use.
/// @param [in]  parent_node    The parent of the child node to find.
/// @param [in]  child_index    The index of the child node held in the parent node.
/// @param [out] out_node_ptr   The child node pointer.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetChildNodePtr(uint64_t blas_index, uint32_t parent_node, uint32_t child_index, uint32_t* out_node_ptr);

/// @brief Get the base address for a given node.
///
/// @param [in]  blas_index     The index of the BLAS to use.
/// @param [in]  node_ptr       The node whose base address is to be found.
/// @param [out] out_address    The base address of the node.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetNodeBaseAddress(uint64_t blas_index, uint32_t node_ptr, uint64_t* out_address);

/// @brief Get the parent of a given node.
///
/// @param [in]  blas_index          The index of the BLAS to use.
/// @param [in]  node_ptr            The node whose parent base address is to be found.
/// @param [out] out_parent_node_ptr The parent's node pointer.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetNodeParent(uint64_t blas_index, uint32_t node_ptr, uint32_t* out_parent_node_ptr);

/// @brief Get the surface area of a given node.
///
/// @param [in]  blas_index          The index of the BLAS to use.
/// @param [in]  node_ptr            The node pointer whose bounding volume is to be found.
/// @param [out] out_surface_area    A pointer to receive the surface area.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetSurfaceArea(uint64_t blas_index, uint32_t node_ptr, float* out_surface_area);

/// @brief Get the surface area heuristic of a given node.
///
/// @param [in]  blas_index                 The index of the BLAS to use.
/// @param [in]  node_ptr                   The node pointer whose SAH is to be found.
/// @param [out] out_surface_area_heuristic A pointer to receive the surface area heuristic.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetSurfaceAreaHeuristic(uint64_t blas_index, uint32_t node_ptr, float* out_surface_area_heuristic);

/// @brief Get the minimum surface area heuristic of a given node's triangle leaf nodes.
///
/// @param [in]  blas_index                     The index of the BLAS to use.
/// @param [in]  node_ptr                       The node pointer whose SAH is to be found.
/// @param [in]  tri_only                       All non-triangle nodes will be ignored if this is true.
/// @param [out] out_min_surface_area_heuristic A pointer to receive the minimum surface area heuristic.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetMinimumSurfaceAreaHeuristic(uint64_t blas_index, uint32_t node_ptr, bool tri_only, float* out_min_surface_area_heuristic);

/// @brief Get the average surface area heuristic of a given node's triangle leaf nodes.
///
/// @param [in]  blas_index                     The index of the BLAS to use.
/// @param [in]  node_ptr                       The node pointer whose SAH is to be found.
/// @param [in]  tri_only                       All non-triangle nodes will be ignored if this is true.
/// @param [out] out_avg_surface_area_heuristic A pointer to receive the average surface area heuristic.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetAverageSurfaceAreaHeuristic(uint64_t blas_index, uint32_t node_ptr, bool tri_only, float* out_avg_surface_area_heuristic);

/// @brief Get the surface area heuristic of a given triangle node.
///
/// @param [in]  blas_index                     The index of the BLAS to use.
/// @param [in]  node_ptr                       The node pointer whose SAH is to be found.
/// @param [out] out_avg_surface_area_heuristic A pointer to receive the average surface area heuristic.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetTriangleSurfaceAreaHeuristic(uint64_t blas_index, uint32_t node_ptr, float* out_tri_surface_area_heuristic);

/// @brief Get the bounding volume extents of a given node.
///
/// @param [in]  blas_index          The index of the BLAS to use.
/// @param [in]  node_ptr            The node pointer whose bounding volume is to be found.
/// @param [out] out_extents         The bounding volume extents structure.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetBoundingVolumeExtents(uint64_t blas_index, uint32_t node_ptr, struct BoundingVolumeExtents* out_extents);

/// @brief Retrieve the number of unique triangles in a BLAS mesh.
///
/// @param [in] blas_index           The index of the BLAS to use.
/// @param [out] out_triangle_count  The number of triangles in the BLAS.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetUniqueTriangleCount(uint64_t blas_index, uint32_t* out_triangle_count);

/// @brief Get the number of primitives that are marked active.
///
/// Invalid primitives may have zero or invalid transforms / references, or degenerated primitives (NaNs).
///
/// @param [in] blas_index           The index of the BLAS to use.
/// @param [out] out_triangle_count  The number of active primitives in the BLAS.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetActivePrimitiveCount(uint64_t blas_index, uint32_t* out_triangle_count);

/// @brief Retrieve the number of triangle nodes in a BLAS mesh.
///
/// @param [in] blas_index           The index of the BLAS to use.
/// @param [out] out_triangle_count  The number of triangle nodes in the BLAS.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetTriangleNodeCount(uint64_t blas_index, uint32_t* out_triangle_count);

/// @brief Retrieve the total number of procedural nodes in a BLAS mesh.
///
/// @param [in] blas_index                  The index of the BLAS to use.
/// @param [out] out_procedural_node_count  The number of procedural nodes in the BLAS.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetProceduralNodeCount(uint64_t blas_index, uint32_t* out_procedural_node_count);

/// @brief Retrieve the geometry index for the triangle node.
///
/// @param [in]  blas_index         The index of the BLAS to use.
/// @param [in]  node_ptr           The node whose geometry index is to be found.
/// @param [out] out_geometry_index	The geometry index for the triangle node.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetGeometryIndex(uint64_t blas_index, uint32_t node_ptr, uint32_t* out_geometry_index);

/// @brief Retrieve the number of primitives in a geometry.
///
/// @param [in]  blas_index          The index of the BLAS to use.
/// @param [in]  geometry_index      The index of the geometry to use.
/// @param [out] out_primitive_count The number of primitives in this geometry.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetGeometryPrimitiveCount(uint64_t blas_index, uint32_t geometry_index, uint32_t* out_primitive_count);

/// @brief Retrieve the number of geometries in a BLAS.
///
/// @param [in]  blas_index         The index of the BLAS to use.
/// @param [out] out_geometry_count	The number of geometries this BLAS has.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetGeometryCount(uint64_t blas_index, uint32_t* out_geometry_count);

/// @brief Retrieve the primitive index for the triangle node.
///
/// @param [in]  blas_index				The index of the BLAS to use.
/// @param [in]  node_ptr				The node whose primitive index is to be found.
/// @param [in]  local_primitive_index	The local primitive index within the given node.
/// @param [out] out_primitive_index	The primitive index for the triangle node.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetPrimitiveIndex(uint64_t blas_index, uint32_t node_ptr, uint32_t local_primitive_index, uint32_t* out_primitive_index);

/// @brief Retrieve the geometry flags for a triangle node.
///
/// @param [in]  blas_index         The index of the BLAS to use.
/// @param [in]  node_ptr           The node whose geometry flags are to be retrieved.
/// @param [out] out_geometry_flags	The geometry flags for the triangle node.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetGeometryFlags(uint64_t blas_index, uint32_t geometry_index, uint32_t* out_geometry_flags);

/// @brief Retrieve whether the node is inactive.
///
/// @param [in]  blas_index         The index of the BLAS to use.
/// @param [in]  node_ptr           The node whose geometry flags are to be retrieved.
/// @param [out] out_is_inactive	Whether the node is inactive.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetIsInactive(uint64_t blas_index, uint32_t node_ptr, bool* out_is_inactive);

/// @brief Retrieve the number of triangles on a given node.
///
/// @param [in] blas_index The index of the BLAS to use.
/// @param [in] node_ptr The node ID to retrieve the triangles from.
/// @param [out] out_triangle_count The number of triangles in the BLAS.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetNodeTriangleCount(uint64_t blas_index, uint32_t node_ptr, uint32_t* out_triangle_count);

/// @brief Retrieve the triangles stored in the given node id.
///
/// @param [in] blas_index The index of the BLAS to use.
/// @param [in] node_ptr The node ID to retrieve the triangles for.
/// @param [out] out_triangles A preallocated pointer to dump the triangles into.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetNodeTriangles(uint64_t blas_index, uint32_t node_ptr, struct TriangleVertices* out_triangles);

/// @brief Retrieve the vertices stored in the given node id.
///
/// @param [in] blas_index The index of the BLAS to use.
/// @param [in] node_ptr The node ID to retrieve the vertices for.
/// @param [out] out_triangles A preallocated pointer to dump the vertices into.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetNodeVertices(uint64_t blas_index, uint32_t node_ptr, struct VertexPosition* out_vertices);

/// @brief Retrieve the build flags used to build this BLAS.
///
/// @param [in] blas_index The index of the BLAS to use.
/// @param [out] out_flags The build flags.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetBuildFlags(uint64_t blas_index, VkBuildAccelerationStructureFlagBitsKHR* out_flags);

/// @brief Get the size of the BLAS, in bytes.
///
/// @param [in] blas_index         The index of the BLAS to use.
/// @param [out] out_size_in_bytes The size.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBlasGetSizeInBytes(uint64_t blas_index, uint32_t* out_size_in_bytes);

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus
#endif  // RRA_BACKEND_PUBLIC_RRA_BLAS_H_
