//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the BLAS list model.
//=============================================================================

#ifndef RRA_MODELS_TLAS_BLAS_LIST_MODEL_H_
#define RRA_MODELS_TLAS_BLAS_LIST_MODEL_H_

#include "qt_common/utils/model_view_mapper.h"

#include "models/tlas/blas_list_item_model.h"
#include "models/tlas/blas_list_proxy_model.h"

namespace rra
{
    /// @brief Enum containing indices for the widgets shared between the model and UI.
    enum BlasListWidgets
    {
        kBlasListNumWidgets,
    };

    /// @brief Container class that holds model data for the BLAS list pane.
    class BlasListModel : public ModelViewMapper
    {
    public:
        /// @brief Constructor.
        ///
        /// param [in] num_model_widgets the number of widgets that require updating by the model.
        explicit BlasListModel(int32_t num_model_widgets);

        /// @brief Destructor.
        virtual ~BlasListModel();

        /// @brief Initialize blank data for the model.
        void ResetModelValues();

        /// @brief Initialize the table model.
        ///
        /// @param [in] table_view  The view to the table.
        /// @param [in] num_rows    Total rows of the table.
        /// @param [in] num_columns Total columns of the table.
        void InitializeTableModel(QTableView* table_view, uint num_rows, uint num_columns);

        /// @brief Update the acceleration structure table.
        ///
        /// Only needs to be done when loading in a new trace.
        ///
        /// param [in] tlas_index  The index of the TLAS used to update the table.
        void UpdateTable(uint64_t tlas_index);

        /// @brief Get the BLAS index from the underlying proxy model.
        ///
        /// The index can't be directly returned since the rows may be different
        /// due to sorting.
        ///
        /// @param [in] model_index The model index of the item selected in the table.
        ///
        /// @ return The BLAS index as it appears in the BLAS viewer combo box, or -1 if
        /// error.
        int32_t GetBlasIndex(const QModelIndex& model_index);

        /// @brief Get the model index of the row in the table corresponding to the BLAS index.
        ///
        /// The row will need to come from the underlying proxy model.
        ///
        /// @param [in] blas_index The blas index.
        ///
        /// @return the model index of the row in the table where this blas_index is located.
        QModelIndex GetTableModelIndex(uint64_t blas_index) const;

        /// @brief Get the proxy model. Used to set up a connection between the table being sorted and the UI update.
        ///
        /// @return the proxy model.
        BlasListProxyModel* GetProxyModel() const;

    public slots:
        /// @brief Handle what happens when the search filter changes.
        ///
        /// @param [in] filter The search text filter.
        void SearchTextChanged(const QString& filter);

    private:
        BlasListItemModel*  table_model_;  ///< Holds the BLAS list table data.
        BlasListProxyModel* proxy_model_;  ///< Proxy model for the BLAS list table.
    };
}  // namespace rra

#endif  // RRA_MODELS_TLAS_BLAS_LIST_MODEL_H_

