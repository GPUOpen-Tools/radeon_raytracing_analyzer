//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test log file header.
///
/// The log file will contain detailed information about the test run so the
/// exact failure can be determined.
//=============================================================================

#ifndef RRA_BACKEND_TEST_LOG_H_
#define RRA_BACKEND_TEST_LOG_H_

#include <stdio.h>

namespace backend_test
{
    class Log
    {
    public:
        /// @brief Constructor.
        Log();

        /// @brief Destructor.
        ~Log();

        /// @brief Open / create the log file.
        ///
        /// @param [in] filename The name of the trace file being tested.
        /// @param [in] new_suffix The suffix to add to the name of the log file.
        void Open(const char* filename, const char* new_suffix);

        /// @brief Write a string to the log file.
        ///
        /// @param [in] log_message The string to write.
        void Write(const char* log_message, ...) const;

        /// @brief Write a string to both the console and the log file.
        ///
        /// @param [in] log_message The string to write.
        void WriteConsole(const char* log_message, ...) const;

        /// @brief Close the log file.
        void Close();

    private:
        /// @brief Write a string to the log file and optionally the console.
        ///
        /// @param [in] log_message         The string to write.
        /// @param [in] arg_ptr             Additional arguments dependent on <c><i>log_message</i></c> formatting.
        /// @param [in] write_to_console    If true, the message is output to the console in addition to the log file.
        void WriteImpl(const char* log_message, va_list arg_ptr, const bool write_to_console) const;

    private:
        FILE* log_file_handle_;  ///< Handle to the log file.
    };
}  // namespace backend_test

#endif  // RRA_BACKEND_TEST_LOG_H_
