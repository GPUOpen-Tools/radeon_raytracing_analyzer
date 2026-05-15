//=============================================================================
// Copyright (c) 2025-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the public BLAS header interface.
///
/// Contains public functions specific to the BLAS header.
//=============================================================================

#ifndef RRA_BACKEND_PUBLIC_RRA_BLAS_HEADER_H_
#define RRA_BACKEND_PUBLIC_RRA_BLAS_HEADER_H_

#include "rra_error.h"

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

/// @brief Get the metadata size from the BLAS header.
///
/// @param [in]  blas_index         The BLAS index.
/// @param [out] out_size_in_bytes  The metadata size, in bytes.
///
/// @return kRraOk if successful, other values specifying error.
RraErrorCode RraBlasHeaderGetMetaDataSize(uint64_t blas_index, uint32_t* out_size_in_bytes);

/// @brief Get the interior node count from the BLAS header.
///
/// @param [in]  blas_index      The BLAS index.
/// @param [out] out_node_count  The interior node count.
///
/// @return kRraOk if successful, other values specifying error.
RraErrorCode RraBlasHeaderGetInteriorNodeCount(uint64_t blas_index, uint32_t* out_node_count);

/// @brief Get the leaf node count from the BLAS header.
///
/// @param [in]  blas_index      The BLAS index.
/// @param [out] out_node_count  The leaf node count.
///
/// @return kRraOk if successful, other values specifying error.
RraErrorCode RraBlasHeaderGetLeafNodeCount(uint64_t blas_index, uint32_t* out_node_count);

/// @brief Get the primitive count from the BLAS header.
///
/// @param [in]  blas_index           The BLAS index.
/// @param [out] out_primitive_count  The number of primitives in the BLAS.
///
/// @return kRraOk if successful, other values specifying error.
RraErrorCode RraBlasHeaderGetPrimitiveCount(uint64_t blas_index, uint32_t* out_primitive_count);

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus
#endif  // RRA_BACKEND_PUBLIC_RRA_BLAS_HEADER_H_

