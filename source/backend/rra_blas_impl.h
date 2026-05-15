//=============================================================================
// Copyright (c) 2021-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the BLAS interface.
///
/// Contains functions specific to the BLAS that are not exposed to the public
/// interface.
//=============================================================================

#ifndef RRA_BACKEND_RRA_BLAS_IMPL_H_
#define RRA_BACKEND_RRA_BLAS_IMPL_H_

#include "public/rra_blas.h"

#include "bvh/dxr_definitions.h"
#include "bvh/rtip31/primitive_node.h"
#include "bvh/rtip_common/encoded_bottom_level_bvh.h"

/// @brief Get a pointer to the BLAS from the blas index passed in.
///
/// @param [in] blas_index        The index of the BLAS to retrieve.
///
/// @return  A pointer to the TLAS (or nullptr if it doesn't exist).
rta::EncodedBottomLevelBvh* RraBlasGetBlasFromBlasIndex(uint64_t blas_index);

/// @brief Get the surface area for a given BLAS node.
///
/// @param [in]  blas               The bottom level acceleration structure.
/// @param [in]  node_id            The node whose surface area is to be calculated.
/// @param [in]  child_index        The node's child index.
/// @param [in]  global_child_index The node's global child index.
/// @param [out] out_surface_area   The calculated surface area.
///
/// @return RraOk if successful, an error code if not.
RraErrorCode RraBlasGetSurfaceAreaImpl(const rta::EncodedBottomLevelBvh* blas,
                                       uint32_t                          node_id,
                                       uint32_t                          child_index,
                                       uint32_t                          global_child_index,
                                       float*                            out_surface_area);

#endif  // RRA_BACKEND_RRA_BLAS_IMPL_H_

