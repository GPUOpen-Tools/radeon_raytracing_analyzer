//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the Recent traces pane.
//=============================================================================

#ifndef RRA_VIEWS_START_RECENT_TRACES_PANE_H_
#define RRA_VIEWS_START_RECENT_TRACES_PANE_H_

#include "ui_recent_traces_pane.h"

#include <QVector>
#include <QWidget>
#include <QLabel>

#include "qt_common/custom_widgets/recent_trace_widget.h"

#include "views/base_pane.h"

/// @brief Class declaration.
class RecentTracesPane : public BasePane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit RecentTracesPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~RecentTracesPane();

signals:
    /// @brief A file in the recent file list was deleted.
    void RecentFileDeleted();

private slots:
    /// @brief Set up the list of recent traces in the UI.
    void SetupFileList();

    /// @brief Slot to delete a trace from the Recent traces list.
    ///
    /// Only removes it from the list; doesn't actually delete the file.
    ///
    /// @param [in] path The path to the trace file.
    void DeleteTrace(const QString& path);

    /// Slot to display an error message when a trace is not found.
    ///
    /// @param [in] path The path of the trace that wasn't found.
    void HandleTraceNotFoundError(const QString path);

private:
    Ui::RecentTracesPane* ui_;  ///< Pointer to the Qt UI design.

    QVector<RecentTraceWidget*> trace_widgets_;                ///< Array of trace widgets.
    QVBoxLayout*                vbox_layout_;                  ///< The vertical layout to handle custom widgets.
    QWidget*                    scroll_area_widget_contents_;  ///< The scroll area widget contents widget.
    QLabel*                     no_traces_label_;              ///< The no traces label.
};

#endif  // RRA_VIEWS_START_RECENT_TRACES_PANE_H_
