//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the Colors and Themes pane.
//=============================================================================

#ifndef RRA_VIEWS_SETTINGS_THEMES_AND_COLORS_PANE_H_
#define RRA_VIEWS_SETTINGS_THEMES_AND_COLORS_PANE_H_

#include <QButtonGroup>

#include "ui_themes_and_colors_pane.h"

#include "qt_common/custom_widgets/scaled_push_button.h"

#include "views/base_pane.h"
#include "views/custom_widgets/themes_and_colors_item_button.h"

/// @brief Class declaration.
class ThemesAndColorsPane : public BasePane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit ThemesAndColorsPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~ThemesAndColorsPane();

private:
    /// @brief Updates all pane elements so they reflect the settings currently set in the Settings.
    void Refresh();

    /// @brief Get the settings color which corresponds to the button indicated by button_id.
    ///
    /// @param [in] button_id The button id.
    ///
    /// @return The color setting.
    QColor GetSettingsColor(int button_id) const;

    /// @brief Get the settings palette id which corresponds to the button indicated by button_id.
    ///
    /// @param [in] button_id The button id.
    ///
    /// @return The palette id setting.
    int GetSettingsPaletteId(int button_id) const;

    /// @brief Set the settings palette id which corresponds to the button indicated by button_id.
    ///
    /// @param [in] button_id  The button id.
    /// @param [in] palette_id The palette id to assign in the settings.
    void SetSettingsPaletteId(int button_id, int palette_id);

signals:
    /// @brief Signal to indicate that the colors have been refreshed.
    void RefreshedColors();

private slots:
    /// @brief Picker color selected slot.
    ///
    /// Called when a color is selected on the picker (also triggered by calling ColorPickerWidget::select()).
    ///
    /// @param [in] palette_id  The palette of the selected color.
    /// @param [in] theme_color The color value of the selected color.
    void PickerColorSelected(int palette_id, const QColor& theme_color);

    /// @brief Item button clicked slot.
    ///
    /// Called when one of the item buttons is clicked.
    ///
    /// @param [in] button_id The id of the selected button.
    void ItemButtonClicked(int button_id);

    /// @brief Handle Color theme changed in the settings.
    ///
    /// color_theme_option Color theme option that was selected.
    void ColorThemeOptionSelected(QListWidgetItem* color_theme_option);

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    /// @brief Handle Color scheme changed in the OS.
    ///
    /// color_scheme The color scheme selected by the OS.
    void OsColorSchemeChanged(Qt::ColorScheme color_scheme);
#endif

    /// @brief Default settings button clicked slot.
    ///
    /// Called when the default settings button is clicked.
    void DefaultSettingsButtonClicked();

    /// @brief Default palette button clicked slot.
    ///
    /// Called when the default palette button is clicked.
    void DefaultPaletteButtonClicked();

    /// @brief RGB value changed slot.
    ///
    /// Called when the on screen RGB values are changed, either by using the sliders or the spinboxes.
    void RgbValuesChanged();

private:
    Ui::ThemesAndColorsPane* ui_;            ///< Pointer to the Qt UI design.
    QButtonGroup             button_group_;  ///< Group for item buttons.
};

#endif  // RRA_VIEWS_SETTINGS_THEMES_AND_COLORS_PANE_H_
