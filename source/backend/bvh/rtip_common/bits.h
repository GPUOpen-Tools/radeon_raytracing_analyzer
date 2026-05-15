//=============================================================================
// Copyright (c) 2024-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Bit manipulation helper functions.
//=============================================================================

#ifndef RRA_BACKEND_BITS_H
#define RRA_BACKEND_BITS_H

#include <cstdint>

//=====================================================================================================================
// Helper function for producing a 32 bit mask of one bit
inline uint32_t bit(uint32_t index)
{
    return 1u << index;
}

//=====================================================================================================================
// Helper function for generating a 32-bit bit mask
inline uint32_t bits(uint32_t bitcount)
{
    return (bitcount == 32) ? 0xFFFFFFFF : ((1u << bitcount) - 1);
}

//=====================================================================================================================
// Helper function for generating a 16-bit bit mask
inline uint16_t bits16(uint16_t bitcount)
{
    return (bitcount == 16) ? uint16_t(0xFFFFu) : uint16_t((1u << bitcount) - 1);
}

//=====================================================================================================================
// Helper function for generating a 32-bit bit mask
inline uint64_t bits64(uint64_t bitcount)
{
    return (bitcount == 64) ? 0xFFFFFFFFFFFFFFFFull : ((1ull << bitcount) - 1ull);
}

//=====================================================================================================================
// Helper function for inserting data into a src bitfield and returning the output
static uint32_t bitFieldInsert(uint32_t src, uint32_t bitOffset, uint32_t numBits, uint32_t data)
{
    const uint32_t mask = bits(numBits);
    src &= ~(mask << bitOffset);
    return (src | ((data & mask) << bitOffset));
}

//=====================================================================================================================
// Helper function for inserting data into a uint16_t src bitfield and returning the output
static uint16_t bitFieldInsert16(uint16_t src, uint16_t bitOffset, uint16_t numBits, uint16_t data)
{
    const uint16_t mask = bits16(numBits);
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
// Helper function for extracting data from a src bitfield
static uint16_t bitFieldExtract16(uint16_t src, uint16_t bitOffset, uint16_t numBits)
{
    return (src >> bitOffset) & bits16(numBits);
}

//=====================================================================================================================
// Helper function for extracting data from a uint64_t src bitfield
static uint64_t bitFieldExtract64(uint64_t src, uint64_t bitOffset, uint64_t numBits)
{
    return (src >> bitOffset) & bits64(numBits);
}

//=====================================================================================================================
static uint32_t Pow2Align(uint32_t value,      ///< Value to align.
                          uint32_t alignment)  ///< Desired alignment (must be a power of 2).
{
    return ((value + alignment - 1) & ~(alignment - 1));
}

#endif  // RRA_BACKEND_BITS_H

