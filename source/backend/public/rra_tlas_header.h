//=============================================================================
// Copyright (c) 2025-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the public TLAS header interface.
///
/// Contains public functions specific to the TLAS header.
//=============================================================================

#ifndef RRA_BACKEND_PUBLIC_RRA_TLAS_HEADER_H_
#define RRA_BACKEND_PUBLIC_RRA_TLAS_HEADER_H_

#include "rra_error.h"

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

/// @brief Get the metadata size from the TLAS header.
///
/// @param [in]  tlas_index         The TLAS index.
/// @param [out] out_size_in_bytes  The metadata size, in bytes.
///
/// @return kRraOk if successful, other values specifying error.
RraErrorCode RraTlasHeaderGetMetaDataSize(uint64_t tlas_index, uint32_t* out_size_in_bytes);

/// @brief Get the interior node count from the TLAS header.
///
/// @param [in]  tlas_index      The TLAS index.
/// @param [out] out_node_count  The interior node count.
///
/// @return kRraOk if successful, other values specifying error.
RraErrorCode RraTlasHeaderGetInteriorNodeCount(uint64_t tlas_index, uint32_t* out_node_count);

/// @brief Get the leaf node count from the TLAS header.
///
/// @param [in]  tlas_index      The TLAS index.
/// @param [out] out_node_count  The leaf node count.
///
/// @return kRraOk if successful, other values specifying error.
RraErrorCode RraTlasHeaderGetLeafNodeCount(uint64_t tlas_index, uint32_t* out_node_count);

/// @brief Get the primitive count from the TLAS header.
///
/// @param [in]  tlas_index           The TLAS index.
/// @param [out] out_primitive_count  The number of primitives in the TLAS.
///
/// @return kRraOk if successful, other values specifying error.
RraErrorCode RraTlasHeaderGetPrimitiveCount(uint64_t tlas_index, uint32_t* out_primitive_count);

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus
#endif  // RRA_BACKEND_PUBLIC_RRA_TLAS_HEADER_H_

