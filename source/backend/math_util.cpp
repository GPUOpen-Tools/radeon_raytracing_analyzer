//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the math utilities.
//=============================================================================

#include "bvh/dxr_definitions.h"
#include "glm/glm/glm.hpp"
#include "math_util.h"
#include "rra_bvh_impl.h"

namespace rra
{
    namespace math_util
    {
        /// @brief Find the minimum value of the 2 vectors provided, on a component basis.
        ///
        /// @param [in] v1 The first vector.
        /// @param [in] v2 The second vector.
        ///
        /// @return A vector containing the minimum values for each component.
        static glm::vec3 Min(const glm::vec3& v1, const glm::vec3& v2)
        {
            glm::vec3 result;
            result.x = std::min(v1.x, v2.x);
            result.y = std::min(v1.y, v2.y);
            result.z = std::min(v1.z, v2.z);
            return result;
        }

        /// @brief Find the maximum value of the 2 vectors provided, on a component basis.
        ///
        /// @param [in] v1 The first vector.
        /// @param [in] v2 The second vector.
        ///
        /// @return A vector containing the maximum values for each component.
        static glm::vec3 Max(const glm::vec3& v1, const glm::vec3& v2)
        {
            glm::vec3 result;
            result.x = std::max(v1.x, v2.x);
            result.y = std::max(v1.y, v2.y);
            result.z = std::max(v1.z, v2.z);
            return result;
        }

        BoundingVolumeExtents TransformAABB(const dxr::amd::AxisAlignedBoundingBox& bounding_box, const dxr::Matrix3x4& transform)
        {
            glm::vec3 right(transform[0], transform[4], transform[8]);
            glm::vec3 up(transform[1], transform[5], transform[9]);
            glm::vec3 backwards(transform[2], transform[6], transform[10]);

            glm::vec3 xa = right * bounding_box.min.x;
            glm::vec3 xb = right * bounding_box.max.x;
            glm::vec3 ya = up * bounding_box.min.y;
            glm::vec3 yb = up * bounding_box.max.y;
            glm::vec3 za = backwards * bounding_box.min.z;
            glm::vec3 zb = backwards * bounding_box.max.z;

            glm::vec3 translation(transform[3], transform[7], transform[11]);
            glm::vec3 min = Min(xa, xb) + Min(ya, yb) + Min(za, zb) + translation;
            glm::vec3 max = Max(xa, xb) + Max(ya, yb) + Max(za, zb) + translation;

            BoundingVolumeExtents result;

            result.min_x = min.x;
            result.min_y = min.y;
            result.min_z = min.z;
            result.max_x = max.x;
            result.max_y = max.y;
            result.max_z = max.z;

            return result;
        }
    }  // namespace math_util
}  // namespace rra
