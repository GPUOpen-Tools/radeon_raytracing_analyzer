//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Implementation of RRA's ray inspector ray list item delegate.
//=============================================================================

#include "models/ray/ray_inspector_ray_list_item_delegate.h"
#include "models/ray/ray_inspector_model.h"

RayInspectorRayTableItemDelegate::RayInspectorRayTableItemDelegate(QObject* parent)
    : TableItemDelegate(parent)
{
}

RayInspectorRayTableItemDelegate::~RayInspectorRayTableItemDelegate()
{
}

bool RayInspectorRayTableItemDelegate::CheckboxAt(int row, int column) const
{
    Q_UNUSED(row);
    switch (column)
    {
    case rra::kRayInspectorRayListColumnHit:
        return true;

    default:
        return false;
    }
}
