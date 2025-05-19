//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Implementation of a vertical QPushButton.
//=============================================================================

#include "views/custom_widgets/vertical_button_widget.h"

#include <QPainter>

static const float kVerticalLabelPointFontSize = 9.0F;  ///< Font point size for vertical labels.

VerticalButtonWidget::VerticalButtonWidget(QWidget* parent)
    : QPushButton(parent)
{
    QFont tmp_font = font();
    tmp_font.setPointSizeF(kVerticalLabelPointFontSize);
    setFont(tmp_font);
}

VerticalButtonWidget::~VerticalButtonWidget()
{
}

void VerticalButtonWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    QFont    tmp_font = font();
    tmp_font.setPointSizeF(kVerticalLabelPointFontSize);
    painter.setFont(tmp_font);
    painter.rotate(90);
    painter.drawText(0, -5, text());
}

QSize VerticalButtonWidget::minimumSizeHint() const
{
    QSize minimum_size_hint = QFontMetrics(font()).size(0, text());

    // Swap width and height.
    return QSize(minimum_size_hint.height(), minimum_size_hint.width());
}

QSize VerticalButtonWidget::sizeHint() const
{
    QSize size_hint = QFontMetrics(font()).size(0, text());

    // Swap width and height.
    return QSize(size_hint.height(), size_hint.width());
}

