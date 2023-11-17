//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for flag, bitfield and bit magic helpers that do not depend
/// on specific types.
//=============================================================================

#ifndef RRA_BACKEND_BVH_FLAGS_UTIL_H_
#define RRA_BACKEND_BVH_FLAGS_UTIL_H_

#ifdef _MSC_VER                 /* Visual Studio */
#pragma warning(disable : 4505) /* disable: C4505: Removing unused local function */
#endif

#include <numeric>
#include <cstdint>
#include <type_traits>

// Contains all flag, bitfield and bit magic helpers that do not depend on specific types.
namespace rta
{
    // Helper function to extract the Value of an class enum
    template <typename E>
    constexpr auto to_underlying(E e) noexcept
    {
        return static_cast<std::underlying_type_t<E>>(e);
    }

    // Set and clear flag helper for int and uint

#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

    static void SetFlag(uint32_t& flagField, const uint32_t flag)
    {
        flagField = flagField | flag;
    }

    static void SetFlag(uint8_t& flagField, const uint8_t flag)
    {
        flagField = flagField | flag;
    }

    static void SetFlag(int& flagField, const int flag)
    {
        flagField = flagField | flag;
    }

    static void ClearFlag(uint32_t& flagField, const uint32_t flag)
    {
        flagField = flagField & (~flag);
    }

    static void ClearFlag(uint8_t& flagField, const uint8_t flag)
    {
        flagField = flagField & (~flag);
    }

    static void ClearFlag(int& flagField, const int flag)
    {
        flagField = flagField & (~flag);
    }

    static constexpr bool IsFlagSet(const uint32_t flagField, const uint32_t flag)
    {
        return flagField & flag;
    }

    static constexpr bool IsFlagSet(const uint8_t flagField, const uint8_t flag)
    {
        return flagField & flag;
    }

    static constexpr bool IsFlagSet(const int flagField, const int flag)
    {
        return flagField & flag;
    }

    static void SetOrClearFlag(uint32_t& flagField, const uint32_t flag, const bool value)
    {
        if (value)
        {
            SetFlag(flagField, flag);
        }
        else
        {
            ClearFlag(flagField, flag);
        }
    }

    static void SetOrClearFlag(int& flagField, const int flag, const bool value)
    {
        if (value)
        {
            SetFlag(flagField, flag);
        }
        else
        {
            ClearFlag(flagField, flag);
        }
    }

#ifndef _WIN32
#pragma GCC diagnostic pop
#endif

    template <typename T>
    static bool IsFlagSet(const T flagField, const T flag)
    {
        return IsFlagSet(to_underlying(flagField), to_underlying(flag));
    }

    template <typename T>
    static void SetFlag(T& flagField, const T flag)
    {
        auto tmp = to_underlying(flagField);
        SetFlag(tmp, to_underlying(flag));
        flagField = static_cast<T>(tmp);
    }

    template <typename T>
    static void ClearFlag(T& flagField, const T flag)
    {
        auto tmp = to_underlying(flagField);
        ClearFlag(tmp, to_underlying(flag));
        flagField = static_cast<T>(tmp);
    }

    template <typename T>
    static void SetOrClearFlag(T& flagField, const T flag, const bool value)
    {
        auto tmp = to_underlying(flagField);
        SetOrClearFlag(tmp, to_underlying(flag), value);
        flagField = static_cast<T>(tmp);
    }

}  // namespace rta

#endif  // RRA_BACKEND_BVH_FLAGS_UTIL_H_
