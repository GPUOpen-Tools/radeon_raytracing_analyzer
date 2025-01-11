//=============================================================================
// Copyright (c) 2020-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of a number of string utilities.
//=============================================================================

#ifndef RRA_UTIL_STRING_UTIL_H_
#define RRA_UTIL_STRING_UTIL_H_

#include <QString>
#include <vector>
#include <string>

#include "vulkan/include/vulkan/vulkan_core.h"

namespace rra
{
    namespace string_util
    {
        /// @brief Convert string to upper case.
        ///
        /// Using Qt's toUpper() throws a warning so just do this manually here.
        /// Assumes ASCII and really only intended to capitalize hex number representations.
        ///
        /// @param [in] string The string to capitalize.
        ///
        /// @return capitalized string.
        QString ToUpperCase(const QString& string);

        /// @brief Split a string into tokens separated by a deliminator.
        ///
        /// @param [in] s The string to split.
        /// @param [in] delim The deliminator.
        /// @returns Vector of string tokens.
        std::vector<std::string> Split(const std::string& s, const std::string& delim);

        /// @brief Removes all whitespace leading and trailing a string.
        ///
        /// Whitespace is tabs, spaces, and newlines.
        /// @param s The string to trim.
        /// @returns The string with the whitespace removed.
        std::string Trim(std::string s);

        /// @brief Given an integer, return a string localized to English format.
        ///
        /// @param [in] value The value to convert.
        ///
        /// @return The localized string.
        QString LocalizedValue(int64_t value);

        /// @brief Given an integer, return a string localized to English format.
        ///
        /// @param [in] value The value to convert.
        ///
        /// @return The localized string.
        QString LocalizedValuePrecise(double value);

        /// @brief Get the localized string as a memory size.
        ///
        /// Append the memory units to the end of the string. Base 10 or base 2 can
        /// be selected. Base 2 uses 1024 rather than 1000. Units are appended to
        /// display XB for base 10 or XiB for base 2.
        ///
        /// @param [in] value   The value to display.
        /// @param [in] base_10 If true, use base 10 values, otherwise base 2.
        ///
        /// @return The localized string.
        QString LocalizedValueMemory(double value, bool base_10, bool use_round);

        /// @brief Get the build type string based on the build flags.
        ///
        /// @param [in] build_flags The flags used to build the BLAS.
        ///
        /// @return String describing the build type.
        QString GetBuildTypeString(VkBuildAccelerationStructureFlagBitsKHR build_flags);

    }  // namespace string_util
}  // namespace rra

#endif  // RRA_UTIL_STRING_UTIL_H_
