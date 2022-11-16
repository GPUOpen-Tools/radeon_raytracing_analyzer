//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the Settings pane.
//=============================================================================

#ifndef RRA_VIEWS_SETTINGS_SETTINGS_PANE_H_
#define RRA_VIEWS_SETTINGS_SETTINGS_PANE_H_

#include "ui_settings_pane.h"

#include "views/base_pane.h"

/// @brief Class declaration.
class SettingsPane : public BasePane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit SettingsPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~SettingsPane();

    /// @brief Overridden show event. Fired when this pane is opened.
    ///
    /// @param [in] event The show event object.
    virtual void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;

    /// @brief Update treeview Node ID.
    void SwitchTreeviewNodeId();

private slots:
    /// @brief Slot to handle what happens when the auto updates box changes.
    ///
    /// Update and save the settings.
    void CheckForUpdatesOnStartupStateChanged();

    /// @brief Slot to handle what happens when camera reset box changes.
    ///
    /// Update and save the settings.
    void CameraResetOnStyleChangeStateChanged();

    /// @brief Slot to handle what happens when the Treeview Node ID combo box changes.
    ///
    /// Update and save the settings.
    void TreeviewNodeIdChanged();

    /// @brief Slot to handle what happens when the traversal counter maximum is changed.
    ///
    /// @param new_max The new traversal slider maximum.
    void TraversalCounterMaximumChanged(int new_max);

    /// @brief Slot to handle what happens when camera movement speed limit is changed.
    ///
    /// @param new_limit The new slider speed limit of the camera.
    void MovementSpeedLimitChanged(int new_limit);

    /// @brief Slot to handle what happens when small object culling is changed.
    ///
    /// @param value The value set for the slider.
    void SmallObjectCullingIsChanged(int value);

    /// @brief Slot to handle what happens when decimal precision is changed.
    ///
    /// @param new_precision The new decimal precision.
    void DecimalPrecisionChanged(int new_precision);

private:
    /// @brief Update the Treeview node ID combo box.
    ///
    /// @param [in] index The combo box index.
    void UpdateTreeviewComboBox(int index);

    Ui::SettingsPane* ui_;  ///< Pointer to the Qt UI design.
};

#endif  // RRA_VIEWS_SETTINGS_SETTINGS_PANE_H_
