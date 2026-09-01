//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  BVH Index reference map implementation.
//=============================================================================

#include "bvh/bvh_index_reference_map.h"

#include "amdrdf.h"

#include "public/rra_assert.h"

namespace rta
{
    // Collection of bvhs of different acceleration structure types.
    struct IndexReferenceEntry
    {
        std::uint64_t  index;
        std::uintptr_t ptr;
    };

}  // namespace rta

