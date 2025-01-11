//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of miscellaneous utility functions.
//=============================================================================

#include "utils.h"

#include <array>
#include <cstring>  // --> Linux, memcpy
#include <immintrin.h>
#include <cmath>  // --> isnan, isinf, ceil

namespace rta
{
    void ConvertHalfToFloat(const std::uint16_t* input, float* output, std::int32_t count)
    {
        std::array<std::uint8_t, 8 * sizeof(float)> buffer;

        while (count > 0)
        {
            std::memcpy(buffer.data(), input, std::min(count, 8) * sizeof(std::uint16_t));

            __m128i half_vector  = _mm_loadu_si128(reinterpret_cast<__m128i*>(buffer.data()));
            __m256  float_vector = _mm256_cvtph_ps(half_vector);
            _mm256_storeu_ps(reinterpret_cast<float*>(buffer.data()), float_vector);

            std::memcpy(output, buffer.data(), std::min(count, 8) * sizeof(float));

            // Advance pointers
            input += 8;
            output += 8;

            // Decrement left elements
            count -= 8;
        }
    }

    std::uint32_t ComputeBoxNodePerInteriorNodeCount(const std::uint32_t interior_node_branching_factor)
    {
        return static_cast<std::uint32_t>(std::ceil(interior_node_branching_factor / 4.f));
    }

}  // namespace rta
