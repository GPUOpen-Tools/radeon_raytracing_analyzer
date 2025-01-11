//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Implementation of RRA's geometry list table item delegate.
//=============================================================================

#include "models/blas/blas_geometries_table_item_delegate.h"

#include "models/blas/blas_geometries_item_model.h"

GeometriesTableItemDelegate::GeometriesTableItemDelegate(QObject* parent)
    : TableItemDelegate(parent)
{
}

GeometriesTableItemDelegate::GeometriesTableItemDelegate(ScaledTableView* table_view, QObject* parent)
    : TableItemDelegate(table_view, parent)
{
}

GeometriesTableItemDelegate::~GeometriesTableItemDelegate()
{
}

bool GeometriesTableItemDelegate::CheckboxAt(int row, int column) const
{
    Q_UNUSED(row);
    switch (column)
    {
    case rra::kBlasGeometriesColumnGeometryFlagOpaque:
    case rra::kBlasGeometriesColumnGeometryFlagNoDuplicateAnyHit:
        return true;

    default:
        return false;
    }
}
