//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for ray intersections.
//=============================================================================

#include "glm/glm/gtx/intersect.hpp"

#include "public/intersect.h"

namespace rra
{
    namespace renderer
    {

        bool IntersectAABB(const glm::vec3& ray_origin, const glm::vec3& ray_direction, const glm::vec3& min, const glm::vec3& max, float& closest)
        {
            constexpr float infinity = std::numeric_limits<float>::infinity();

            const glm::vec3 box_min_rel = min - ray_origin;
            const glm::vec3 box_max_rel = max - ray_origin;

            const glm::vec3 inverse_direction = 1.0f / ray_direction;

            const glm::vec3 t_plane_min = box_min_rel * inverse_direction;
            const glm::vec3 t_plane_max = box_max_rel * inverse_direction;

            glm::vec3 min_interval, max_interval;

            min_interval.x = inverse_direction.x >= 0.0f ? t_plane_min.x : t_plane_max.x;
            max_interval.x = inverse_direction.x >= 0.0f ? t_plane_max.x : t_plane_min.x;

            min_interval.y = inverse_direction.y >= 0.0f ? t_plane_min.y : t_plane_max.y;
            max_interval.y = inverse_direction.y >= 0.0f ? t_plane_max.y : t_plane_min.y;

            min_interval.z = inverse_direction.z >= 0.0f ? t_plane_min.z : t_plane_max.z;
            max_interval.z = inverse_direction.z >= 0.0f ? t_plane_max.z : t_plane_min.z;

            float min_of_intervals_t = glm::max(min_interval.x, glm::max(min_interval.y, min_interval.z));
            float max_of_intervals_t = glm::min(max_interval.x, glm::min(max_interval.y, max_interval.z));

            float min_t = glm::max(min_of_intervals_t, 0.0f);
            float max_t = glm::min(max_of_intervals_t, infinity);

            if (std::isnan(min_of_intervals_t) || std::isnan(max_of_intervals_t))
            {
                min_t = infinity;
                max_t = -infinity;
            }

            closest = min_t;

            const float eps = 5.960464478e-8f;  // 2^-24;
            return (min_t <= (max_t * (1 + 6 * eps)));
        }

        /// @brief Transform a ray for a given OBB transform.
        ///
        /// @param [in]  transform    The OBB transform.
        /// @param [in]  origin       The ray origin.
        /// @param [in]  direction    The ray direction.
        /// @param [out] newOrigin    The transformed ray origin.
        /// @param [out] newDirection The transformed ray direction.
        static void OBBTransform(const glm::mat3& transform, const glm::vec3& origin, const glm::vec3& direction, glm::vec3* newOrigin, glm::vec3* newDirection)
        {
            glm::vec3 t0 = transform[0];
            glm::vec3 t1 = transform[1];
            glm::vec3 t2 = transform[2];

            float r0x = origin.z * t0.z;
            float r0y = origin.z * t1.z;
            float r0z = origin.z * t2.z;

            float r1x = direction.z * t0.z;
            float r1y = direction.z * t1.z;
            float r1z = direction.z * t2.z;

            r0x = origin.y * t0.y + r0x;
            r0y = origin.y * t1.y + r0y;
            r0z = origin.y * t2.y + r0z;

            r1x = direction.y * t0.y + r1x;
            r1y = direction.y * t1.y + r1y;
            r1z = direction.y * t2.y + r1z;

            r0x = origin.x * t0.x + r0x;
            r0y = origin.x * t1.x + r0y;
            r0z = origin.x * t2.x + r0z;

            r1x = direction.x * t0.x + r1x;
            r1y = direction.x * t1.x + r1y;
            r1z = direction.x * t2.x + r1z;

            *newOrigin    = glm::vec3(r0x, r0y, r0z);
            *newDirection = glm::vec3(r1x, r1y, r1z);
        }

        bool IntersectOBB(const glm::vec3& ray_origin,
                          const glm::vec3& ray_direction,
                          const glm::vec3& min,
                          const glm::vec3& max,
                          const glm::mat3& rotation,
                          float&           closest)
        {
            glm::vec3 new_origin{};
            glm::vec3 new_direction{};
            OBBTransform(rotation, ray_origin, ray_direction, &new_origin, &new_direction);
            return IntersectAABB(new_origin, new_direction, min, max, closest);
        }

        bool IntersectTriangle(const glm::vec3& ray_origin,
                               const glm::vec3& ray_direction,
                               const glm::vec3& v1,
                               const glm::vec3& v2,
                               const glm::vec3& v3,
                               float*           hit_distance)
        {
            constexpr float infinity = std::numeric_limits<float>::infinity();

            glm::vec3 e1 = v2 - v1;
            glm::vec3 e2 = v3 - v1;
            glm::vec3 e3 = ray_origin - v1;

            glm::vec4 result;

            const glm::vec3 s1 = glm::cross(ray_direction, e2);
            const glm::vec3 s2 = glm::cross(e3, e1);

            result.x = glm::dot(e2, s2);
            result.y = glm::dot(s1, e1);
            result.z = glm::dot(e3, s1);
            result.w = glm::dot(ray_direction, s2);

            float t = result.x / result.y;
            float u = result.z / result.y;
            float v = result.w / result.y;

            int triangle_missed = ((u < 0.f) || (u > 1.f));
            triangle_missed |= ((v < 0.f) || (u + v > 1.f));
            triangle_missed |= (t < 0.f);

            const float inf = infinity;
            result.x        = triangle_missed ? inf : result.x;
            result.y        = triangle_missed ? 1.0f : result.y;

            result.z = u;
            result.w = v;

            if (!triangle_missed)
            {
                *hit_distance = result.x / result.y;
            }

            return !triangle_missed;
        }
    }  // namespace renderer
}  // namespace rra

