//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for a helper class for custom button rendering for the
/// themes and colors pane.
//=============================================================================

#ifndef RRA_VIEWS_CUSTOM_WIDGETS_THEMES_AND_COLORS_ITEM_BUTTON_H_
#define RRA_VIEWS_CUSTOM_WIDGETS_THEMES_AND_COLORS_ITEM_BUTTON_H_

#include "qt_common/custom_widgets/scaled_push_button.h"

/// @brief Helper class for custom button rendering.
class ThemesAndColorsItemButton : public ScaledPushButton
{
public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit ThemesAndColorsItemButton(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~ThemesAndColorsItemButton();

    /// @brief Themes and colors button color setter.
    ///
    /// Stores color and font color values for use in custom drawing.
    ///
    /// @param [in] color The color to set.
    void SetColor(const QColor& color);

    /// @brief Themes and colors button paint event.
    ///
    /// Overrides button draw function to implement custom drawing functionality.
    ///
    /// @param [in] event Qt paint event.
    virtual void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;

private:
    QColor button_color_;  ///< Color of this button.
    QColor font_color_;    ///< Font color of this button.
};

#endif  // RRA_VIEWS_CUSTOM_WIDGETS_THEMES_AND_COLORS_ITEM_BUTTON_H_
