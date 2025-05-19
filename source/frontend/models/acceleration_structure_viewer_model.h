//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of an acceleration structure viewer model base class.
//=============================================================================

#ifndef RRA_MODELS_ACCELERATION_STRUCTURE_VIEWER_MODEL_H_
#define RRA_MODELS_ACCELERATION_STRUCTURE_VIEWER_MODEL_H_

#include <QStandardItemModel>

#include "qt_common/custom_widgets/arrow_icon_combo_box.h"
#include "qt_common/custom_widgets/scaled_table_view.h"
#include "qt_common/custom_widgets/scaled_tree_view.h"
#include "qt_common/utils/model_view_mapper.h"

#include "public/camera.h"
#include "public/render_state_adapter.h"
#include "public/renderer_interface.h"

#include "models/acceleration_structure_tree_view_item_delegate.h"
#include "models/acceleration_structure_tree_view_model.h"
#include "models/scene_collection_model.h"
#include "models/tree_view_proxy_model.h"

namespace rra
{
    class ViewerIO;

    /// @brief Get the scene info needed for creating the graphics context.
    /// @return Scene info needed by graphics context.
    std::shared_ptr<renderer::GraphicsContextSceneInfo> GetGraphicsContextSceneInfo();

    class AccelerationStructureViewerModel : public ModelViewMapper
    {
        Q_OBJECT

    public:
        /// @brief Explicit constructor.
        ///
        /// @param [in] tree_view   The UI representation of the tree view.
        /// @param [in] num_widgets The number of widgets that need to be updated by the model.
        /// @param [in] is_tlas     Does this model represent a TLAS.
        explicit AccelerationStructureViewerModel(ScaledTreeView* tree_view, uint32_t num_widgets, bool is_tlas);

        /// @brief Destructor.
        virtual ~AccelerationStructureViewerModel();

        /// @brief Expand/Collapse state for treeview.
        enum TreeViewExpandMode
        {
            kCollapsed = 0,  ///< Collapse all entries.
            kExpanded  = 1   ///< Expand all entries.
        };

        /// @brief Initialize the extents table model, used by the extents table in the viewer left-side pane.
        ///
        /// @param [in] table_view  The table view widget.
        void InitializeExtentsTableModel(ScaledTableView* table_view);

        /// @brief Initialize the rotation table model, used by the rotation table in the viewer left-side pane.
        ///
        /// @param [in] table_view  The table view widget.
        void InitializeRotationTableModel(ScaledTableView* table_view);

        /// @brief Populate the rotation table.
        ///
        /// @param [in] rotation The node rotation.
        void PopulateRotationTable(const glm::mat3& rotation);

        /// @brief Connect the incoming map of RendererAdapter instances with the model.
        ///
        /// @param [in] adapters A renderer adapter map used to alter various render states.
        void SetRendererAdapters(const rra::renderer::RendererAdapterMap& adapters);

        /// @brief Get the number of acceleration structures in the loaded trace.
        ///
        /// @param [out] out_count A pointer to receive the number of acceleration structures.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode AccelerationStructureGetCount(uint64_t* out_count) const = 0;

        /// @brief Get the base address for the index given.
        ///
        /// @param [in]  index       The index of the acceleration structure to use.
        /// @param [out] out_address A pointer to receive the base address of the acceleration structure.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode AccelerationStructureGetBaseAddress(uint64_t index, uint64_t* out_address) const = 0;

        /// @brief Get the total number of nodes for the index given.
        ///
        /// The total number of nodes is the sum of the internal nodes and leaf nodes.
        ///
        /// @param [in]  index          The index of the acceleration structure to use.
        /// @param [out] out_node_count A pointer to receive the total node count of the acceleration structure.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode AccelerationStructureGetTotalNodeCount(uint64_t index, uint64_t* out_node_count) const = 0;

        /// @brief Is the acceleration structure empty.
        ///
        /// @param [in]  index  The index of the acceleration structure to use.
        ///
        /// @return true if empty, false if not.
        virtual bool AccelerationStructureGetIsEmpty(uint64_t index) const = 0;

        /// @brief Get the function pointer of the function that gets the child node.
        ///
        /// Will be different for TLAS/BLAS.
        ///
        /// @return The function pointer.
        virtual GetChildNodeFunction AccelerationStructureGetChildNodeFunction() const = 0;

        /// @brief Update the UI elements based on what is selected in the tree view.
        ///
        /// @param [in] model_index The model index of the item selected in the tree view.
        /// @param [in] index       The index of the acceleration structure selected (from the combo box).
        virtual void UpdateUI(const QModelIndex& model_index, uint64_t index) = 0;

        /// @brief Refresh the UI elements based on what is selected in the tree view.
        ///
        /// @param [in] index The index of the acceleration structure selected (from the combo box).
        void RefreshUI(uint64_t index);

        /// @brief Reset the model to its default (empty) state.
        ///
        /// @param [in] reset_scene Should the scene be reset? Should be set to true when
        /// closing a trace file.
        virtual void ResetModelValues(bool reset_scene);

        /// @brief Set the scene selection by providing a tree view model index.
        ///
        /// @param [in] model_index The model index of the item selected in the tree view.
        /// @param [in] index       The index of the acceleration structure selected (from the combo box).
        virtual void SetSceneSelection(const QModelIndex& model_index, uint64_t index) = 0;

        /// @brief Get the address in a format to be displayed by the UI.
        ///
        /// @param tlas_index The index of the current BVH.
        /// @param node_id The ID of the selected node.
        ///
        /// @return A string ready to be displayed by the UI.
        virtual QString AddressString(uint64_t bvh_index, uint32_t node_id) const = 0;

        /// @brief Clear the current scene object selection.
        ///
        /// @param [in] index       The index of the acceleration structure selected (from the combo box).
        void ClearSelection(uint64_t index);

        /// @brief Set up the list of acceleration structures in the combo box.
        ///
        /// @param [in] combo_box The combo box in which to add the list of acceleration structures.
        void SetupAccelerationStructureList(ArrowIconComboBox* combo_box);

        /// @brief Get the acceleration structure index from the given combo box.
        ///
        /// @param [in] combo_box The combo box.
        ///
        /// @returns The acceleration structure index.
        uint64_t FindAccelerationStructureIndex(ArrowIconComboBox* combo_box);

        /// @brief Get the combobox row corresponding to a given BLAS index.
        ///
        /// @param [in] blas_index The BLAS index to look for.
        ///
        /// @return The combo box row corresponding to the BLAS index.
        int FindRowFromAccelerationStructureIndex(uint64_t blas_index);

        /// @brief Populate the treeview with data from the current acceleration structure.
        ///
        /// @param [in] index The acceleration structure index to display.
        void PopulateTreeView(uint64_t index);

        /// @brief Populate the scene with rendering data from the BVH at the given index.
        ///
        /// @param [in] renderer The renderer to use to display the BVH scene.
        /// @param [in] index The index of the BVH to load.
        void PopulateScene(renderer::RendererInterface* renderer, uint64_t index);

        /// @brief Select from the scene given normalized window coordinates.
        ///
        /// @param [in] scene_index The scene index to select from.
        /// @param [in] camera The camera to use to calculate coordinate projection (using camera).
        /// @param [in] normalized_window_coords The normalized coordinates to select with. The coordinate space is from -1.0 to 1.0.
        ///
        /// @returns The closest hit information.
        SceneCollectionModelClosestHit SelectFromScene(uint64_t scene_index, const renderer::Camera* camera, glm::vec2 normalized_window_coords);

        /// @brief Get the tree model index associated with the node.
        ///
        /// @param [in] node_id The acceleration structure node id.
        ///
        /// @returns The tree model index for the given node.
        QModelIndex GetModelIndexForNode(uint32_t node_id) const;

        /// @brief Get the tree model index associated with the node and a triangle if applicable.
        ///
        /// @param [in] node_id The acceleration structure node id.
        /// @param [in] triangle_index The index of the triangle if this node has more than one triangles.
        ///
        /// @returns The tree model index for the given node and its triangle.
        QModelIndex GetModelIndexForNodeAndTriangle(uint32_t node_id, uint32_t triangle_index) const;

        /// @brief Get the all-scenes model.
        ///
        /// @returns The all-scenes model.
        SceneCollectionModel* GetSceneCollectionModel() const;

        /// @brief Toggle the instance transform wireframe rendering.
        void ToggleInstanceTransformWireframe();

        /// @brief Toggle the BVH wireframe rendering.
        void ToggleBVHWireframe();

        /// @brief Toggle the Mesh wireframe rendering.
        void ToggleMeshWireframe();

        /// @brief Toggle the rendering og geometry.
        void ToggleRenderGeometry();

        /// @brief Adapts the counter range to the view.
        void AdaptTraversalCounterRangeToView();

        /// @brief Remember which node in the tree is selected.
        ///
        /// It may need to be redrawn if the user changes the node display type from offset <-> address.
        ///
        /// @param [in] model_index The model index of the item selected in the tree view (or invalid if
        /// nothing in the tree is selected.
        void SetSelectedNodeIndex(const QModelIndex& model_index);

        /// @brief Is the selected model index a valid instance node.
        ///
        /// @param [in] index  The index of the TLAS selected (from the combo box).
        ///
        /// @return true if model index is an instance node, false if not.
        bool IsInstanceNode(uint64_t tlas_index) const;

        /// @brief Is the selected model index a valid triangle node.
        ///
        /// @param [in] index  The index of the BLAS selected (from the combo box).
        ///
        /// @return true if model index is an triangle node, false if not.
        bool IsTriangleNode(uint64_t blas_index) const;

        /// @brief Is the selected model index a rebraided instance node.
        ///
        /// @param [in] index The index of the TLAS selected (from the combo box).
        ///
        /// @return true if model index is a rebraided instance node, false if not.
        bool IsRebraidedNode(uint64_t tlas_index) const;

        /// @brief Is the selected model index a split triangle node.
        ///
        /// @param [in] index The index of the TLAS selected (from the combo box).
        ///
        /// @return true if model index is a split triangle node, false if not.
        bool IsTriangleSplit(uint64_t tlas_index) const;

        /// @brief Get the current options avaiable for the selection in the given scene.
        ///
        /// @param [in] bvh_index The index of the scene to get the options for.
        /// @param [in] request The request that the options are requested on.
        ///
        /// @returns The selection context options with their corresponding functions.
        SceneContextMenuOptions GetSceneContextOptions(uint64_t bvh_index, SceneContextMenuRequest request);

        /// @brief Get whether the selected node is an instance.
        ///
        /// @returns True if selected node is an instance, false otherwise.
        virtual bool SelectedNodeIsLeaf() const = 0;

        /// @brief Update internal state tracking whether the last selected node is a leaf node.
        ///
        /// param [in] model_index The instance node model index.
        /// param [in] index The BVH index.
        virtual void UpdateLastSelectedNodeIsLeaf(const QModelIndex& model_index, uint64_t index) = 0;

        /// @brief Hides the currently selected node.
        void HideSelectedNodes(uint32_t bvh_index_);

        /// @brief Makes all the nodes in the scene visible.
        void ShowAllNodes(uint32_t bvh_index_);

        /// @brief Sets whether or not selecting multiple instance nodes is enabled.
        ///
        /// @param multi_select True if multiselect should be enabled.
        void SetMultiSelect(bool multi_select);

        /// @brief Sets the camera controller.
        ///
        /// @param controller The camera controller.
        void SetCameraController(rra::ViewerIO* controller);

        /// @brief Get the camera controller.
        ///
        /// @returns The camera controller.
        rra::ViewerIO* GetCameraController() const;

    public slots:
        /// @brief Slot to handle what happens when the user clicks on a the collapse/expand button.
        ///
        /// @param [in] index     See values in ExpandMode, above.
        void ExpandCollapseTreeView(int index);

        /// @brief Slot to handle what happens when the text in the search box changes.
        ///
        /// @param [in] search_text The text to be searched.
        void SearchTextChanged(const QString& search_text);

    signals:
        /// @brief Signal emitted when the scene selection has been changed.
        void SceneSelectionChanged();

    protected:
        /// @brief Get the node id associated with the model index.
        ///
        /// @param [in] model_index The model index of the item selected in the tree view.
        /// @param [in] index       The index of the acceleration structure selected (from the combo box).
        /// @param [in] is_tlas     A flag indicating if the model is a TLAS or BLAS.
        ///
        /// @returns The node id associated with the model index.
        uint32_t GetNodeIdFromModelIndex(const QModelIndex& model_index, uint64_t index, bool is_tlas) const;

        /// @brief Check if the given model index is a node.
        ///
        /// @param [in] model_index The model index of the item selected in the tree view.
        ///
        /// @returns True if the model index represents a node.
        bool IsModelIndexNode(const QModelIndex& model_index) const;

        /// @brief Populate the extents table.
        ///
        /// @param [in] bounding_volume_extents The extents bounding volume.
        void PopulateExtentsTable(const BoundingVolumeExtents& bounding_volume_extents);

        std::unordered_map<uint64_t, uint64_t> as_index_to_row_index_map_;          ///< A mapping of acceleration structure index to combo box index.
        SceneCollectionModel*                  scene_collection_model_  = nullptr;  ///< The scene model.
        rra::renderer::RenderStateAdapter*     render_state_adapter_    = nullptr;  ///< The adapter used to toggle mesh render states.
        Scene*                                 last_clicked_node_scene_ = nullptr;  ///< The last scene in which a node was clicked.
        ScaledTreeView*                        tree_view_               = nullptr;  ///< The scaled tree view.

    private:
        AccelerationStructureTreeViewModel* tree_view_model_       = nullptr;  ///< The model for the acceleration structure tree view.
        rra::ViewerIO*                      camera_controller_     = nullptr;  ///< The active camera controller.
        TreeViewProxyModel*                 tree_view_proxy_model_ = nullptr;  ///< The treeview proxy model, used for text search filtering.
        QStandardItemModel*                 extents_table_model_   = nullptr;  ///< Model associated with the extents table.
        QStandardItemModel*                 bottom_table_model_    = nullptr;  ///< Model associated with the rotation table.
        QModelIndex selected_node_index_;  ///< The model index for the selected node in the treeview (can be invalid - nothing selected).
        std::map<uint64_t, AccelerationStructureTreeViewItemDelegate*> item_delegate_map_;  ///< The item delegates for the tree view.
        TreeViewExpandMode treeview_expand_state_ = kCollapsed;                             ///< The state of the treeview (expanded/collapsed).
        bool               is_tlas_;                                                        ///< Is this a TLAS BVH.
    };
}  // namespace rra

#endif  // RRA_MODELS_ACCELERATION_STRUCTURE_VIEWER_MODEL_H_

