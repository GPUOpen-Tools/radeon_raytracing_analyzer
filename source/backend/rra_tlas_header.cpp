//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the public TLAS header interface.
///
/// Contains public functions specific to the TLAS header.
//=============================================================================

#include "public/rra_tlas_header.h"

#include "rra_tlas_impl.h"

RraErrorCode RraTlasHeaderGetMetaDataSize(uint64_t tlas_index, uint32_t* out_size_in_bytes)
{
    rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_size_in_bytes = tlas->GetHeader().GetMetaDataSize();
    return kRraOk;
}

RraErrorCode RraTlasHeaderGetInteriorNodeCount(uint64_t tlas_index, uint32_t* out_node_count)
{
    rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_node_count = tlas->GetHeader().GetInteriorNodeCount();
    return kRraOk;
}

RraErrorCode RraTlasHeaderGetLeafNodeCount(uint64_t tlas_index, uint32_t* out_node_count)
{
    rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_node_count = tlas->GetHeader().GetLeafNodeCount();
    return kRraOk;
}

RraErrorCode RraTlasHeaderGetPrimitiveCount(uint64_t tlas_index, uint32_t* out_primitive_count)
{
    rta::EncodedTopLevelBvh* tlas = RraTlasGetTlasFromTlasIndex(tlas_index);
    if (tlas == nullptr)
    {
        return kRraErrorInvalidPointer;
    }

    *out_primitive_count = tlas->GetHeader().GetPrimitiveCount();
    return kRraOk;
}

