//=============================================================================
//  Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Math functions used in some RT IP 3.1 (Navi4x) specific code.
//=============================================================================

#ifndef RRA_BACKEND_MATH_H_
#define RRA_BACKEND_MATH_H_

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4505)  // Disable unreferenced local function warning.
#pragma warning(disable : 4189)  // Disable unreferenced local variable warning.
#pragma warning(disable : 4100)  // Disable unreferenced formal parameter warning.
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#include <cstdint>
#include <algorithm>
#include <cfenv>
#include "ray_tracing_defs.h"

#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_TiesToEven 0x0
#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_TowardPositive 0x1
#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_TowardNegative 0x2
#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_TowardZero 0x3

#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_Add 0x0
#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_Subtract 0x1
#define AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_Multiply 0x2

static constexpr uint32_t RoundModeTable[] = {
    FE_TONEAREST,
    FE_UPWARD,
    FE_DOWNWARD,
    FE_TOWARDZERO,
};

#ifdef RRA_INTERNAL_COMMENTS
// FloatOpWithRoundMode is taken from
// https://github.amd.com/AMD-Radeon-Driver/gpurt/blob/87ec895fba48df3c3a15f81409a6043407d75b3e/src/shaders/Extensions.hlsl
#endif

inline float asfloat(uint32_t t)
{
    return *reinterpret_cast<float*>(&t);
}

inline uint32_t asuint(float t)
{
    return *reinterpret_cast<uint32_t*>(&t);
}

inline glm::vec3 asfloat(glm::uvec3& v)
{
    return {
        *reinterpret_cast<float*>(&v[0]),
        *reinterpret_cast<float*>(&v[1]),
        *reinterpret_cast<float*>(&v[2]),
    };
}

inline glm::uvec3 asuint(glm::vec3& v)
{
    return {
        *reinterpret_cast<uint32_t*>(&v[0]),
        *reinterpret_cast<uint32_t*>(&v[1]),
        *reinterpret_cast<uint32_t*>(&v[2]),
    };
}

//=====================================================================================================================
static glm::vec3 FloatOpWithRoundMode(uint32_t roundMode, uint32_t operation, glm::vec3 src0, glm::vec3 src1)
{
    std::fesetround(RoundModeTable[roundMode]);

    glm::vec3 result;

    switch (operation)
    {
    case AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_Add:
        result = src0 + src1;
        break;

    case AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_Subtract:
        result = src0 - src1;
        break;

    case AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_Multiply:
        result = src0 * src1;
        break;

    default:
        printf("Unknown operation for FloatOpWithRoundMode\n");
        assert(false);
        break;
    }

    std::fesetround(FE_TONEAREST);

    return result;
}

//=====================================================================================================================
static BoundingBox CombineAABB(BoundingBox b0, BoundingBox b1)
{
    BoundingBox bbox;

    bbox.min.x = std::min(b0.min.x, b1.min.x);
    bbox.min.y = std::min(b0.min.y, b1.min.y);
    bbox.min.z = std::min(b0.min.z, b1.min.z);
    bbox.max.x = std::max(b0.max.x, b1.max.x);
    bbox.max.y = std::max(b0.max.y, b1.max.y);
    bbox.max.z = std::max(b0.max.z, b1.max.z);

    return bbox;
}

//=====================================================================================================================
// Helper function for producing a 32 bit mask of one bit
inline uint32_t bit(uint32_t index)
{
    return 1u << index;
}

//=====================================================================================================================
// Helper function for producing a 64 bit mask of one bit
inline uint64_t bit64(uint32_t index)
{
    return 1ull << index;
}

//=====================================================================================================================
// Helper function for generating a 32-bit bit mask
inline uint32_t bits(uint32_t bitcount)
{
    return (bitcount == 32) ? 0xFFFFFFFF : ((1u << bitcount) - 1);
}

//=====================================================================================================================
// Helper function for generating a 32-bit bit mask
inline uint64_t bits64(uint64_t bitcount)
{
    return (bitcount == 64) ? 0xFFFFFFFFFFFFFFFFull : ((1ull << bitcount) - 1ull);
}

//=====================================================================================================================
//# The uint64 overload for countbits() is broken in older versions of the DXIL translator. This is a workaround until
//# SSC is updated.
/* Temporarily comment out since some things in this function are undefined.
inline uint32_t countbits64(uint64_t val)
{
    return countbits(LowPart(val)) + countbits(HighPart(val));
}
*/

//=====================================================================================================================
// Helper function for inserting data into a src bitfield and returning the output
static uint32_t bitFieldInsert(uint32_t src, uint32_t bitOffset, uint32_t numBits, uint32_t data)
{
    const uint32_t mask = bits(numBits);
    src &= ~(mask << bitOffset);
    return (src | ((data & mask) << bitOffset));
}

//=====================================================================================================================
// Helper function for inserting data into a uint64_t src bitfield and returning the output
static uint64_t bitFieldInsert64(uint64_t src, uint64_t bitOffset, uint64_t numBits, uint64_t data)
{
    const uint64_t mask = bits64(numBits);
    src &= ~(mask << bitOffset);
    return (src | ((data & mask) << bitOffset));
}

//=====================================================================================================================
// Helper function for extracting data from a src bitfield
static uint32_t bitFieldExtract(uint32_t src, uint32_t bitOffset, uint32_t numBits)
{
    return (src >> bitOffset) & bits(numBits);
}

//=====================================================================================================================
static uint32_t Pow2Align(uint32_t value,      ///< Value to align.
                          uint32_t alignment)  ///< Desired alignment (must be a power of 2).
{
    return ((value + alignment - 1) & ~(alignment - 1));
}

//=====================================================================================================================
// Search the mask data from least significant bit (LSB) / Lowest Order bit
// to the most significant bit (MSB) for a set bit (1).
//  firstbitlow   <--> _BitScanForward   LSB -> MSB  (lowBit -> HighBit)
static uint32_t ScanForward(uint32_t value)
{
#if defined(__cplusplus)
#ifdef _WIN32
    unsigned long outV;
    char          result = _BitScanForward(&outV, value);
    return result ? outV : 32;
#else
    for (uint32_t i{0}; i < 32; ++i)
    {
        if (value & (1 << i))
        {
            return i;
        }
    }
    return 32;
#endif
#else
    // Gets the location of the first set bit starting from the highest order bit and working downward, per component.
    return firstbitlow(value);
#endif
}

//=====================================================================================================================
// Search the mask data from most significant bit (MSB) / Highest Order bit
// to least significant bit (LSB) for a set bit (1)
//  firstbithigh  <--> _BitScanReverse   MSB -> LSB  (HighBit -> LowBit)
static uint32_t ScanReverse(uint32_t value)
{
    // This function is different than "clz"
    // if the input value is 0, this function needs to return 32 instaed.
    uint32_t ret = (value > 0) ? (31u - glm::findMSB(value)) : 32u;

    return ret;
}

//=====================================================================================================================
static uint32_t CommonTrailingZeroBits(uint32_t unions)
{
    uint32_t scanSuffix = ScanForward(unions);
    return scanSuffix;
}

//=====================================================================================================================
static uint32_t CommonPrefixBit(uint32_t diff)
{
    uint32_t prefixLength = ScanReverse(diff);
    return prefixLength == 32 ? 31u : prefixLength;
}

//=====================================================================================================================
static glm::uvec3 CommonPrefixBits(glm::uvec3 diffs)
{
    uint32_t xPrefixLength = CommonPrefixBit(diffs.x);
    uint32_t yPrefixLength = CommonPrefixBit(diffs.y);
    uint32_t zPrefixLength = CommonPrefixBit(diffs.z);

    glm::uvec3 retData = glm::uvec3(xPrefixLength, yPrefixLength, zPrefixLength);
    return retData;
}

//=====================================================================================================================
static uint32_t LeadingZeroBits(uint32_t u)
{
    return ScanReverse(u);
}

//=====================================================================================================================
// Compute common exponent
static glm::uvec3 ComputeCommonExponent(glm::vec3 minOfMins, glm::vec3 maxOfMaxs)
{
    // Rounding mode here is necessary for cases where floating point precision
    // loses the MSB of min in the subtract. For example, 33554432 - (-0.1) = 33554432.
    // The correct exponent for this operation should be log2(2 * 33554432) not log2(33554432)
    glm::vec3 extent = FloatOpWithRoundMode(
        AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_TowardPositive, AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_Subtract, maxOfMaxs, minOfMins);

    glm::uvec3 exponents = asuint(extent) + glm::uvec3(0x7FFFFF);
    exponents.x >>= 23;
    exponents.y >>= 23;
    exponents.z >>= 23;

    // Due to a HW-savings measure in the dequantize function, boxs using very small exponents (<= 12)
    // cannot make effective use of all plane values. This is because the dequantization occurs
    // separately from the origin so it's possible to generate denormal numbers.
    // For example, if exponent is 2 and the encoded value is 1 then decoding would produce:
    // (1 / 4096) * 2 ** (2 - 127) = 2 ** (-12) * 2 ** (-127) = 2 ** (-139) = 2 ** (-12 - 127)
    // negative exponents result in denormal numbers in ieee float32

    // | Exponent   | Valid Plane Encodings |
    // | 0          | 0                     |
    // | 1          | 1                     |
    // | 2          | 2 (0, 2048)           |
    // | 3          | 4 (0,1024,2048,3192)  |
    // | 4          | 8                     |
    // ...
    // | 12         | 2048                  |
    // | 13         | 4096                  |

    // Given this restriction, it is equivalent to use exponent 13 instead of most precise exponent when exponent <= 12.
    // For example, if the exponent is 3 and a plane is encoded with 1024 it would decode to
    // (1024 / 4096) * 2 ** (3 - 127) = (1/4) * 2 ** (-124) = [2 ** (-2)] * [2 ** (-124)] = 2 ** -126
    // Now the same using an exponent value of 13
    // (1 / 4096) * 2 ** (13 - 127) = 2 ** (-12) * 2 ** (-114) = 2 ** -126

    // Forcing the minimum exponent to 13 saves up to 48 multiplications in the quantizaiton loop
    // with no loss of precision (beyond the existing hardware limitation).
    // 0 is a special case of infinitly-thin geometry that is slightly more precise
    // than using exponent = 13
    exponents.x = exponents.x == 0 ? 0 : std::max(13u, exponents.x);
    exponents.y = exponents.y == 0 ? 0 : std::max(13u, exponents.y);
    exponents.z = exponents.z == 0 ? 0 : std::max(13u, exponents.z);

    return exponents;
}

//=====================================================================================================================
// Compute N-bit quantized max
static glm::uvec3 ComputeQuantizedMax(glm::vec3 maxValue, glm::vec3 origin, glm::vec3 rcpExponents, uint32_t numBits)
{
    glm::vec3 diff = FloatOpWithRoundMode(
        AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_TowardPositive, AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_Subtract, maxValue, origin);

    glm::vec3 fquantMax = FloatOpWithRoundMode(
        AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_TowardPositive, AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_Multiply, diff, rcpExponents);

    fquantMax = ceil(fquantMax);

    const float maxFloat = (1u << numBits) * 1.0f;

    // Map the max to a value in [1,2^n].
    // Clamp is necessary to map 2^n + 1 back to a valid number (possible when using upward rounding mode.)
    const glm::uvec3 quantMax =
        glm::vec3{std::clamp(fquantMax.x, 1.0f, maxFloat), std::clamp(fquantMax.y, 1.0f, maxFloat), std::clamp(fquantMax.z, 1.0f, maxFloat)};

    // Subtract 1 to map value to a N-bit number
    return quantMax - glm::uvec3(1, 1, 1);
}

//=====================================================================================================================
// Compute 8-bit integer reciprocal. Should this function take in an encoding granularity? It looks like this
// assumes 12-bit encoding
static glm::vec3 ComputeFastExpReciprocal(glm::uvec3 exponents)
{
    // Computing rcpExponents guarentees that the compiler will not emit
    // transcendental ops for the plane quantization.
    // The + 12 comes from the fact that 2 ** 12 = 4096 which is the encoding granularity
    glm::uvec3 rcpExponentsUint = (glm::uvec3(254, 254, 254) - exponents + glm::uvec3(12));
    rcpExponentsUint.x <<= 23;
    rcpExponentsUint.y <<= 23;
    rcpExponentsUint.z <<= 23;
    const glm::vec3 rcpExponents = asfloat(rcpExponentsUint);

    // Note that this optimization results in this function being ill-defined for inputs with exponent == 254.
    // It is unlikely any app will use geometry with enourmous bounds like this. This could be handled with a special
    // case or by falling back to infinitely large boxes and accepting all hits and pushing the problem down the BVH.
    return rcpExponents;
}

//=====================================================================================================================
static glm::uvec3 ComputeQuantizedMin(glm::vec3 minValue, glm::vec3 origin, glm::vec3 rcpExponents, uint32_t numBits)
{
    glm::vec3 diff = FloatOpWithRoundMode(
        AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_TowardNegative, AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_Subtract, minValue, origin);

    glm::vec3 fquantMin = FloatOpWithRoundMode(
        AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_TowardNegative, AmdExtD3DShaderIntrinsicsFloatOpWithRoundMode_Multiply, diff, rcpExponents);

    fquantMin = floor(fquantMin);

    const float maxFloat = ((1u << numBits) - 1) * 1.0f;

    const glm::uvec3 quantMin = clamp(fquantMin, glm::vec3(0., 0., 0.), glm::vec3(maxFloat, maxFloat, maxFloat));
    return quantMin;
}

//=====================================================================================================================
// Decode a quantized integer into float
static float Dequantize(float origin, uint32_t exponent, uint32_t plane, uint32_t numBits)
{
    uint32_t result = 0;

    if (plane)
    {
        int signedPos      = glm::findMSB(plane) - numBits;  // 31 - numBits - count_leading_zeros(plane)
        int signedExponent = signedPos + exponent;

        if (signedExponent > 0)
        {
            result = (plane << (23 - numBits - signedPos)) & bits(23);  // set mantissa
            result |= (signedExponent << 23) & bits(31);                // set exponent
        }
    }

    return asfloat(result) + origin;
}

#ifdef _WIN32
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

#endif  // RRA_BACKEND_MATH_H_
