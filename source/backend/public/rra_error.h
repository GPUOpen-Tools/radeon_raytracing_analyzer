//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Error codes.
//=============================================================================

#ifndef RRA_BACKEND_PUBLIC_RRA_ERROR_H_
#define RRA_BACKEND_PUBLIC_RRA_ERROR_H_

#include <stdint.h>

#ifndef _WIN32
#include <stdlib.h>
#include <errno.h>
#endif

#include "public/rra_assert.h"

/// Typedef for error codes returned from functions in the RRA backend.
typedef int32_t RraErrorCode;

static const RraErrorCode kRraOk                          = 0;           /// The operation completed successfully.
static const RraErrorCode kRraErrorInvalidPointer         = 0x80000000;  /// The operation failed due to an invalid pointer.
static const RraErrorCode kRraErrorInvalidAlignment       = 0x80000001;  /// The operation failed due to an invalid alignment.
static const RraErrorCode kRraErrorInvalidSize            = 0x80000002;  /// The operation failed due to an invalid size.
static const RraErrorCode kRraEof                         = 0x80000003;  /// The end of the file was encountered.
static const RraErrorCode kRraErrorInvalidPath            = 0x80000004;  /// The operation failed because the specified path was invalid.
static const RraErrorCode kRraEndOfFile                   = 0x80000005;  /// The operation failed because end of file was reached.
static const RraErrorCode kRraErrorMalformedData          = 0x80000006;  /// The operation failed because of some malformed data.
static const RraErrorCode kRraErrorFileNotOpen            = 0x80000007;  /// The operation failed because a file was not open.
static const RraErrorCode kRraErrorOutOfMemory            = 0x80000008;  /// The operation failed because it ran out memory.
static const RraErrorCode kRraErrorIndexOutOfRange        = 0x80000009;  /// The operation failed because an index was out of range.
static const RraErrorCode kRraErrorPlatformFunctionFailed = 0x8000000a;  /// The operation failed because a platform-specific function failed.
static const RraErrorCode kRraErrorInvalidChildNode       = 0x8000000b;  /// The operation failed because the child node was invalid.
static const RraErrorCode kRraErrorNoASChunks             = 0x8000000c;  /// The operation failed because there were no acceleration structure chunks in the loaded trace.

/// Helper macro to return error code y from a function when a specific condition, x, is not met.
#define RRA_RETURN_ON_ERROR(x, y) \
    if (!(x))                     \
    {                             \
        return (y);               \
    }

/// Helper macro to bubble error code from a function if it is not kRraOk
#define RRA_BUBBLE_ON_ERROR(x)                             \
    {                                                      \
        RraErrorCode error_code = x;                       \
        if (error_code != kRraOk)                          \
        {                                                  \
            RraAssertReport(__FILE__, __LINE__, #x, NULL); \
            return error_code;                             \
        }                                                  \
    }

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus
#endif  // #ifndef RRA_BACKEND_PUBLIC_RRA_ERROR_H_
