//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a proxy filter that processes the BLAS triangles
/// table.
//=============================================================================

#include "models/blas/blas_triangles_proxy_model.h"

#include <QTableView>

#include "public/rra_assert.h"

#include "models/blas/blas_triangles_item_model.h"

namespace rra
{
    BlasTrianglesProxyModel::BlasTrianglesProxyModel(QObject* parent)
        : TableProxyModel(parent)
    {
    }

    BlasTrianglesProxyModel::~BlasTrianglesProxyModel()
    {
    }

    BlasTrianglesItemModel* BlasTrianglesProxyModel::InitializeAccelerationStructureTableModels(QTableView* view, int num_rows, int num_columns)
    {
        BlasTrianglesItemModel* model = new BlasTrianglesItemModel();
        model->SetRowCount(num_rows);
        model->SetColumnCount(num_columns);

        setSourceModel(model);
        SetFilterKeyColumns({
            kBlasTrianglesColumnPrimitiveIndex,
            kBlasTrianglesColumnNodeAddress,
            kBlasTrianglesColumnNodeOffset,
            kBlasTrianglesColumnGeometryIndex,
            kBlasTrianglesColumnActive,
            kBlasTrianglesColumnTriangleSurfaceArea,
            kBlasTrianglesColumnSAH,
            kBlasTrianglesColumnVertex0,
            kBlasTrianglesColumnVertex1,
            kBlasTrianglesColumnVertex2,
        });

        view->setModel(this);

        return model;
    }

    bool BlasTrianglesProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
    {
        if (FilterSearchString(source_row, source_parent) == false)
        {
            return false;
        }
        return true;
    }

    void BlasTrianglesProxyModel::sort(int column, Qt::SortOrder order)
    {
        switch (column)
        {
            // Disable sorting on the vertex columns.
        case kBlasTrianglesColumnVertex0:
        case kBlasTrianglesColumnVertex1:
        case kBlasTrianglesColumnVertex2:
            return;

        default:
            QSortFilterProxyModel::sort(column, order);
        }
    }

    bool BlasTrianglesProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        int left_column  = left.column();
        int right_column = right.column();
        if (left_column == right_column)
        {
            switch (left_column)
            {
            case kBlasTrianglesColumnNodeAddress:
            case kBlasTrianglesColumnNodeOffset:
            {
                const uint64_t left_data  = left.data(Qt::UserRole).toULongLong();
                const uint64_t right_data = right.data(Qt::UserRole).toULongLong();
                return left_data < right_data;
            }
            case kBlasTrianglesColumnPrimitiveIndex:
            case kBlasTrianglesColumnGeometryIndex:
            {
                const uint32_t left_data  = left.data(Qt::UserRole).toUInt();
                const uint32_t right_data = right.data(Qt::UserRole).toUInt();
                return left_data < right_data;
            }

            case kBlasTrianglesColumnTriangleSurfaceArea:
            case kBlasTrianglesColumnSAH:
            {
                const float left_data  = left.data(Qt::UserRole).toFloat();
                const float right_data = right.data(Qt::UserRole).toFloat();
                return left_data < right_data;
            }

            case kBlasTrianglesColumnActive:
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
