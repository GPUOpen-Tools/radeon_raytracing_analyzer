//=============================================================================
/// Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief Widget for a colored legend.
//=============================================================================

#include "dispatch_legend.h"

#include "qt_common/utils/common_definitions.h"
#include "qt_common/utils/qt_util.h"

const float kLegendHeightRatio = 2.0f / 3.0f;

DispatchLegend::DispatchLegend(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
}

DispatchLegend::~DispatchLegend()
{
}

void DispatchLegend::SetColor(const QColor& color)
{
    color_ = color;
}

QSize DispatchLegend::sizeHint() const
{
    int w = (int)(height() * kLegendHeightRatio);
    return QSize(w, w);
}

void DispatchLegend::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    // Set up the painter.
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = height() * kLegendHeightRatio;
    int x = w / 2;
    int y = (height() - w + 1) / 2; // Add 1 to round up instead of down.

    painter.fillRect(x, y, w, w, color_);
    painter.end();
}
