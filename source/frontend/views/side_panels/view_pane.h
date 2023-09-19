//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the View side pane.
//=============================================================================

#ifndef RRA_VIEWS_SIDE_PANELS_VIEW_PANE_H_
#define RRA_VIEWS_SIDE_PANELS_VIEW_PANE_H_

#include "ui_view_pane.h"

#include "managers/pane_manager.h"
#include "models/side_panels/view_model.h"

/// Signal handler for the view pane. Signals emitted from here will be picked
/// up by all instances of the View pane; if the signals were emitted from the ViewPane,
/// only the current ViewPane would get the signal.
/// The idea here is that if the camera is changed on the TLAS pane, it will get updated
/// on the BLAS pane too, and vice versa.
class ViewPaneSignalHandler : public QObject
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ViewPaneSignalHandler()
    {
    }

signals:
    /// @brief Signal to indicate that the camera parameters have changed.
    ///
    /// @param [in] controller_changed Flag indicating if the camera controller type has changed. If so, the camera
    /// controller will need changing.
    void CameraParametersChanged(bool controller_changed, rra::RRAPaneId pane_id);

    /// @brief Signal to indicate that the camera hotkeys have changed and need updating.
    ///
    /// @param [in] hotkeys_widget The UI widget to update.
    void CameraHotkeysChanged(QWidget* hotkeys_widget);
};

/// @brief The View Pane class declaration.
class ViewPane : public QWidget
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit ViewPane(QWidget* parent);

    /// @brief Destructor.
    virtual ~ViewPane();

    /// @brief Handle what happens when a trace is opened.
    void OnTraceOpen();

    /// @brief Overridden window show event.
    ///
    /// @param [in] event The show event object.
    virtual void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;

    /// @brief Apply the state of the UI from the state in the settings.
    ///
    /// @param [in] pane The parent pane containing this viewer container.
    void ApplyUIStateFromSettings(rra::RRAPaneId pane);

    /// @brief Get a pointer to the underlying View model instance.
    ///
    /// @returns A pointer to the underlying View model instance.
    rra::ViewModel* GetModel() const;

    /// @brief Hides the tlas specific widgets.
    void HideTLASWidgets();

    /// @brief Hides the ray specific widgets.
    void HideRAYWidgets();

    /// @brief Hides or shows widgets that aren't needed for BLASes with procedural geometry.
    ///
    /// @param [in] hidden Shows non-procedural widgets if true, hides them if false.
    void NonProceduralWidgetsHidden(bool hidden);

    /// @brief Set the parent pane for this instance of the view pane.
    ///
    /// @param [in] pane_if The parent pane ID.
    void SetParentPaneId(rra::RRAPaneId pane_id);

signals:
    /// @brief A signal to notify for control style changes.
    void ControlStyleChanged();

    /// @brief A signal to notify for render mode changes.
    void RenderModeChanged(bool geometry_node);

public slots:
    /// @brief Set the control style for the camera.
    ///
    /// Interrogates the UI control and passes the data off to the model.
    void SetControlStyle(int index);

private slots:
    /// @brief Enable/disable whether to render the geometry.
    ///
    /// Interrogates the UI control and passes the data off to the model.
    void SetRenderGeometry(bool update_model);

    /// @brief Enable/disable whether to render the BVH.
    ///
    /// Interrogates the UI control and passes the data off to the model.
    void SetRenderBVH(bool update_model);

    /// @brief Enable/disable whether to render the instance pretransoforms.
    ///
    /// Interrogates the UI control and passes the data off to the model.
    void SetRenderInstancePretransform(bool update_model);

    /// @brief Enable/disable whether to show a wireframe overlay.
    ///
    /// Interrogates the UI control and passes the data off to the model.
    void SetWireframeOverlay();

    /// @brief Set the culling mode.
    ///
    /// Interrogates the UI control and passes the data off to the model.
    void SetCullingMode();

    /// @brief Set the max traversal counter range.
    ///
    /// @param [in] min_value The minimum counter value.
    /// @param [in] max_value The maximum counter value.
    ///
    /// Interrogates the UI control and passes the data off to the model.
    void SetTraversalCounterRange(int min_value, int max_value);

    /// @brief Adapts the traversal counter range to the view.
    void AdaptTraversalCounterRangeToView();

    /// @brief Update the box sort heuristic label.
    void UpdateBoxSortHeuristicLabel();

    /// @brief Set Architecture to Navi2.
    void SetArchitectureToNavi2();

    /// @brief Set Architecture to Navi3.
    void SetArchitectureToNavi3();

    /// @brief Toggle the accept first hit flag.
    void ToggleRayFlagsAcceptFirstHit();

    /// @brief Toggle the cull back facing triangles flag.
    void ToggleRayFlagsCullBackFacingTriangles();

    /// @brief Toggle the cull front facing triangles flag.
    void ToggleRayFlagsCullFrontFacingTriangles();

    /// @brief Move the camera to (0, 0, 0).
    void MoveCameraToOrigin();

    /// @brief Toggle the inspector camera lock. This persists camera position when changing dispatch coordinate selection.
    void ToggleCameraLock();

    /// @brief Toggles traversal counter continuous update.
    void ToggleTraversalCounterContinuousUpdate();

    /// @brief Check whether paste was successful, and pop dialogue box if not.
    ///
    /// @param status Result code of pasting camera state.
    void CheckPasteResult(rra::ViewerIOCameraPasteResult result);

    /// @brief Copy camera's transform to the clipboard.
    ///
    /// Intended to be pasted back into RRA.
    void CopyCameraButtonClicked();

    /// @brief Paste camera's transform to the clipboard.
    ///
    /// Intended to be pasted back into RRA.
    void PasteCameraButtonClicked();

    /// @brief Slot to handle what happens when the camera's x position has changed from the side pane.
    ///
    /// Needs to adjust the camera's actual position to keep them in sync.
    void CameraPositionChangedX(double);

    /// @brief Slot to handle what happens when the camera's y position has changed from the side pane.
    ///
    /// Needs to adjust the camera's actual position to keep them in sync.
    void CameraPositionChangedY(double);

    /// @brief Slot to handle what happens when the camera's z position has changed from the side pane.
    ///
    /// Needs to adjust the camera's actual position to keep them in sync.
    void CameraPositionChangedZ(double);

    /// @brief Set the projection mode for the camera.
    ///
    /// @param [in] index The index into the projection dropdown.
    void SetProjectionMode(int index);

    /// @brief Toggles the horizontal axis inversion.
    void ToggleVerticalAxisInverted();

    /// @brief Toggles the horizontal axis inversion.
    void ToggleHorizontalAxisInverted();

    /// @brief Set the up axis as X.
    void SetUpAxisAsX();

    /// @brief Set the up axis as Y.
    void SetUpAxisAsY();

    /// @brief Set the up axis as Z.
    void SetUpAxisAsZ();

    /// @brief Configure for geometry rendering layout.
    void ConfigureForGeometryRenderingLayout();

    /// @brief Configure for traversal rendering layout.
    void ConfigureForTraversalRenderingLayout();

    /// @brief Toggles the hotkey layout to show or hide.
    void ToggleHotkeyLayout();

    /// @brief Save the field of view selected in the UI to the settings.
    ///
    /// @param [in] value The value to save.
    void SetFieldOfView(int value);

    /// @brief Save the movement speed selected in the UI to the settings.
    ///
    /// @param [in] value The value to save.
    void SetMovementSpeed(int value);

private:
    /// @brief Updates the orientation widgets.
    void UpdateOrientationWidgets();

    Ui::ViewPane*   ui_             = nullptr;              ///< Pointer to the Qt UI design.
    rra::ViewModel* model_          = nullptr;              ///< The model for this pane.
    rra::RRAPaneId  parent_pane_id_ = rra::kPaneIdInvalid;  ///< The parent pane id.

    static ViewPaneSignalHandler signal_handler;  ///< The singal handler for camera events.
};

#endif  // RRA_VIEWS_SIDE_PANELS_VIEW_PANE_H_
