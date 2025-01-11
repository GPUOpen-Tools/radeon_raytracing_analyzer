//==============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Class definition for the Trace Manager.
//==============================================================================

#ifndef RRA_MANAGERS_TRACE_MANAGER_H_
#define RRA_MANAGERS_TRACE_MANAGER_H_

#include <functional>

#include <QFileInfo>
#include <QObject>
#include <QVector>

#include "public/rra_error.h"
#include "public/renderer_types.h"

Q_DECLARE_METATYPE(RraErrorCode);

/// @brief Enum of trace loading thread return codes.
enum TraceLoadReturnCode
{
    kTraceLoadReturnError,
    kTraceLoadReturnSuccess,
    kTraceLoadReturnFailNoAS,
    kTraceLoadReturnFailMissingAS,
    kTraceLoadReturnFailIncompatible,
    kTraceLoadReturnFail,
    kTraceLoadReturnAlreadyOpened,
    kTraceLoadUnrecognizedRtIpLevel,
};

Q_DECLARE_METATYPE(TraceLoadReturnCode)

namespace rra
{
    /// @brief This class owns and manages growth and updating of the dataset.
    class TraceManager : public QObject
    {
        Q_OBJECT

    public:
        /// @brief Constructor.
        ///
        /// @param [in] parent The parent widget.
        explicit TraceManager(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~TraceManager();

        /// @brief Accessor for singleton instance.
        static TraceManager& Get();

        /// @brief Initialize the trace manager.
        ///
        /// @param [in] main_window Pointer to main window widget. Used as the parent for
        ///  pop up message boxes.
        void Initialize(QWidget* main_window);

        /// @brief Determine if we're ready to load a trace.
        ///
        /// @return true if ready.
        bool ReadyToLoadTrace() const;

        /// @brief Load a trace into memory.
        ///
        /// Note: This function runs in a separate thread so doesn't have access to
        /// anything QT-related (including the Debug Window).
        ///
        /// @param [in] trace_file_name The name of the trace file.
        ///
        /// @return An error code returned from the loading thread.
        TraceLoadReturnCode TraceLoad(const QString& trace_file_name);

        /// @brief Clear a trace from memory.
        ///
        /// This function should effectively clean up the ActiveTraceData struct.
        void ClearTrace();

        /// @brief Get the full path to the trace file.
        ///
        /// @return The trace path.
        const QString& GetTracePath() const;

        /// @brief Set the loading finished callback.
        /// Note: please consider the safety implications of this callback executing in a different thread.
        ///
        /// @param [in] loading_finished_callback The callback to be executed after the loading has finished in the loading thread.
        void SetLoadingFinishedCallback(std::function<void()> loading_finished_callback);

        /// @brief Set the clear trace callback.
        /// Note: please consider the safety implications of this callback while the panes are still open.
        ///
        /// @param [in] clear_trace_callback The callback to be executed after the trace has been closed.
        void SetClearTraceCallback(std::function<void()> clear_trace_callback);

        /// @brief Get the default executable name (OS-aware).
        ///
        /// @return The default name string.
        QString GetDefaultExeName() const;

    public slots:
        /// @brief Load a trace.
        ///
        /// @param [in] path The path to the trace file.
        void LoadTrace(const QString& path);

    signals:
        /// @brief Signal to indicate that the trace loading thread has finished.
        ///
        /// @param [in] error_code The error code returned from the loader.
        void TraceLoadThreadFinished(TraceLoadReturnCode error_code);

        /// @brief Signal to indicate that a trace file has been removed from the
        /// recent traces list.
        void TraceFileRemoved();

        /// @brief Signal to indicate that a trace file has been loaded and opened
        /// and is ready to show in the UI.
        void TraceOpened();

        /// @brief Signal to indicate that a trace file has been closed and should be
        /// disabled in the UI.
        void TraceClosed();

    private slots:
        /// @brief Finalize the trace loading process.
        ///
        /// Destroy loading thread, evaluate thread loading error codes and inform the
        /// UI via a signal that the trace is ready to be viewed.
        ///
        /// @param [in] error_code An error code from the loading thread indicating if load was successful.
        void FinalizeTraceLoading(TraceLoadReturnCode error_code);

    private:
        /// @brief Compare a trace with one that is already open.
        ///
        /// @param [in] new_trace path to trace to compare with.
        ///
        /// @return true if both traces are the same.
        bool SameTrace(const QFileInfo& new_trace) const;

        /// @brief Show a message box dependent on whether the active_trace_path is in the recent file list.
        ///
        /// @param [in] active_trace_path
        /// @param [in] title
        /// @param [in] text
        void ShowMessageBox(const QString& active_trace_path, const QString& title, const QString& text);

        QWidget*              parent_;             ///< Pointer to the parent pane.
        QString               active_trace_path_;  ///< The path to currently opened file.
        std::function<void()> loading_finished_callback_ =
            nullptr;  ///< An arbitrary callback to be executed right after the loading has finished in the loading thread.
        std::function<void()> clear_trace_callback_ = nullptr;  ///< An arbitrary callback to be executed right after the trace has been closed.
    };
}  // namespace rra

#endif  // RRA_MANAGERS_TRACE_MANAGER_H_
