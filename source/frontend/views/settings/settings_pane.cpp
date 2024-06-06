//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of Settings pane.
//=============================================================================

#include "views/settings/settings_pane.h"

#include "managers/message_manager.h"
#include "settings/settings.h"
#include "views/custom_widgets/colored_checkbox.h"
#include "views/widget_util.h"
#include "views/custom_widgets/slider_style.h"

constexpr float kMaxCullRatio{0.01f};  ///< The maximum cull ratio able to be set by the settings slider.

float SliderToFrustumCullRatio(int slider_value)
{
    return kMaxCullRatio * (slider_value / 100.0f);
}

int FrustumCullRatioToSlider(float ratio)
{
    return ratio * (100.0f / kMaxCullRatio);
}

SettingsPane::SettingsPane(QWidget* parent)
    : BasePane(parent)
    , ui_(new Ui::SettingsPane)
{
    ui_->setupUi(this);

    rra::widget_util::ApplyStandardPaneStyle(this, ui_->main_content_, ui_->main_scroll_area_);

    ui_->check_for_updates_on_startup_checkbox_->Initialize(rra::Settings::Get().GetCheckForUpdatesOnStartup(), rra::kCheckboxEnableColor);
    connect(ui_->check_for_updates_on_startup_checkbox_, &ColoredCheckbox::Clicked, this, &SettingsPane::CheckForUpdatesOnStartupStateChanged);

    ui_->reset_camera_on_style_change_checkbox_->Initialize(rra::Settings::Get().GetCameraResetOnStyleChange(), rra::kCheckboxEnableColor);
    connect(ui_->reset_camera_on_style_change_checkbox_, &ColoredCheckbox::Clicked, this, &SettingsPane::CameraResetOnStyleChangeStateChanged);

    // Populate the treeview combo box.
    rra::widget_util::InitSingleSelectComboBox(parent, ui_->treeview_combo_push_button_, rra::text::kSettingsTreeviewOffset, false);
    ui_->treeview_combo_push_button_->ClearItems();
    ui_->treeview_combo_push_button_->AddItem(rra::text::kSettingsTreeviewOffset);
    ui_->treeview_combo_push_button_->AddItem(rra::text::kSettingsTreeviewAddress);
    ui_->treeview_combo_push_button_->SetSelectedRow(0);
    connect(ui_->treeview_combo_push_button_, &ArrowIconComboBox::SelectionChanged, this, &SettingsPane::TreeviewNodeIdChanged);

    ui_->content_max_traversal_slider_value_->setMinimum(10);
    ui_->content_max_traversal_slider_value_->setMaximum(100000);
    ui_->content_max_traversal_slider_value_->setValue(rra::Settings::Get().GetTraversalCounterMaximum());
    connect(ui_->content_max_traversal_slider_value_, SIGNAL(valueChanged(int)), this, SLOT(TraversalCounterMaximumChanged(int)));

    ui_->content_max_movement_speed_->setMinimum(10);
    ui_->content_max_movement_speed_->setMaximum(2000000000);
    ui_->content_max_movement_speed_->setValue(rra::Settings::Get().GetMovementSpeedLimit());
    connect(ui_->content_max_movement_speed_, SIGNAL(valueChanged(int)), this, SLOT(MovementSpeedLimitChanged(int)));

    ui_->small_object_culling_content_->setValue(FrustumCullRatioToSlider(rra::Settings::Get().GetFrustumCullRatio()));
    connect(ui_->small_object_culling_content_, SIGNAL(valueChanged(int)), this, SLOT(SmallObjectCullingIsChanged(int)));

    ui_->content_decimal_precision_->setMinimum(1);
    ui_->content_decimal_precision_->setMaximum(9);
    ui_->content_decimal_precision_->setValue(rra::Settings::Get().GetDecimalPrecision());
    connect(ui_->content_decimal_precision_, SIGNAL(valueChanged(int)), this, SLOT(DecimalPrecisionChanged(int)));

    ui_->small_object_culling_content_->setStyle(new AbsoluteSliderPositionStyle(ui_->small_object_culling_content_->style()));

    ui_->viewer_state_checkbox_->Initialize(rra::Settings::Get().GetPersistentUIState(), rra::kCheckboxEnableColor);
    connect(ui_->viewer_state_checkbox_, &ColoredCheckbox::Clicked, this, &SettingsPane::PersistentUIStateChanged);
}

SettingsPane::~SettingsPane()
{
}

void SettingsPane::TraversalCounterMaximumChanged(int new_max)
{
    rra::Settings::Get().SetTraversalCounterMaximum(new_max);
    rra::Settings::Get().SaveSettings();
}

void SettingsPane::MovementSpeedLimitChanged(int new_limit)
{
    rra::Settings::Get().SetMovementSpeedLimit(new_limit);
    rra::Settings::Get().SaveSettings();
}

void SettingsPane::SmallObjectCullingIsChanged(int value)
{
    rra::Settings::Get().SetFrustumCullRatio(SliderToFrustumCullRatio(value));
    rra::Settings::Get().SaveSettings();
}

void SettingsPane::DecimalPrecisionChanged(int new_precision)
{
    rra::Settings::Get().SetDecimalPrecision(new_precision);
    rra::Settings::Get().SaveSettings();
}

void SettingsPane::showEvent(QShowEvent* event)
{
    // Update the combo box push button text.
    int node_id_type = rra::Settings::Get().GetTreeviewNodeIdType();
    UpdateTreeviewComboBox(node_id_type);

    QWidget::showEvent(event);
}

void SettingsPane::CheckForUpdatesOnStartupStateChanged()
{
    rra::Settings::Get().SetCheckForUpdatesOnStartup(ui_->check_for_updates_on_startup_checkbox_->isChecked());
    rra::Settings::Get().SaveSettings();
}

void SettingsPane::CameraResetOnStyleChangeStateChanged()
{
    rra::Settings::Get().SetCameraResetOnStyleChange(ui_->reset_camera_on_style_change_checkbox_->isChecked());
    rra::Settings::Get().SaveSettings();
}

void SettingsPane::PersistentUIStateChanged()
{
    rra::Settings::Get().SetPersistentUIState(ui_->viewer_state_checkbox_->isChecked());
    rra::Settings::Get().SaveSettings();
}

void SettingsPane::UpdateTreeviewComboBox(int index)
{
    ui_->treeview_combo_push_button_->SetSelectedRow(index);
}

void SettingsPane::TreeviewNodeIdChanged()
{
    int index = ui_->treeview_combo_push_button_->CurrentRow();
    rra::Settings::Get().SetTreeviewNodeIdType(static_cast<TreeviewNodeIDType>(index));
    rra::Settings::Get().SaveSettings();
}

void SettingsPane::SwitchTreeviewNodeId()
{
    int node_id_type = rra::Settings::Get().GetTreeviewNodeIdType();
    UpdateTreeviewComboBox(node_id_type);
}
