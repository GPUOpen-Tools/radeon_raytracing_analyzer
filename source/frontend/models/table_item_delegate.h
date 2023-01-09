//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Definition of RRA's table item delegate.
//=============================================================================

#ifndef RRA_MODELS_TABLE_ITEM_DELEGATE_H_
#define RRA_MODELS_TABLE_ITEM_DELEGATE_H_

#include <QAbstractItemDelegate>

class ScaledTableView;

/// @brief table item delegate - determines table cells are painted. Intended
/// to provide base-class functionality.
class TableItemDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent object.
    TableItemDelegate(QObject* parent = 0);

    /// @brief Constructor.
    ///
    /// @param [in] parent The parent object.
    /// @param [in] table_view The table view that contains this widget.
    TableItemDelegate(ScaledTableView* table_view, QObject* parent = 0);

    /// @brief Destructor.
    virtual ~TableItemDelegate();

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

    /// @brief Test if there is a vec3 at the given row and column.
    ///
    /// @param [in] row     The row to check.
    /// @param [in] column  The column to check.
    ///
    /// @return true if a vec3 is in the specified cell, false if not.
    virtual bool Vec3At(int row, int column) const;

    /// @brief Overridden size hint function for this item delegate - determines desired size of barriers table cell items.
    ///
    /// @param [in] option Qt style option parameter.
    /// @param [in] index  Qt model index parameter.
    ///
    /// @return The desired size of table cell items.
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const Q_DECL_OVERRIDE;

    /// @brief Overridden paint function for this item delegate - determines how individual cells are painted.
    ///
    /// @param [in] painter The painter object to use.
    /// @param [in] option  Qt style option parameter.
    /// @param [in] index   Qt model index parameter.
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const Q_DECL_OVERRIDE;

private:
    ScaledTableView* table_view_{};  ///< The ScaledTableView that contains this delegate.
};

#endif  // RRA_MODELS_TABLE_ITEM_DELEGATE_H_
