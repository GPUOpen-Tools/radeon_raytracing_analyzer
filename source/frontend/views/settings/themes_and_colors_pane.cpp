//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of Colors and Themes pane.
//=============================================================================

#include "views/settings/themes_and_colors_pane.h"

#include "settings/settings.h"
#include "util/rra_util.h"
#include "views/widget_util.h"
#include "util/string_util.h"
#include "views/debug_window.h"

const static int kPickerRows    = 4;
const static int kPickerColumns = 8;

ThemesAndColorsPane::ThemesAndColorsPane(QWidget* parent)
    : BasePane(parent)
    , ui_(new Ui::ThemesAndColorsPane)
{
    ui_->setupUi(this);

    rra::widget_util::ApplyStandardPaneStyle(this, ui_->main_content_, ui_->main_scroll_area_);

    // Set up buttons using SettingID's as buttongroup id's.
    button_group_.addButton(ui_->button_node_box16_, kSettingThemesAndColorsBoundingVolumeBox16);
    button_group_.addButton(ui_->button_node_box32_, kSettingThemesAndColorsBoundingVolumeBox32);
    button_group_.addButton(ui_->button_node_instance_, kSettingThemesAndColorsBoundingVolumeInstance);
    button_group_.addButton(ui_->button_node_triangle_, kSettingThemesAndColorsBoundingVolumeTriangle);
    button_group_.addButton(ui_->button_node_procedural_, kSettingThemesAndColorsBoundingVolumeProcedural);
    button_group_.addButton(ui_->button_node_selected_, kSettingThemesAndColorsBoundingVolumeSelected);
    button_group_.addButton(ui_->button_wireframe_normal_, kSettingThemesAndColorsWireframeNormal);
    button_group_.addButton(ui_->button_wireframe_selected_, kSettingThemesAndColorsWireframeSelected);
    button_group_.addButton(ui_->button_geometry_selected_, kSettingThemesAndColorsGeometrySelected);
    button_group_.addButton(ui_->button_background_1_, kSettingThemesAndColorsBackground1);
    button_group_.addButton(ui_->button_background_2_, kSettingThemesAndColorsBackground2);
    button_group_.addButton(ui_->button_non_opaque_, kSettingThemesAndColorsNonOpaque);
    button_group_.addButton(ui_->button_opaque_, kSettingThemesAndColorsOpaque);
    button_group_.addButton(ui_->button_positive_, kSettingThemesAndColorsPositive);
    button_group_.addButton(ui_->button_negative_, kSettingThemesAndColorsNegative);
    button_group_.addButton(ui_->button_build_algorithm_none_, kSettingThemesAndColorsBuildAlgorithmNone);
    button_group_.addButton(ui_->button_build_algorithm_fast_build_, kSettingThemesAndColorsBuildAlgorithmFastBuild);
    button_group_.addButton(ui_->button_build_algorithm_fast_trace_, kSettingThemesAndColorsBuildAlgorithmFastTrace);
    button_group_.addButton(ui_->button_build_algorithm_both_, kSettingThemesAndColorsBuildAlgorithmBoth);
    button_group_.addButton(ui_->button_instance_opaque_none_, kSettingThemesAndColorsInstanceOpaqueNone);
    button_group_.addButton(ui_->button_instance_opaque_force_opaque_, kSettingThemesAndColorsInstanceOpaqueForceOpaque);
    button_group_.addButton(ui_->button_instance_opaque_force_no_opaque_, kSettingThemesAndColorsInstanceOpaqueForceNoOpaque);
    button_group_.addButton(ui_->button_instance_opaque_both_, kSettingThemesAndColorsInstanceOpaqueBoth);

    // Slot/signal connection for various widgets.
    connect(ui_->color_widget_, &ColorPickerWidget::ColorSelected, this, &ThemesAndColorsPane::PickerColorSelected);
    connect(&button_group_, SIGNAL(buttonClicked(int)), this, SLOT(ItemButtonClicked(int)));
    connect(ui_->default_settings_button_, SIGNAL(clicked(bool)), this, SLOT(DefaultSettingsButtonClicked()));
    connect(ui_->default_palette_button_, SIGNAL(clicked(bool)), this, SLOT(DefaultPaletteButtonClicked()));
    connect(ui_->spin_box_color_red_, SIGNAL(valueChanged(int)), this, SLOT(RgbValuesChanged()));
    connect(ui_->spin_box_color_green_, SIGNAL(valueChanged(int)), this, SLOT(RgbValuesChanged()));
    connect(ui_->spin_box_color_blue_, SIGNAL(valueChanged(int)), this, SLOT(RgbValuesChanged()));
    connect(ui_->spin_box_color_red_, SIGNAL(valueChanged(int)), ui_->slider_color_red_, SLOT(setValue(int)));
    connect(ui_->spin_box_color_green_, SIGNAL(valueChanged(int)), ui_->slider_color_green_, SLOT(setValue(int)));
    connect(ui_->spin_box_color_blue_, SIGNAL(valueChanged(int)), ui_->slider_color_blue_, SLOT(setValue(int)));
    connect(ui_->slider_color_red_, SIGNAL(valueChanged(int)), ui_->spin_box_color_red_, SLOT(setValue(int)));
    connect(ui_->slider_color_green_, SIGNAL(valueChanged(int)), ui_->spin_box_color_green_, SLOT(setValue(int)));
    connect(ui_->slider_color_blue_, SIGNAL(valueChanged(int)), ui_->spin_box_color_blue_, SLOT(setValue(int)));

    // Set up color picker.
    ui_->color_widget_->SetRowAndColumnCount(kPickerRows, kPickerColumns);
    ui_->color_widget_->SetPalette(rra::Settings::Get().GetColorPalette());

    // Initial checked item.
    ui_->button_node_box16_->setChecked(true);

    // Add margins around the color picker label.
    ui_->selected_color_label_->setContentsMargins(10, 5, 10, 5);

    // Initial refresh.
    Refresh();

    // Safety measure to guarantee settings values are in range (should prevent crashing
    // from an invalid settings file).
    for (QAbstractButton* button : button_group_.buttons())
    {
        int button_id  = button_group_.id(button);
        int palette_id = GetSettingsPaletteId(button_id);

        if (palette_id < 0 || palette_id >= (kPickerRows * kPickerColumns))
        {
            SetSettingsPaletteId(button_id, 0);
        }
        else
        {
            // Invalid settings strings which still produce an integer value in range.
            // Should be overwritten with that integer value.
            SetSettingsPaletteId(button_id, palette_id);
        }
    }

    // Set the cursor to pointing hand cursor for the sliders.
    ui_->slider_color_red_->setCursor(Qt::PointingHandCursor);
    ui_->slider_color_green_->setCursor(Qt::PointingHandCursor);
    ui_->slider_color_blue_->setCursor(Qt::PointingHandCursor);
}

ThemesAndColorsPane::~ThemesAndColorsPane()
{
}

void ThemesAndColorsPane::PickerColorSelected(int palette_id, const QColor& color)
{
    Q_UNUSED(color)

    int button_id = button_group_.checkedId();

    // Set palette id in the settings.
    SetSettingsPaletteId(button_id, palette_id);

    Refresh();
}

void ThemesAndColorsPane::ItemButtonClicked(int button_id)
{
    Q_UNUSED(button_id)

    Refresh();
}

void ThemesAndColorsPane::DefaultSettingsButtonClicked()
{
    // Restore default palette ids.
    rra::Settings::Get().RestoreDefaultColors();

    Refresh();
}

void ThemesAndColorsPane::DefaultPaletteButtonClicked()
{
    // Restore default palette settings.
    rra::Settings::Get().RestoreDefaultPalette();

    Refresh();
}

void ThemesAndColorsPane::RgbValuesChanged()
{
    ColorPalette palette    = ui_->color_widget_->GetPalette();
    int          palette_id = ui_->color_widget_->GetSelectedPaletteId();

    // Get color from spinbox values.
    QColor color(ui_->spin_box_color_red_->value(), ui_->spin_box_color_green_->value(), ui_->spin_box_color_blue_->value());

    // Set the new color in the palette.
    palette.SetColor(palette_id, color);
    rra::Settings::Get().SetColorPalette(palette);

    Refresh();
}

void ThemesAndColorsPane::Refresh()
{
    QColor color;

    // Set button color values from corresponding settings.
    for (QAbstractButton* button : button_group_.buttons())
    {
        int button_id = button_group_.id(button);

        if (button->isChecked())
        {
            // Select the picker color that matches this buttons color (default to first color).
            int palette_id = GetSettingsPaletteId(button_id);
            ui_->color_widget_->Select(palette_id);
        }

        // Get colors.
        color = GetSettingsColor(button_id);

        static_cast<ThemesAndColorsItemButton*>(button)->SetColor(color);
    }

    // Set color picker palette.
    ui_->color_widget_->SetPalette(rra::Settings::Get().GetColorPalette());

    // Set RGB spinbox/slider values.
    color = ui_->color_widget_->GetSelectedColor();
    ui_->spin_box_color_red_->setValue(color.red());
    ui_->spin_box_color_green_->setValue(color.green());
    ui_->spin_box_color_blue_->setValue(color.blue());

    // Set selected color hex label.
    QString color_string      = rra::string_util::ToUpperCase(QString("#") + QString::number(color.rgb(), 16));
    QString font_color_string = QString("#") + QString::number(rra_util::GetTextColorForBackground(color_string).rgb(), 16);
    ui_->selected_color_label_->setText("");
    ui_->selected_color_label_->setStyleSheet(QString("background-color:%1;color:%2;").arg(color_string).arg(font_color_string));

    // Indicate the colors may have changed.
    emit RefreshedColors();
}

QColor ThemesAndColorsPane::GetSettingsColor(int button_id) const
{
    return rra::Settings::Get().GetColorPalette().GetColor(GetSettingsPaletteId(button_id));
}

void ThemesAndColorsPane::SetSettingsPaletteId(int button_id, int palette_id)
{
    switch (button_id)
    {
    case kSettingThemesAndColorsBoundingVolumeBox16:
    case kSettingThemesAndColorsBoundingVolumeBox32:
    case kSettingThemesAndColorsBoundingVolumeInstance:
    case kSettingThemesAndColorsBoundingVolumeTriangle:
    case kSettingThemesAndColorsBoundingVolumeProcedural:
    case kSettingThemesAndColorsBoundingVolumeSelected:
    case kSettingThemesAndColorsWireframeNormal:
    case kSettingThemesAndColorsWireframeSelected:
    case kSettingThemesAndColorsGeometrySelected:
    case kSettingThemesAndColorsBackground1:
    case kSettingThemesAndColorsBackground2:
    case kSettingThemesAndColorsNonOpaque:
    case kSettingThemesAndColorsOpaque:
    case kSettingThemesAndColorsPositive:
    case kSettingThemesAndColorsNegative:
    case kSettingThemesAndColorsBuildAlgorithmNone:
    case kSettingThemesAndColorsBuildAlgorithmFastBuild:
    case kSettingThemesAndColorsBuildAlgorithmFastTrace:
    case kSettingThemesAndColorsBuildAlgorithmBoth:
    case kSettingThemesAndColorsInstanceOpaqueNone:
    case kSettingThemesAndColorsInstanceOpaqueForceOpaque:
    case kSettingThemesAndColorsInstanceOpaqueForceNoOpaque:
    case kSettingThemesAndColorsInstanceOpaqueBoth:
        rra::Settings::Get().SetPaletteId(static_cast<SettingID>(button_id), palette_id);
        break;

    default:
        DebugWindow::DbgMsg("Warning: Hit unused default switch case.");
        break;
    }
}

int ThemesAndColorsPane::GetSettingsPaletteId(int button_id) const
{
    switch (button_id)
    {
    case kSettingThemesAndColorsBoundingVolumeBox16:
    case kSettingThemesAndColorsBoundingVolumeBox32:
    case kSettingThemesAndColorsBoundingVolumeInstance:
    case kSettingThemesAndColorsBoundingVolumeTriangle:
    case kSettingThemesAndColorsBoundingVolumeProcedural:
    case kSettingThemesAndColorsBoundingVolumeSelected:
    case kSettingThemesAndColorsWireframeNormal:
    case kSettingThemesAndColorsWireframeSelected:
    case kSettingThemesAndColorsGeometrySelected:
    case kSettingThemesAndColorsBackground1:
    case kSettingThemesAndColorsBackground2:
    case kSettingThemesAndColorsNonOpaque:
    case kSettingThemesAndColorsOpaque:
    case kSettingThemesAndColorsPositive:
    case kSettingThemesAndColorsNegative:
    case kSettingThemesAndColorsBuildAlgorithmNone:
    case kSettingThemesAndColorsBuildAlgorithmFastBuild:
    case kSettingThemesAndColorsBuildAlgorithmFastTrace:
    case kSettingThemesAndColorsBuildAlgorithmBoth:
    case kSettingThemesAndColorsInstanceOpaqueNone:
    case kSettingThemesAndColorsInstanceOpaqueForceOpaque:
    case kSettingThemesAndColorsInstanceOpaqueForceNoOpaque:
    case kSettingThemesAndColorsInstanceOpaqueBoth:
        return rra::Settings::Get().GetPaletteId(static_cast<SettingID>(button_id));

    default:
        return -1;
    }
}
