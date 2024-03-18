//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for useful utility functions.
//=============================================================================

#ifndef RRA_UTIL_RRA_UTIL_H_
#define RRA_UTIL_RRA_UTIL_H_

#include <QString>

namespace file_util
{
    /// @brief Get file path to RRA log/settings file.
    ///
    /// Find the 'Temp' folder on the local OS and create an RRA subfolder (on linux, create .RRA folder).
    ///
    /// @return The location of the settings and log file.
    QString GetFileLocation();

};  // namespace rra_util

#endif  // RRA_UTIL_RRA_UTIL_H_
