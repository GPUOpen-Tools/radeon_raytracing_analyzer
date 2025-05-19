//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for a proxy filter that processes the BLAS list table.
//=============================================================================

#ifndef RRA_MODELS_TLAS_BLAS_LIST_PROXY_MODEL_H_
#define RRA_MODELS_TLAS_BLAS_LIST_PROXY_MODEL_H_

#include <QTableView>

#include "models/table_proxy_model.h"
#include "models/tlas/blas_list_item_model.h"

namespace rra
{
    /// @brief Class to filter out and sort the BLAS list table.
    class BlasListProxyModel : public TableProxyModel
    {
        Q_OBJECT

    public:
        /// @brief Constructor.
        ///
        /// @param [in] parent The parent widget.
        explicit BlasListProxyModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~BlasListProxyModel();

        /// @brief Initialize the acceleration structure table model.
        ///
        /// @param [in] view        The table view.
        /// @param [in] num_rows    The table row count.
        /// @param [in] num_columns The table column count.
        ///
        /// @return the model for the BLAS table model.
        BlasListItemModel* InitializeAccelerationStructureTableModels(QTableView* view, int num_rows, int num_columns);

        /// @brief Get data from the model.
        ///
        /// @param [in] index  The model index.
        /// @param [in] role   The role.
        ///
        /// @return The data.
        virtual QVariant data(const QModelIndex& index, int role) const override;

        /// @brief Set whether BLASes in the list should be hidden based on their allow update flag.
        ///
        /// @param filter If true, BLASes are only shown if they contain the allow update flag.
        void SetFilterByAllowUpdate(bool filter);

        /// @brief Set whether BLASes in the list should be hidden based on their allow compaction flag.
        ///
        /// @param filter If true, BLASes are only shown if they contain the allow compaction flag.
        void SetFilterByAllowCompaction(bool filter);

        /// @brief Set whether BLASes in the list should be hidden based on their low memory flag.
        ///
        /// @param filter If true, BLASes are only shown if they contain the low memory flag.
        void SetFilterByLowMemory(bool filter);

        /// @brief Set whether BLASes in the list should be hidden based on their fast build flag.
        ///
        /// @param filter If true, BLASes are only shown if they contain the fast build flag.
        void SetFilterByFastBuild(bool filter);

        /// @brief Set whether BLASes in the list should be hidden based on their fast trace flag.
        ///
        /// @param filter If true, BLASes are only shown if they contain the fast trace flag.
        void SetFilterByFastTrace(bool filter);

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
        bool filter_by_allow_update_     = false;  ///< Hide BLASes in list if they don't have allow update flag.
        bool filter_by_allow_compaction_ = false;  ///< Hide BLASes in list if they don't have allow compaction flag.
        bool filter_by_low_memory_       = false;  ///< Hide BLASes in list if they don't have low memory flag.
        bool filter_by_fast_build_       = false;  ///< Hide BLASes in list if they don't have fast build flag.
        bool filter_by_fast_trace_       = false;  ///< Hide BLASes in list if they don't have fast trace flag.
    };
}  // namespace rra

#endif  // RRA_MODELS_TLAS_BLAS_LIST_PROXY_MODEL_H_

