//=============================================================================
// Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Definition of RRA's geometry list table item delegate.
//=============================================================================

#ifndef RRA_MODELS_GEOMETRIES_TABLE_ITEM_DELEGATE_H_
#define RRA_MODELS_GEOMETRIES_TABLE_ITEM_DELEGATE_H_

#include "models/table_item_delegate.h"

class ScaledTableView;

class GeometriesTableItemDelegate : public TableItemDelegate
{
    Q_OBJECT
public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent object.
    GeometriesTableItemDelegate(QObject* parent = 0);

    /// @brief Constructor.
    ///
    /// @param [in] parent The parent object.
    /// @param [in] table_view The table view that contains this widget.
    GeometriesTableItemDelegate(ScaledTableView* table_view, QObject* parent = 0);

    /// @brief Destructor.
    virtual ~GeometriesTableItemDelegate();

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
    bool CheckboxAt(int row, int column) const;
};

#endif  // RRA_MODELS_GEOMETRIES_TABLE_ITEM_DELEGATE_H_
