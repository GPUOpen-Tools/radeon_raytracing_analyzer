//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Implementation of RRA's blas list table item delegate.
//=============================================================================

#include "models/ray/ray_list_table_item_delegate.h"

#include "models/ray/ray_history_model.h"

RayListTableItemDelegate::RayListTableItemDelegate(QObject* parent)
    : TableItemDelegate(parent)
{
}

RayListTableItemDelegate::~RayListTableItemDelegate()
{
}
