//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for a proxy filter that processes the ray inspector ray table.
//=============================================================================

#ifndef RRA_MODELS_RAY_INSPECTOR_RAY_TABLE_PROXY_MODEL_H_
#define RRA_MODELS_RAY_INSPECTOR_RAY_TABLE_PROXY_MODEL_H_

#include <QTableView>

#include "models/table_proxy_model.h"
#include "models/ray/ray_inspector_ray_table_proxy_model.h"

namespace rra
{
    /// @brief Class to filter out and sort the BLAS list table.
    class RayInspectorRayTableProxyModel : public TableProxyModel
    {
        Q_OBJECT

    public:
        /// @brief Constructor.
        ///
        /// @param [in] parent The parent widget.
        explicit RayInspectorRayTableProxyModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~RayInspectorRayTableProxyModel();

        /// @brief Initialize the ray table model.
        ///
        /// @param [in] view        The table view.
        /// @param [in] num_rows    The table row count.
        /// @param [in] num_columns The table column count.
        ///
        /// @return the model for the ray table model.
        RayListItemModel* InitializeRayTableModels(QTableView* view, int num_rows, int num_columns);

        /// @brief Set whether rays in the list should be hidden based on their global invocation ID.
        ///
        /// @param min Filter rows with invocation IDs less than min.
        /// @param max Filter rows with invocation IDs greater than max.
        void SetFilterByInvocationId(GlobalInvocationID min, GlobalInvocationID max);

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
        GlobalInvocationID filter_min_{};  ///< Hide rows with a global invocaion ID less than this value.
        GlobalInvocationID filter_max_{};  ///< Hide rows with a global invocaion ID greater than this value.
    };
}  // namespace rra

#endif  // RRA_MODELS_RAY_INSPECTOR_RAY_TABLE_PROXY_MODEL_H_
