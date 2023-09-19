//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a proxy filter that processes the ray list
/// table.
//=============================================================================

#include <math.h>

#include "models/ray/ray_inspector_ray_list_proxy_model.h"

#include <QTableView>

#include "public/rra_assert.h"

#include "models/ray/ray_inspector_ray_list_item_model.h"
#include "util/string_util.h"

namespace rra
{
    RayInspectorRayListProxyModel::RayInspectorRayListProxyModel(QObject* parent)
        : TableProxyModel(parent)
    {
    }

    RayInspectorRayListProxyModel::~RayInspectorRayListProxyModel()
    {
    }

    RayInspectorRayListItemModel* RayInspectorRayListProxyModel::InitializeRayTableModels(QTableView* view, int num_rows, int num_columns)
    {
        RayInspectorRayListItemModel* model = new RayInspectorRayListItemModel();
        model->SetRowCount(num_rows);
        model->SetColumnCount(num_columns);

        setSourceModel(model);
        SetFilterKeyColumns({
            kRayInspectorRayListColumnTraversalLoopCount,
            kRayInspectorRayListColumnInstanceIntersections,
            kRayInspectorRayListColumnAnyHitInvocations,
            kRayInspectorRayListColumnHit,
        });

        view->setModel(this);

        return model;
    }

    bool RayInspectorRayListProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
    {
        RRA_UNUSED(source_row);
        RRA_UNUSED(source_parent);
        return true;
    }

    bool RayInspectorRayListProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
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
