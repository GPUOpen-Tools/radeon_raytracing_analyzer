//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the BLAS geometries model.
//=============================================================================

#ifndef RRA_MODELS_BLAS_BLAS_GEOMETRIES_MODEL_H_
#define RRA_MODELS_BLAS_BLAS_GEOMETRIES_MODEL_H_

#include "qt_common/custom_widgets/scaled_table_view.h"
#include "qt_common/utils/model_view_mapper.h"

#include "models/blas/blas_geometries_item_model.h"
#include "models/blas/blas_geometries_proxy_model.h"

namespace rra
{
    /// @brief Enum containing indices for the widgets shared between the model and UI.
    enum BlasGeometriesWidgets
    {
        kTlasGeometriesBaseAddress,
        kBlasGeometriesBaseAddress,

        kBlasGeometriesNumWidgets,
    };

    /// @brief Container class that holds model data for the BLAS list pane.
    class BlasGeometriesModel : public ModelViewMapper
    {
    public:
        /// @brief Constructor.
        ///
        /// param [in] num_model_widgets the number of widgets that require updating by the model.
        explicit BlasGeometriesModel(int32_t num_model_widgets);

        /// @brief Destructor.
        virtual ~BlasGeometriesModel();

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
        /// @return true if table populated, false if not.
        bool UpdateTable(uint64_t tlas_index, uint64_t blas_index);

        /// @brief Find the geometry ModelIndex based on the geometry index.
        ///
        /// @param [in] geometry_index  The geometry index to find.
        ///
        /// @return The model index of the table row corresponding to the node id passed in.
        QModelIndex FindGeometryIndex(uint32_t geometry_index) const;

        /// @brief Get the proxy model. Used to set up a connection between the table being sorted and the UI update.
        ///
        /// @return the proxy model.
        BlasGeometriesProxyModel* GetProxyModel() const;

    public slots:
        /// @brief Handle what happens when the search filter changes.
        ///
        /// @param [in] filter The search text filter.
        void SearchTextChanged(const QString& filter);

    private:
        BlasGeometriesItemModel*  table_model_;  ///< Holds the BLAS geometry list table data.
        BlasGeometriesProxyModel* proxy_model_;  ///< Proxy model for the BLAS geometry list table.
    };
}  // namespace rra

#endif  // RRA_MODELS_BLAS_BLAS_GEOMETRIES_MODEL_H_
