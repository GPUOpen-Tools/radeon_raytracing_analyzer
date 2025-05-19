//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the BLAS triangles model.
//=============================================================================

#ifndef RRA_MODELS_BLAS_BLAS_TRIANGLES_MODEL_H_
#define RRA_MODELS_BLAS_BLAS_TRIANGLES_MODEL_H_

#include "qt_common/utils/model_view_mapper.h"

#include "models/blas/blas_triangles_item_model.h"
#include "models/blas/blas_triangles_proxy_model.h"

namespace rra
{
    /// @brief Enum containing indices for the widgets shared between the model and UI.
    enum BlasTrianglesWidgets
    {
        kTlasTrianglesBaseAddress,
        kBlasTrianglesBaseAddress,

        kBlasTrianglesNumWidgets,
    };

    /// @brief Container class that holds model data for the BLAS list pane.
    class BlasTrianglesModel : public ModelViewMapper
    {
    public:
        /// @brief Constructor.
        ///
        /// param [in] num_model_widgets the number of widgets that require updating by the model.
        explicit BlasTrianglesModel(int32_t num_model_widgets);

        /// @brief Destructor.
        virtual ~BlasTrianglesModel();

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
        /// @param [in] tlas_index  The index of the TLAS used to update the table.
        /// @param [in] blas_index  The index of the BLAS used to update the table.
        ///
        /// @return true if table populated, false if there are no triangles in the TLAS.
        bool UpdateTable(uint64_t tlas_index, uint64_t blas_index);

        /// @brief Find the triangle ModelIndex based on the node id.
        ///
        /// @param [in] triangle_node_id  The triangle node id to find.
        /// @param [in] blas_index        The index where the triangle is to be found.
        ///
        /// @return The model index of the table row corresponding to the node id passed in.
        QModelIndex FindTriangleIndex(uint32_t triangle_node_id, uint64_t blas_index) const;

        /// @brief Get the node ID for the triangle at a given row.
        ///
        /// @param row The row in the table.
        ///
        /// @return The node ID.
        uint32_t GetNodeId(int row) const;

        /// @brief Get the proxy model. Used to set up a connection between the table being sorted and the UI update.
        ///
        /// @return the proxy model.
        BlasTrianglesProxyModel* GetProxyModel() const;

    public slots:
        /// @brief Handle what happens when the search filter changes.
        ///
        /// @param [in] filter The search text filter.
        void SearchTextChanged(const QString& filter);

    private:
        BlasTrianglesItemModel*  table_model_;  ///< Holds the BLAS triangle list table data.
        BlasTrianglesProxyModel* proxy_model_;  ///< Proxy model for the BLAS triangle list table.
    };
}  // namespace rra

#endif  // RRA_MODELS_BLAS_BLAS_TRIANGLES_MODEL_H_

