//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the LogFileWriter.
///
/// The LogFileWriter facilitates writing of log messages to a log file.
///
//=============================================================================

#include "util/log_file_writer.h"

#ifdef _WIN32
#include <Windows.h>
#include <Shlobj.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#endif

#include <QFile>

#include "util/file_util.h"

namespace rra
{
    // LogFileWriter instance.
    LogFileWriter* LogFileWriter::instance_ = nullptr;

    LogFileWriter::LogFileWriter()
#ifdef _DEBUG
        : log_level_(kDebug)
#else
        : log_level_(kError)
#endif
    {
        // Delete the log file from the previous instance.
        QFile::remove(GetLogFileLocation());
    }

    LogFileWriter::~LogFileWriter()
    {
    }

    LogFileWriter& LogFileWriter::Get()
    {
        if (instance_ == nullptr)
        {
            instance_ = new LogFileWriter();
        }

        return *instance_;
    }

    void LogFileWriter::WriteLogMessage(const char* log_message)
    {
        // Lock the mutex before writing out the log to the file.
        // The mutex gets released when this method returns.
        QMutexLocker locker(&mutex_);

        // Get the file name and location.
        QFile file(GetLogFileLocation());

        // Open the file.
        if (file.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            // Write the data to the file.
            file.write(log_message);

            // Add a new line to the file.
            file.write("\r\n");

            // Now close the file.
            file.close();
        }
    }

    void LogFileWriter::WriteLog(LogLevel log_level, const char* log_message, ...)
    {
        if (log_level <= log_level_)
        {
            static const int kBufferSize = 2048;
            char             buffer[kBufferSize];
            va_list          args;

            va_start(args, log_message);
            vsnprintf(buffer, kBufferSize, log_message, args);
            WriteLogMessage(buffer);
            va_end(args);
        }
    }

    QString LogFileWriter::GetLogFileLocation()
    {
        QString log_file = "";

        // Get file location.
        log_file = file_util::GetFileLocation();

        // Add the file name.
        log_file.append("/RRALogFile.txt");

        return log_file;
    }
}  // namespace rra
