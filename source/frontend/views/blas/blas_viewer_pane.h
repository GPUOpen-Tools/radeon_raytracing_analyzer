//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the BLAS viewer pane.
//=============================================================================

#ifndef RRA_VIEWS_BLAS_BLAS_VIEWER_PANE_H_
#define RRA_VIEWS_BLAS_BLAS_VIEWER_PANE_H_

#include "ui_blas_viewer_pane.h"

#include "models/acceleration_structure_flags_table_item_delegate.h"
#include "models/blas/blas_viewer_model.h"
#include "views/acceleration_structure_viewer_pane.h"

namespace rra
{
    class BlasSceneCollectionModel;
}

/// @brief Class declaration.
class BlasViewerPane : public AccelerationStructureViewerPane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit BlasViewerPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~BlasViewerPane();

    /// @brief Overridden window show event.
    ///
    /// @param [in] event The show event object.
    virtual void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;

    /// @brief Trace closed.
    virtual void OnTraceClose() Q_DECL_OVERRIDE;

    /// @brief Trace open.
    virtual void OnTraceOpen() Q_DECL_OVERRIDE;

    /// @brief Select the leaf node under the mouse. This will be a triangle node.
    ///
    /// @param [in] blas_index The blas index selected.
    /// @param [in] navigate_to_triangles_pane  If true, navigate to the Triangles pane.
    virtual void SelectLeafNode(const uint64_t blas_index, const bool navigate_to_triangles_pane) Q_DECL_OVERRIDE;

    /// @brief Get the BLAS scene collection model.
    ///
    /// @return The scene model collection.
    rra::BlasSceneCollectionModel* GetSceneCollection();

    /// @brief Set the BLAS root nodes.
    ///
    /// @param blas_root_nodes The root nodes.
    void SetBlasRootNodes(std::vector<rra::SceneNode*>* blas_root_nodes);

    /// @brief Unwind one level of CLAS drill-down (walk back CLAS -> CBLAS within the BLAS pane).
    ///
    /// CLAS visualization is nested instancing (TLAS -> CBLAS -> CLAS), all shown in the BLAS pane. Since
    /// drilling CBLAS -> CLAS stays in the same pane, the global (pane-based) navigation manager can't
    /// distinguish these levels. The main window consults this before the navigation manager on Back so the
    /// back-stack walks CLAS -> CBLAS -> TLAS. Non-CLAS navigation never populates the drill stack.
    ///
    /// @return true if a drill level was unwound (Back consumed); false if there's nothing to unwind.
    bool PopClusterDrillBack();

protected:
    /// @brief Updates widgets depending on the model.
    ///
    /// @param [in] index The model index of the selected node.
    virtual void UpdateWidgets(const QModelIndex& index) override;

    /// @brief Get the BLAS root nodes.
    ///
    /// @return The BLAS root nodes.
    virtual std::vector<rra::SceneNode*>* GetBlasRootNodes() override;

    /// @brief Update the tooltip in the renderer widget.
    ///
    /// @param [in] tool_tip_string  The tooltip text to be displayed.
    virtual void UpdateToolTip(QString tool_tip_string) override;

private slots:
    /// @brief Slot to handle whan happens when one of the slider handles is moved.
    ///
    /// @param [in] min_value The value of the left (minimum) slider handle.
    /// @param [in] max_value The value of the right (maximum) slider handle.
    void UpdateTreeDepths(int min_value, int max_value);

    /// @brief Slot to select a triangle in the currently viewed BLAS.
    ///
    /// @param [in] triangle_node_index The triangle node index selected.
    void SelectTriangle(uint64_t triangle_node_id);

    /// @brief Select the parent of the currently selected node.
    ///
    /// @param [in] checked The checked state of the button (unused).
    void SelectParentNode(bool checked);

    /// @brief Update the triangles pane when the selected tree node has been changed.
    ///
    /// @param [in] selected The selected model indices.
    /// @param [in] deselected The deselected model indices.
    void TreeNodeChanged(const QItemSelection& selected, const QItemSelection& deselected);

private:
    /// @brief Update the pane after the selected BLAS has been changed.
    ///
    /// Calls the base class and emits a signal indicating the BLAS index has changed.
    void UpdateSelectedBlas();

    /// @brief Set the AS-type label ("BLAS:", "CBLAS:", or "CLAS:") for the currently inspected AS.
    void UpdateBvhTypeLabel();

    /// @brief Set the BLAS index in the BLAS combo box.
    ///
    /// Selecting the index in the BLAS combo box should force a redraw of
    /// the UI.
    ///
    /// @param [in] blas_index The index to select in the combo box.
    void SetBlasSelection(uint64_t blas_index);

    /// @brief Drill into the CLAS referenced by a Cluster BLAS (CBLAS) cluster-ref leaf, switching the inspected AS.
    ///
    /// A CLAS is itself a BLAS index, so this stays within the BLAS pane and switches the inspected AS to the CLAS.
    ///
    /// @param [in] clas_blas_index The BLAS index of the referenced CLAS.
    void SelectClusterBlas(uint64_t clas_blas_index);

    /// @brief Switch the inspected AS to the given BLAS/CLAS index within the BLAS pane (no drill-stack push).
    ///
    /// Shared by the forward drill (SelectClusterBlas) and the Back unwind (PopClusterDrillBack).
    ///
    /// @param [in] blas_index The BLAS/CLAS index to inspect.
    void SwitchToClusterBlas(uint64_t blas_index);

    /// @brief Select the leaf node under the mouse. This will be a triangle node.
    ///
    /// @param [in] navigate_to_triangles_pane  If true, navigate to the Triangles pane.
    void SelectLeafNode(const bool navigate_to_triangles_pane);

    /// @brief Show the buttons linking to the triangle node's siblings.
    ///
    /// @param [in] scene The current scene.
    /// @param [in] blas_index The current BLAS index.
    /// @param [in] geometry_index Geometry index of the selected triangle.
    /// @param [in] primitive_index Primitive index of the selected triangle.
    /// @param [in] node_id The ID of the selected node.
    void UpdateTriangleSplitUI(rra::Scene* scene, uint32_t blas_index, uint32_t geometry_index, uint32_t primitive_index, uint32_t node_id);

    /// @brief Update the pane based on the color theme.
    void OnColorThemeUpdated();

    Ui::BlasViewerPane*            ui_;                       ///< Pointer to the Qt UI design.
    rra::BlasViewerModel*          derived_model_ = nullptr;  ///< Pointer to the model. This is a copy of the model in the base class and is deleted there.
    FlagTableItemDelegate*         flag_table_delegate_ = nullptr;   ///< Delegate for drawing the geometry flags table.
    std::vector<ScaledPushButton*> split_triangle_sibling_buttons_;  ///< The buttons to navigate to split triangle siblings for split triangles.
    std::vector<QWidget*>          triangle_widgets_;                ///< A list of dynamic triangle widgets in the pane which need manual deletion.

    std::vector<uint64_t> cluster_drill_stack_;   ///< AS indices to return to when unwinding CLAS drill-down (parent per drill level).
    bool                  in_cluster_nav_ = false;  ///< True while a CLAS drill/unwind is switching the AS, so the drill stack isn't reset by the resulting BlasSelected.
};

#endif  // RRA_VIEWS_BLAS_BLAS_VIEWER_PANE_H_

