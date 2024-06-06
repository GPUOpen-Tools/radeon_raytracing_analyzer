//=============================================================================
/// Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Implementation of a double slider with heatmap widget.
//=============================================================================

#include "double_slider_heatmap_widget.h"

#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QStyleOptionSlider>
#include <QStylePainter>

#include "qt_common/utils/common_definitions.h"

/// Stylesheet for the double slider widget.
static const QString kCustomSliderStylesheet(
    "QSlider::groove:horizontal {"
    "height: 2px;"
    "background-color : #AFAFAF;"
    "margin: 2px 0;"
    "}"

    "QSlider::handle:horizontal {"
    "background-color : #0074D6;"
    "width: 6px;"
    "height:15px;"
    "margin: -15px 0;"
    "}"

    "QSlider::handle:horizontal:hover {"
    "background-color : black;"
    "width: 6px;"
    "height:15px;"
    "margin: -15px 0"
    "}"

    "QSlider::handle:disabled {"
    "background-color : grey;"
    "width: 6px;"
    "height:15px;"
    "margin: -15px 0"
    "}");

DoubleSliderHeatmapWidget::DoubleSliderHeatmapWidget(QWidget* parent)
    : QSlider(parent)
{
    Init();
}

DoubleSliderHeatmapWidget::DoubleSliderHeatmapWidget(Qt::Orientation orientation, QWidget* parent)
    : QSlider(orientation, parent)
{
    Init();
}

void DoubleSliderHeatmapWidget::Init()
{
    lower_value_           = 0;
    upper_value_           = 0;
    lower_pos_             = 0;
    upper_pos_             = 0;
    offset_pos_            = 0;
    position_              = 0;
    last_pressed_span_     = DoubleSliderHeatmapWidget::kNoHandle;
    main_span_control_     = DoubleSliderHeatmapWidget::kLowerHandle;
    lower_pressed_control_ = QStyle::SC_None;
    upper_pressed_control_ = QStyle::SC_None;
    handle_movement_       = DoubleSliderHeatmapWidget::kNoOverlapping;
    is_first_movement_     = false;
    block_tracking_        = false;

    connect(this, &DoubleSliderHeatmapWidget::rangeChanged, this, &DoubleSliderHeatmapWidget::UpdateRange);
    connect(this, &DoubleSliderHeatmapWidget::SliderReleased, this, &DoubleSliderHeatmapWidget::MovePressedHandle);

    setStyleSheet(kCustomSliderStylesheet);

    // Sets the upper and lower position to range of a slider.
    SetSpan(minimum(), maximum());
}

DoubleSliderHeatmapWidget::~DoubleSliderHeatmapWidget()
{
    disconnect(this, &DoubleSliderHeatmapWidget::rangeChanged, this, &DoubleSliderHeatmapWidget::UpdateRange);
    disconnect(this, &DoubleSliderHeatmapWidget::SliderReleased, this, &DoubleSliderHeatmapWidget::MovePressedHandle);
}

void DoubleSliderHeatmapWidget::InitStyleOption(QStyleOptionSlider* option, DoubleSliderHeatmapWidget::SpanHandle span_handle) const
{
    QSlider::initStyleOption(option);
    option->sliderPosition = (span_handle == DoubleSliderHeatmapWidget::kLowerHandle ? lower_pos_ : upper_pos_);
    option->sliderValue    = (span_handle == DoubleSliderHeatmapWidget::kLowerHandle ? lower_value_ : upper_value_);
}

int DoubleSliderHeatmapWidget::PixelPosToRangeValue(int pixel_position) const
{
    QStyleOptionSlider option;
    initStyleOption(&option);

    int            slider_min    = 0;
    int            slider_max    = 0;
    int            slider_length = 0;
    const QSlider* slider        = this;
    const QRect    groove_rect   = slider->style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, slider);
    const QRect    handle_rect   = slider->style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, slider);

    if (slider->orientation() == Qt::Horizontal)
    {
        slider_length = handle_rect.width();
        slider_min    = groove_rect.x();
        slider_max    = groove_rect.right() - slider_length + 1;
    }
    else
    {
        slider_length = handle_rect.height();
        slider_min    = groove_rect.y();
        slider_max    = groove_rect.bottom() - slider_length + 1;
    }

    return QStyle::sliderValueFromPosition(slider->minimum(), slider->maximum(), pixel_position - slider_min, slider_max - slider_min, option.upsideDown);
}

void DoubleSliderHeatmapWidget::HandleMousePress(const QPoint&                         mouse_position,
                                                 QStyle::SubControl&                   control,
                                                 int                                   value,
                                                 DoubleSliderHeatmapWidget::SpanHandle span_handle)
{
    QStyleOptionSlider option;
    InitStyleOption(&option, span_handle);

    const QStyle::SubControl old_control = control;
    control                              = style()->hitTestComplexControl(QStyle::CC_Slider, &option, mouse_position, this);
    const QRect handle_rect              = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, this);

    if (control == QStyle::SC_SliderHandle)
    {
        position_          = value;
        offset_pos_        = Pick(mouse_position - handle_rect.topLeft());
        last_pressed_span_ = span_handle;
        setSliderDown(true);
        emit SliderPressed(span_handle);
    }

    if (control != old_control)
    {
        update(handle_rect);
    }
}

void DoubleSliderHeatmapWidget::SetupPainter(QPainter* painter, Qt::Orientation orientation, qreal start_x, qreal start_y, qreal end_x, qreal end_y) const
{
    QColor          highlight = palette().color(QPalette::Highlight);
    QLinearGradient gradient(start_x, start_y, end_x, end_y);
    gradient.setColorAt(0, highlight.darker(120));
    gradient.setColorAt(1, highlight.lighter(108));
    painter->setBrush(gradient);

    if (orientation == Qt::Horizontal)
    {
        painter->setPen(QPen(highlight.darker(130), 0));
    }
    else
    {
        painter->setPen(QPen(highlight.darker(150), 0));
    }
}

void DoubleSliderHeatmapWidget::DrawHeatmap(QStylePainter* painter, const QRect& span_area) const
{
    // draw heatmap
    painter->drawPixmap(span_area, heatmap_);
}

void DoubleSliderHeatmapWidget::SetHeatmap(QPixmap heatmap)
{
    heatmap_ = heatmap;
}

void DoubleSliderHeatmapWidget::DrawHandle(QStylePainter* painter, DoubleSliderHeatmapWidget::SpanHandle span_handle) const
{
    QStyleOptionSlider option;
    InitStyleOption(&option, span_handle);
    option.subControls = QStyle::SC_SliderHandle;

    QStyle::SubControl pressed = (span_handle == DoubleSliderHeatmapWidget::kLowerHandle ? lower_pressed_control_ : upper_pressed_control_);

    if (pressed == QStyle::SC_SliderHandle)
    {
        option.activeSubControls = pressed;
        option.state |= QStyle::State_Sunken;
    }

    painter->drawComplexControl(QStyle::CC_Slider, option);
}

void DoubleSliderHeatmapWidget::TriggerAction(QAbstractSlider::SliderAction slider_action, bool main_action)
{
    int                                         slider_value     = 0;
    bool                                        no_slider_action = false;
    bool                                        up_slider_action = false;
    const int                                   min_value        = minimum();
    const int                                   max_value        = maximum();
    const DoubleSliderHeatmapWidget::SpanHandle alt_control =
        (main_span_control_ == DoubleSliderHeatmapWidget::kLowerHandle ? DoubleSliderHeatmapWidget::kUpperHandle : DoubleSliderHeatmapWidget::kLowerHandle);

    block_tracking_ = true;

    switch (slider_action)
    {
    case QAbstractSlider::SliderSingleStepAdd:
        if ((main_action && main_span_control_ == DoubleSliderHeatmapWidget::kUpperHandle) ||
            (!main_action && alt_control == DoubleSliderHeatmapWidget::kUpperHandle))
        {
            slider_value     = qBound(min_value, upper_value_ + singleStep(), max_value);
            up_slider_action = true;
            break;
        }

        slider_value = qBound(min_value, lower_value_ + singleStep(), max_value);
        break;

    case QAbstractSlider::SliderSingleStepSub:
        if ((main_action && main_span_control_ == DoubleSliderHeatmapWidget::kUpperHandle) ||
            (!main_action && alt_control == DoubleSliderHeatmapWidget::kUpperHandle))
        {
            slider_value     = qBound(min_value, upper_value_ - singleStep(), max_value);
            up_slider_action = true;
            break;
        }

        slider_value = qBound(min_value, lower_value_ - singleStep(), max_value);
        break;

    case QAbstractSlider::SliderToMinimum:
        slider_value = min_value;

        if ((main_action && main_span_control_ == DoubleSliderHeatmapWidget::kUpperHandle) ||
            (!main_action && alt_control == DoubleSliderHeatmapWidget::kUpperHandle))
        {
            up_slider_action = true;
        }

        break;

    case QAbstractSlider::SliderToMaximum:
        slider_value = max_value;

        if ((main_action && main_span_control_ == DoubleSliderHeatmapWidget::kUpperHandle) ||
            (!main_action && alt_control == DoubleSliderHeatmapWidget::kUpperHandle))
        {
            up_slider_action = true;
        }

        break;

    case QAbstractSlider::SliderMove:
        if ((main_action && main_span_control_ == DoubleSliderHeatmapWidget::kUpperHandle) ||
            (!main_action && alt_control == DoubleSliderHeatmapWidget::kUpperHandle))
        {
            up_slider_action = true;
        }
        no_slider_action = true;
        break;
    case QAbstractSlider::SliderNoAction:
        no_slider_action = true;
        break;

    default:
        qWarning("DoubleSliderWidget::TriggerAction: Unknown action");
        break;
    }

    if (!no_slider_action)
    {
        if (up_slider_action)
        {
            if (handle_movement_ == DoubleSliderHeatmapWidget::kNoCrossing)
            {
                slider_value = qMax(slider_value, lower_value_);
            }
            else if (handle_movement_ == DoubleSliderHeatmapWidget::kNoOverlapping)
            {
                slider_value = qMax(slider_value, lower_value_ + 1);
            }

            if (handle_movement_ == DoubleSliderHeatmapWidget::kFreeMovement && slider_value < lower_value_)
            {
                SwapControls();
                SetLowerPosition(slider_value);
            }
            else
            {
                SetUpperPosition(slider_value);
            }
        }
        else
        {
            if (handle_movement_ == DoubleSliderHeatmapWidget::kNoCrossing)
            {
                slider_value = qMin(slider_value, upper_value_);
            }
            else if (handle_movement_ == DoubleSliderHeatmapWidget::kNoOverlapping)
            {
                slider_value = qMin(slider_value, upper_value_ - 1);
            }

            if (handle_movement_ == DoubleSliderHeatmapWidget::kFreeMovement && slider_value > upper_value_)
            {
                SwapControls();
                SetUpperPosition(slider_value);
            }
            else
            {
                SetLowerPosition(slider_value);
            }
        }
    }

    block_tracking_ = false;
    SetLowerValue(lower_pos_);
    SetUpperValue(upper_pos_);
}

void DoubleSliderHeatmapWidget::SwapControls()
{
    qSwap(lower_value_, upper_value_);
    qSwap(lower_pressed_control_, upper_pressed_control_);

    last_pressed_span_ =
        (last_pressed_span_ == DoubleSliderHeatmapWidget::kLowerHandle ? DoubleSliderHeatmapWidget::kUpperHandle : DoubleSliderHeatmapWidget::kLowerHandle);
    main_span_control_ =
        (main_span_control_ == DoubleSliderHeatmapWidget::kLowerHandle ? DoubleSliderHeatmapWidget::kUpperHandle : DoubleSliderHeatmapWidget::kLowerHandle);
}

void DoubleSliderHeatmapWidget::UpdateRange(int min_value, int max_value)
{
    Q_UNUSED(min_value);
    Q_UNUSED(max_value);

    // SetSpan() takes care of keeping span in range
    SetSpan(lower_value_, upper_value_);
}

void DoubleSliderHeatmapWidget::MovePressedHandle()
{
    switch (last_pressed_span_)
    {
    case DoubleSliderHeatmapWidget::kLowerHandle:
        if (lower_pos_ != lower_value_)
        {
            bool main_action = (main_span_control_ == DoubleSliderHeatmapWidget::kLowerHandle);
            TriggerAction(QAbstractSlider::SliderMove, main_action);
        }
        break;

    case DoubleSliderHeatmapWidget::kUpperHandle:
        if (upper_pos_ != upper_value_)
        {
            bool main_action = (main_span_control_ == DoubleSliderHeatmapWidget::kUpperHandle);
            TriggerAction(QAbstractSlider::SliderMove, main_action);
        }
        break;

    default:
        break;
    }
}

DoubleSliderHeatmapWidget::HandleMovementModeType DoubleSliderHeatmapWidget::HandleMovementMode() const
{
    return handle_movement_;
}

void DoubleSliderHeatmapWidget::SetHandleMovementMode(DoubleSliderHeatmapWidget::HandleMovementModeType movement_mode)
{
    handle_movement_ = movement_mode;
}

int DoubleSliderHeatmapWidget::LowerValue() const
{
    return qMin(lower_value_, upper_value_);
}

void DoubleSliderHeatmapWidget::SetLowerValue(int lower_value)
{
    SetSpan(lower_value, upper_value_);
}

int DoubleSliderHeatmapWidget::UpperValue() const
{
    return qMax(lower_value_, upper_value_);
}

void DoubleSliderHeatmapWidget::SetUpperValue(int upper_value)
{
    SetSpan(lower_value_, upper_value);
}

void DoubleSliderHeatmapWidget::SetSpan(int lower_value, int upper_value)
{
    const int lower_handle_value = qBound(minimum(), qMin(lower_value, upper_value), maximum());
    const int upper_handle_value = qBound(minimum(), qMax(lower_value, upper_value), maximum());

    if (lower_handle_value != lower_value_ || upper_handle_value != upper_value_)
    {
        if (lower_handle_value != lower_value_)
        {
            lower_value_ = lower_handle_value;
            lower_pos_   = lower_handle_value;
            emit LowerValueChanged(lower_handle_value);
        }

        if (upper_handle_value != upper_value_)
        {
            upper_value_ = upper_handle_value;
            upper_pos_   = upper_handle_value;
            emit UpperValueChanged(upper_handle_value);
        }

        emit SpanChanged(lower_value_, upper_value_);
        update();
    }
}

int DoubleSliderHeatmapWidget::LowerPosition() const
{
    return lower_pos_;
}

void DoubleSliderHeatmapWidget::SetLowerPosition(int lower_position)
{
    if (lower_pos_ != lower_position)
    {
        lower_pos_ = lower_position;

        if (!hasTracking())
        {
            update();
        }

        if (isSliderDown())
        {
            emit LowerPositionChanged(lower_position);
        }

        if (hasTracking() && !block_tracking_)
        {
            bool main_action = (main_span_control_ == DoubleSliderHeatmapWidget::kLowerHandle);
            TriggerAction(SliderMove, main_action);
        }
    }
}

int DoubleSliderHeatmapWidget::UpperPosition() const
{
    return upper_pos_;
}

void DoubleSliderHeatmapWidget::SetUpperPosition(int upper_position)
{
    if (upper_pos_ != upper_position)
    {
        upper_pos_ = upper_position;

        if (!hasTracking())
        {
            update();
        }

        if (isSliderDown())
        {
            emit UpperPositionChanged(upper_position);
        }

        if (hasTracking() && !block_tracking_)
        {
            bool main_action = (main_span_control_ == DoubleSliderHeatmapWidget::kUpperHandle);
            TriggerAction(SliderMove, main_action);
        }
    }
}

int DoubleSliderHeatmapWidget::Pick(const QPoint& handle_point) const
{
    return orientation() == Qt::Horizontal ? handle_point.x() : handle_point.y();
}

void DoubleSliderHeatmapWidget::keyPressEvent(QKeyEvent* event)
{
    QSlider::keyPressEvent(event);

    bool         main_action   = true;
    SliderAction slider_action = SliderNoAction;

    switch (event->key())
    {
    case Qt::Key_Left:
        main_action   = (orientation() == Qt::Horizontal);
        slider_action = !invertedAppearance() ? SliderSingleStepSub : SliderSingleStepAdd;
        break;

    case Qt::Key_Right:
        main_action   = (orientation() == Qt::Horizontal);
        slider_action = !invertedAppearance() ? SliderSingleStepAdd : SliderSingleStepSub;
        break;

    case Qt::Key_Up:
        main_action   = (orientation() == Qt::Vertical);
        slider_action = invertedControls() ? SliderSingleStepSub : SliderSingleStepAdd;
        break;

    case Qt::Key_Down:
        main_action   = (orientation() == Qt::Vertical);
        slider_action = invertedControls() ? SliderSingleStepAdd : SliderSingleStepSub;
        break;

    case Qt::Key_Home:
        main_action   = (main_span_control_ == DoubleSliderHeatmapWidget::kLowerHandle);
        slider_action = SliderToMinimum;
        break;

    case Qt::Key_End:
        main_action   = (main_span_control_ == DoubleSliderHeatmapWidget::kUpperHandle);
        slider_action = SliderToMaximum;
        break;

    default:
        event->ignore();
        break;
    }

    if (slider_action != SliderNoAction)
    {
        TriggerAction(slider_action, main_action);
    }
}

void DoubleSliderHeatmapWidget::mousePressEvent(QMouseEvent* event)
{
    if (minimum() == maximum() || (event->buttons() ^ event->button()))
    {
        event->ignore();
        return;
    }

    HandleMousePress(event->pos(), upper_pressed_control_, upper_value_, DoubleSliderHeatmapWidget::kUpperHandle);

    if (upper_pressed_control_ == QStyle::SC_SliderHandle)
    {
        is_first_movement_ = true;
        event->accept();
        return;
    }

    HandleMousePress(event->pos(), lower_pressed_control_, lower_value_, DoubleSliderHeatmapWidget::kLowerHandle);

    if (lower_pressed_control_ == QStyle::SC_SliderHandle)
    {
        is_first_movement_ = true;
        event->accept();
        return;
    }

    // Neither handle was hit, so find the closest one, and then execute a mouseMoveEvent also.

    int new_position = PixelPosToRangeValue(Pick(event->pos()) - offset_pos_);
    if (new_position < 0.5f * (lower_pos_ + upper_pos_))
    {
        lower_pressed_control_ = QStyle::SC_SliderHandle;
        position_              = lower_pos_;
    }
    else
    {
        upper_pressed_control_ = QStyle::SC_SliderHandle;
        position_              = upper_pos_;
    }

    is_first_movement_ = true;

    // call the mouse move helper function
    HandleMouseMove(event->pos());

    event->accept();
}

void DoubleSliderHeatmapWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (lower_pressed_control_ != QStyle::SC_SliderHandle && upper_pressed_control_ != QStyle::SC_SliderHandle)
    {
        event->ignore();
        return;
    }

    // call the helper function
    HandleMouseMove(event->pos());

    event->accept();
}

void DoubleSliderHeatmapWidget::HandleMouseMove(const QPoint& mouse_position)
{
    QStyleOptionSlider option;
    initStyleOption(&option);
    const int pixel_metric = style()->pixelMetric(QStyle::PM_MaximumDragDistance, &option, this);
    int       new_position = PixelPosToRangeValue(Pick(mouse_position) - offset_pos_);

    if (pixel_metric >= 0)
    {
        const QRect slider_rect = rect().adjusted(-pixel_metric, -pixel_metric, pixel_metric, pixel_metric);

        if (!slider_rect.contains(mouse_position))
        {
            new_position = position_;
        }
    }

    // Pick the preferred handle on the first movement
    if (is_first_movement_)
    {
        if (lower_value_ == upper_value_)
        {
            if (new_position < LowerValue())
            {
                SwapControls();
                is_first_movement_ = false;
            }
        }
        else
        {
            is_first_movement_ = false;
        }
    }

    if (lower_pressed_control_ == QStyle::SC_SliderHandle)
    {
        if (handle_movement_ == kNoCrossing)
        {
            new_position = qMin(new_position, UpperValue());
        }
        else if (handle_movement_ == kNoOverlapping)
        {
            new_position = qMin(new_position, UpperValue() - 1);
        }

        if (handle_movement_ == kFreeMovement && new_position > upper_value_)
        {
            SwapControls();
            SetUpperPosition(new_position);
        }
        else
        {
            SetLowerPosition(new_position);
        }
    }
    else if (upper_pressed_control_ == QStyle::SC_SliderHandle)
    {
        if (handle_movement_ == kNoCrossing)
        {
            new_position = qMax(new_position, LowerValue());
        }
        else if (handle_movement_ == kNoOverlapping)
        {
            new_position = qMax(new_position, LowerValue() + 1);
        }

        if (handle_movement_ == kFreeMovement && new_position < lower_value_)
        {
            SwapControls();
            SetLowerPosition(new_position);
        }
        else
        {
            SetUpperPosition(new_position);
        }
    }
}

void DoubleSliderHeatmapWidget::mouseReleaseEvent(QMouseEvent* event)
{
    QSlider::mouseReleaseEvent(event);
    setSliderDown(false);
    lower_pressed_control_ = QStyle::SC_None;
    upper_pressed_control_ = QStyle::SC_None;
    emit SliderReleased();
    update();
}

void DoubleSliderHeatmapWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QStylePainter painter(this);

    // groove & ticks
    QStyleOptionSlider option;
    initStyleOption(&option);
    option.sliderValue    = 0;
    option.sliderPosition = 0;
    option.subControls    = QStyle::SC_SliderGroove | QStyle::SC_SliderTickmarks;
    painter.drawComplexControl(QStyle::CC_Slider, option);

    // handle rects
    option.sliderPosition          = lower_pos_;
    const QRect lower_handle_rect  = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, this);
    const int   lower_handle_value = Pick(lower_handle_rect.center());
    option.sliderPosition          = upper_pos_;
    const QRect upper_handle_rect  = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, this);
    const int   upper_handle_value = Pick(upper_handle_rect.center());

    // span
    const int    min_value    = qMin(lower_handle_value, upper_handle_value);
    const int    max_value    = qMax(lower_handle_value, upper_handle_value);
    const QPoint center_point = QRect(lower_handle_rect.center(), upper_handle_rect.center()).center();
    QRect        span_rect;

    auto scaled_factor = 1.0;
    if (orientation() == Qt::Horizontal)
    {
        auto heatmap_half_height = static_cast<int>(height() / (4 * scaled_factor));
        span_rect                = QRect(QPoint(min_value, center_point.y() - heatmap_half_height), QPoint(max_value, center_point.y() + heatmap_half_height));
    }
    else
    {
        auto heatmap_half_width = static_cast<int>(width()  / (4 * scaled_factor));
        span_rect               = QRect(QPoint(center_point.x() - heatmap_half_width, min_value), QPoint(center_point.x() + heatmap_half_width, max_value));
    }

    DrawHeatmap(&painter, span_rect);

    // handles
    switch (last_pressed_span_)
    {
    case DoubleSliderHeatmapWidget::kLowerHandle:
        DrawHandle(&painter, DoubleSliderHeatmapWidget::kUpperHandle);
        DrawHandle(&painter, DoubleSliderHeatmapWidget::kLowerHandle);
        break;

    case DoubleSliderHeatmapWidget::kUpperHandle:
    default:
        DrawHandle(&painter, DoubleSliderHeatmapWidget::kLowerHandle);
        DrawHandle(&painter, DoubleSliderHeatmapWidget::kUpperHandle);
        break;
    }
}
