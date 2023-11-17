//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of the TLAS viewer model.
//=============================================================================

#ifndef RRA_MODELS_TLAS_TLAS_VIEWER_MODEL_H_
#define RRA_MODELS_TLAS_TLAS_VIEWER_MODEL_H_

#include "qt_common/custom_widgets/arrow_icon_combo_box.h"
#include "qt_common/custom_widgets/scaled_tree_view.h"

#include "models/acceleration_structure_tree_view_model.h"
#include "models/acceleration_structure_viewer_model.h"
#include "models/acceleration_structure_flags_table_item_model.h"

namespace rra
{
    /// @brief Enum containing indices for the widgets shared between the model and UI.
    enum TlasStatsWidgets
    {
        kTlasStatsAddress,
        kTlasStatsType,
        kTlasStatsBlasAddress,
        kTlasStatsParent,
        kTlasStatsInstanceIndex,
        kTlasStatsInstanceId,
        kTlasStatsInstanceMask,
        kTlasStatsInstanceHitGroupIndex,
        kTlasStatsFocus,

        kTlasStatsNumWidgets,
    };

    class TlasViewerModel : public AccelerationStructureViewerModel
    {
    public:
        /// @brief Explicit constructor.
        ///
        /// @param [in] tree_view The UI representation of the tree view.
        explicit TlasViewerModel(ScaledTreeView* tree_view);

        /// @brief Destructor.
        virtual ~TlasViewerModel();

        /// @brief Initialize the transform table model, used by the transform table in the TLAS viewer left-side pane.
        ///
        /// @param [in] table_view  The table view widget.
        void InitializeTransformTableModel(ScaledTableView* table_view);

        /// @brief Initialize the position table model, used by the position table in the TLAS viewer left-side pane.
        ///
        /// @param [in] table_view  The table view widget.
        void InitializePositionTableModel(ScaledTableView* table_view);

        /// @brief Get the number of TLAS's in the loaded trace.
        ///
        /// @param [out] out_count A pointer to receive the number of TLAS's.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode AccelerationStructureGetCount(uint64_t* out_count) const override;

        /// @brief Get the base address for the acceleration structure index given.
        ///
        /// @param [in]  index       The index of the TLAS to use.
        /// @param [out] out_address A pointer to receive the base address of the TLAS.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode AccelerationStructureGetBaseAddress(uint64_t index, uint64_t* out_address) const override;

        /// @brief Get the total number of nodes for the index given.
        ///
        /// The total number of nodes is the sum of the internal nodes and leaf nodes.
        ///
        /// @param [in]  index          The index of the TLAS to use.
        /// @param [out] out_node_count A pointer to receive the total node count of the TLAS.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode AccelerationStructureGetTotalNodeCount(uint64_t index, uint64_t* out_node_count) const override;

        /// @brief Is the TLAS empty.
        ///
        /// @param [in]  index  The index of the TLAS to use.
        ///
        /// @return true if empty, false if not.
        virtual bool AccelerationStructureGetIsEmpty(uint64_t index) const override;

        /// @brief Get the function pointer of the function that gets the child node.
        ///
        /// @return The function pointer.
        virtual GetChildNodeFunction AccelerationStructureGetChildNodeFunction() const override;

        /// @brief Get the BLAS index from the TLAS index and instance node data.
        ///
        /// @param [in] tlas_index   The index of the TLAS to use.
        /// @param [in] model_index  The model index of the TLAS instance selected in the UI.
        ///
        /// @return The BLAS index.
        uint64_t GetBlasIndex(int tlas_index, const QModelIndex& model_index) const;

        /// @brief Get the instance index from the TLAS index and instance node data.
        ///
        /// @param [in] tlas_index   The index of the TLAS to use.
        /// @param [in] model_index  The model index of the instance selected in the UI.
        ///
        /// @return The instance index.
        uint32_t GetInstanceIndex(int tlas_index, const QModelIndex& model_index) const;

        /// @brief Get the unique instance index from the TLAS index and instance node id.
        ///
        /// @param [in] tlas_index   The index of the TLAS to use.
        /// @param [in] node_id  The node id of an instance node.
        ///
        /// @return The unique instance index.
        uint32_t GetInstanceUniqueIndexFromNode(int tlas_index, const uint32_t node_id) const;

        /// @brief Get the instance index from the TLAS index and instance node id.
        ///
        /// @param [in] tlas_index   The index of the TLAS to use.
        /// @param [in] node_id  The node id of an instance node.
        ///
        /// @return The instance index.
        uint32_t GetInstanceIndexFromNode(int tlas_index, const uint32_t node_id) const;

        /// @brief Ensure that a BLAS is valid.
        ///
        /// Make sure the index is valid and it isn't empty.
        ///
        /// @param blas_index The index of the BLAS to check.
        ///
        /// @return true if the BLAS is valid, false if not.
        uint64_t BlasValid(uint64_t blas_index) const;

        /// @brief Update the UI elements based on what is selected in the tree view.
        ///
        /// @param [in] model_index The model index of the item selected in the tree view.
        /// @param [in] index       The index of the TLAS selected (from the combo box).
        virtual void UpdateUI(const QModelIndex& model_index, uint64_t index) override;

        /// @brief Initialize the flags table model, used by the flags table in the viewer left-side pane.
        ///
        /// @param [in] table_view  The table view widget.
        void InitializeFlagsTableModel(ScaledTableView* table_view);

        /// @brief Populate the flags table.
        ///
        /// @param [in] flags The flags to display in the table.
        void PopulateFlagsTable(uint32_t flags);

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

        /// @brief Get whether the selected node is an instance.
        ///
        /// @returns True if selected node is an instance, false otherwise.
        virtual bool SelectedNodeIsLeaf() const override;

        /// @brief Update internal state tracking whether the last selected node is a leaf node.
        ///
        /// param [in] model_index The instance node model index.
        /// param [in] index The BVH index.
        virtual void UpdateLastSelectedNodeIsLeaf(const QModelIndex& model_index, uint64_t index) override;

        /// @brief Get the address in a format to be displayed by the UI.
        ///
        /// @param tlas_index The index of the current BVH.
        /// @param node_id The ID of the selected node.
        ///
        /// @return A string ready to be displayed by the UI.
        virtual QString AddressString(uint64_t bvh_index, uint32_t node_id) const override;

    private:
        bool                 last_selected_node_is_instance_ = false;    ///< True if the last selected node is an instance node.
        QStandardItemModel*  transform_table_model_          = nullptr;  ///< Model associated with the transform table.
        QStandardItemModel*  position_table_model_           = nullptr;  ///< Model associated with the position table.
        FlagsTableItemModel* flags_table_model_              = nullptr;  ///< Model associated with the flags table.
    };
}  // namespace rra

#endif  // RRA_MODELS_TLAS_TLAS_VIEWER_MODEL_H_
