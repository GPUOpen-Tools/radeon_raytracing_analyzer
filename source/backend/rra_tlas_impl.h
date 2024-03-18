//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the TLAS interface.
///
/// Contains all functions specific to the TLAS.
//=============================================================================

#ifndef RRA_BACKEND_RRA_TLAS_IMPL_H_
#define RRA_BACKEND_RRA_TLAS_IMPL_H_

#include "bvh/rtip11/encoded_rt_ip_11_top_level_bvh.h"
#include "bvh/rtip11/encoded_rt_ip_11_bottom_level_bvh.h"
#include "public/rra_tlas.h"

/// @brief Get a pointer to the TLAS from the tlas index passed in.
///
/// @param [in] tlas_index        The index of the TLAS to retrieve.
///
/// @return  A pointer to the TLAS (or nullptr if it doesn't exist).
rta::EncodedRtIp11TopLevelBvh* RraTlasGetTlasFromTlasIndex(uint64_t tlas_index);

/// @brief Get the surface area for a given TLAS node.
///
/// @param [in]  tlas             The top level acceleration structure containing the node.
/// @param [in]  node_ptr         The node of interest.
/// @param [out] out_surface_area The calculated surface area.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetSurfaceAreaImpl(const rta::EncodedRtIp11TopLevelBvh* tlas, const dxr::amd::NodePointer* node_ptr, float* out_surface_area);

/// @brief Get the BLAS associated with a given instance node.
///
/// @param [in]  tlas             The top level acceleration structure containing the node.
/// @param [in]  node_ptr         The instance node of interest.
/// @param [out] out_blas         The BLAS associated with the instance node.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetBlasFromInstanceNode(const rta::EncodedRtIp11TopLevelBvh*     tlas,
                                            const dxr::amd::NodePointer*             node_ptr,
                                            const rta::EncodedRtIp11BottomLevelBvh** out_blas);

/// @brief Get the transformed surface area for an instance node.
///
/// Calculate the surface area for an instance node by taking the BLAS and applying
/// the transform for the given instance node.
///
/// @param [in]  tlas             The top level acceleration structure containing the node.
/// @param [in]  node_ptr         The node of interest.
/// @param [in]  volume_bvh       The BLAS associated with the instance node, whose bounding volume is to be transformed.
/// @param [out] out_surface_area The calculated surface area.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraTlasGetNodeTransformedSurfaceArea(const rta::EncodedRtIp11TopLevelBvh* tlas,
                                                  const dxr::amd::NodePointer*         node_ptr,
                                                  const rta::IEncodedRtIp11Bvh*        volume_bvh,
                                                  float*                               out_surface_area);

#endif  // RRA_BACKEND_RRA_TLAS_IMPL_H_
