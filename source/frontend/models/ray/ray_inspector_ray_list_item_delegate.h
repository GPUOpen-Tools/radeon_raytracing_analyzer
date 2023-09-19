//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Definition of RRA's ray inspector ray list table item delegate.
//=============================================================================

#ifndef RRA_MODELS_RAY_INSPECTOR_TABLE_ITEM_DELEGATE_H_
#define RRA_MODELS_RAY_INSPECTOR_TABLE_ITEM_DELEGATE_H_

#include "models/table_item_delegate.h"

class RayInspectorRayTableItemDelegate : public TableItemDelegate
{
    Q_OBJECT
public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent object.
    RayInspectorRayTableItemDelegate(QObject* parent = 0);

    /// @brief Destructor.
    virtual ~RayInspectorRayTableItemDelegate();

    /// @brief Test if there is a checkbox at the given row and column.
    ///
    /// Called from the paint function to use a custom painter to draw checkboxes.
    /// This function can be overriden in derived classes to specify rows and
    /// columns containing checkboxes. By default, no checkboxes are present.
    ///
    /// @param [in] row     The row to check.
    /// @param [in] column  The column to check.
    ///
    /// @return true if a checkbox is in the specified cell, false if not.
    virtual bool CheckboxAt(int row, int column) const override;
};

#endif  // RRA_MODELS_RAY_LIST_TABLE_ITEM_DELEGATE_H_
