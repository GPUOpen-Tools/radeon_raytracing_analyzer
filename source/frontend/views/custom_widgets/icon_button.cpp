//=============================================================================
/// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Implementation of a QPushButton with vector based icons for various states.
//=============================================================================

#include "views/custom_widgets/icon_button.h"

#include <QIcon>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPushButton>
#include <QStyleOptionButton>
#include <QStylePainter>

RraIconButton::RraIconButton(QWidget* parent)
    : QPushButton(parent)
{
    setFlat(true);

    setBaseSize(QSize(kDefaultIconSize, kDefaultIconSize));
}

RraIconButton::RraIconButton(QWidget* parent, const QString& normal_icon_resource, const QString& hover_icon_resource)
    : QPushButton(parent)
    , normal_icon_(QIcon(normal_icon_resource))
    , hover_icon_(QIcon(hover_icon_resource))
{
    setFlat(true);

    setBaseSize(QSize(kDefaultIconSize, kDefaultIconSize));
}

RraIconButton::~RraIconButton()
{
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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void RraIconButton::enterEvent(QEvent* event)
#else
void RraIconButton::enterEvent(QEnterEvent* event)
#endif
{
    QPushButton::enterEvent(event);
    setAttribute(Qt::WA_UnderMouse, true);
}

void RraIconButton::leaveEvent(QEvent* event)
{
    QPushButton::leaveEvent(event);
    setAttribute(Qt::WA_UnderMouse, false);
}

