//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the debug window.
//=============================================================================

#ifndef RRA_VIEWS_DEBUG_WINDOW_H_
#define RRA_VIEWS_DEBUG_WINDOW_H_

#include "ui_debug_window.h"

#include <QDialog>

/// @brief Support for the debug window.
class DebugWindow : public QDialog
{
    Q_OBJECT

public:
    /// @brief Constructor.
    explicit DebugWindow();

    /// @brief Destructor.
    ~DebugWindow();

    /// @brief Send a message to the debug window. Supports multiple arguments.
    ///
    /// @param [in] format The string containing format for each argument.
    static void DbgMsg(const char* format, ...);

signals:
    /// @brief Signal that gets emitted when the debug window has new text to add.
    ///
    /// This will be picked up by the slot below.
    ///
    /// @param [in] string The new line of text to add.
    void EmitSetText(const QString& string);

private slots:
    /// @brief Add a new line of text to the debug window.
    ///
    /// @param [in] string The new line of text to add.
    void SetText(const QString& string);

private:
    /// @brief Helper function which to automatically scroll to the bottom on new line.
    void ScrollToBottom();

    /// @brief Register the Debug Window such that it is accessible.
    ///
    /// This is only to be called once, when initializing MainWindow.
    void RegisterDbgWindow();

    Ui::DebugWindow* ui_;  ///< Pointer to the Qt UI design.
};

#endif  // RRA_VIEWS_DEBUG_WINDOW_H_
