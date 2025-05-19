//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Implementation of RRA's instance list table item delegate.
//=============================================================================

#include "models/instance_list_table_item_delegate.h"

#include "models/instances_item_model.h"

InstanceListTableItemDelegate::InstanceListTableItemDelegate(QObject* parent)
    : TableItemDelegate(parent)
{
}

InstanceListTableItemDelegate::~InstanceListTableItemDelegate()
{
}

bool InstanceListTableItemDelegate::CheckboxAt(int row, int column) const
{
    Q_UNUSED(row);
    switch (column)
    {
    case rra::kInstancesColumnCullDisableFlag:
    case rra::kInstancesColumnFlipFacingFlag:
    case rra::kInstancesColumnForceOpaqueFlag:
    case rra::kInstancesColumnForceNoOpaqueFlag:
        return true;

    default:
        return false;
    }
}

