//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the TLAS viewer pane.
//=============================================================================

#ifndef RRA_VIEWS_TLAS_TLAS_VIEWER_PANE_H_
#define RRA_VIEWS_TLAS_TLAS_VIEWER_PANE_H_

#include "ui_tlas_viewer_pane.h"
#include <vector>

#include "models/tlas/tlas_viewer_model.h"
#include "views/acceleration_structure_viewer_pane.h"
#include "models/acceleration_structure_flags_table_item_delegate.h"

/// @brief Class declaration.
class TlasViewerPane : public AccelerationStructureViewerPane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit TlasViewerPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~TlasViewerPane();

    /// @brief Overridden window show event.
    ///
    /// @param [in] event The show event object.
    virtual void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;

    /// @brief Trace closed.
    virtual void OnTraceClose() Q_DECL_OVERRIDE;

    /// @brief Trace open.
    virtual void OnTraceOpen() Q_DECL_OVERRIDE;

    /// @brief Select the leaf node under the mouse. This will be an instance of a BLAS.
    ///
    /// @param [in] blas_index The blas index selected.
    /// @param [in] navigate_to_blas_pane  If true, navigate to the BLAS pane.
    virtual void SelectLeafNode(const uint64_t blas_index, const bool navigate_to_blas_pane) Q_DECL_OVERRIDE;

protected:
    /// @brief Updates widgets depending on the model.
    ///
    /// @param [in] index The model index of the selected node.
    virtual void UpdateWidgets(const QModelIndex& index) override;

private slots:
    /// @brief Slot to handle what happens when one of the slider handles is moved.
    ///
    /// @param [in] min_value The value of the left (minimum) slider handle.
    /// @param [in] max_value The value of the right (maximum) slider handle.
    void UpdateTreeDepths(int min_value, int max_value);

    /// @brief Slot to handle what happens when a tree node is changed.
    ///
    /// For the TLAS pane, if the user selects an instance node, enable the address 'button'.
    ///
    /// @param [in] selected The selected model indices.
    /// @param [in] deselected The deselected model indices.
    void TreeNodeChanged(const QItemSelection& selected, const QItemSelection& deselected);

    /// @brief Navigate to the BLAS pane after clicking on the BLAS address.
    ///
    /// @param [in] checked The checked state of the button (unused).
    void GotoBlasPaneFromBlasAddress(bool checked);

    /// @brief Select the parent of the currently selected node.
    ///
    /// @param [in] checked The checked state of the button (unused).
    void SelectParentNode(bool checked);

    /// @brief Set the TLAS index.
    ///
    /// Called when the user chooses a different TLAS.
    ///
    /// @param [in] tlas_index The TLAS index to set.
    void SetTlasIndex(uint64_t tlas_index);

private:
    /// @brief Update the pane after the selected TLAS has been changed.
    ///
    /// Calls the base class and emits a signal indicating the TLAS index has changed.
    void UpdateSelectedTlas();

    /// @brief Set the selected BLAS instance within the TLAS scene.
    ///
    /// @param [in] tlas_index The index of the selected TLAS.
    /// @param [in] blas_index The index of the selected BLAS.
    /// @param [in] instance_index The index of the selected BLAS instance.
    void SetBlasInstanceSelection(uint64_t tlas_index, uint64_t blas_index, uint64_t instance_index);

    /// @brief A BLAS has been selected in the tree view.
    ///
    /// Make sure the BLAS is selected in the BLAS pane for when the user navigates to it.
    ///
    /// The user has selected a BLAS instance from the UI, so select it in the BLAS
    /// combo box, set up the BLAS UI and navigate to the BLAS viewer pane.
    ///
    /// @param [in] index                  The model index of the item selected in the TLAS combo box.
    /// @param [in] navigate_to_blas_pane  If true, navigate to the BLAS pane.
    void SelectBlasFromTree(const QModelIndex& index, const bool navigate_to_blas_pane);

    /// @brief Show the buttons linking to the rebraided instance's siblings.
    /// 
    /// @param [in] scene The current scene.
    /// @param [in] tlas_index The current TLAS index.
    /// @param [in] instance_index Index of the selected instance.
    /// @param [in] node_id The ID of the selected node.
    void UpdateRebraidUI(rra::Scene* scene, uint32_t tlas_index, uint32_t instance_index, uint32_t node_id);

    /// @brief Handle the selection changed signal.
    void HandleSceneSelectionChanged();

    Ui::TlasViewerPane*            ui_;                       ///< Pointer to the Qt UI design.
    rra::TlasViewerModel*          derived_model_;            ///< Pointer to the model. This is a copy of the model in the base class and is deleted there.
    FlagTableItemDelegate*         flag_table_delegate_;      ///< Delegate for drawing the instance flags table.
    std::vector<ScaledPushButton*> rebraid_sibling_buttons_;  ///< The buttons to navigate to rebraid siblings for rebraided instances.
};

#endif  // RRA_VIEWS_TLAS_TLAS_VIEWER_PANE_H_
