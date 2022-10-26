//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a proxy filter that processes the TLAS instances
/// table.
//=============================================================================

#include "models/tlas/tlas_instances_proxy_model.h"

#include <QTableView>

#include "public/rra_assert.h"

#include "models/tlas/tlas_instances_item_model.h"

namespace rra
{
    TlasInstancesProxyModel::TlasInstancesProxyModel(QObject* parent)
        : TableProxyModel(parent)
    {
    }

    TlasInstancesProxyModel::~TlasInstancesProxyModel()
    {
    }

    TlasInstancesItemModel* TlasInstancesProxyModel::InitializeAccelerationStructureTableModels(QTableView* view, int num_rows, int num_columns)
    {
        TlasInstancesItemModel* model = new TlasInstancesItemModel();
        model->SetRowCount(num_rows);
        model->SetColumnCount(num_columns);

        setSourceModel(model);
        SetFilterKeyColumns({
            kTlasInstancesColumnInstanceAddress,
            kTlasInstancesColumnInstanceOffset,
            kTlasInstancesColumnXPosition,
            kTlasInstancesColumnYPosition,
            kTlasInstancesColumnZPosition,
            kTlasInstancesColumnM11,
            kTlasInstancesColumnM12,
            kTlasInstancesColumnM13,
            kTlasInstancesColumnM21,
            kTlasInstancesColumnM22,
            kTlasInstancesColumnM23,
            kTlasInstancesColumnM31,
            kTlasInstancesColumnM32,
            kTlasInstancesColumnM33,
        });

        view->setModel(this);

        return model;
    }

    bool TlasInstancesProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
    {
        if (FilterSearchString(source_row, source_parent) == false)
        {
            return false;
        }
        return true;
    }

    bool TlasInstancesProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        int left_column  = left.column();
        int right_column = right.column();
        if (left_column == right_column)
        {
            switch (left_column)
            {
            case kTlasInstancesColumnInstanceAddress:
            case kTlasInstancesColumnInstanceOffset:
            {
                const uint64_t left_data  = left.data(Qt::UserRole).toULongLong();
                const uint64_t right_data = right.data(Qt::UserRole).toULongLong();
                return left_data < right_data;
            }

            case kTlasInstancesColumnXPosition:
            case kTlasInstancesColumnYPosition:
            case kTlasInstancesColumnZPosition:
            case kTlasInstancesColumnM11:
            case kTlasInstancesColumnM12:
            case kTlasInstancesColumnM13:
            case kTlasInstancesColumnM21:
            case kTlasInstancesColumnM22:
            case kTlasInstancesColumnM23:
            case kTlasInstancesColumnM31:
            case kTlasInstancesColumnM32:
            case kTlasInstancesColumnM33:
            {
                const float left_data  = left.data(Qt::UserRole).toFloat();
                const float right_data = right.data(Qt::UserRole).toFloat();
                return left_data < right_data;
            }

            default:
                break;
            }
        }

        return QSortFilterProxyModel::lessThan(left, right);
    }
}  // namespace rra
