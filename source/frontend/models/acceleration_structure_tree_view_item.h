//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of an acceleration structure (AS) tree-view item.
//=============================================================================

#ifndef RRA_MODELS_ACCELERATION_STRUCTURE_TREE_VIEW_ITEM_H_
#define RRA_MODELS_ACCELERATION_STRUCTURE_TREE_VIEW_ITEM_H_

#include <QAbstractItemModel>
#include <QString>
#include <QMetaObject>

namespace rra
{
    static const int kNumColumns     = 1;   ///< The number of columns for the Acceleration treeview.
    static const int kTreeViewIndent = 10;  ///< The indentation of the tree items.

    // @brief A data structure to pass to delegates. Note: Q_DECLARE_METATYPE at the bottom of this file.
    struct AccelerationStructureTreeViewItemData
    {
        uint32_t node_id = 0;
        QString  display_name;
    };

    class AccelerationStructureTreeViewItem
    {
    public:
        /// @brief Default constructor.
        AccelerationStructureTreeViewItem();

        /// @brief Destructor.
        ~AccelerationStructureTreeViewItem();

        /// @brief Initialize an acceleration structure tree view item.
        ///
        /// @param [in] node_data The raw data containing information about this node.
        /// @param [in] parent    A pointer to the parent item.
        void Initialize(uint32_t node_data, AccelerationStructureTreeViewItem* parent);

        /// @brief Initialize an acceleration structure tree view item.
        ///
        /// @param [in] non_node_index The index of this node under the parent node.
        /// @param [in] parent    A pointer to the parent item.
        void InitializeAsNonNode(uint32_t non_node_index, AccelerationStructureTreeViewItem* parent);

        /// @brief Append an item to the list of child items.
        ///
        /// @param [in] item The item to add to the list.
        void AppendChild(AccelerationStructureTreeViewItem* item);

        /// @brief Get child item at the specified row.
        ///
        /// @param [in] row The row of the child required.
        ///
        /// @return The child item.
        AccelerationStructureTreeViewItem* Child(int row) const;

        /// @brief Get the number of child items in the current item.
        ///
        /// @return The number of children.
        int ChildCount() const;

        /// @brief Get the number of columns in the current item.
        ///
        /// @return The number of columns.
        int ColumnCount() const;

        /// @brief Get the row that this item is in.
        ///
        /// @return The row this item is in.
        int Row() const;

        /// @brief Get the parent of this item.
        ///
        /// @return The parent item.
        AccelerationStructureTreeViewItem* ParentItem() const;

        /// @brief Check if this item is a node or not.
        ///
        /// @returns True if this item is a node.
        bool IsNode() const;

        /// @brief Get the data in this item.
        ///
        /// Passes in a role parameter so will work the same way as other Qt objects which
        /// return data depending on role (such as table entries).
        ///
        /// @param [in] column The column in the tree that the data is in.
        /// @param [in] role   The role of the data. For the DisplayRole, the
        /// data is returned as a string and this will be what is shown in the treeview. For
        /// the UserRole, the raw data is returned.
        /// @param [in] is_tlas Is the parent treeview displaying information from a TLAS.
        /// Used so different data can be shown in the treeview for TLAS and BLAS.
        /// @param [in] as_index The acceleration structure index, either the index for the
        /// TLAS or BLAS, depending on the is_tlas flag.
        ///
        /// @return The data, as a QVariant, depending on the role.
        QVariant Data(int column, int role, bool is_tlas, uint64_t as_index) const;

    private:
        QList<AccelerationStructureTreeViewItem*> child_items_{};   ///< A list of child items for this item.
        AccelerationStructureTreeViewItem*        parent_item_{};   ///< A pointer to the parent item.
        uint32_t                                  node_data_{};     ///< The encoded data contained in this item for column 0.
        bool                                      is_node_ = true;  ///< The indicator to describe if this item is not a node.
    };
}  // namespace rra

Q_DECLARE_METATYPE(rra::AccelerationStructureTreeViewItemData);  // Declare as QT meta type to be used by the delegate.

#endif  // RRA_MODELS_ACCELERATION_STRUCTURE_TREE_VIEW_ITEM_H_
