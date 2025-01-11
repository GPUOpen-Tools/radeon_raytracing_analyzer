//==============================================================================
// Copyright (c) 2016-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Linux implementation of Windows safe CRT functions.
//==============================================================================

#if !defined(_WIN32)

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "public/linux/safe_crt.h"
#include "public/rra_macro.h"

static errno_t validate_string_params(char* destination, size_t size, const char* source)
{
    errno_t result = 0;

    if (destination == nullptr)
    {
        result = EINVAL;
    }
    else if (source == nullptr)
    {
        destination[0] = '\0';
        result         = EINVAL;
    }
    else if (size == 0)
    {
        result = ERANGE;
    }
    else if (strlen(source) > size)
    {
        destination[0] = '\0';
        result         = ERANGE;
    }

    return result;
}

errno_t fopen_s(FILE** file, const char* filename, const char* mode)
{
    if (file == 0 || filename == 0 || mode == 0)
    {
        return EINVAL;
    }

    *file = fopen(filename, mode);

    if (*file == nullptr)
    {
        return errno;
    }

    return 0;
}

int sprintf_s(char* buffer, size_t size_of_buffer, const char* format, ...)
{
    int     ret_val;
    va_list arg_ptr;

    va_start(arg_ptr, format);
    ret_val = vsnprintf(buffer, size_of_buffer, format, arg_ptr);
    va_end(arg_ptr);
    return ret_val;
}

int fprintf_s(FILE* stream, const char* format, ...)
{
    int     ret_val;
    va_list arg_ptr;

    va_start(arg_ptr, format);
    ret_val = vfprintf(stream, format, arg_ptr);
    va_end(arg_ptr);
    return ret_val;
}

size_t fread_s(void* buffer, size_t buffer_size, size_t element_size, size_t count, FILE* stream)
{
    if ((element_size * count) > buffer_size)
    {
        return 0;
    }

    return fread(buffer, element_size, buffer_size, stream);
}

errno_t strcpy_s(char* destination, size_t size, const char* source)
{
    errno_t result = validate_string_params(destination, size, source);

    if (result == 0)
    {
        if (strncpy(destination, source, size) == nullptr)
        {
            result = ERANGE;
        }
    }

    assert(result == 0);

    return result;
}

errno_t strncpy_s(char* out_destination, const size_t destination_size, const char* source, const size_t max_count)
{
    errno_t result             = 0;
    size_t  characters_to_copy = RRA_MINIMUM(destination_size, max_count);

    if (out_destination == nullptr)
    {
        result = EINVAL;
    }
    else if (source == nullptr)
    {
        out_destination[0] = '\0';
        result             = EINVAL;
    }
    else if (characters_to_copy == 0)
    {
        result = ERANGE;
    }

    if (result == 0)
    {
        strncpy(out_destination, source, characters_to_copy);
        *(out_destination + max_count) = '\0';
    }

    return result;
}

errno_t strcat_s(char* destination, size_t size, const char* source)
{
    errno_t result = validate_string_params(destination, size, source);

    if (result == 0)
    {
        if (strncat(destination, source, size) == nullptr)
        {
            result = ERANGE;
        }
    }

    assert(result == 0);

    return result;
}

#endif  // !_WIN32
