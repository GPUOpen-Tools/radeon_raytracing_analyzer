//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of the BLAS viewer model.
//=============================================================================

#ifndef RRA_MODELS_BLAS_BLAS_VIEWER_MODEL_H_
#define RRA_MODELS_BLAS_BLAS_VIEWER_MODEL_H_

#include "qt_common/custom_widgets/arrow_icon_combo_box.h"
#include "qt_common/custom_widgets/scaled_tree_view.h"

#include "models/acceleration_structure_tree_view_model.h"
#include "models/acceleration_structure_viewer_model.h"
#include "models/acceleration_structure_flags_table_item_model.h"

namespace rra
{
    /// @brief Enum containing indices for the widgets shared between the model and UI.
    enum BlasStatsWidgets
    {
        kBlasStatsAddress,
        kBlasStatsType,
        kBlasStatsCurrentSAH,
        kBlasStatsSAHSubTreeMax,
        kBlasStatsSAHSubTreeMean,
        kBlasStatsPrimitiveIndexTriangle1,
        kBlasStatsPrimitiveIndexTriangle2,
        kBlasStatsGeometryIndex,
        kBlasStatsParent,

        kBlasStatsNumWidgets,
    };

    class BlasViewerModel : public AccelerationStructureViewerModel
    {
    public:
        /// @brief Explicit constructor.
        ///
        /// @param [in] tree_view The UI representation of the tree view.
        explicit BlasViewerModel(ScaledTreeView* tree_view);

        /// @brief Destructor.
        virtual ~BlasViewerModel();

        /// @brief Initialize the vertex tables model, used by the vertex tables in the BLAS viewer left-side pane.
        ///
        /// @param [in] table_view_triangle_1  The table view widget.
        /// @param [in] table_view_triangle_2  The table view widget.
        void InitializeVertexTableModels(ScaledTableView* table_view_triangle_1, ScaledTableView* table_view_triangle_2);

        /// @brief Initialize the geometry flags table model.
        ///
        /// @param table_view The table view widget.
        void InitializeFlagsTableModel(ScaledTableView* table_view);

        /// @brief Populate the flags table.
        ///
        /// @param [in] flags_table The flags table to populate.
        /// @param [in] flags       The flags to display in the table.
        void PopulateFlagsTable(FlagsTableItemModel* flags_table, uint32_t flags);

        /// @brief Get the number of BLAS's in the loaded trace.
        ///
        /// @param [out] out_count A pointer to receive the number of BLAS's.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode AccelerationStructureGetCount(uint64_t* out_count) const override;

        /// @brief Get the base address for the acceleration structure index given.
        ///
        /// @param [in]  index       The index of the BLAS to use.
        /// @param [out] out_address A pointer to receive the base address of the BLAS.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode AccelerationStructureGetBaseAddress(uint64_t index, uint64_t* out_address) const override;

        /// @brief Get the total number of nodes for the index given.
        ///
        /// The total number of nodes is the sum of the internal nodes and leaf nodes.
        ///
        /// @param [in]  index          The index of the BLAS to use.
        /// @param [out] out_node_count A pointer to receive the total node count of the BLAS.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode AccelerationStructureGetTotalNodeCount(uint64_t index, uint64_t* out_node_count) const override;

        /// @brief Is the BLAS empty.
        ///
        /// @param [in]  index  The index of the BLAS to use.
        ///
        /// @return true if empty, false if not.
        virtual bool AccelerationStructureGetIsEmpty(uint64_t index) const override;

        /// @brief Get the function pointer of the function that gets the child node.
        ///
        /// @return The function pointer.
        virtual GetChildNodeFunction AccelerationStructureGetChildNodeFunction() const override;

        /// @brief Update the UI elements based on what is selected in the tree view.
        ///
        /// @param [in] model_index The model index of the item selected in the tree view.
        /// @param [in] index       The index of the BLAS selected (from the combo box).
        virtual void UpdateUI(const QModelIndex& model_index, uint64_t index) override;

        /// @brief Reset the model to its default (empty) state.
        ///
        /// @param [in] reset_scene Should the scene be reset? Should be set to true when
        /// closing a trace file.
        virtual void ResetModelValues(bool reset_scene) override;

        /// @brief Set the scene selection by providing a tree view model index.
        ///
        /// @param [in] model_index The model index of the item selected in the tree view.
        /// @param [in] index       The index of the acceleration structure selected (from the combo box).
        virtual void SetSceneSelection(const QModelIndex& model_index, uint64_t index) override;

        /// @brief Get whether the selected node is a triangle node.
        ///
        /// @returns True if selected node is a triangle node, false otherwise.
        virtual bool SelectedNodeIsLeaf() const override;

        /// @brief Update internal state tracking whether the last selected node is a leaf node.
        ///
        /// @param [in] model_index The instance node model index.
        /// @param [in] index The BVH index.
        virtual void UpdateLastSelectedNodeIsLeaf(const QModelIndex& model_index, uint64_t index) override;

        /// @brief Get the address in a format to be displayed by the UI.
        ///
        /// @param tlas_index The index of the current BVH.
        /// @param node_id The ID of the selected node.
        ///
        /// @return A string ready to be displayed by the UI.
        virtual QString AddressString(uint64_t bvh_index, uint32_t node_id) const override;

        /// @brief Get the number of procedural nodes in this BLAS.
        ///
        /// @param [in] The index of the BLAS to get the procedural node count of.
        /// @returns The number of procedural nodes in the BLAS.
        uint32_t GetProceduralNodeCount(uint64_t blas_index) const;

        /// @brief Check if the current node has a second triangle.
        ///
        /// @returns True if the current node has a second triangle.
        bool SelectedNodeHasSecondTriangle() const;

    private:
        /// @brief Update the statistics for the selected BLAS node.
        ///
        /// @param [in] blas_index      The index of the BLAS to use.
        /// @param [in] node_id         The selected node in the BLAS.
        void UpdateStatistics(uint64_t blas_index, uint32_t node_id);

        bool                 last_selected_node_is_tri_         = false;    ///< True if last selected node is triangle node.
        bool                 last_selected_node_has_second_tri_ = false;    ///< True if last selected node has second triangle.
        QStandardItemModel*  vertex_table_model_triangle_1_     = nullptr;  ///< Model associated with the vertex table for triangle 1.
        QStandardItemModel*  vertex_table_model_triangle_2_     = nullptr;  ///< Model associated with the vertex table for triangle 2.
        FlagsTableItemModel* geometry_flags_table_model_        = nullptr;  ///< Model associated with the geometry flags table.
    };
}  // namespace rra

#endif  // RRA_MODELS_BLAS_BLAS_VIEWER_MODEL_H_
