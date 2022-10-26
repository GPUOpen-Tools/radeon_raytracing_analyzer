//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for min max intersection with a ray.
//=============================================================================

#ifndef RRA_UTIL_INTERSECT_MIN_MAX_H_
#define RRA_UTIL_INTERSECT_MIN_MAX_H_

#include "glm/glm/glm.hpp"

namespace rra
{
    namespace renderer
    {
        /// @brief Check for the intersection between a ray and a min max.
        ///
        /// @param [in] origin      The origin of the ray.
        /// @param [in] direction   The direction of the ray.
        /// @param [in] min         The min bound of a volume.
        /// @param [in] origin      The max bound of a volume.
        ///
        /// @return True if the ray hits the min max.
        bool IntersectMinMax(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& min, const glm::vec3& max);
    }  // namespace renderer

}  // namespace rra

#endif
