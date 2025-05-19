//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Printing helper functions for RRA.
//=============================================================================

#ifndef RRA_BACKEND_PUBLIC_RRA_PRINT_H_
#define RRA_BACKEND_PUBLIC_RRA_PRINT_H_

#include <stdbool.h>

#include "public/rra_error.h"

/// Callback function for printing.
typedef void (*RraPrintingCallback)(const char* msg);

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

/// @brief Set the printing callback for backend functions to do logging.
///
/// @param [in] callback_func   The callback function to use for printing.
/// @param [in] enable_printing Enable the print function that prints to stdout if no callback specified.
///                             May be useful to disable in case the backend is being used outside of RRA.
void RraSetPrintingCallback(RraPrintingCallback callback_func, bool enable_printing);

/// @brief Printing function to use. Will use printing function set with
/// <c><i>RraSetPrintingCallback</i></c>. If nothing is set, then
/// printf will be used.
///
/// @param [in] format The formatting string.
/// @param [in] ...    Variable parameters determined by <c><i>format</i></c>.
void RraPrint(const char* format, ...);

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus
#endif  // #ifndef RRA_BACKEND_PUBLIC_RRA_PRINT_H_

