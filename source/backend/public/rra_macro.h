//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Utility macro definitions.
//=============================================================================

#ifndef RRA_BACKEND_PUBLIC_RRA_MACRO_H_
#define RRA_BACKEND_PUBLIC_RRA_MACRO_H_

/// Helper macro to avoid warnings about unused variables.
#define RRA_UNUSED(x) ((void)(x))

/// Helper macro to align an integer to the specified power of 2 boundary
#define RRA_ALIGN_UP(x, y) (((x) + ((y)-1)) & ~((y)-1))

/// Helper macro to check if a value is aligned.
#define RRA_IS_ALIGNED(x) (((x) != 0) && ((x) & ((x)-1)))

/// Helper macro to stringify a value.
#define RRA_STR(s) RRA_XSTR(s)
#define RRA_XSTR(s) #s

/// Helper macro to return the maximum of two values.
#define RRA_MAXIMUM(x, y) (((x) > (y)) ? (x) : (y))

/// Helper macro to return the minimum of two values.
#define RRA_MINIMUM(x, y) (((x) < (y)) ? (x) : (y))

/// Helper macro to do safe free on a pointer.
#define RRA_SAFE_FREE(x) \
    if (x)               \
    free(x)

/// Helper macro to return the abs of an integer value.
#define RRA_ABSOLUTE(x) (((x) < 0) ? (-(x)) : (x))

/// Helper macro to return sign of a value.
#define RRA_SIGN(x) (((x) < 0) ? -1 : 1)

/// Helper macro to work out the number of elements in an array.
#define RRA_ARRAY_ELEMENTS(x) (int32_t)((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

#endif  // #ifndef RRA_BACKEND_PUBLIC_RRA_MACRO_H_
