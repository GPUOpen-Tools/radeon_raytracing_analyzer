//=============================================================================
/// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Implementation of a QPushButton with vector based icons for various states.
//=============================================================================

#include "icon_button.h"

#include <QPaintEvent>
#include <QMouseEvent>
#include <QPushButton>
#include <QIcon>
#include <QStylePainter>
#include <QStylePainter>
#include <QStyleOptionButton>

#include "qt_common/utils/scaling_manager.h"

RraIconButton::RraIconButton(QWidget* parent)
    : QPushButton(parent)
{
    setFlat(true);

    setBaseSize(QSize(kDefaultIconSize, kDefaultIconSize));

    connect(&ScalingManager::Get(), &ScalingManager::ScaleFactorChanged, this, &RraIconButton::updateGeometry);
}

RraIconButton::RraIconButton(QWidget* parent, const QString& normal_icon_resource, const QString& hover_icon_resource)
    : QPushButton(parent)
    , normal_icon_(QIcon(normal_icon_resource))
    , hover_icon_(QIcon(hover_icon_resource))
{
    setFlat(true);

    setBaseSize(QSize(kDefaultIconSize, kDefaultIconSize));

    connect(&ScalingManager::Get(), &ScalingManager::ScaleFactorChanged, this, &RraIconButton::updateGeometry);
}

RraIconButton::~RraIconButton()
{
    disconnect(&ScalingManager::Get(), &ScalingManager::ScaleFactorChanged, this, &RraIconButton::updateGeometry);
}

void RraIconButton::SetNormalIcon(const QIcon& icon)
{
    normal_icon_ = icon;
}

void RraIconButton::SetHoverIcon(const QIcon& icon)
{
    hover_icon_ = icon;
}

QSize RraIconButton::sizeHint() const
{
    QSize size_hint(baseSize());

    size_hint = ScalingManager::Get().Scaled(size_hint);

    return size_hint;
}

void RraIconButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QStylePainter painter(this);
    painter.setPen(QPen(palette().color(QPalette::Window)));
    if (testAttribute(Qt::WA_UnderMouse))
    {
        hover_icon_.paint(&painter, rect());
    }
    else
    {
        normal_icon_.paint(&painter, rect());
    }
}

void RraIconButton::enterEvent(QEvent* event)
{
    QPushButton::enterEvent(event);
    setAttribute(Qt::WA_UnderMouse, true);
}

void RraIconButton::leaveEvent(QEvent* event)
{
    QPushButton::leaveEvent(event);
    setAttribute(Qt::WA_UnderMouse, false);
}
