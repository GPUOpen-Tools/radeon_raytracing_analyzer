//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief Definition of RRA's ray list table item delegate.
//=============================================================================

#ifndef RRA_MODELS_RAY_LIST_TABLE_ITEM_DELEGATE_H_
#define RRA_MODELS_RAY_LIST_TABLE_ITEM_DELEGATE_H_

#include "models/table_item_delegate.h"

class RayListTableItemDelegate : public TableItemDelegate
{
    Q_OBJECT
public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent object.
    RayListTableItemDelegate(QObject* parent = 0);

    /// @brief Destructor.
    virtual ~RayListTableItemDelegate();
};

#endif  // RRA_MODELS_RAY_LIST_TABLE_ITEM_DELEGATE_H_

