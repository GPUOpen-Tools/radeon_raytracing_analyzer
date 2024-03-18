//=============================================================================
// Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for a read-only checkbox. Used for displaying
/// boolean values in tables.
//=============================================================================

#include <QPen>
#include <QPainter>

#include "views/custom_widgets/read_only_checkbox.h"

#include "views/widget_util.h"

ReadOnlyCheckBox::ReadOnlyCheckBox(QWidget* parent)
    : QCheckBox(parent)
{
    setContentsMargins(10, 5, 10, 5);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setFocusPolicy(Qt::NoFocus);
}

ReadOnlyCheckBox::~ReadOnlyCheckBox()
{
}

void ReadOnlyCheckBox::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    QRect    rect = QRect(0, 0, this->size().width(), this->size().height());

    bool checked = (checkState() == Qt::CheckState::Checked) ? true : false;
    rra::widget_util::DrawCheckboxCell(&painter, rect, checked, false);
}
