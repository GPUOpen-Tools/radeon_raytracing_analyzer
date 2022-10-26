//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for min max intersection with a ray.
//=============================================================================

#include "glm/glm/gtx/intersect.hpp"

#include "public/intersect_min_max.h"

namespace rra
{
    namespace renderer
    {
        bool IntersectMinMax(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& min, const glm::vec3& max)
        {
            glm::vec3 a, b, c;
            glm::vec3 hit_position;

            // Left plane
            {
                a = glm::vec3(min.x, min.y, min.z);
                b = glm::vec3(min.x, max.y, min.z);
                c = glm::vec3(min.x, max.y, max.z);

                if (glm::intersectLineTriangle(origin, direction, a, b, c, hit_position))
                {
                    return true;
                }

                a = glm::vec3(min.x, min.y, min.z);
                b = glm::vec3(min.x, min.y, max.z);
                c = glm::vec3(min.x, max.y, max.z);

                if (glm::intersectLineTriangle(origin, direction, a, b, c, hit_position))
                {
                    return true;
                }
            }

            // Right Plane
            {
                a = glm::vec3(max.x, min.y, min.z);
                b = glm::vec3(max.x, max.y, min.z);
                c = glm::vec3(max.x, max.y, max.z);

                if (glm::intersectLineTriangle(origin, direction, a, b, c, hit_position))
                {
                    return true;
                }

                a = glm::vec3(max.x, min.y, min.z);
                b = glm::vec3(max.x, min.y, max.z);
                c = glm::vec3(max.x, max.y, max.z);

                if (glm::intersectLineTriangle(origin, direction, a, b, c, hit_position))
                {
                    return true;
                }
            }

            // Back plane
            {
                a = glm::vec3(min.x, min.y, min.z);
                b = glm::vec3(max.x, min.y, min.z);
                c = glm::vec3(max.x, max.y, min.z);

                if (glm::intersectLineTriangle(origin, direction, a, b, c, hit_position))
                {
                    return true;
                }

                a = glm::vec3(min.x, min.y, min.z);
                b = glm::vec3(min.x, max.y, min.z);
                c = glm::vec3(max.x, max.y, min.z);

                if (glm::intersectLineTriangle(origin, direction, a, b, c, hit_position))
                {
                    return true;
                }
            }

            // Front plane
            {
                a = glm::vec3(min.x, min.y, max.z);
                b = glm::vec3(max.x, min.y, max.z);
                c = glm::vec3(max.x, max.y, max.z);

                if (glm::intersectLineTriangle(origin, direction, a, b, c, hit_position))
                {
                    return true;
                }

                a = glm::vec3(min.x, min.y, max.z);
                b = glm::vec3(min.x, max.y, max.z);
                c = glm::vec3(max.x, max.y, max.z);

                if (glm::intersectLineTriangle(origin, direction, a, b, c, hit_position))
                {
                    return true;
                }
            }

            // Bottom plane
            {
                a = glm::vec3(min.x, min.y, min.z);
                b = glm::vec3(max.x, min.y, min.z);
                c = glm::vec3(max.x, min.y, max.z);

                if (glm::intersectLineTriangle(origin, direction, a, b, c, hit_position))
                {
                    return true;
                }

                a = glm::vec3(min.x, min.y, min.z);
                b = glm::vec3(min.x, min.y, max.z);
                c = glm::vec3(max.x, min.y, max.z);

                if (glm::intersectLineTriangle(origin, direction, a, b, c, hit_position))
                {
                    return true;
                }
            }

            // Top plane
            {
                a = glm::vec3(min.x, max.y, min.z);
                b = glm::vec3(max.x, max.y, min.z);
                c = glm::vec3(max.x, max.y, max.z);

                if (glm::intersectLineTriangle(origin, direction, a, b, c, hit_position))
                {
                    return true;
                }

                a = glm::vec3(min.x, max.y, min.z);
                b = glm::vec3(min.x, max.y, max.z);
                c = glm::vec3(max.x, max.y, max.z);

                if (glm::intersectLineTriangle(origin, direction, a, b, c, hit_position))
                {
                    return true;
                }
            }

            // No hit.
            return false;
        }
    }  // namespace renderer
}  // namespace rra
