//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the TLAS partitions model.
//=============================================================================

#ifndef RRA_MODELS_TLAS_TLAS_PARTITIONS_MODEL_H_
#define RRA_MODELS_TLAS_TLAS_PARTITIONS_MODEL_H_

#include "qt_common/utils/model_view_mapper.h"

#include "models/partitions_item_model.h"
#include "models/partitions_proxy_model.h"

namespace rra
{
    /// @brief Enum containing indices for the widgets shared between the model and UI.
    enum TlasPartitionsWidgets
    {
        kTlasPartitionsTlasBaseAddress,

        kTlasPartitionsNumWidgets,
    };

    /// @brief Container class that holds model data for the partitions list pane.
    class TlasPartitionsModel : public ModelViewMapper
    {
    public:
        /// @brief Constructor.
        ///
        /// param [in] num_model_widgets the number of widgets that require updating by the model.
        explicit TlasPartitionsModel(int32_t num_model_widgets);

        /// @brief Destructor.
        virtual ~TlasPartitionsModel();

        /// @brief Initialize blank data for the model.
        void ResetModelValues();

        /// @brief Initialize the table model.
        ///
        /// @param [in] table_view  The view to the table.
        /// @param [in] num_rows    Total rows of the table.
        /// @param [in] num_columns Total columns of the table.
        void InitializeTableModel(QTableView* table_view, uint num_rows, uint num_columns);

        /// @brief Update the partitions table.
        ///
        /// Only needs to be done when loading in a new trace.
        ///
        /// @param [in] tlas_index  The index of the TLAS used to update the table.
        ///
        /// @return true if table populated, false if the TLAS is not partitioned.
        bool UpdateTable(uint64_t tlas_index);

        /// @brief Get the partition index from the underlying proxy model.
        ///
        /// The index can't be directly returned since the rows may be different
        /// due to sorting.
        ///
        /// @param [in] model_index The model index of the item selected in the table.
        ///
        /// @return The partition index, or -1 on error.
        int32_t GetPartitionIndex(const QModelIndex& model_index);

        /// @brief Get the proxy model. Used to set up a connection between the table being sorted and the UI update.
        ///
        /// @return the proxy model.
        PartitionsProxyModel* GetProxyModel() const;

    public slots:
        /// @brief Handle what happens when the search filter changes.
        ///
        /// @param [in] filter The search text filter.
        void SearchTextChanged(const QString& filter);

    private:
        PartitionsItemModel*  table_model_;  ///< Holds the partition list table data.
        PartitionsProxyModel* proxy_model_;  ///< Proxy model for the partition list table.
    };
}  // namespace rra

#endif  // RRA_MODELS_TLAS_TLAS_PARTITIONS_MODEL_H_

