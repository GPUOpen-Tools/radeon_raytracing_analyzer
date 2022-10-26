//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Definition of RRA's triangle list table item delegate.
//=============================================================================

#ifndef RRA_MODELS_TRIANGLES_TABLE_ITEM_DELEGATE_H_
#define RRA_MODELS_TRIANGLES_TABLE_ITEM_DELEGATE_H_

#include "models/table_item_delegate.h"

class ScaledTableView;

class TrianglesTableItemDelegate : public TableItemDelegate
{
    Q_OBJECT
public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent object.
    TrianglesTableItemDelegate(QObject* parent = 0);

    /// @brief Constructor.
    ///
    /// @param [in] parent The parent object.
    /// @param [in] table_view The table view that contains this widget.
    TrianglesTableItemDelegate(ScaledTableView* table_view, QObject* parent = 0);

    /// @brief Destructor.
    virtual ~TrianglesTableItemDelegate();

    /// @brief Test if there is a vec3 at the given row and column.
    ///
    /// @param [in] row     The row to check.
    /// @param [in] column  The column to check.
    ///
    /// @return true if a vec3 is in the specified cell, false if not.
    virtual bool Vec3At(int row, int column) const override;
};

#endif  // RRA_MODELS_TRIANGLES_TABLE_ITEM_DELEGATE_H_
