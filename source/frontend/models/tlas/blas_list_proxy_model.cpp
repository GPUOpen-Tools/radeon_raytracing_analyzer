//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a proxy filter that processes the BLAS list
/// table.
//=============================================================================

#include <math.h>

#include "models/tlas/blas_list_proxy_model.h"

#include <QTableView>

#include "public/rra_assert.h"

#include "models/tlas/blas_list_item_model.h"

namespace rra
{
    BlasListProxyModel::BlasListProxyModel(QObject* parent)
        : TableProxyModel(parent)
    {
    }

    BlasListProxyModel::~BlasListProxyModel()
    {
    }

    BlasListItemModel* BlasListProxyModel::InitializeAccelerationStructureTableModels(QTableView* view, int num_rows, int num_columns)
    {
        BlasListItemModel* model = new BlasListItemModel();
        model->SetRowCount(num_rows);
        model->SetColumnCount(num_columns);

        setSourceModel(model);
        SetFilterKeyColumns({
            kBlasListColumnAddress,
            kBlasListColumnAllowUpdate,
            kBlasListColumnAllowCompaction,
            kBlasListColumnLowMemory,
            kBlasListColumnBuildType,
            kBlasListColumnInstanceCount,
            kBlasListColumnNodeCount,
            kBlasListColumnBoxCount,
            kBlasListColumnBox32Count,
            kBlasListColumnBox16Count,
            kBlasListColumnTriangleNodeCount,
            kBlasListColumnProceduralNodeCount,
            kBlasListColumnRootSAH,
            kBlasListColumnMinSAH,
            kBlasListColumnMeanSAH,
            kBlasListColumnMaxDepth,
            kBlasListColumnAvgDepth,
            kBlasListColumnBlasIndex,
        });

        view->setModel(this);

        return model;
    }

    bool BlasListProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
    {
        if (FilterSearchString(source_row, source_parent) == false)
        {
            return false;
        }
        return true;
    }

    bool BlasListProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        int left_column  = left.column();
        int right_column = right.column();
        if (left_column == right_column)
        {
            switch (left_column)
            {
            case kBlasListColumnAddress:
            case kBlasListColumnInstanceCount:
            case kBlasListColumnNodeCount:
            case kBlasListColumnBoxCount:
            case kBlasListColumnBlasIndex:
            {
                const uint64_t left_data  = left.data(Qt::UserRole).toULongLong();
                const uint64_t right_data = right.data(Qt::UserRole).toULongLong();
                return left_data < right_data;
            }

            case kBlasListColumnRootSAH:
            case kBlasListColumnMinSAH:
            case kBlasListColumnMeanSAH:
            {
                const float left_data  = left.data(Qt::UserRole).toFloat();
                const float right_data = right.data(Qt::UserRole).toFloat();

                if (isnan(right_data))
                {
                    return false;
                }

                if (isnan(left_data))
                {
                    return true;
                }

                return left_data < right_data;
            }

            case kBlasListColumnAllowUpdate:
            case kBlasListColumnAllowCompaction:
            case kBlasListColumnLowMemory:
            case kBlasListColumnBuildType:
            case kBlasListColumnBox32Count:
            case kBlasListColumnBox16Count:
            case kBlasListColumnTriangleNodeCount:
            case kBlasListColumnProceduralNodeCount:
            case kBlasListColumnMemoryUsage:
            case kBlasListColumnMaxDepth:
            case kBlasListColumnAvgDepth:
            {
                const uint32_t left_data  = left.data(Qt::UserRole).toUInt();
                const uint32_t right_data = right.data(Qt::UserRole).toUInt();
                return left_data < right_data;
            }
            default:
                break;
            }
        }

        return QSortFilterProxyModel::lessThan(left, right);
    }
}  // namespace rra
