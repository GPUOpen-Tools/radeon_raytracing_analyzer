//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of assert.
//=============================================================================

#include "public/rra_assert.h"

#include <stdlib.h>  // for malloc()

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <stdio.h>    // required for sprintf_s
#include <windows.h>  // required for OutputDebugString()
#endif                // #ifndef _WIN32

static RraAssertCallback s_assert_callback;

// set the printing callback function
void RraAssertSetPrintingCallback(RraAssertCallback callback)
{
    s_assert_callback = callback;
    return;
}

// implementation of assert reporting
bool RraAssertReport(const char* file, int32_t line, const char* condition, const char* message)
{
    if (!file)
    {
        return true;
    }

#ifdef _WIN32
    // form the final assertion string and output to the TTY.
    const size_t buffer_size = (size_t)snprintf(nullptr, 0, "%s(%d): ASSERTION FAILED. %s\n", file, line, message ? message : condition) + 1;
    char*        temp_buffer = static_cast<char*>(malloc(buffer_size));
    if (!temp_buffer)
    {
        return true;
    }

    if (!message)
    {
        sprintf_s(temp_buffer, buffer_size, "%s(%d): ASSERTION FAILED. %s\n", file, line, condition);
    }
    else
    {
        sprintf_s(temp_buffer, buffer_size, "%s(%d): ASSERTION FAILED. %s\n", file, line, message);
    }

    if (!s_assert_callback)
    {
        OutputDebugStringA(temp_buffer);
    }
    else
    {
        s_assert_callback(temp_buffer);
    }

    // free the buffer.
    free(temp_buffer);

#else
    RRA_UNUSED(line);
    RRA_UNUSED(condition);
    RRA_UNUSED(message);
#endif

    return true;
}

