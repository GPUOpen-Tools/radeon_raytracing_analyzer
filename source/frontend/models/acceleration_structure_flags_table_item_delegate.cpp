//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Implementation of RRA's instance flags table item delegate.
//=============================================================================

#include "models/acceleration_structure_flags_table_item_delegate.h"
#include "models/acceleration_structure_flags_table_item_model.h"
#include "qpainter.h"

FlagTableItemDelegate::FlagTableItemDelegate(QObject* parent)
    : TableItemDelegate(parent)
{
}

FlagTableItemDelegate::~FlagTableItemDelegate()
{
}

void FlagTableItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QColor color = index.data(Qt::ForegroundRole).value<QColor>();

    // We specify the color for the instance flags so we draw it manually.
    if (color.isValid())
    {
        QString flag_text = index.data(Qt::DisplayRole).value<QString>();

        painter->setPen(color);
        painter->drawText(option.rect, Qt::AlignLeft | Qt::AlignVCenter, flag_text);
    }
    else
    {
        TableItemDelegate::paint(painter, option, index);
    }
}

bool FlagTableItemDelegate::CheckboxAt(int row, int column) const
{
    Q_UNUSED(row);
    switch (column)
    {
    case rra::kInstanceFlagsTableColumnCheckbox:
        return true;

    default:
        return false;
    }
}
