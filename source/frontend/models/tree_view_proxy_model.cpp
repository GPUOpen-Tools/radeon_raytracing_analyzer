//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a proxy filter used to filter a tree view.
//=============================================================================

#include "models/tree_view_proxy_model.h"

#include "models/acceleration_structure_tree_view_item.h"

namespace rra
{
    TreeViewProxyModel::TreeViewProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
    }

    TreeViewProxyModel::~TreeViewProxyModel()
    {
    }

    void TreeViewProxyModel::SetSearchText(const QString& search_text)
    {
        search_text_ = search_text;
        invalidate();
    }

    bool TreeViewProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
    {
        QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
        auto item_data = qvariant_cast<AccelerationStructureTreeViewItemData>(index.data(Qt::DisplayRole));
        return (item_data.display_name.contains(search_text_));
    }

}  // namespace rra
