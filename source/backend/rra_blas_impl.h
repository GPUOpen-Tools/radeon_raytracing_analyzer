//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the BLAS interface.
///
/// Contains all functions specific to the BLAS.
//=============================================================================

#ifndef RRA_BACKEND_RRA_BLAS_IMPL_H_
#define RRA_BACKEND_RRA_BLAS_IMPL_H_

#include "bvh/dxr_definitions.h"
#include "bvh/rtip11/encoded_rt_ip_11_bottom_level_bvh.h"
#include "public/rra_blas.h"
#include "bvh/rtip31/primitive_node.h"

/// @brief Get a pointer to the BLAS from the blas index passed in.
///
/// @param [in] blas_index        The index of the BLAS to retrieve.
///
/// @return  A pointer to the TLAS (or nullptr if it doesn't exist).
rta::EncodedBottomLevelBvh* RraBlasGetBlasFromBlasIndex(uint64_t blas_index);

/// @brief Get the surface area for a given triangle node.
///
/// @param [in] triangle_node Reference to the triangle node whose surface area is to be calculated.
/// @param [in] tri_count Number of triangles in triangle node.
///
/// @return The triangle node surface area.
float RraBlasGetTriangleSurfaceArea(const dxr::amd::TriangleNode& triangle_node, uint32_t tri_count);

/// @brief Get the surface area for a given BLAS node.
///
/// @param [in]  blas             The bottom level acceleration structure.
/// @param [in]  node_ptr         The node pointer whose surface area is to be calculated.
/// @param [out] out_surface_area The calculated surface area.
///
/// @return RraOk if successful, an error code if not.
RraErrorCode RraBlasGetSurfaceAreaImpl(const rta::EncodedBottomLevelBvh* blas, const dxr::amd::NodePointer* node_ptr, float* out_surface_area);

#endif  // RRA_BACKEND_RRA_BLAS_IMPL_H_
