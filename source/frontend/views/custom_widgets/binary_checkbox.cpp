//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a widget that implements a custom check box.
//=============================================================================

#include "views/custom_widgets/binary_checkbox.h"

#include <cmath>
#include <QApplication>
#include <QPainter>
#include <QStylePainter>

#include "qt_common/utils/qt_util.h"
#include "qt_common/utils/scaling_manager.h"

BinaryCheckbox::BinaryCheckbox(QWidget* parent)
    : QCheckBox(parent)
    , button_text_ratio_(1.0)
{
    setMouseTracking(true);
    setChecked(false);
    setCursor(Qt::PointingHandCursor);

    connect(&ScalingManager::Get(), &ScalingManager::ScaleFactorChanged, this, &BinaryCheckbox::OnScaleFactorChanged);
}

BinaryCheckbox::~BinaryCheckbox()
{
    disconnect(&ScalingManager::Get(), &ScalingManager::ScaleFactorChanged, this, &BinaryCheckbox::OnScaleFactorChanged);
}

void BinaryCheckbox::Initialize(bool checked, const QColor& primary_color)
{
    UpdatePrimaryColor(primary_color);
    setChecked(checked);
}

void BinaryCheckbox::paintEvent(QPaintEvent* event)
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
    const qreal switch_width  = switch_height * (2.0 / 3.0);
    const qreal half_height   = switch_height / 2;
    const qreal switch_radius = switch_height / 5.0;

    const qreal outline_width = switch_height * 0.05;
    const qreal space_y_coord = outline_width;
    const qreal space_x_coord = outline_width;
    const qreal space_height  = switch_height - 2 * outline_width;
    const qreal space_width   = switch_width - 2 * outline_width;
    const qreal space_radius  = (space_height / switch_height) * switch_radius;

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

    qreal text_height = font_metrics.capHeight();
    qreal text_base   = half_height + (text_height / 2.0);

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

        // Draw binary bit.
        painter.setPen(Qt::black);
        int   text_width           = font_metrics.width(QString("0"));
        qreal text_horizontal_base = (switch_width - text_width) * 0.5;
        painter.drawText(text_horizontal_base, text_base, QString("0"));
    }
    else
    {
        const QRectF outer_rect(0, 0, switch_width, switch_height);

        // Draw single color switch.
        painter.setBrush(color_);
        painter.drawRoundedRect(outer_rect, switch_radius, switch_radius);

        // Draw binary bit.
        painter.setPen(Qt::white);
        int   text_width           = font_metrics.width(QString("1"));
        qreal text_horizontal_base = (switch_width - text_width) * 0.5;
        painter.drawText(text_horizontal_base, text_base, QString("1"));
    }

    painter.setPen(Qt::black);
    painter.drawText(switch_width + scaled_padding, text_base, this->text());
}

void BinaryCheckbox::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}

void BinaryCheckbox::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    setChecked(!isChecked());
    emit Clicked();
    this->update();
}

QSize BinaryCheckbox::sizeHint() const
{
    QFontMetricsF font_metrics = fontMetrics();

    // Height is defined by max of font height and scaled checkbox height.
    const qreal base_height = GetSwitchHeight(font_metrics);
    int         height      = std::max<int>(font_metrics.height(), base_height);

    // Width is defined by switch width plus the spacing between a check box indicator and its label, plus the width of the text.
    int switch_width = base_height * (2.0 / 3.0);
    return QSize(switch_width, height);
}

void BinaryCheckbox::OnScaleFactorChanged()
{
    QtCommon::QtUtils::InvalidateFontMetrics(this);
    this->updateGeometry();
}

void BinaryCheckbox::UpdateText(const QString& text)
{
    setText(text);
    QtCommon::QtUtils::InvalidateFontMetrics(this);
    this->updateGeometry();
}

void BinaryCheckbox::UpdatePrimaryColor(const QColor& color)
{
    color_ = color;
}

void BinaryCheckbox::SetButtonTextRatio(qreal button_text_ratio)
{
    button_text_ratio_ = button_text_ratio;
}

qreal BinaryCheckbox::GetButtonTextRatio() const
{
    return button_text_ratio_;
}

qreal BinaryCheckbox::GetSwitchHeight(const QFontMetricsF& font_metrics) const
{
    return font_metrics.height() * button_text_ratio_;
}
