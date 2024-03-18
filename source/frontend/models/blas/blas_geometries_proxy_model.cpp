//=============================================================================
// Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a proxy filter that processes the BLAS geometries
/// table.
//=============================================================================

#include "models/blas/blas_geometries_proxy_model.h"

#include <QTableView>

#include "public/rra_assert.h"

#include "models/blas/blas_geometries_item_model.h"

namespace rra
{
    BlasGeometriesProxyModel::BlasGeometriesProxyModel(QObject* parent)
        : TableProxyModel(parent)
    {
    }

    BlasGeometriesProxyModel::~BlasGeometriesProxyModel()
    {
    }

    BlasGeometriesItemModel* BlasGeometriesProxyModel::InitializeAccelerationStructureTableModels(QTableView* view, int num_rows, int num_columns)
    {
        BlasGeometriesItemModel* model = new BlasGeometriesItemModel();
        model->SetRowCount(num_rows);
        model->SetColumnCount(num_columns);

        setSourceModel(model);
        SetFilterKeyColumns({
            kBlasGeometriesColumnGeometryIndex,
            kBlasGeometriesColumnGeometryFlagOpaque,
            kBlasGeometriesColumnGeometryFlagNoDuplicateAnyHit,
            kBlasGeometriesColumnPrimitiveCount,
        });

        view->setModel(this);

        return model;
    }

    bool BlasGeometriesProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
    {
        if (FilterSearchString(source_row, source_parent) == false)
        {
            return false;
        }
        return true;
    }

    void BlasGeometriesProxyModel::sort(int column, Qt::SortOrder order)
    {
        QSortFilterProxyModel::sort(column, order);
    }

    bool BlasGeometriesProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        int left_column  = left.column();
        int right_column = right.column();
        if (left_column == right_column)
        {
            switch (left_column)
            {
            case kBlasGeometriesColumnPrimitiveCount:
            case kBlasGeometriesColumnGeometryIndex:
            {
                const uint32_t left_data  = left.data(Qt::UserRole).toUInt();
                const uint32_t right_data = right.data(Qt::UserRole).toUInt();
                return left_data < right_data;
            }

            case kBlasGeometriesColumnGeometryFlagOpaque:
            case kBlasGeometriesColumnGeometryFlagNoDuplicateAnyHit:
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
