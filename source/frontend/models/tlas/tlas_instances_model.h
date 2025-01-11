//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the TLAS instances model.
//=============================================================================

#ifndef RRA_MODELS_TLAS_TLAS_INSTANCES_MODEL_H_
#define RRA_MODELS_TLAS_TLAS_INSTANCES_MODEL_H_

#include "qt_common/utils/model_view_mapper.h"

#include "models/instances_item_model.h"
#include "models/instances_proxy_model.h"

namespace rra
{
    /// @brief Enum containing indices for the widgets shared between the model and UI.
    enum TlasInstancesWidgets
    {
        kTlasInstancesTlasBaseAddress,

        kTlasInstancesNumWidgets,
    };

    /// @brief A structure to keep track of blas and instance index coupling.
    struct TlasInstancesModelAddress
    {
        uint64_t blas_index;
        uint64_t instance_index;
    };

    /// @brief Container class that holds model data for the BLAS list pane.
    class TlasInstancesModel : public ModelViewMapper
    {
    public:
        /// @brief Constructor.
        ///
        /// param [in] num_model_widgets the number of widgets that require updating by the model.
        explicit TlasInstancesModel(int32_t num_model_widgets);

        /// @brief Destructor.
        virtual ~TlasInstancesModel();

        /// @brief Initialize blank data for the model.
        void ResetModelValues();

        /// @brief Initialize the table model.
        ///
        /// @param [in] table_view  The view to the table.
        /// @param [in] num_rows    Total rows of the table.
        /// @param [in] num_columns Total columns of the table.
        void InitializeTableModel(ScaledTableView* table_view, uint num_rows, uint num_columns);

        /// @brief Update the acceleration structure table.
        ///
        /// Only needs to be done when loading in a new trace.
        ///
        /// @param [in] tlas_index  The index of the TLAS used to update the table.
        /// @param [in] blas_index  The index of the BLAS used to update the table.
        ///
        /// @return true if table populated, false if there are no instances in the TLAS.
        bool UpdateTable(uint64_t tlas_index);

        /// @brief Get the TLAS instance index from the underlying proxy model.
        ///
        /// The index can't be directly returned since the rows may be different
        /// due to sorting.
        ///
        /// @param [in] model_index The model index of the item selected in the table.
        ///
        /// @ return The BLAS index as it appears in the BLAS viewer combo box, or -1 if
        /// error.
        int32_t GetInstanceIndex(const QModelIndex& model_index);

        /// @brief Get the model index of the row in the table corresponding to the instance index.
        ///
        /// The row will need to come from the underlying proxy model.
        ///
        /// @param [in] instance_index The instance index.
        ///
        /// @return the model index of the row in the table where this instance_index is located.
        QModelIndex GetTableModelIndex(uint64_t instance_index) const;

        /// @brief Get the blas index and blas instance index with the instance index.
        ///
        /// @param [in] model_index The model index of the item selected in the table.
        ///
        /// @return The address for the instance.
        TlasInstancesModelAddress GetInstanceAddress(int32_t instance_index);

        /// @brief Get the proxy model. Used to set up a connection between the table being sorted and the UI update.
        ///
        /// @return the proxy model.
        InstancesProxyModel* GetProxyModel() const;

    public slots:
        /// @brief Handle what happens when the search filter changes.
        ///
        /// @param [in] filter The search text filter.
        void SearchTextChanged(const QString& filter);

    private:
        InstancesItemModel*  table_model_;  ///< Holds the TLAS instance list table data.
        InstancesProxyModel* proxy_model_;  ///< Proxy model for the TLAS instance list table.
        std::map<int64_t, TlasInstancesModelAddress>
            addressable_instance_index_;  ///< A map to address a blas aligned instance index to a non aligned instance index.
    };
}  // namespace rra

#endif  // RRA_MODELS_TLAS_TLAS_INSTANCES_MODEL_H_
