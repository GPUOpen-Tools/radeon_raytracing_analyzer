//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of printing helper functions for RRA.
//=============================================================================

#include "public/rra_print.h"

#include <stdarg.h>
#include <stdio.h>   // for sprintf
#include <string.h>  // for strcat

#include "rra_configuration.h"

#ifndef _WIN32
#include "public/linux/safe_crt.h"
#else
#include <Windows.h>
#endif

// The printing callback function.
static RraPrintingCallback printing_func       = nullptr;
static bool                is_printing_enabled = false;

void RraSetPrintingCallback(RraPrintingCallback callback_func, bool enable_printing)
{
    printing_func       = callback_func;
    is_printing_enabled = enable_printing;
}

void RraPrint(const char* format, ...)
{
    if (!is_printing_enabled)
    {
        return;
    }

    va_list args;
    va_start(args, format);

    if (printing_func == nullptr)
    {
        char buffer[RRA_STRING_BUFFER_SIZE];
        vsnprintf(buffer, RRA_STRING_BUFFER_SIZE, format, args);
#ifdef _WIN32
        const size_t len = strlen(buffer);
        buffer[len]      = '\n';
        buffer[len + 1]  = '\0';
        OutputDebugString(buffer);
#else
        printf("%s\n", buffer);
#endif
    }
    else
    {
        char buffer[RRA_STRING_BUFFER_SIZE];
        vsnprintf(buffer, RRA_STRING_BUFFER_SIZE, format, args);
        printing_func(buffer);
    }

    va_end(args);
}
