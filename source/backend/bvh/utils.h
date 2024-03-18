//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for miscellaneous utility functions.
//=============================================================================

#ifndef RRA_BACKEND_BVH_UTILS_H_
#define RRA_BACKEND_BVH_UTILS_H_

#include <cstdint>

namespace rta
{
    /// @brief Converts count half-precision floats (uint16) stored in input array to floats in output array.
    ///
    /// @param [in]  input  The array of float16s to convert.
    /// @param [out] output The array to hold to converted float32s.
    /// @param [in]  count  The size of the array to convert.
    void ConvertHalfToFloat(const std::uint16_t* input, float* output, std::int32_t count);

    /// @brief Calculate how many box nodes per interior node count.
    ///
    /// @param [in] interior_node_branching_factor The branching factor.
    ///
    /// @return The node count.
    std::uint32_t ComputeBoxNodePerInteriorNodeCount(const std::uint32_t interior_node_branching_factor);

}  // namespace rta

#endif  // RRA_BACKEND_BVH_UTILS_H_
