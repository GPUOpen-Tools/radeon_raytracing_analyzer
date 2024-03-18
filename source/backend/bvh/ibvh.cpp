//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  BVH base class implementations.
//=============================================================================

#include "bvh/ibvh.h"

namespace rta
{
    IBvh::~IBvh()
    {
    }

    bool IBvh::IsCompacted() const
    {
        return IsCompactedImpl();
    }

    bool IBvh::IsEmpty() const
    {
        return IsEmptyImpl();
    }

    uint64_t IBvh::GetInactiveInstanceCount() const
    {
        return GetInactiveInstanceCountImpl();
    }

}  // namespace rta
