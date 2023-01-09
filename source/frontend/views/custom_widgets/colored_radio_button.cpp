//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a widget that implements a custom radio button.
//=============================================================================

#include "views/custom_widgets/colored_radio_button.h"

#include <cmath>
#include <QApplication>
#include <QPainter>
#include <QStylePainter>

#include "qt_common/utils/qt_util.h"
#include "qt_common/utils/scaling_manager.h"

ColoredRadioButton::ColoredRadioButton(QWidget* parent)
    : QRadioButton(parent)
    , button_text_ratio_(1.0)
{
    setMouseTracking(true);
    setChecked(false);
    setCursor(Qt::PointingHandCursor);

    connect(&ScalingManager::Get(), &ScalingManager::ScaleFactorChanged, this, &ColoredRadioButton::OnScaleFactorChanged);
}

ColoredRadioButton::~ColoredRadioButton()
{
    disconnect(&ScalingManager::Get(), &ScalingManager::ScaleFactorChanged, this, &ColoredRadioButton::OnScaleFactorChanged);
}

void ColoredRadioButton::Initialize(bool checked, const QColor& primary_color)
{
    UpdatePrimaryColor(primary_color);
    setChecked(checked);
}

void ColoredRadioButton::paintEvent(QPaintEvent* event)
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

    const qreal radio_button_height = GetSwitchHeight(font_metrics);
    const qreal radio_button_width  = radio_button_height;
    const qreal half_height   = radio_button_height / 2;
    const qreal radio_button_radius = half_height;

    const qreal inner_circle_diameter          = radio_button_height * 0.8;
    const qreal inner_circle_diameter_selected = radio_button_height * 0.5;
    // Center the white in the circle.
    const qreal inner_circle_xy_coord          = 0.5 * (radio_button_height - inner_circle_diameter);
    const qreal inner_circle_xy_coord_selected = 0.5 * (radio_button_height - inner_circle_diameter_selected);

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
        QRectF outer_rect(0, 0, radio_button_width, radio_button_height);
        painter.drawRoundedRect(outer_rect, radio_button_radius, radio_button_radius);

        // Draw space.
        painter.setBrush(Qt::white);
        const QRectF inner_rec2 = QRectF(inner_circle_xy_coord, inner_circle_xy_coord, inner_circle_diameter, inner_circle_diameter);
        painter.drawEllipse(inner_rec2);
    }
    else
    {
        const QRectF outer_rect(0, 0, radio_button_width, radio_button_height);

        // Draw single color switch.
        painter.setBrush(color_);
        painter.drawRoundedRect(outer_rect, radio_button_radius, radio_button_radius);

        // Draw white button in the middle.
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        painter.setBrush(Qt::white);
        const QRectF button_rect =
            QRectF(inner_circle_xy_coord_selected, inner_circle_xy_coord_selected, inner_circle_diameter_selected, inner_circle_diameter_selected);
        painter.drawEllipse(button_rect);
    }

    qreal text_height = font_metrics.capHeight();
    qreal text_base   = half_height + (text_height / 2.0);

    painter.setPen(Qt::black);
    painter.drawText(radio_button_width + scaled_padding, text_base, this->text());
}

void ColoredRadioButton::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}

void ColoredRadioButton::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    setChecked(!isChecked());
    emit Clicked();
    this->update();
}

QSize ColoredRadioButton::sizeHint() const
{
    QFontMetricsF font_metrics = fontMetrics();

    // Height is defined by max of font height and scaled checkbox height.
    const qreal base_height = GetSwitchHeight(font_metrics);
    int         height      = std::max<int>(font_metrics.height(), base_height);

    // Width is defined by switch width (which is 2*height) plus the spacing between a check box indicator and its label, plus the width of the text.
    int   switch_width   = base_height * 2;
    qreal scaled_padding = this->style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, nullptr, this);
    int   text_width     = font_metrics.width(this->text());

    return QSize(switch_width + scaled_padding + text_width, height);
}

void ColoredRadioButton::OnScaleFactorChanged()
{
    QtCommon::QtUtils::InvalidateFontMetrics(this);
    this->updateGeometry();
}

void ColoredRadioButton::UpdateText(const QString& text)
{
    setText(text);
    QtCommon::QtUtils::InvalidateFontMetrics(this);
    this->updateGeometry();
}

void ColoredRadioButton::UpdatePrimaryColor(const QColor& color)
{
    color_ = color;
}

void ColoredRadioButton::SetButtonTextRatio(qreal button_text_ratio)
{
    button_text_ratio_ = button_text_ratio;
}

qreal ColoredRadioButton::GetButtonTextRatio() const
{
    return button_text_ratio_;
}

qreal ColoredRadioButton::GetSwitchHeight(const QFontMetricsF& font_metrics) const
{
    return font_metrics.height() * button_text_ratio_;
}
