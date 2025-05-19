//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of Colors and Themes pane.
//=============================================================================

#include "views/settings/themes_and_colors_pane.h"

#include <QDir>
#include <QProcess>
#include <QStyleHints>

#include "qt_common/utils/qt_util.h"

#include "constants.h"
#include "managers/message_manager.h"
#include "managers/trace_manager.h"
#include "settings/settings.h"
#include "util/rra_util.h"
#include "util/string_util.h"
#include "views/debug_window.h"
#include "views/widget_util.h"

const static int kPickerRows    = 4;
const static int kPickerColumns = 8;

const static QString kLightThemeOption = "Light";
const static QString kDarkThemeOption  = "Dark";
const static QString kDetectOsOption   = "Detect OS";

ThemesAndColorsPane::ThemesAndColorsPane(QWidget* parent)
    : BasePane(parent)
    , ui_(new Ui::ThemesAndColorsPane)
{
    ui_->setupUi(this);

    rra::widget_util::ApplyStandardPaneStyle(ui_->main_scroll_area_);

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
    button_group_.addButton(ui_->button_background_light_1_, kSettingThemesAndColorsBackgroundLight1);
    button_group_.addButton(ui_->button_background_light_2_, kSettingThemesAndColorsBackgroundLight2);
    button_group_.addButton(ui_->button_background_dark_1_, kSettingThemesAndColorsBackgroundDark1);
    button_group_.addButton(ui_->button_background_dark_2_, kSettingThemesAndColorsBackgroundDark2);
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
    button_group_.addButton(ui_->button_invocation_raygen_, kSettingThemesAndColorsInvocationRaygen);
    button_group_.addButton(ui_->button_invocation_closest_hit_, kSettingThemesAndColorsInvocationClosestHit);
    button_group_.addButton(ui_->button_invocation_any_hit_, kSettingThemesAndColorsInvocationAnyHit);
    button_group_.addButton(ui_->button_invocation_intersection_, kSettingThemesAndColorsInvocationIntersection);
    button_group_.addButton(ui_->button_invocation_miss_, kSettingThemesAndColorsInvocationMiss);
    button_group_.addButton(ui_->button_selected_ray_color_, kSettingThemesAndColorsSelectedRayColor);
    button_group_.addButton(ui_->button_ray_color_, kSettingThemesAndColorsRayColor);
    button_group_.addButton(ui_->button_shadow_ray_color_, kSettingThemesAndColorsShadowRayColor);
    button_group_.addButton(ui_->button_zero_mask_ray_color_, kSettingThemesAndColorsZeroMaskRayColor);

    ui_->color_theme_combo_box_->InitSingleSelect(this, kLightThemeOption, false, "Color Theme: ");
    ui_->color_theme_combo_box_->AddItem(kLightThemeOption, kColorThemeTypeLight);
    ui_->color_theme_combo_box_->AddItem(kDarkThemeOption, kColorThemeTypeDark);
    ui_->color_theme_combo_box_->AddItem(kDetectOsOption, kColorThemeTypeCount);

    // Slot/signal connection for various widgets.
    connect(ui_->color_widget_, &ColorPickerWidget::ColorSelected, this, &ThemesAndColorsPane::PickerColorSelected);
    connect(&button_group_, &QButtonGroup::idClicked, this, &ThemesAndColorsPane::ItemButtonClicked);
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

    connect(ui_->color_theme_combo_box_, &ArrowIconComboBox::SelectedItem, this, &ThemesAndColorsPane::ColorThemeOptionSelected);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, &ThemesAndColorsPane::OsColorSchemeChanged);
#endif

    // Set up color picker.
    ui_->color_widget_->SetRowAndColumnCount(kPickerRows, kPickerColumns);
    ui_->color_widget_->SetPalette(rra::Settings::Get().GetColorPalette());

    // Initial checked item.
    ui_->button_node_box16_->setChecked(true);

    // Add margins around the color picker label.
    ui_->selected_color_label_->setContentsMargins(10, 5, 10, 5);

    ui_->color_theme_combo_box_->SetSelectedRow(rra::Settings::Get().GetColorTheme());

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
    delete ui_;
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

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
void ThemesAndColorsPane::OsColorSchemeChanged(Qt::ColorScheme color_scheme)
{
    if (rra::Settings::Get().GetColorTheme() != kColorThemeTypeCount)
    {
        return;
    }

    if (color_scheme == Qt::ColorScheme::Unknown)
    {
        return;
    }

    ColorThemeType color_mode = kColorThemeTypeLight;
    if (color_scheme == Qt::ColorScheme::Light)
    {
        color_mode = kColorThemeTypeLight;
    }
    else if (color_scheme == Qt::ColorScheme::Dark)
    {
        color_mode = kColorThemeTypeDark;
    }

    if (color_mode == QtCommon::QtUtils::ColorTheme::Get().GetColorTheme())
    {
        return;
    }

    QtCommon::QtUtils::ColorTheme::Get().SetColorTheme(color_mode);

    qApp->setPalette(QtCommon::QtUtils::ColorTheme::Get().GetCurrentPalette());

    // Load application stylesheet.
    QFile style_sheet(rra::resource::kStylesheet);
    if (style_sheet.open(QFile::ReadOnly))
    {
        QString app_stylesheet = style_sheet.readAll();

        if (color_mode == kColorThemeTypeDark)
        {
            QFile dark_style_sheet(rra::resource::kDarkStylesheet);
            if (dark_style_sheet.open(QFile::ReadOnly))
            {
                app_stylesheet.append(dark_style_sheet.readAll());
            }
        }
        else
        {
            QFile light_style_sheet(rra::resource::kLightStylesheet);
            if (light_style_sheet.open(QFile::ReadOnly))
            {
                app_stylesheet.append(light_style_sheet.readAll());
            }
        }

        qApp->setStyleSheet(app_stylesheet);
    }

    rra::Settings::Get().SaveSettings();

    emit QtCommon::QtUtils::ColorTheme::Get().ColorThemeUpdated();
}
#endif

void ThemesAndColorsPane::ColorThemeOptionSelected(QListWidgetItem* color_theme_option)
{
    ColorThemeType selected_color_mode = static_cast<ColorThemeType>(color_theme_option->data(Qt::UserRole).toInt());

    // If the setting was not changed, return early.
    if (selected_color_mode == rra::Settings::Get().GetColorTheme())
    {
        return;
    }

    ColorThemeType color_mode = selected_color_mode;

    if (selected_color_mode == kColorThemeTypeCount)
    {
        color_mode = QtCommon::QtUtils::DetectOsSetting();
    }

    // If the setting was changed, but won't result in a color theme change, apply the setting then return.
    if (color_mode == QtCommon::QtUtils::ColorTheme::Get().GetColorTheme())
    {
        rra::Settings::Get().SetColorTheme(selected_color_mode);
        return;
    }

    QString color_theme_changed_title = "Color Theme Changed. Restart Application?";
    QString color_theme_changed_text =
        "Not all UI elements will update to reflect the change in color theme until the application has restarted. Restart Application?";

    int ret = QtCommon::QtUtils::ShowMessageBox(
        this, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Question, color_theme_changed_title, color_theme_changed_text);

    if (ret == QMessageBox::Cancel)
    {
        for (int i = 0; i <= kColorThemeTypeCount; i++)
        {
            if (i == rra::Settings::Get().GetColorTheme())
            {
                ui_->color_theme_combo_box_->SetSelectedRow(i);
            }
        }
    }
    else
    {
        rra::Settings::Get().SetColorTheme(selected_color_mode);

        QtCommon::QtUtils::ColorTheme::Get().SetColorTheme(color_mode);

        if (ret == QMessageBox::Yes)
        {
            QString path        = rra::TraceManager::Get().GetTracePath();
            QString native_path = QDir::toNativeSeparators(path);

            // Fire up a new instance if desired trace is different than current.
            // Attempt to open a new instance of RRA using the selected trace file as an argument.
            const QString executable_name = qApp->applicationDirPath() + rra::TraceManager::Get().GetDefaultExeName();

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
                        const QString text = rra::text::kOpenRecentTraceStart + file.fileName() + rra::text::kOpenRecentTraceEnd;
                        QtCommon::QtUtils::ShowMessageBox(this, QMessageBox::Ok, QMessageBox::Critical, rra::text::kOpenRecentTraceTitle, text);
                    }
                }
            }
            else
            {
                // If the executable does not exist, put up a message box.
                const QString text = executable_name + " does not exist";
                QtCommon::QtUtils::ShowMessageBox(this, QMessageBox::Ok, QMessageBox::Critical, rra::text::kOpenRecentTraceTitle, text);
            }

            qApp->quit();
        }
        else if (ret == QMessageBox::No)
        {
            qApp->setPalette(QtCommon::QtUtils::ColorTheme::Get().GetCurrentPalette());

            // Load application stylesheet.
            QFile style_sheet(rra::resource::kStylesheet);
            if (style_sheet.open(QFile::ReadOnly))
            {
                QString app_stylesheet = style_sheet.readAll();

                if (color_mode == kColorThemeTypeDark)
                {
                    QFile dark_style_sheet(rra::resource::kDarkStylesheet);
                    if (dark_style_sheet.open(QFile::ReadOnly))
                    {
                        app_stylesheet.append(dark_style_sheet.readAll());
                    }
                }
                else
                {
                    QFile light_style_sheet(rra::resource::kLightStylesheet);
                    if (light_style_sheet.open(QFile::ReadOnly))
                    {
                        app_stylesheet.append(light_style_sheet.readAll());
                    }
                }

                qApp->setStyleSheet(app_stylesheet);
            }

            emit QtCommon::QtUtils::ColorTheme::Get().ColorThemeUpdated();
        }
    }
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
    case kSettingThemesAndColorsBackgroundLight1:
    case kSettingThemesAndColorsBackgroundLight2:
    case kSettingThemesAndColorsBackgroundDark1:
    case kSettingThemesAndColorsBackgroundDark2:
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
    case kSettingThemesAndColorsInvocationRaygen:
    case kSettingThemesAndColorsInvocationClosestHit:
    case kSettingThemesAndColorsInvocationAnyHit:
    case kSettingThemesAndColorsInvocationIntersection:
    case kSettingThemesAndColorsInvocationMiss:
    case kSettingThemesAndColorsSelectedRayColor:
    case kSettingThemesAndColorsRayColor:
    case kSettingThemesAndColorsShadowRayColor:
    case kSettingThemesAndColorsZeroMaskRayColor:
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
    case kSettingThemesAndColorsBackgroundLight1:
    case kSettingThemesAndColorsBackgroundLight2:
    case kSettingThemesAndColorsBackgroundDark1:
    case kSettingThemesAndColorsBackgroundDark2:
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
    case kSettingThemesAndColorsInvocationRaygen:
    case kSettingThemesAndColorsInvocationClosestHit:
    case kSettingThemesAndColorsInvocationAnyHit:
    case kSettingThemesAndColorsInvocationIntersection:
    case kSettingThemesAndColorsInvocationMiss:
    case kSettingThemesAndColorsSelectedRayColor:
    case kSettingThemesAndColorsRayColor:
    case kSettingThemesAndColorsShadowRayColor:
    case kSettingThemesAndColorsZeroMaskRayColor:
        return rra::Settings::Get().GetPaletteId(static_cast<SettingID>(button_id));

    default:
        return -1;
    }
}

