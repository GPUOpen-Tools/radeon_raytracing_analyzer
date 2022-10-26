//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for an acceleration structure viewer pane base class.
//=============================================================================

#ifndef RRA_VIEWS_ACCELERATION_STRUCTURE_VIEWER_PANE_H_
#define RRA_VIEWS_ACCELERATION_STRUCTURE_VIEWER_PANE_H_

#include "qt_common/custom_widgets/arrow_icon_combo_box.h"

#include "models/acceleration_structure_viewer_model.h"
#include "views/base_pane.h"
#include "views/viewer_container_widget.h"
#include "side_panels/side_pane_container.h"

#include "public/renderer_widget.h"
#include "public/orientation_gizmo.h"

/// @brief Class declaration.
class AccelerationStructureViewerPane : public BasePane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent  The parent widget.
    explicit AccelerationStructureViewerPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~AccelerationStructureViewerPane();

    /// @brief Overridden pane key press event.
    ///
    /// @param [in] event The key press event object.
    virtual void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;

    /// @brief Overridden window show event.
    ///
    /// @param [in] event The show event object.
    virtual void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;

    /// @brief Overridden window hide event.
    ///
    /// @param [in] event The hide event object.
    virtual void hideEvent(QHideEvent* event) Q_DECL_OVERRIDE;

    /// @brief Overridden window resize event.
    ///
    /// @param [in] event The resize event object.
    virtual void resizeEvent(QResizeEvent* event) Q_DECL_OVERRIDE;

    /// @brief Trace closed.
    virtual void OnTraceClose() Q_DECL_OVERRIDE;

    /// @brief Reset the UI.
    virtual void Reset() Q_DECL_OVERRIDE;

    /// @brief Filter or modify events before they reach their destination object.
    ///
    /// @param [in] object The object associated with the event.
    /// @param [in] event The event.
    ///
    /// @returns False if the filter should be passed on the object, true otherwise.
    bool eventFilter(QObject* object, QEvent* event);

    /// @brief Select the leaf node under the mouse.
    ///
    /// Will be dependent on whether TLAS or BLAS. For a TLAS, this will be an instance of
    /// a BLAS. For a BLAS, this will be a triangle. Implementation in this class
    /// does nothing.
    ///
    /// @param [in] blas_index The blas index selected.
    /// @param [in] navigate_to_pane  If true, navigate to another pane. The pane to navigate to will
    /// be determined in the derived class implementation.
    virtual void SelectLeafNode(const uint64_t blas_index, const bool navigate_to_pane);

public slots:
    /// @brief Update the pane when the selected tree node has been changed.
    ///
    /// @param [in] selected The selected model indices.
    /// @param [in] deselected The deselected model indices.
    void SelectedTreeNodeChanged(const QItemSelection& selected, const QItemSelection& deselected);

    /// @brief Update the pane after the selected acceleration structure has been changed.
    ///
    /// Uses the combo box index as the acceleration structure index. Called when the user changes the
    /// acceleration structure combobox selection.
    ///
    /// @return The selected BVH index.
    uint64_t UpdateSelectedBvh();

protected:
    /// @brief Initialize the given renderer widget.
    ///
    /// @param [in] renderer_widget The renderer widget to initialize.
    /// @param [in] side_panel_container The side panel container instance.
    /// @param [in] viewer_container_widget The side panel container widget.
    /// @param [in] bvh_type The type of BVH viewer being initialized.
    void InitializeRendererWidget(RendererWidget*             renderer_widget,
                                  SidePaneContainer*          side_panel_container,
                                  ViewerContainerWidget*      viewer_container_widget,
                                  rra::renderer::BvhTypeFlags bvh_type);

    /// @brief Update the pane after a new selection has been picked in the scene.
    ///
    /// @param [in] tree_view The tree view to use.
    void UpdateSceneSelection(QTreeView* tree_view);

    /// @brief Sets the internal params for the camera controller.
    void UpdateCameraController();

    /// @brief Updates widgets depending on the model.
    ///
    /// @param [in] index The model index of the selected node.
    virtual void UpdateWidgets(const QModelIndex& index) = 0;

    /// @brief Set the parameters for any tables used in the viewer panes.
    ///
    /// @param [in] table_view The table to apply the parameters to.
    void SetTableParams(ScaledTableView* table_view);

    rra::AccelerationStructureViewerModel* model_                            = nullptr;  ///< The model backing the view.
    ArrowIconComboBox*                     acceleration_structure_combo_box_ = nullptr;  ///< The combo box showing the acceleration structures.
    rra::renderer::RendererInterface*      renderer_interface_               = nullptr;  ///< The renderer used to draw frames of the scene.
    RendererWidget*                        renderer_widget_                  = nullptr;  ///< The renderer widget instance used to draw the scene.
    bool                                   trace_loaded_                     = false;    ///< Is a trace file loaded?
    uint64_t                               last_selected_as_id_              = 0;        ///< The last selected acceleration blas id.
    rra::ViewerIO*                         last_camera_controller_           = nullptr;  ///< The last camera controller.

private slots:
    /// @brief This signal is emitted when a mouse button is pressed within the widget.
    ///
    /// @param [in] mouse_event The mouse button press event data.
    void MousePressed(QMouseEvent* mouse_event);

    /// @brief This signal is emitted when a mouse button is double clicked within the widget.
    ///
    /// @param [in] mouse_event The mouse button double click event data.
    void MouseDoubleClicked(QMouseEvent* mouse_event);

    /// @brief This signal is emitted when a mouse button is released within the widget.
    ///
    /// @param [in] mouse_event The mouse button release event data.
    void MouseReleased(QMouseEvent* mouse_event);

    /// @brief Make mouse wrap around to other side of widget if it passes boundary.
    ///
    /// @param [in] pos Current mouse position in local space of widget.
    /// @returns True if mouse was relocated to other side of the screen, false otherwise.
    bool WrapMouseMovement(QPoint pos);

    /// @brief This signal is emitted when the mouse is moved within the widget.
    ///
    /// @param [in] mouse_event The mouse move event data.
    void MouseMoved(QMouseEvent* mouse_event);

    /// @brief This signal is emitted when a mouse wheel is moved within the widget.
    ///
    /// @param [in] wheel_event The mouse wheel move event.
    void MouseWheelMoved(QWheelEvent* wheel_event);

    /// @brief This signal is emitted when a key is pressed while the widget has focus.
    ///
    /// @param [in] key_event The pressed key event data.
    void KeyPressed(QKeyEvent* key_event);

    /// @brief This signal is emitted when a key is released while the widget has focus.
    ///
    /// @param [in] key_event The released key event data.
    void KeyReleased(QKeyEvent* key_event);

    /// @brief This signal is emitted when the focus is about to go out.
    void FocusOut();

    /// @brief This signal is emitted when the focus is about to go in.
    void FocusIn();

protected:
    /// @brief Select the given item in the provided tree view.
    ///
    /// @param [in] tree_view The tree view to expand elements in.
    /// @param [in] index The tree model index to show in the tree view.
    void SelectTreeItem(QTreeView* tree_view, const QModelIndex& selected_index);

private:
    /// @brief Reset the UI to its initial (empty) state.
    ///
    /// @param [in] reset_scene Should the scene be reset? Should be set to true when
    /// closing a trace file.
    void ResetUI(bool reset_scene);

    /// @brief Populate the acceleration structure UI widgets.
    ///
    /// Called after a trace is loaded.
    void PopulateAccelerationStructure();

    /// @brief Setup the acceleration structure. Set up the scene and the tree view.
    ///
    /// @return The index of the acceleration structure used to populate the UI.
    uint64_t SetupAccelerationStructure();

    /// @brief Resets the camera orientation to match the current scene.
    void ResetCamera();

    /// @brief Halt rendering of new frames in the active renderer widget.
    void HaltRendererWidget();

    /// @brief Update the UI elements based on what is selected in the tree view.
    ///
    /// @param [in] model_index The model index of the item selected in the tree view.
    void HandleTreeNodeSelected(const QModelIndex& model_index);

    Qt::MouseButton last_mouse_button_pressed_ = Qt::MouseButton::NoButton;  ///< The last button press state to make decisions on mouse callbacks.
    QCursor         hand_cursor_{};                                          ///< The cursor instance used for changing the cursor shape.
    QPoint          mouse_start_dragging_pos_{};                             ///< The position of the cursor when the mouse starts being dragged.
    bool            cursor_overridden_{false};                               ///< True if the cursor has been overridden by the hand cursor.

    rra::renderer::OrientationGizmoHitType last_gizmo_hit_{
        rra::renderer::OrientationGizmoHitType::kNone};  ///< The orientation gizmo hit type that occured at the last mouse movement.
};

#endif  // RRA_VIEWS_ACCELERATION_STRUCTURE_VIEWER_PANE_H_
