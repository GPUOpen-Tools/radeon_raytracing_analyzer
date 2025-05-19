//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for a proxy filter that processes the ray inspector ray tree.
//=============================================================================

#ifndef RRA_MODELS_RAY_INSPECTOR_RAY_LIST_PROXY_MODEL_H_
#define RRA_MODELS_RAY_INSPECTOR_RAY_LIST_PROXY_MODEL_H_

#include <QTableView>

#include "models/ray/ray_inspector_ray_tree_model.h"
#include "models/tree_view_proxy_model.h"

namespace rra
{
    /// @brief Class to filter out and sort the BLAS list table.
    class RayInspectorRayTreeProxyModel : public TreeViewProxyModel
    {
        Q_OBJECT

    public:
        /// @brief Constructor.
        ///
        /// @param [in] parent The parent widget.
        explicit RayInspectorRayTreeProxyModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~RayInspectorRayTreeProxyModel();

        /// @brief Initialize the ray table model.
        ///
        /// @param [in] view        The table view.
        /// @param [in] num_rows    The table row count.
        /// @param [in] num_columns The table column count.
        ///
        /// @return the model for the ray table model.
        RayInspectorRayTreeModel* InitializeRayTreeModels(ScaledTreeView* view);

    protected:
        /// @brief Make the filter run across multiple columns.
        ///
        /// @param [in] source_row    The target row.
        /// @param [in] source_parent The source parent.
        ///
        /// @return true if the row passed the filter, false if not.
        virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

        /// @brief The sort comparator.
        ///
        /// @param [in] left  The left item to compare.
        /// @param [in] right The right item to compare.
        ///
        /// @return true if left is less than right, false otherwise.
        virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

    private:
    };
}  // namespace rra

#endif  // RRA_MODELS_RAY_INSPECTOR_RAY_LIST_PROXY_MODEL_H_

