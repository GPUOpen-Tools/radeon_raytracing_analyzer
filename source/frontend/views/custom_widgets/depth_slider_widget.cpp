//=============================================================================
/// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Implementation of a depth slider widget.
//=============================================================================

#include "depth_slider_widget.h"

#include <QPainter>
#include <QMouseEvent>

#include "qt_common/utils/scaling_manager.h"

static const float kSegmentBorder = 2.0f;  ///< The border around each segment, in pixels.

DepthSliderWidget::DepthSliderWidget(QWidget* parent)
    : QWidget(parent)
    , clicked_value_(0)
    , lower_value_(0)
    , upper_value_(0)
    , upper_bound_(0)
{
}

DepthSliderWidget::~DepthSliderWidget()
{
}

QSize DepthSliderWidget::sizeHint() const
{
    return ScalingManager::Get().Scaled(QWidget::sizeHint());
}

void DepthSliderWidget::SetLowerValue(int lower_value)
{
    lower_value_ = lower_value;
    emit SpanChanged(lower_value, upper_value_);
}

void DepthSliderWidget::SetUpperValue(int upper_value)
{
    upper_value_ = upper_value;
    emit SpanChanged(lower_value_, upper_value);
}

void DepthSliderWidget::SetUpperBound(int upper_bound)
{
    upper_bound_ = upper_bound;
}

float DepthSliderWidget::GetSegmentOffset(QMouseEvent* event)
{
    int x_pos = event->pos().x();

    // Upper bound is inclusive.
    float num_segments  = upper_bound_ + 1.0f;
    float segment_width = width() / num_segments;

    // check bounds in case user scrolls outside of the widget.
    float segment_offset = x_pos / segment_width;
    if (segment_offset < 0.0f)
    {
        segment_offset = 0.0f;
    }
    else if (segment_offset > upper_bound_)
    {
        segment_offset = upper_bound_;
    }
    return segment_offset;
}

void DepthSliderWidget::mousePressEvent(QMouseEvent* event)
{
    float segment_index = GetSegmentOffset(event);

    clicked_value_ = static_cast<int>(segment_index);

    // Mouse is pressed for the first time, so set upper and lower values to the same segment.
    upper_value_ = clicked_value_;
    lower_value_ = clicked_value_;

    repaint();
    emit SpanChanged(lower_value_, upper_value_);
}

void DepthSliderWidget::mouseMoveEvent(QMouseEvent* event)
{
    float segment_index = GetSegmentOffset(event);

    // Calculate the upper and lower values based on where the mouse was first clicked
    // and where the mouse is now.
    if (segment_index < clicked_value_)
    {
        lower_value_ = static_cast<int>(segment_index);
        upper_value_ = clicked_value_;
    }
    else
    {
        lower_value_ = clicked_value_;
        upper_value_ = static_cast<int>(segment_index);
    }

    repaint();
    emit SpanChanged(lower_value_, upper_value_);
}

void DepthSliderWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw widget background.
    painter.setBrush(Qt::black);
    painter.setPen(Qt::NoPen);

    float widget_width  = width();
    float widget_height = height();

    painter.drawRect(0, 0, widget_width, widget_height);

    // Draw the required number of segments.
    // Upper bound is inclusive.
    float num_segments  = upper_bound_ + 1.0f;
    float segment_width = widget_width / num_segments;

    // Paint the segments. Light gray for unselected, white for selected.
    float x_offset            = kSegmentBorder;
    float segment_rect_width  = segment_width - (2.0 * kSegmentBorder);
    float segment_rect_height = widget_height - (kSegmentBorder * 2.0f);
    for (auto segment = 0; segment < num_segments; segment++)
    {
        if (segment >= lower_value_ && segment <= upper_value_)
        {
            painter.setBrush(Qt::lightGray);
        }
        else
        {
            painter.setBrush(Qt::darkGray);
        }
        painter.drawRect(x_offset, kSegmentBorder, segment_rect_width, segment_rect_height);
        x_offset += segment_width;
    }
}
