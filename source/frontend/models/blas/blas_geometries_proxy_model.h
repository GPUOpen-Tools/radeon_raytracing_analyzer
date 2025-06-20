//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for a proxy filter that processes the BLAS geometries table.
//=============================================================================

#ifndef RRA_MODELS_BLAS_BLAS_GEOMETRIES_PROXY_MODEL_H_
#define RRA_MODELS_BLAS_BLAS_GEOMETRIES_PROXY_MODEL_H_

#include <QTableView>

#include "models/blas/blas_geometries_item_model.h"
#include "models/table_proxy_model.h"

namespace rra
{
    /// @brief Class to filter out and sort the BLAS list table.
    class BlasGeometriesProxyModel : public TableProxyModel
    {
        Q_OBJECT

    public:
        /// @brief Constructor.
        ///
        /// @param [in] parent The parent widget.
        explicit BlasGeometriesProxyModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~BlasGeometriesProxyModel();

        /// @brief Initialize the geometry table model.
        ///
        /// @param [in] view        The table view.
        /// @param [in] num_rows    The table row count.
        /// @param [in] num_columns The table column count.
        ///
        /// @return the model for the BLAS table model.
        BlasGeometriesItemModel* InitializeAccelerationStructureTableModels(QTableView* view, int num_rows, int num_columns);

        /// @brief Overridden sort function. Allows for disabling sorting on certain columns.
        ///
        /// @param [in] column The column to sort.
        /// @param [in] order  The sort order.
        void sort(int column, Qt::SortOrder order) override;

        /// @brief Get data from the model.
        ///
        /// @param [in] index  The model index.
        /// @param [in] role   The role.
        ///
        /// @return The data.
        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

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
    };
}  // namespace rra

#endif  // RRA_MODELS_BLAS_BLAS_GEOMETRIES_PROXY_MODEL_H_

