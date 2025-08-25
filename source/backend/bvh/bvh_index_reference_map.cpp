//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  BVH Index reference map implementation.
//=============================================================================

#include "bvh/bvh_index_reference_map.h"

#include "rdf/rdf/inc/amdrdf.h"

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
