//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the Ray inspector pane.
//=============================================================================

#ifndef RRA_VIEWS_RAY_RAY_INSPECTOR_PANE_H_
#define RRA_VIEWS_RAY_RAY_INSPECTOR_PANE_H_

#include "ui_ray_inspector_pane.h"

#include "models/ray/ray_inspector_model.h"
#include "models/ray/ray_list_table_item_delegate.h"
#include "views/base_pane.h"
#include <public/orientation_gizmo.h>
#include <models/acceleration_structure_flags_table_item_delegate.h>

/// @brief Class declaration.
class RayInspectorPane : public BasePane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit RayInspectorPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~RayInspectorPane();

    /// @brief Called when the presented ray data needs to be updated.
    void UpdateRayValues();

    /// @brief Updates the result values shown by the ray.
    void UpdateRayResultView();

    /// @brief Deselect any actively selected ray from the list.
    void DeselectRay();

    /// @brief Overridden window resize event.
    ///
    /// @param [in] event The resize event object.
    virtual void resizeEvent(QResizeEvent* event) Q_DECL_OVERRIDE;

    /// @brief Overridden window show event.
    ///
    /// @param [in] event The show event object.
    virtual void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;

    /// @brief Trace open.
    virtual void OnTraceOpen() Q_DECL_OVERRIDE;

    /// @brief Trace closed.
    virtual void OnTraceClose() Q_DECL_OVERRIDE;

    /// @brief Pane hidden.
    virtual void hideEvent(QHideEvent* event) Q_DECL_OVERRIDE;

    /// @brief Update coloring legend.
    void UpdateColoringLegend();

private slots:
    /// @brief Set the currently selected ray coordinate to show stats for.
    ///
    /// @param [in] x The x-component of the currently selected ray coordinate.
    /// @param [in] y The y-component of the currently selected ray coordinate.
    /// @param [in] z The z-component of the currently selected ray coordinate.
    void SetRayCoordinate(uint32_t dispatch_id, uint32_t x, uint32_t y, uint32_t z);

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

    /// @brief Switches the pane to the tlas from the selected ray.
    void GotoTlasPaneFromSelectedRay();

    /// @brief Switches to the pane to tlas from the selected ray but it also selects the instance that the ray hit.
    void GotoTlasPaneFromSelectedRayWithInstance();

    /// @brief Focus on the hit location.
    void FocusOnHitLocation();

    /// @brief Focus on ray origin.
    void FocusOnRayOrigin();

    /// @brief Called when a ray is selected from the ray table.
    void SelectRay();

protected:
    /// @brief Override the widget event handler.
    ///
    /// @param [in] event The event data.
    ///
    /// \returns True if the event was recognized and handled, and false otherwise.
    bool event(QEvent* event) Q_DECL_OVERRIDE;

private:
    /// @brief Sets the internal params for the camera controller.
    void UpdateCameraController();

    /// @brief Resets the camera orientation to match the current scene.
    void FocusOnSelectedRay();

    /// @brief Initialize the given renderer widget.
    ///
    /// @param [in] renderer_widget The renderer widget to initialize.
    /// @param [in] side_panel_container The side panel container instance.
    void InitializeRendererWidget(RendererWidget* renderer_widget, SidePaneContainer* side_panel_container);

    /// @brief Halt rendering of new frames in the active renderer widget.
    void HaltRendererWidget();

    /// @brief Check for the rays on the event of a click
    /// @param camera The camera to use.
    /// @param hit_coords The 0 to 1 screen space coords.
    /// @return The optional index of the ray that was clicked.
    std::optional<uint32_t> CheckRayClick(rra::renderer::Camera& camera, glm::vec2 hit_coords);

    Ui::RayInspectorPane*             ui_{};                                 ///< Pointer to the Qt UI design.
    rra::RayInspectorModel*           model_{};                              ///< The ray inspector model.
    FlagTableItemDelegate*            flag_table_delegate_;                  ///< Delegate for drawing the ray flags table.
    TableItemDelegate*                table_delegate_{};                     ///< The delegate to draw the table rows.
    rra::renderer::RendererInterface* renderer_interface_     = nullptr;     ///< The renderer used to draw frames of the scene.
    RendererWidget*                   renderer_widget_        = nullptr;     ///< The renderer widget instance used to draw the scene.
    rra::ViewerIO*                    last_camera_controller_ = nullptr;     ///< The last camera controller.
    Qt::MouseButton last_mouse_button_pressed_ = Qt::MouseButton::NoButton;  ///< The last button press state to make decisions on mouse callbacks.
    QCursor         hand_cursor_{};                                          ///< The cursor instance used for changing the cursor shape.
    QPoint          mouse_start_dragging_pos_{};                             ///< The position of the cursor when the mouse starts being dragged.
    bool            cursor_overridden_{false};                               ///< True if the cursor has been overridden by the hand cursor.
    rra::renderer::OrientationGizmoHitType last_gizmo_hit_{
        rra::renderer::OrientationGizmoHitType::kNone,
    };  ///< The orientation gizmo hit type that occured at the last mouse movement.
};

#endif  // RRA_VIEWS_RAY_RAY_INSPECTOR_PANE_H_
