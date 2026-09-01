//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the public BVH interface.
///
/// Contains public functions common to all acceleration structures.
//=============================================================================

#ifndef RRA_BACKEND_PUBLIC_RRA_BVH_H_
#define RRA_BACKEND_PUBLIC_RRA_BVH_H_

#include <stdbool.h>

#include "public/rra_error.h"

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

#define MAX_TRIANGLES (8)
#define MAX_CHILD_NODES (8)

/// @brief Structure describing the statistics for a bounding volume.
struct BoundingVolumeExtents
{
    float min_x;  ///< The minimum X extent.
    float min_y;  ///< The minimum Y extent.
    float min_z;  ///< The minimum Z extent.
    float max_x;  ///< The maximum X extent.
    float max_y;  ///< The maximum Y extent.
    float max_z;  ///< The maximum Z extent.
};

/// @brief Structure describing a single vertex in a BLAS mesh.
typedef struct VertexPosition
{
    float x;  ///< The X coordinate.
    float y;  ///< The Y coordinate.
    float z;  ///< The Z coordinate.
} VertexPosition;

/// @brief An enumeration of geometry flags.
enum GeometryFlags
{
    kNone              = 0,  ///< Empty geometry flags.
    kOpaque            = 1,  ///< The geometry is opaque.
    kNoDuplicateAnyHit = 2,  ///< Report 1 anyhit intersection at most.
};

/// @brief Structure describing a single triangle in a BLAS mesh.
struct TriangleVertices
{
    VertexPosition a;  ///< The first vertex in the triangle.
    VertexPosition b;  ///< The second vertex in the triangle.
    VertexPosition c;  ///< The third vertex in the triangle.
};

/// @brief Get the root node pointer for an acceleration structure.
///
/// @param [out] out_node_ptr        A pointer to receive the node pointer.
/// This value is valid for the root index of all the TLAS hierarchies.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBvhGetRootNodePtr(uint32_t* out_node_ptr);

/// @brief Get the surface area of the bounding volume extents provided.
///
/// @param [in]  extents             The bounding volume extents.
/// @param [out] out_surface_area    A pointer to receive the surface area.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBvhGetBoundingVolumeSurfaceArea(const struct BoundingVolumeExtents* extents, float* out_surface_area);

/// @brief Get the offset of the node provided.
///
/// This is encoded in the node data passed in.
///
/// @param [in]  node_id      The node of interest.
/// @param [out] out_offset   A pointer to receive the node offset.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBvhGetNodeOffset(uint32_t node_id, uint64_t* out_offset);

/// @brief Get the number of TLAS's in the loaded trace.
///
/// @param [out] out_count A pointer to receive the number of TLAS's.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBvhGetTlasCount(uint64_t* out_count);

/// @brief Get the total number of BLAS's in the loaded trace.
///
/// This doesn't include the empty BLAS placeholder for missing
/// BLASes.
///
/// @param [out] out_count A pointer to receive the number of BLAS's.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBvhGetBlasCount(uint64_t* out_count);

/// @brief Get the total number of BLAS's in the backend.
///
/// This is the array size used to hold the BLASes.
///
/// @param [out] out_count A pointer to receive the number of BLAS's.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBvhGetTotalBlasCount(uint64_t* out_count);

/// @brief Get the number of missing BLAS's in the loaded trace.
///
/// @param [out] out_count A pointer to receive the number of missing BLAS's.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBvhGetMissingBlasCount(uint64_t* out_count);

/// @brief Get the number of inactive instances in the loaded trace.
///
/// @param [out] out_count A pointer to receive the number of inactive instances.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBvhGetInactiveInstancesCount(uint64_t* out_count);

/// @brief Get the number of empty BLAS's in the loaded trace.
///
/// @param [out] out_count A pointer to receive the number of empty BLAS's.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBvhGetEmptyBlasCount(uint64_t* out_count);

/// @brief Get the size of all TLASes in the trace, in bytes.
///
/// @param [out] out_size_in_bytes The size of the TLASes in the trace.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBvhGetTotalTlasSizeInBytes(uint64_t* out_size_in_bytes);

/// @brief Get the size of all BLASes in the trace, in bytes.
///
/// @param [out] out_size_in_bytes The size of the BLASes in the trace.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBvhGetTotalBlasSizeInBytes(uint64_t* out_size_in_bytes);

/// @brief Get the size of all TLASes and BLASes in the trace, in bytes.
///
/// @param [out] out_size_in_bytes The size of the TLASes and BLASes in the trace.
///
/// @returns kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraBvhGetTotalTraceSizeInBytes(uint64_t* out_size_in_bytes);

/// @brief Get the maximum number of child nodes. Dependent on the RTIP level.
///
/// @returns The maximum number of child nodes.
uint32_t RraBvhGetMaxChildCount();

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus
#endif  // RRA_BACKEND_PUBLIC_RRA_BVH_H_

