//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a proxy filter that processes the instances
/// tables.
//=============================================================================

#include "models/instances_proxy_model.h"

#include <QTableView>

#include "public/rra_assert.h"

#include "models/instances_item_model.h"

namespace rra
{
    InstancesProxyModel::InstancesProxyModel(QObject* parent)
        : TableProxyModel(parent)
    {
    }

    InstancesProxyModel::~InstancesProxyModel()
    {
    }

    InstancesItemModel* InstancesProxyModel::InitializeAccelerationStructureTableModels(QTableView* view, int num_rows, int num_columns)
    {
        InstancesItemModel* model = new InstancesItemModel();
        model->SetRowCount(num_rows);
        model->SetColumnCount(num_columns);

        setSourceModel(model);
        SetFilterKeyColumns({
            kInstancesColumnInstanceIndex,
            kInstancesColumnInstanceAddress,
            kInstancesColumnInstanceOffset,
            kInstancesColumnInstanceMask,
            kInstancesColumnCullDisableFlag,
            kInstancesColumnFlipFacingFlag,
            kInstancesColumnForceOpaqueFlag,
            kInstancesColumnForceNoOpaqueFlag,
            kInstancesColumnRebraidSiblingCount,
            kInstancesColumnXPosition,
            kInstancesColumnYPosition,
            kInstancesColumnZPosition,
            kInstancesColumnM11,
            kInstancesColumnM12,
            kInstancesColumnM13,
            kInstancesColumnM21,
            kInstancesColumnM22,
            kInstancesColumnM23,
            kInstancesColumnM31,
            kInstancesColumnM32,
            kInstancesColumnM33,
            kInstancesColumnUniqueInstanceIndex,
        });

        view->setModel(this);

        return model;
    }

    QVariant InstancesProxyModel::data(const QModelIndex& index, int role) const
    {
        if (index.column() == kInstancesColumnIndex)
        {
            return index.row();
        }
        else
        {
            return TableProxyModel::data(index, role);
        }
    }

    bool InstancesProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
    {
        if (FilterSearchString(source_row, source_parent) == false)
        {
            return false;
        }
        return true;
    }

    bool InstancesProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        int left_column  = left.column();
        int right_column = right.column();
        if (left_column == right_column)
        {
            switch (left_column)
            {
            case kInstancesColumnInstanceAddress:
            case kInstancesColumnInstanceOffset:
            {
                const uint64_t left_data  = left.data(Qt::UserRole).toULongLong();
                const uint64_t right_data = right.data(Qt::UserRole).toULongLong();
                return left_data < right_data;
            }

            case kInstancesColumnXPosition:
            case kInstancesColumnYPosition:
            case kInstancesColumnZPosition:
            case kInstancesColumnM11:
            case kInstancesColumnM12:
            case kInstancesColumnM13:
            case kInstancesColumnM21:
            case kInstancesColumnM22:
            case kInstancesColumnM23:
            case kInstancesColumnM31:
            case kInstancesColumnM32:
            case kInstancesColumnM33:
            {
                const float left_data  = left.data(Qt::UserRole).toFloat();
                const float right_data = right.data(Qt::UserRole).toFloat();
                return left_data < right_data;
            }

            case kInstancesColumnInstanceIndex:
            case kInstancesColumnInstanceMask:
            case kInstancesColumnRebraidSiblingCount:
            case kInstancesColumnUniqueInstanceIndex:
            {
                const uint32_t left_data  = left.data(Qt::UserRole).toUInt();
                const uint32_t right_data = right.data(Qt::UserRole).toUInt();
                return left_data < right_data;
            }

            case kInstancesColumnCullDisableFlag:
            case kInstancesColumnFlipFacingFlag:
            case kInstancesColumnForceOpaqueFlag:
            case kInstancesColumnForceNoOpaqueFlag:
            {
                const bool left_data  = left.data(Qt::UserRole).toBool();
                const bool right_data = right.data(Qt::UserRole).toBool();
                return left_data < right_data;
            }

            default:
                break;
            }
        }

        return QSortFilterProxyModel::lessThan(left, right);
    }
}  // namespace rra

