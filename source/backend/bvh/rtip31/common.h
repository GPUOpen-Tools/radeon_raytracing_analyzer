//=============================================================================
//  Copyright (c) 2024-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 3.1 (Navi4x) specific helper functions.
//=============================================================================

#ifndef RRA_BACKEND_COMMON_H_
#define RRA_BACKEND_COMMON_H_

#include <cmath>

#include "glm/glm/glm.hpp"

#include "bvh/rtip31/primitive_node.h"

#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

//=====================================================================================================================
// Compute instance sideband offset from node offset when compressed node formats are enabled.
inline uint32_t ComputeInstanceSidebandOffset(uint32_t instanceNodeOffset, uint32_t leafNodeOffset, uint32_t sidebandDataOffset)
{
    // Map instance node offset to a sideband slot. Note, this requires that all instance node data is allocated
    // in contiguous memory. Instance sideband data indexing mirrors instance node indexing in leaf node data section
    //
    const uint32_t sidebandIndex = ((instanceNodeOffset - leafNodeOffset) >> 7);
    return sidebandDataOffset + (sidebandIndex * sizeof(InstanceSidebandData));
}

#ifndef _WIN32
#pragma GCC diagnostic pop
#endif

#endif  // RRA_BACKEND_COMMON_H_

