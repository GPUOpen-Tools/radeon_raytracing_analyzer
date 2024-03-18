//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of an acceleration structure tree-view model.
//=============================================================================

#ifndef RRA_MODELS_ACCELERATION_STRUCTURE_TREE_VIEW_MODEL_H_
#define RRA_MODELS_ACCELERATION_STRUCTURE_TREE_VIEW_MODEL_H_

#include <QAbstractItemModel>

#include "public/rra_error.h"

#include "models/acceleration_structure_tree_view_item.h"

namespace rra
{
    /// @brief Typedef for acceleration structure specific function to get child nodes.
    typedef RraErrorCode (*GetChildNodeFunction)(uint64_t, uint32_t, uint32_t, uint32_t*);

    class AccelerationStructureTreeViewModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        /// @brief Explicit constructor.
        ///
        /// @param [in] is_tlas Does this model represent a TLAS.
        /// @param [in] parent Pointer to parent object (the UI control this model is used to provide data for).
        explicit AccelerationStructureTreeViewModel(bool is_tlas, QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~AccelerationStructureTreeViewModel();

        /// @brief Set up the model data. Add the data to the TreeView.
        ///
        /// @param [in] node_count The number of nodes required.
        /// @param [in] index      The index into the acceleration structure.
        /// @param [in] fn         A function pointer to a function to get the child nodes for the acceleration structure.
        ///
        /// @return true if initialization was successful, false otherwise.
        bool InitializeModel(uint64_t node_count, uint32_t index, GetChildNodeFunction fn);

        // Overridden QAbstractItemModel methods.

        /// @brief Overridden rowCount member function.
        ///
        /// @param [in] parent The parent model index.
        int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

        /// @brief Overridden columnCount member function.
        ///
        /// @param [in] parent The parent model index.
        int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

        /// @brief Overridden data member function.
        ///
        /// @param [in] index The model index.
        /// @param [in] role  The display role.
        ///
        /// @return A QVariant object containing the data.
        QVariant data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;

        /// @brief Overridden index member function.
        ///
        /// @param [in] row    The row.
        /// @param [in] column The column.
        /// @param [in] parent The parent model index.
        ///
        /// @return The index of the specified item.
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

        /// Overridden parent member function.
        ///
        /// @param [in] child The model index of the child item to find the parent of.
        ///
        /// @return The parent model index.
        QModelIndex parent(const QModelIndex& child) const Q_DECL_OVERRIDE;

        /// @brief Get the tree view model index for the given node id.
        ///
        /// @param [in] node_id The BVH node id to get the tree model index of.
        ///
        /// @returns The tree view model index associated with the given node id.
        QModelIndex GetModelIndexForNode(uint32_t node_id);

        /// @brief Get the tree view model index for the given node id and triangle index if triangle index is applicable.
        ///
        /// @param [in] node_id The BVH node id to get the tree model index of.
        /// @param [in] triangle_index The possible triangle index under the node.
        ///
        /// @returns The tree view model index associated with the given node id and triangle index.
        QModelIndex GetModelIndexForNodeAndTriangle(uint32_t node_id, uint32_t triangle_index);

        /// @brief Reset any values in the model to their default state.
        void ResetModelValues();

        /// @brief Get all the nodes in the tree view.
        ///
        /// @return The node ids in the tree view.
        std::vector<uint32_t> GetAllNodeIds() const;

    private:
        /// @brief Claim a chunk of pre-allocated memory for a treeview item and initialize the item.
        ///
        /// @param [in] node_data The data for the item.
        /// @param [in] parent    Pointer to the parent item (or nullptr if no parent).
        ///
        /// @return Pointer to the initialized EventTreeViewItem object.
        AccelerationStructureTreeViewItem* AllocateMemory(uint32_t node_data, AccelerationStructureTreeViewItem* parent);

        /// @brief Gets a tree view item from a given index.
        ///
        /// @param [in] index The index of the tree view item to retrieve.
        ///
        /// @return A pointer to the AccelerationStructureTreeViewItem at the specified index, or nullptr if the index is invalid.
        AccelerationStructureTreeViewItem* GetItemAtIndex(uint32_t index) const;

        std::map<uint32_t, AccelerationStructureTreeViewItem*> node_data_to_item_;  ///< The map used to associate node data with a treeview item.
        AccelerationStructureTreeViewItem*                     root_item_;          ///< The item at the root of the tree.
        AccelerationStructureTreeViewItem*                     item_buffer_;        ///< Allocated memory block for EventTreeViewItem objects.
        uint64_t                                               item_buffer_size_;   ///< Number of AccelerationStructureTreeViewItem the memory block can hold.
        uint32_t                                               buffer_item_index_;  ///< Index to next block of free memory.
        bool                                                   is_tlas_;            ///< Does this treeview model represent a TLAS?
        uint64_t                                               as_index_;  ///< The acceleration structure index, obtained from the treeview combo box index.
    };
}  // namespace rra

#endif  // RRA_MODELS_ACCELERATION_STRUCTURE_TREE_VIEW_MODEL_H_
