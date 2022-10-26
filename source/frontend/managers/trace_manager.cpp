//==============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the Trace Manager.
//==============================================================================

#include "managers/trace_manager.h"

#include <QtCore>
#include <QMessageBox>
#include <QByteArray>
#include <vector>

#include "qt_common/utils/qt_util.h"

#ifndef _WIN32
#include "public/linux/safe_crt.h"
#endif

#include "public/rra_assert.h"
#include "public/rra_trace_loader.h"

#include "managers/load_animation_manager.h"
#include "managers/message_manager.h"
#include "settings/settings.h"
#include "util/rra_util.h"

namespace rra
{
    /// @brief Spawns a thread to load a dataset.
    class LoadingThread : public QThread
    {
    public:
        /// @brief Constructor.
        ///
        /// @param [in] path The full path of the data set to load.
        explicit LoadingThread(const QString& path)
            : path_data_(path)
        {
        }

        /// @brief Execute the loading thread.
        void run() Q_DECL_OVERRIDE
        {
            const TraceLoadReturnCode error_code = TraceManager::Get().TraceLoad(path_data_);
            emit                      TraceManager::Get().TraceLoadThreadFinished(error_code);
        }

    private:
        QString path_data_;  ///< The path to the trace being loaded
    };

    /// @brief Pointer to the loading thread object.
    static LoadingThread* loading_thread = nullptr;

    /// The single instance of the trace manager.
    static TraceManager trace_manager;

    TraceManager& TraceManager::Get()
    {
        return trace_manager;
    }

    TraceManager::TraceManager(QObject* parent)
        : QObject(parent)
        , parent_(nullptr)
    {
        int id = qRegisterMetaType<TraceLoadReturnCode>();
        Q_UNUSED(id);

        ClearTrace();
    }

    TraceManager::~TraceManager()
    {
    }

    void TraceManager::Initialize(QWidget* parent)
    {
        parent_ = parent;
    }

    TraceLoadReturnCode TraceManager::TraceLoad(const QString& trace_file_name)
    {
        active_trace_path_ = QDir::toNativeSeparators(trace_file_name);

        // Loading regular binary RRA data.
        QByteArray   latin_1    = trace_file_name.toLatin1();
        const char*  file_name  = latin_1.data();
        RraErrorCode error_code = RraTraceLoaderLoad(file_name);

        if (error_code == kRraErrorNoASChunks)
        {
            return kTraceLoadReturnFailNoAS;
        }
        else if (error_code == kRraErrorMalformedData)
        {
            return kTraceLoadReturnFailMissingAS;
        }
        else if (error_code != kRraOk)
        {
            return kTraceLoadReturnFail;
        }

        // Execute the arbitrary callback.
        if (loading_finished_callback_)
        {
            loading_finished_callback_();
        }

        // Create the default timeline for the data set.
        return kTraceLoadReturnSuccess;
    }

    void TraceManager::ClearTrace()
    {
        if (clear_trace_callback_)
        {
            clear_trace_callback_();
        }

        RraTraceLoaderUnload();
        active_trace_path_.clear();
    }

    void TraceManager::ShowMessageBox(const QString& active_trace_path, const QString& title, const QString& text)
    {
        bool in_recent_trace_list = Settings::Get().InRecentFilesList(active_trace_path);
        if (in_recent_trace_list)
        {
            // The selected trace file is missing on the disk so display a message box stating so.
            const int ret = QtCommon::QtUtils::ShowMessageBox(
                parent_, QMessageBox::Yes | QMessageBox::No, QMessageBox::Question, title, text + text::kRemoveRecentTraceText);
            if (ret == QMessageBox::Yes)
            {
                Settings::Get().RemoveRecentFile(active_trace_path);
                Settings::Get().SaveSettings();
                emit TraceFileRemoved();
            }
        }
        else
        {
            QtCommon::QtUtils::ShowMessageBox(parent_, QMessageBox::Ok, QMessageBox::Critical, title, text);
        }
    }

    void TraceManager::LoadTrace(const QString& path)
    {
        bool result        = false;
        bool missing_trace = false;

        if (ReadyToLoadTrace())
        {
            if (rra_util::TraceValidToLoad(path) == true)
            {
                QFileInfo trace_file(path);

                if (!path.isEmpty() && trace_file.exists())
                {
                    if (!RraTraceLoaderValid())
                    {
                        // Nothing loaded, so load.
                        // Save the file location for future reference.
                        Settings::Get().SetLastFileOpenLocation(path);

                        // Set up callback for when loading thread is done.
                        connect(this, &TraceManager::TraceLoadThreadFinished, this, &TraceManager::FinalizeTraceLoading);

                        loading_thread = new LoadingThread(path);
                        loading_thread->start();

                        result = true;
                    }

                    else if (SameTrace(trace_file) == false)
                    {
                        // Fire up a new instance if desired trace is different than current.
                        // Attempt to open a new instance of RRA using the selected trace file as an argument.
                        const QString executable_name = qApp->applicationDirPath() + GetDefaultExeName();

                        // If the executable does not exist, put up a message box.
                        QFileInfo file(executable_name);
                        if (file.exists())
                        {
                            QProcess* process = new QProcess(this);
                            if (process != nullptr)
                            {
                                QStringList args;
                                args << path;

                                bool process_result = process->startDetached(executable_name, args);

                                if (!process_result)
                                {
                                    // The selected trace file is missing on the disk so display a message box stating so.
                                    const QString text = text::kOpenRecentTraceStart + trace_file.fileName() + text::kOpenRecentTraceEnd;
                                    QtCommon::QtUtils::ShowMessageBox(parent_, QMessageBox::Ok, QMessageBox::Critical, text::kOpenRecentTraceTitle, text);
                                }
                            }
                        }
                        else
                        {
                            // If the executable does not exist, put up a message box.
                            const QString text = executable_name + " does not exist";
                            QtCommon::QtUtils::ShowMessageBox(parent_, QMessageBox::Ok, QMessageBox::Critical, text::kOpenRecentTraceTitle, text);
                        }
                    }
                    else
                    {
                        // Reload the same file.
                        emit TraceClosed();

                        // Set up callback for when loading thread is done.
                        connect(this, &TraceManager::TraceLoadThreadFinished, this, &TraceManager::FinalizeTraceLoading);

                        loading_thread = new LoadingThread(path);
                        loading_thread->start();

                        result = true;
                    }
                }
                else
                {
                    missing_trace = true;
                }
            }
            else
            {
                missing_trace = true;
            }
        }

        if (missing_trace)
        {
            const QString active_trace_path = QDir::toNativeSeparators(path);
            QString       text              = text::kOpenRecentTraceStart + active_trace_path + text::kOpenRecentTraceEnd;

            ShowMessageBox(active_trace_path, text::kOpenRecentTraceTitle, text);
        }

        if (result == true)
        {
            LoadAnimationManager::Get().StartAnimation();
        }
    }

    void TraceManager::FinalizeTraceLoading(TraceLoadReturnCode error_code)
    {
        LoadAnimationManager::Get().StopAnimation();

        if (error_code != kTraceLoadReturnSuccess)
        {
            QFileInfo file_info(active_trace_path_);
            QString   text = text::kDeleteRecentTraceTextFailed;

            // If the trace failed to load, indicate a potential reason for the failure
            // to the user.
            if (error_code == kTraceLoadReturnFailNoAS)
            {
                text += text::kDeleteRecentTraceTextNoAS;
            }
            else if (error_code == kTraceLoadReturnFailMissingAS)
            {
                text += text::kDeleteRecentTraceTextMalformedData;
            }

            ShowMessageBox(active_trace_path_, text::kDeleteRecentTraceTitle, text);
        }

        if (RraTraceLoaderValid())
        {
            Settings::Get().TraceLoaded(active_trace_path_, QString::number(RraTraceLoaderGetCreateTime()));
            Settings::Get().SaveSettings();

            if (error_code == kTraceLoadReturnSuccess)
            {
                emit TraceOpened();
            }
        }

        disconnect(this, &TraceManager::TraceLoadThreadFinished, this, &TraceManager::FinalizeTraceLoading);

        // Defer deleting of this object until later, in case the thread is still executing something
        // under the hood and can't be deleted right now. Even though the thread may have finished working,
        // it may still have access to mutexes and deleting right now might be bad.
        loading_thread->deleteLater();
        loading_thread = nullptr;

        if (error_code != kTraceLoadReturnSuccess)
        {
            ClearTrace();
        }
    }

    bool TraceManager::SameTrace(const QFileInfo& new_trace) const
    {
        const QString new_trace_file_path    = QDir::toNativeSeparators(new_trace.absoluteFilePath());
        const QString active_trace_file_path = QDir::toNativeSeparators(active_trace_path_);

        return (new_trace_file_path.compare(active_trace_file_path) == 0);
    }

    bool TraceManager::ReadyToLoadTrace() const
    {
        return (loading_thread == nullptr || loading_thread->isRunning() == false);
    }

    QString TraceManager::GetDefaultExeName() const
    {
        QString default_exe_name;
        default_exe_name.append(QDir::separator());
        default_exe_name.append(kExecutableBaseFilename);
#ifdef _DEBUG
        default_exe_name.append(kExecutableDebugIdentifier);
#endif

#ifdef WIN32
        // Append an extension only in Windows.
        default_exe_name.append(".exe");
#endif

        return default_exe_name;
    }

    const QString& TraceManager::GetTracePath() const
    {
        return active_trace_path_;
    }

    void TraceManager::SetLoadingFinishedCallback(std::function<void()> loading_finished_callback)
    {
        loading_finished_callback_ = loading_finished_callback;
    }

    void TraceManager::SetClearTraceCallback(std::function<void()> clear_trace_callback)
    {
        clear_trace_callback_ = clear_trace_callback;
    }

}  // namespace rra
