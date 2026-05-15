//=============================================================================
// Copyright (c) 2025-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the public BLAS header interface.
///
/// Contains public functions specific to the BLAS header.
//=============================================================================

#include "public/rra_blas_header.h"

#include "rra_blas_impl.h"

RraErrorCode RraBlasHeaderGetMetaDataSize(uint64_t blas_index, uint32_t* out_size_in_bytes)
{
    rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_size_in_bytes = blas->GetHeader().GetMetaDataSize();
    return kRraOk;
}

RraErrorCode RraBlasHeaderGetInteriorNodeCount(uint64_t blas_index, uint32_t* out_node_count)
{
    rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_node_count = blas->GetHeader().GetInteriorNodeCount();
    return kRraOk;
}

RraErrorCode RraBlasHeaderGetLeafNodeCount(uint64_t blas_index, uint32_t* out_node_count)
{
    rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_node_count = blas->GetHeader().GetLeafNodeCount();
    return kRraOk;
}

RraErrorCode RraBlasHeaderGetPrimitiveCount(uint64_t blas_index, uint32_t* out_primitive_count)
{
    rta::EncodedBottomLevelBvh* blas = RraBlasGetBlasFromBlasIndex(blas_index);
    if (blas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_primitive_count = blas->GetHeader().GetPrimitiveCount();
    return kRraOk;
}

