//=============================================================================
// Copyright (c) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Bit manipulation helper functions.
//=============================================================================

#ifndef RRA_BACKEND_BITS_HLSLI
#define RRA_BACKEND_BITS_HLSLI

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

#endif  // RRA_BACKEND_BITS_HLSLI

