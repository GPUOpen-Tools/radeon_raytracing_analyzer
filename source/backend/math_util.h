//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the math utilities.
//=============================================================================

#ifndef RRA_BACKEND_MATH_UTIL_H_
#define RRA_BACKEND_MATH_UTIL_H_

#include "bvh/dxr_definitions.h"
#include "rra_bvh_impl.h"

namespace rra
{
    namespace math_util
    {
        /// @brief Helper function to transform an AABB by tranforming all its corner points and against taking
        /// the minimum and maximum of these results. May increase the volume of the AABB up to a factor of 2.
        ///
        /// NOTE: A Matrix3x4 is a transposed 4x4 matrix with the translation in the last column rather than the
        /// last row.
        ///
        /// @param [in] bounding_box The bounding box to be transformed.
        /// @param [in] transform    The transformation matrix to be applied.
        ///
        /// @return The transformed bounding box.
        BoundingVolumeExtents TransformAABB(const dxr::amd::AxisAlignedBoundingBox& bounding_box, const dxr::Matrix3x4& transform);

    }  // namespace math_util
}  // namespace rra

#endif  // RRA_BACKEND_MATH_UTIL_H_

