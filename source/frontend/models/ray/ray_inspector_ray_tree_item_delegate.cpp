//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Implementation of RRA's ray inspector ray list item delegate.
//=============================================================================

#include "models/ray/ray_inspector_ray_tree_item_delegate.h"

#include "qt_common/utils/qt_util.h"

#include "models/ray/ray_inspector_model.h"
#include "qpainter.h"
#include <views/widget_util.h>
#include <constants.h>

RayInspectorRayTreeItemDelegate::RayInspectorRayTreeItemDelegate(QObject* parent)
{
    RRA_UNUSED(parent);
}

RayInspectorRayTreeItemDelegate::RayInspectorRayTreeItemDelegate(ScaledTreeView* tree_view, QObject* parent)
{
    RRA_UNUSED(tree_view);
    RRA_UNUSED(parent);
}

RayInspectorRayTreeItemDelegate::~RayInspectorRayTreeItemDelegate()
{
}

bool RayInspectorRayTreeItemDelegate::CheckboxAt(int row, int column) const
{
    Q_UNUSED(row);
    switch (column)
    {
    case rra::kRayInspectorRayListColumnHit:
        return true;

    default:
        return false;
    }
}

bool RayInspectorRayTreeItemDelegate::HierarchyAlignment(int row, int column) const
{
    Q_UNUSED(row);
    switch (column)
    {
    case rra::kRayInspectorRayIndexColumn:
        return true;

    default:
        return false;
    }
}

void RayInspectorRayTreeItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if ((option.state & QStyle::State_Selected) != 0)
    {
        // Draw selection highlight (and set text color to white).
        painter->fillRect(option.rect, QBrush(QtCommon::QtUtils::ColorTheme::Get().GetCurrentThemeColors().row_selected_color));
    }

    if (CheckboxAt(index.row(), index.column()))
    {
        // Draw data as a checkbox.
        rra::widget_util::DrawCheckboxCell(painter, option.rect, index.data(Qt::CheckStateRole).toBool(), true);
    }
    if (HierarchyAlignment(index.row(), index.column()))
    {
        painter->drawText(option.rect, Qt::AlignLeft | Qt::AlignVCenter, index.data().toString());
    }
    else
    {
        painter->drawText(option.rect, Qt::AlignRight | Qt::AlignVCenter, index.data().toString());
    }
}
