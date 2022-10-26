//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Implementation of RRA's triangle list table item delegate.
//=============================================================================

#include "models/blas/blas_triangles_table_item_delegate.h"

#include "models/blas/blas_triangles_item_model.h"

TrianglesTableItemDelegate::TrianglesTableItemDelegate(QObject* parent)
    : TableItemDelegate(parent)
{
}

TrianglesTableItemDelegate::TrianglesTableItemDelegate(ScaledTableView* table_view, QObject* parent)
    : TableItemDelegate(table_view, parent)
{
}

TrianglesTableItemDelegate::~TrianglesTableItemDelegate()
{
}

bool TrianglesTableItemDelegate::Vec3At(int row, int column) const
{
    Q_UNUSED(row);
    switch (column)
    {
    case rra::kBlasTrianglesColumnVertex0:
    case rra::kBlasTrianglesColumnVertex1:
    case rra::kBlasTrianglesColumnVertex2:
    case rra::kBlasTrianglesColumnVertex3:
        return true;

    default:
        return false;
    }
}
