//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for ray intersections.
//=============================================================================

#ifndef RRA_UTIL_INTERSECT_MIN_MAX_H_
#define RRA_UTIL_INTERSECT_MIN_MAX_H_

#include "glm/glm/glm.hpp"

namespace rra
{
    namespace renderer
    {
        /// @brief Check for the intersection between a ray and an axis aligned bounding box.
        ///
        /// @param [in] ray_origin      The origin of the ray.
        /// @param [in] ray_direction   The direction of the ray.
        /// @param [in] min             The min bound of a volume.
        /// @param [in] origin          The max bound of a volume.
        /// @param [out] closest        The closest point intersected on bound.
        ///
        /// @return True if the ray hits the min max.
        bool IntersectAABB(const glm::vec3& ray_origin, const glm::vec3& ray_direction, const glm::vec3& min, const glm::vec3& max, float& closest);

        /// @brief Check for the intersection between a ray and an oriented bounding box.
        ///
        /// @param [in] ray_origin      The origin of the ray.
        /// @param [in] ray_direction   The direction of the ray.
        /// @param [in] min             The min bound of a volume.
        /// @param [in] origin          The max bound of a volume.
        /// @param [in] rotation        Rotation of the OBB.
        /// @param [out] closest        The closest point intersected on bound.
        ///
        /// @return True if the ray hits the min max.
        bool IntersectOBB(const glm::vec3& ray_origin,
                          const glm::vec3& ray_direction,
                          const glm::vec3& min,
                          const glm::vec3& max,
                          const glm::mat3& rotation,
                          float&           closest);

        /// @brief Check for the intersection between a ray and a triangle.
        ///
        /// @param [in] ray_origin      The origin of the ray.
        /// @param [in] ray_direction   The direction of the ray.
        /// @param [in] v1              The vertex #1 of the triangle.
        /// @param [in] v2              The vertex #2 of the triangle.
        /// @param [in] v2              The vertex #3 of the triangle.
        /// @param [out] hit_distance   The hit distance of the ray if it hit the triangle.
        ///
        /// @return True if the ray hits the triangle.
        bool IntersectTriangle(const glm::vec3& ray_origin,
                               const glm::vec3& ray_direction,
                               const glm::vec3& v1,
                               const glm::vec3& v2,
                               const glm::vec3& v3,
                               float*           hit_distance);
    }  // namespace renderer

}  // namespace rra

#endif

