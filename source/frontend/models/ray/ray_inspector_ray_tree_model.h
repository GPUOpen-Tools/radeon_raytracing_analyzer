//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the ray inspector ray tree model.
///
//=============================================================================

#ifndef RRA_MODELS_RAY_INSPECTOR_RAY_TREE_MODEL_H_
#define RRA_MODELS_RAY_INSPECTOR_RAY_TREE_MODEL_H_

#include <QAbstractItemModel>
#include <optional>

#include "qt_common/custom_widgets/scaled_tree_view.h"
#include "public/rra_ray_history.h"
#include "ray_inspector_ray_tree_item.h"

namespace rra
{

    /// @brief A class to handle the model data associated with ray list table.
    class RayInspectorRayTreeModel : public QAbstractItemModel
    {
    public:
        /// @brief Constructor.
        explicit RayInspectorRayTreeModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~RayInspectorRayTreeModel();

        /// @brief Initialize the ray list table.
        ///
        /// An instance of this table is present in the TLAS and BLAS list panes.
        ///
        /// @param [in] ray_tree The tree to initialize.
        void Initialize(ScaledTreeView* ray_tree);

        /// @brief Add new rays to the tree.
        ///
        /// @param [in] item_list  The item list to add to the tree.
        void AddNewRays(RayTreeItemList item_list);

        /// @brief Clears all the rays.
        void ClearRays();

        // QAbstractItemModel overrides. See Qt documentation for parameter and return values
        virtual QVariant      data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;
        virtual Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual QVariant      headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
        virtual QModelIndex   index(int row, int column, const QModelIndex& parent) const Q_DECL_OVERRIDE;
        virtual QModelIndex   parent(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual int           rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
        virtual int           columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

    private:
        RayInspectorRayTreeItem* root_ = nullptr;  ///< Root item of the tree.
    };
}  // namespace rra

#endif  // RRA_MODELS_RAY_INSPECTOR_RAY_TREE_MODEL_H_
