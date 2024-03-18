//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for a helper class for custom button rendering for
/// the themes and colors pane.
//=============================================================================

#include <QPen>
#include <QPainter>

#include "qt_common/utils/scaling_manager.h"

#include "util/rra_util.h"
#include "views/custom_widgets/themes_and_colors_item_button.h"

ThemesAndColorsItemButton::ThemesAndColorsItemButton(QWidget* parent)
    : ScaledPushButton(parent)
{
    setContentsMargins(10, 5, 10, 5);
}

ThemesAndColorsItemButton::~ThemesAndColorsItemButton()
{
}

void ThemesAndColorsItemButton::SetColor(const QColor& color)
{
    button_color_ = color;
    font_color_   = rra_util::GetTextColorForBackground(color);
    update();
}

void ThemesAndColorsItemButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    ScalingManager& sm = ScalingManager::Get();
    QPainter        painter(this);

    const int position_adjust = sm.Scaled(1);
    const int size_adjust     = position_adjust * 2;
    const int outline_width   = sm.Scaled(2);

    // Rectangles used for drawing button and its border.
    QRect r1(position_adjust, position_adjust, this->size().width() - size_adjust, this->size().height() - size_adjust);
    QRect r2(r1.left() + outline_width, r1.top() + outline_width, r1.width() - (outline_width * 2), r1.height() - (outline_width * 2));
    QRect r3(r2.left(), r2.top(), r2.width() - 1, r2.height() - 1);

    if (this->isChecked() || this->underMouse())
    {
        // Fill rect with black to form border.
        painter.fillRect(r1, QBrush(Qt::black));
        painter.fillRect(r2, QBrush(button_color_));
        painter.setPen(QPen(Qt::white, 1));
        painter.drawRect(r3);
    }
    else
    {
        // Fill rect with black to form border.
        painter.fillRect(r1, button_color_);
    }

    // Draw text.
    painter.setPen(QPen(font_color_, 1));
    painter.drawText(r1, Qt::AlignHCenter | Qt::AlignVCenter, this->text());
}
