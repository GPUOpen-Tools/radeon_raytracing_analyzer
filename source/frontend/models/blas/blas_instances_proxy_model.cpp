//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a proxy filter that processes the BLAS instances
/// table.
//=============================================================================

#include "models/blas/blas_instances_proxy_model.h"

#include <QTableView>

#include "public/rra_assert.h"

#include "models/blas/blas_instances_item_model.h"

namespace rra
{
    BlasInstancesProxyModel::BlasInstancesProxyModel(QObject* parent)
        : TableProxyModel(parent)
    {
    }

    BlasInstancesProxyModel::~BlasInstancesProxyModel()
    {
    }

    BlasInstancesItemModel* BlasInstancesProxyModel::InitializeAccelerationStructureTableModels(QTableView* view, int num_rows, int num_columns)
    {
        BlasInstancesItemModel* model = new BlasInstancesItemModel();
        model->SetRowCount(num_rows);
        model->SetColumnCount(num_columns);

        setSourceModel(model);
        SetFilterKeyColumns({
            kBlasInstancesColumnInstanceAddress,
            kBlasInstancesColumnInstanceOffset,
            kBlasInstancesColumnXPosition,
            kBlasInstancesColumnYPosition,
            kBlasInstancesColumnZPosition,
            kBlasInstancesColumnM11,
            kBlasInstancesColumnM12,
            kBlasInstancesColumnM13,
            kBlasInstancesColumnM21,
            kBlasInstancesColumnM22,
            kBlasInstancesColumnM23,
            kBlasInstancesColumnM31,
            kBlasInstancesColumnM32,
            kBlasInstancesColumnM33,
        });

        view->setModel(this);

        return model;
    }

    bool BlasInstancesProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
    {
        if (FilterSearchString(source_row, source_parent) == false)
        {
            return false;
        }
        return true;
    }

    bool BlasInstancesProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        int left_column  = left.column();
        int right_column = right.column();
        if (left_column == right_column)
        {
            switch (left_column)
            {
            case kBlasInstancesColumnInstanceAddress:
            case kBlasInstancesColumnInstanceOffset:
            {
                const uint64_t left_data  = left.data(Qt::UserRole).toULongLong();
                const uint64_t right_data = right.data(Qt::UserRole).toULongLong();
                return left_data < right_data;
            }

            case kBlasInstancesColumnXPosition:
            case kBlasInstancesColumnYPosition:
            case kBlasInstancesColumnZPosition:
            case kBlasInstancesColumnM11:
            case kBlasInstancesColumnM12:
            case kBlasInstancesColumnM13:
            case kBlasInstancesColumnM21:
            case kBlasInstancesColumnM22:
            case kBlasInstancesColumnM23:
            case kBlasInstancesColumnM31:
            case kBlasInstancesColumnM32:
            case kBlasInstancesColumnM33:
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
