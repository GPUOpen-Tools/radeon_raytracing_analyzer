//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Version number info.
//=============================================================================

#ifndef RRA_VERSION_H
#define RRA_VERSION_H

#define STRINGIFY_VALUE(x) STRINGIFY(x)
#define STRINGIFY(x) #x

#define PRODUCT_APP_NAME "Radeon Raytracing Analyzer"  ///< Application name

#ifdef BETA
#define PRODUCT_BUILD_SUFFIX " - Beta"  ///< The build suffix to apply to the product name.(alpha, beta etc.)
#else
#define PRODUCT_BUILD_SUFFIX ""  ///< The build suffix to apply to the product name.(alpha, beta etc.)
#endif

#define PRODUCT_MAJOR_VERSION 1                 ///< Major version number
#define PRODUCT_MINOR_VERSION 0                 ///< Minor version number
#define PRODUCT_BUGFIX_NUMBER 0                 ///< Bugfix number
#define PRODUCT_BUILD_NUMBER 0                  ///< Build number
#define PRODUCT_BUILD_DATE_STRING "05/11/2022"  ///< Build date string
#define PRODUCT_BUILD_CURRENT_YEAR 2022         ///< The current year.

#define PRODUCT_COPYRIGHT_STRING "Copyright (C) " STRINGIFY_VALUE(PRODUCT_BUILD_CURRENT_YEAR) " Advanced Micro Devices, Inc. All rights reserved."

#define PRODUCT_VERSION_STRING                                                                                 \
    STRINGIFY_VALUE(PRODUCT_MAJOR_VERSION)                                                                     \
    "." STRINGIFY_VALUE(PRODUCT_MINOR_VERSION) "." STRINGIFY_VALUE(PRODUCT_BUGFIX_NUMBER) "." STRINGIFY_VALUE( \
        PRODUCT_BUILD_NUMBER)  ///< The full revision string.

#endif  // RRA_VERSION_H
