//=============================================================================
// Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the dispatch loading bar.
//=============================================================================

#include "dispatch_loading_bar.h"

#include <QPainter>

#include "qt_common/utils/qt_util.h"

DispatchLoadingBar::DispatchLoadingBar(QWidget* parent)
    : QWidget(parent)
    , fill_percentage_(0)
{
}

DispatchLoadingBar::~DispatchLoadingBar()
{
}

void DispatchLoadingBar::SetFillPercentage(qreal percentage)
{
    fill_percentage_ = std::max(0.0, std::min(100.0, percentage));
    emit FillPercentageChanged(percentage);
}

void DispatchLoadingBar::SetText(QString loader_text)
{
    loader_text_ = loader_text;
}

void DispatchLoadingBar::MarkError()
{
    in_error_state_ = true;
}

QSize DispatchLoadingBar::sizeHint() const
{
    return QSize(kDefaultWidth_, kDefaultHeight_);
}

void DispatchLoadingBar::paintEvent(QPaintEvent* paint_event)
{
    Q_UNUSED(paint_event);

    QPainter painter(this);

    QRect rect = this->rect();

    QBrush brush(kEmptyColor_[QtCommon::QtUtils::ColorTheme::Get().GetColorTheme()]);

    QPen pen(Qt::NoPen);
    painter.setPen(pen);
    painter.fillRect(rect, brush);
    painter.drawRect(rect);

    float percentage = fill_percentage_;
    auto  color      = kFillColor_;
    auto  text       = loader_text_;

    if (in_error_state_)
    {
        percentage = 100.0f;
        color      = kErrorColor_;
        text       = "Error!";
    }

    rect.setRight(rect.right() * (percentage / 100.0f));
    brush.setColor(color);

    painter.setBrush(brush);
    painter.fillRect(rect, brush);
    painter.drawRect(rect);

    rect = this->rect();
    painter.setPen(palette().windowText().color());
    painter.drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter, text);
}
