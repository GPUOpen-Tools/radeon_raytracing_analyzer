//=============================================================================
// Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Implementation of RRA's blas list table item delegate.
//=============================================================================

#include "models/tlas/blas_list_table_item_delegate.h"

#include "models/tlas/blas_list_model.h"

BlasListTableItemDelegate::BlasListTableItemDelegate(QObject* parent)
    : TableItemDelegate(parent)
{
}

BlasListTableItemDelegate::~BlasListTableItemDelegate()
{
}

bool BlasListTableItemDelegate::CheckboxAt(int row, int column) const
{
    Q_UNUSED(row);
    switch (column)
    {
    case rra::kBlasListColumnAllowCompaction:
    case rra::kBlasListColumnAllowUpdate:
    case rra::kBlasListColumnLowMemory:
        return true;

    default:
        return false;
    }
}
