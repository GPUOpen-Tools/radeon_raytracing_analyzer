//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for a QTableHeader class which prevents header selection
/// on a specified column (usually column 0, the row index).
//=============================================================================

#include <unordered_set>

#include "views/custom_widgets/index_header_view.h"

IndexHeaderView::IndexHeaderView(int non_selectable_index, Qt::Orientation orientation, QWidget* parent)
    : QHeaderView(orientation, parent)
{
    non_selectable_index_.insert(non_selectable_index);
}

IndexHeaderView::IndexHeaderView(std::unordered_set<int> non_selectable_index, Qt::Orientation orientation, QWidget* parent)
    : QHeaderView(orientation, parent)
{
    non_selectable_index_ = std::move(non_selectable_index);
}

void IndexHeaderView::mousePressEvent(QMouseEvent* event)
{
    const int index = logicalIndexAt(event->position().x());
    if (non_selectable_index_.find(index) != non_selectable_index_.end())
    {
        setSectionsClickable(false);
    }
    QHeaderView::mousePressEvent(event);
    setSectionsClickable(true);
}

void IndexHeaderView::mouseReleaseEvent(QMouseEvent* event)
{
    const int index = logicalIndexAt(event->position().x());
    if (non_selectable_index_.find(index) != non_selectable_index_.end())
    {
        setSectionsClickable(false);
    }
    QHeaderView::mouseReleaseEvent(event);
    setSectionsClickable(true);
}

