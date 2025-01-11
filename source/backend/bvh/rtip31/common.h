//=============================================================================
//  Copyright (c) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 3.1 (Navi4x) specific helper functions.
//=============================================================================

#ifndef RRA_BACKEND_COMMON_H_
#define RRA_BACKEND_COMMON_H_

#include <cmath>
#include "glm/glm/glm.hpp"
#include "primitive_node.h"

#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

//=====================================================================================================================
// Transform a ray for a given OBB transform. TODO: Remove, opt for refactor of InstanceTransform instead.
static void OBBTransform(const glm::mat3& transform, const glm::vec3& origin, const glm::vec3& direction, glm::vec3* newOrigin, glm::vec3* newDirection)
{
    glm::vec3 t0 = transform[0];
    glm::vec3 t1 = transform[1];
    glm::vec3 t2 = transform[2];

    float r0x = std::fmaf(origin.z, t0.z, 0.0);
    float r0y = std::fmaf(origin.z, t1.z, 0.0);
    float r0z = std::fmaf(origin.z, t2.z, 0.0);

    float r1x = direction.z * t0.z;
    float r1y = direction.z * t1.z;
    float r1z = direction.z * t2.z;

    r0x = std::fmaf(origin.y, t0.y, r0x);
    r0y = std::fmaf(origin.y, t1.y, r0y);
    r0z = std::fmaf(origin.y, t2.y, r0z);

    r1x = std::fmaf(direction.y, t0.y, r1x);
    r1y = std::fmaf(direction.y, t1.y, r1y);
    r1z = std::fmaf(direction.y, t2.y, r1z);

    r0x = std::fmaf(origin.x, t0.x, r0x);
    r0y = std::fmaf(origin.x, t1.x, r0y);
    r0z = std::fmaf(origin.x, t2.x, r0z);

    r1x = std::fmaf(direction.x, t0.x, r1x);
    r1y = std::fmaf(direction.x, t1.x, r1y);
    r1z = std::fmaf(direction.x, t2.x, r1z);

    *newOrigin    = glm::vec3(r0x, r0y, r0z);
    *newDirection = glm::vec3(r1x, r1y, r1z);
}

//=====================================================================================================================
// Compute instance sideband offset from node offset when compressed node formats are enabled.
static uint32_t ComputeInstanceSidebandOffset(uint32_t instanceNodeOffset, uint32_t leafNodeOffset, uint32_t sidebandDataOffset)
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
