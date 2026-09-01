//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test log file implementation
//=============================================================================

#include "log.h"

#include <stdarg.h>
#include <iostream>
#include <string>

#ifdef _LINUX
#include "linux/safe_crt.h"
#endif

namespace backend_test
{
    Log::Log()
        : log_file_handle_(nullptr)
    {
    }

    Log::~Log()
    {
        Close();
    }

    void Log::Open(const char* filename, const char* new_suffix)
    {
        const char* kTraceFileExtension = ".rra";
        if (filename != nullptr)
        {
            bool        success = false;
            std::string output_filename(filename);
            size_t      offset = output_filename.find(kTraceFileExtension);
            if (offset != std::string::npos)
            {
                const std::string old_suffix = kTraceFileExtension;
                output_filename.replace(offset, old_suffix.length(), new_suffix);

                errno_t err = fopen_s(&log_file_handle_, output_filename.c_str(), "wt");
                if (err == 0)
                {
                    success = true;
                }
            }

            if (!success)
            {
                WriteConsole("ERROR: Unable to open log file %s", output_filename.c_str());
            }
        }
    }

    void Log::Write(const char* log_message, ...) const
    {
        va_list arg_ptr;
        va_start(arg_ptr, log_message);
        WriteImpl(log_message, arg_ptr, false);
        va_end(arg_ptr);
    }

    void Log::WriteConsole(const char* log_message, ...) const
    {
        va_list arg_ptr;
        va_start(arg_ptr, log_message);
        WriteImpl(log_message, arg_ptr, true);
        va_end(arg_ptr);
    }

    void Log::WriteImpl(const char* log_message, va_list arg_ptr, const bool write_to_console) const
    {
        if (log_message != nullptr)
        {
            if (write_to_console)
            {
                vprintf(log_message, arg_ptr);
                printf("\n");
            }

            if (log_file_handle_ != nullptr)
            {
                vfprintf(log_file_handle_, log_message, arg_ptr);
                fprintf(log_file_handle_, "\n");
            }
        }
    }

    void Log::Close()
    {
        if (log_file_handle_ != nullptr)
        {
            fclose(log_file_handle_);
            log_file_handle_ = nullptr;
        }
    }

}  // namespace backend_test

