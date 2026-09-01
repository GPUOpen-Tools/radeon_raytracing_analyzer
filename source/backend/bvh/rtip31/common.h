//=============================================================================
//  Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
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

//=====================================================================================================================
// Compute instance sideband offset for the new RTIP3.1 layout (GPURT v16.12+), where the sideband is indexed
// directly by instance index rather than by leaf-node slot. The stride is still 64 bytes.
inline uint32_t ComputeInstanceSidebandOffsetFromIndex(uint32_t instanceIndex, uint32_t sidebandDataOffset)
{
    return sidebandDataOffset + (instanceIndex * sizeof(InstanceSidebandData));
}

//=====================================================================================================================
inline bool IsInvalidBoundingBox(const BoundingBox& box)
{
    return box.min.x > box.max.x;
}

//=====================================================================================================================
// Compute N-bit quantized bounds
inline UintBoundingBox ComputeQuantizedBounds(const BoundingBox& bounds, const glm::vec3& origin, const glm::vec3& rcpExponents, uint32_t numQuantBits)
{
    UintBoundingBox  result;
    const glm::uvec3 invalidMin = glm::uvec3(bits(numQuantBits), bits(numQuantBits), bits(numQuantBits));
    const glm::uvec3 invalidMax = glm::uvec3(0, 0, 0);
    const bool       isInvalid  = IsInvalidBoundingBox(bounds);

    result.min = isInvalid ? invalidMin : ComputeQuantizedMin(bounds.min, origin, rcpExponents, numQuantBits);
    result.max = isInvalid ? invalidMax : ComputeQuantizedMax(bounds.max, origin, rcpExponents, numQuantBits);

    return result;
}

#ifndef _WIN32
#pragma GCC diagnostic pop
#endif

#endif  // RRA_BACKEND_COMMON_H_

