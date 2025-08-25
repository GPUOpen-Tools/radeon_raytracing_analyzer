//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the Keyboard Shortcuts pane.
//=============================================================================

#ifndef RRA_VIEWS_SETTINGS_KEYBOARD_SHORTCUTS_PANE_H_
#define RRA_VIEWS_SETTINGS_KEYBOARD_SHORTCUTS_PANE_H_

#include "ui_keyboard_shortcuts_pane.h"

#include "views/base_pane.h"

/// @brief Class declaration.
class KeyboardShortcutsPane : public BasePane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit KeyboardShortcutsPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~KeyboardShortcutsPane();

private:
    Ui::KeyboardShortcutsPane* ui_;  ///< Pointer to the Qt UI design.
};

#endif  // RRA_VIEWS_SETTINGS_KEYBOARD_SHORTCUTS_PANE_H_
