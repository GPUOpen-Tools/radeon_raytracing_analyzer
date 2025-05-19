//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a proxy filter that processes the ray tree
/// table.
//=============================================================================

#include <math.h>

#include <QTreeView>

#include "public/rra_assert.h"

#include "models/ray/ray_inspector_ray_tree_model.h"
#include "models/ray/ray_inspector_ray_tree_proxy_model.h"
#include "util/string_util.h"

namespace rra
{
    RayInspectorRayTreeProxyModel::RayInspectorRayTreeProxyModel(QObject* parent)
        : TreeViewProxyModel(parent)
    {
    }

    RayInspectorRayTreeProxyModel::~RayInspectorRayTreeProxyModel()
    {
    }

    RayInspectorRayTreeModel* RayInspectorRayTreeProxyModel::InitializeRayTreeModels(ScaledTreeView* view)
    {
        RayInspectorRayTreeModel* model = new RayInspectorRayTreeModel();

        setSourceModel(model);
        view->setModel(this);

        return model;
    }

    bool RayInspectorRayTreeProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
    {
        RRA_UNUSED(source_row);
        RRA_UNUSED(source_parent);
        return true;
    }

    bool RayInspectorRayTreeProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        int left_column  = left.column();
        int right_column = right.column();
        if (left_column == right_column)
        {
            switch (left_column)
            {
            case kRayInspectorRayIndexColumn:
            case kRayInspectorRayListColumnTraversalLoopCount:
            case kRayInspectorRayListColumnInstanceIntersections:
            case kRayInspectorRayListColumnAnyHitInvocations:
            case kRayInspectorRayListColumnHit:
            {
                const uint64_t left_data  = left.data(Qt::UserRole).toULongLong();
                const uint64_t right_data = right.data(Qt::UserRole).toULongLong();
                return left_data < right_data;
            }
            default:
                break;
            }
        }

        return QSortFilterProxyModel::lessThan(left, right);
    }
}  // namespace rra

