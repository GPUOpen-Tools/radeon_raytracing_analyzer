//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a widget that implements a custom check box.
//=============================================================================

#include "views/custom_widgets/colored_checkbox.h"

#include <cmath>
#include <QApplication>
#include <QPainter>
#include <QStylePainter>

#include "qt_common/utils/qt_util.h"
#include "qt_common/utils/scaling_manager.h"

ColoredCheckbox::ColoredCheckbox(QWidget* parent)
    : QCheckBox(parent)
    , button_text_ratio_(1.0)
{
    setMouseTracking(true);
    setChecked(false);
    setCursor(Qt::PointingHandCursor);

    connect(&ScalingManager::Get(), &ScalingManager::ScaleFactorChanged, this, &ColoredCheckbox::OnScaleFactorChanged);
}

ColoredCheckbox::~ColoredCheckbox()
{
    disconnect(&ScalingManager::Get(), &ScalingManager::ScaleFactorChanged, this, &ColoredCheckbox::OnScaleFactorChanged);
}

void ColoredCheckbox::Initialize(bool checked, const QColor& primary_color)
{
    UpdatePrimaryColor(primary_color);
    setChecked(checked);
}

void ColoredCheckbox::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QStylePainter painter(this);

    QFontMetricsF font_metrics = painter.fontMetrics();

    // Example sizes:
    // height is 20 px
    // switch height is 20px
    // white space height is 16px
    // colored dot is 12 px

    // switch width is 40px
    // white space width is 36px
    // colored dot is 12 px

    const qreal switch_height = GetSwitchHeight(font_metrics);
    const qreal switch_width  = switch_height * 2;
    const qreal half_height   = switch_height / 2;
    const qreal switch_radius = half_height;

    const qreal space_y_coord = switch_height * 0.1;
    const qreal space_x_coord = switch_height * 0.1;
    const qreal space_height  = switch_height * 0.8;
    const qreal space_width   = switch_width * 0.9;
    const qreal space_radius  = space_height / 2;

    const qreal button_diameter    = switch_height * 0.6;
    const qreal button_y_coord     = switch_height * 0.2;
    const qreal button_x_coord_off = switch_height * 0.2;
    const qreal button_x_coord_on  = switch_width * 0.6;

    int scaled_padding = this->style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, nullptr, this);

    painter.setFont(QWidget::font());

    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(Qt::NoPen);

    // Set 'off' color based on enabled state.
    Qt::GlobalColor default_color = Qt::black;
    bool            enabled       = isEnabled();
    if (!enabled)
    {
        default_color = Qt::lightGray;
    }

    // If not checked or disabled, draw grayed out and unchecked.
    if (isChecked() == false || enabled == false)
    {
        // Draw switch.
        painter.setBrush(default_color);
        QRectF outer_rect(0, 0, switch_width, switch_height);
        painter.drawRoundedRect(outer_rect, switch_radius, switch_radius);

        // Draw space.
        painter.setBrush(Qt::white);
        QRectF inner_rect(space_x_coord, space_y_coord, space_width, space_height);
        painter.drawRoundedRect(inner_rect, space_radius, space_radius);

        // Draw button.
        painter.setBrush(default_color);
        const QRectF button_rect = QRectF(button_x_coord_off, button_y_coord, button_diameter, button_diameter);
        painter.drawEllipse(button_rect);
    }
    else
    {
        const QRectF outer_rect(0, 0, switch_width, switch_height);

        // Draw single color switch.
        painter.setBrush(color_);
        painter.drawRoundedRect(outer_rect, switch_radius, switch_radius);

        // Draw white button in the middle.
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        painter.setBrush(Qt::white);
        const QRectF button_rect = QRectF(button_x_coord_on, button_y_coord, button_diameter, button_diameter);
        painter.drawEllipse(button_rect);
    }

    qreal text_height = font_metrics.capHeight();
    qreal text_base   = half_height + (text_height / 2.0);

    painter.setPen(Qt::black);
    painter.drawText(switch_width + scaled_padding, text_base, this->text());
}

void ColoredCheckbox::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}

void ColoredCheckbox::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    setChecked(!isChecked());
    emit Clicked();
    this->update();
}

QSize ColoredCheckbox::sizeHint() const
{
    QFontMetricsF font_metrics = fontMetrics();

    // Height is defined by max of font height and scaled checkbox height.
    const qreal base_height = GetSwitchHeight(font_metrics);
    int         height      = std::max<int>(font_metrics.height(), base_height);

    // Width is defined by switch width (which is 2*height) plus the spacing between a check box indicator and its label, plus the width of the text.
    int   switch_width   = base_height * 2;
    qreal scaled_padding = this->style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, nullptr, this);
    int   text_width     = font_metrics.horizontalAdvance(this->text());

    return QSize(switch_width + scaled_padding + text_width, height);
}

void ColoredCheckbox::OnScaleFactorChanged()
{
    QtCommon::QtUtils::InvalidateFontMetrics(this);
    this->updateGeometry();
}

void ColoredCheckbox::UpdateText(const QString& text)
{
    setText(text);
    QtCommon::QtUtils::InvalidateFontMetrics(this);
    this->updateGeometry();
}

void ColoredCheckbox::UpdatePrimaryColor(const QColor& color)
{
    color_ = color;
}

void ColoredCheckbox::SetButtonTextRatio(qreal button_text_ratio)
{
    button_text_ratio_ = button_text_ratio;
}

qreal ColoredCheckbox::GetButtonTextRatio() const
{
    return button_text_ratio_;
}

qreal ColoredCheckbox::GetSwitchHeight(const QFontMetricsF& font_metrics) const
{
    return font_metrics.height() * button_text_ratio_;
}
