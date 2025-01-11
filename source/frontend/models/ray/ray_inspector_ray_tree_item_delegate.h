//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Definition of RRA's ray inspector ray tree item delegate.
//=============================================================================

#ifndef RRA_MODELS_RAY_INSPECTOR_TABLE_ITEM_DELEGATE_H_
#define RRA_MODELS_RAY_INSPECTOR_TABLE_ITEM_DELEGATE_H_

#include "models/table_item_delegate.h"
#include <QStyledItemDelegate>
#include "qt_common/custom_widgets/scaled_tree_view.h"

class RayInspectorRayTreeItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    /// @brief Constructor.
    ///
    /// @param [in] parent The parent object.
    RayInspectorRayTreeItemDelegate(QObject* parent = 0);

    /// @brief Constructor with table view.
    /// @param table_view
    /// @param parent
    RayInspectorRayTreeItemDelegate(ScaledTreeView* tree_view, QObject* parent = 0);

    /// @brief Destructor.
    virtual ~RayInspectorRayTreeItemDelegate();

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
    virtual bool CheckboxAt(int row, int column) const;

    /// @brief Check to see if the alignment should reflect hierarchy.
    /// @param [in] row     The row to check.
    /// @param [in] column  The column to check.
    /// @return True if hierarchy should be preserved
    virtual bool HierarchyAlignment(int row, int column) const;

    /// @brief Custom painting for the flag table item.
    ///
    /// @param painter The Qt painter.
    /// @param option  The Qt option.
    /// @param index   The Qt index.
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

#endif  // RRA_MODELS_RAY_LIST_TABLE_ITEM_DELEGATE_H_
