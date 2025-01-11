//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Definition of RRA's instance flag table item delegate.
//=============================================================================

#ifndef RRA_MODELS_TLAS_INSTANCE_FLAGS_TABLE_ITEM_DELEGATE_H_
#define RRA_MODELS_TLAS_INSTANCE_FLAGS_TABLE_ITEM_DELEGATE_H_

#include "models/table_item_delegate.h"

class FlagTableItemDelegate : public TableItemDelegate
{
    Q_OBJECT
public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent object.
    FlagTableItemDelegate(QObject* parent = 0);

    /// @brief Destructor.
    virtual ~FlagTableItemDelegate();

    /// @brief Custom painting for the flag table item.
    ///
    /// @param painter The Qt painter.
    /// @param option  The Qt option.
    /// @param index   The Qt index.
    virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

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

#endif  // RRA_MODELS_TLAS_INSTANCE_FLAGS_TABLE_ITEM_DELEGATE_H_
