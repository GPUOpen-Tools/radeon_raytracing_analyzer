//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the View side pane model.
//=============================================================================

#ifndef RRA_MODELS_SIDE_PANELS_VIEW_MODEL_H_
#define RRA_MODELS_SIDE_PANELS_VIEW_MODEL_H_

#include "side_panel_model.h"
#include "public/view_state_adapter.h"
#include "io/camera_controllers.h"
#include "managers/pane_manager.h"

namespace rra
{
    /// @brief Enum containing indices for the widgets shared between the model and UI.
    enum SidePaneViewWidgets
    {
        kSidePaneViewRenderGeometry,
        kSidePaneViewRenderBVH,
        kSidePaneViewRenderInstanceTransforms,
        kSidePaneViewWireframeOverlay,
        kSidePaneViewCullingMode,

        kSidePaneCameraRotationRow0Col0,
        kSidePaneCameraRotationRow0Col1,
        kSidePaneCameraRotationRow0Col2,
        kSidePaneCameraRotationRow1Col0,
        kSidePaneCameraRotationRow1Col1,
        kSidePaneCameraRotationRow1Col2,
        kSidePaneCameraRotationRow2Col0,
        kSidePaneCameraRotationRow2Col1,
        kSidePaneCameraRotationRow2Col2,
        kSidePaneCameraPositionX,
        kSidePaneCameraPositionY,
        kSidePaneCameraPositionZ,

        kSidePaneViewFieldOfView,
        kSidePaneViewFieldOfViewSlider,
        kSidePaneViewNearPlaneSlider,
        kSidePaneViewMovementSpeed,
        kSidePaneViewMovementSpeedSlider,

        kSidePaneViewInvertVertical,
        kSidePaneViewInvertHorizontal,

        kSidePaneArchitectureNavi2,
        kSidePaneArchitectureNavi3,

        kSidePaneRayFlagsAcceptFirstHit,

        kSidePaneViewXUp,
        kSidePaneViewYUp,
        kSidePaneViewZUp,
        kSidePaneViewGeometryMode,
        kSidePaneViewTraversalMode,

        kSidePaneViewTraversalContinuousUpdate,

        kSidePaneViewNumWidgets,
    };

    constexpr float kDefaultSpeedDiagonalMultiplier = 0.25f;     ///< The default speed is this multiple of the scene bounding volume diagonal.
    constexpr float kMinimumMovementSpeedMultiplier = 0.00005f;  ///< The minimum movement speed is this multiple of the maximum movement speed.
    const int32_t   kMovementSliderMaximum          = 1000000;   ///< The number of tick marks on the slider, essentially.

    /// @brief The View model class declaration.
    class ViewModel : public SidePanelModel
    {
    public:
        /// @brief Constructor.
        explicit ViewModel();

        /// @brief Destructor.
        virtual ~ViewModel() = default;

        /// @brief Set the renderer adapter instance.
        ///
        /// @param [in] adapter The renderer adapter instance.
        virtual void SetRendererAdapters(const rra::renderer::RendererAdapterMap& adapters) override;

        /// @brief Update the model with the current BVH state.
        ///
        /// Propagate the state to the UI.
        virtual void Update() override;

        /// @brief Update the camera's rotation matrix and position in the side pane.
        void UpdateCameraTransformUI();

        /// @brief Get the culling modes supported by the view.
        ///
        /// @return A list of strings containing the culling modes.
        std::vector<std::string>& GetViewportCullingModes() const;

        /// @brief Enable/disable whether to render the geometry.
        ///
        /// @param [in] enabled Flag indicating whether to render the geometry.
        void SetRenderGeometry(bool enabled);

        /// @brief Enable/disable whether to render the bounding volume hierarchy.
        ///
        /// @param [in] enabled Flag indicating whether to render the BVH.
        void SetRenderBVH(bool enabled);

        /// @brief Enable/disable whether to render the instance pretransform.
        ///
        /// @param [in] enabled Flag indicating whether to render the instance pretransform.
        void SetRenderInstancePretransform(bool enabled);

        /// @brief Enable/disable whether to render a wireframe overlay.
        ///
        /// @param [in] enabled Flag indicating whether to render a wireframe overlay.
        void SetWireframeOverlay(bool enabled);

        /// @brief Set the renderer culling mode.
        ///
        /// @param [in] index An index selected in the combo box corresponding to the culling mode.
        void SetViewportCullingMode(int index);

        /// @brief Set the max traversal counter range.
        ///
        /// @param [in] min_value The minimum counter value.
        /// @param [in] max_value The maximum counter value.
        void SetTraversalCounterRange(uint32_t min_value, uint32_t max_value);

        /// @brief Update the value render state adapter's value of the max traversal count setting.
        void UpdateTraversalCounterMaximumFromSettings();

        /// @brief Enable/disable whether to render the traversal.
        ///
        /// @param [in] enabled Flag indicating whether to render the traversal.
        void SetRenderTraversal(bool enabled);

        /// @brief Adapts the counter range to the view.
        ///
        /// @param [in] update_function The callback to use when the range has been acquired.
        void AdaptTraversalCounterRangeToView(std::function<void(uint32_t min, uint32_t max)> update_function);

        /// @brief Toggles the traversal counter continuous updates.
        ///
        /// @param [in] continuous_update Whether continous update is enabled or not.
        /// @param [in] update_function   The callback to use when the range has been acquired.
        void SetTraversalCounterContinuousUpdate(bool continous_update, std::function<void(uint32_t min, uint32_t max)> update_function);

        /// @brief Sets the histogram data update function to populate histogram from traversal counter.
        ///
        /// @param [in] update_function The callback to use after the traversal shader runs to populate histogram.
        /// @param [in] traversal_max_setting The maximum traversal count set in the settings.
        void SetHistogramUpdateFunction(std::function<void(const std::vector<uint32_t>&, uint32_t, uint32_t)> update_function, uint32_t traversal_max_setting);

        /// @brief Checks if the traversal counter continuous update is set.
        ///
        /// @returns True if the traversal counter continuous update is set.
        bool IsTraversalCounterContinuousUpdateSet();

        /// @brief Get the projection types supported by the view.
        ///
        /// @return A list of strings containing the projection types.
        std::vector<std::string>& GetProjectionTypes() const;

        /// @brief Get the control styles supported by the view.
        ///
        /// @return A list of strings containing the control style types.
        std::vector<std::string> GetControlStyles() const;

        /// @brief Get the projection modes supported by the camera controller.
        ///
        /// @return A list of strings containing the projection mode types.
        std::vector<std::string> GetProjectionModes() const;

        /// @brief Get the current camera controller index.
        ///
        /// This is so the selected camera controller is the default in the camera controller
        /// combo box in the TLAS & BLAS view panes.
        ///
        /// @return The controller index.
        int GetCurrentControllerIndex() const;

        /// @brief Get the current camera controller.
        ///
        /// @return The controller.
        ViewerIO* GetCurrentController();

        /// @param [in] hotkeys_widget A widget that contains the hotkey information.
        void UpdateControlHotkeys(QWidget* hotkeys_widget);

        /// @brief Returns the current camera controller orientation.
        ///
        /// @return The camera controller orientation;
        const ViewerIOOrientation& GetCameraControllerOrientation() const;

        /// @brief Set the field of view from the field of view slider value.
        ///
        /// The slider value is scaled to the correct FOV, in degrees.
        ///
        /// @param [in] slider_value The field of view slider value.
        float SetFieldOfViewFromSlider(int slider_value);

        /// @brief Set the field of view.
        ///
        /// @param [in] fov The field of view value, in degrees.
        void SetFieldOfView(int fov);

        /// @brief Set the near place.
        ///
        /// @param [in] value The near plane value.
        void SetNearPlane(int value);

        /// @brief Set the movement speed multiplier from the slider value.
        ///
        /// @param [in] slider_value The movement speed multiplier.
        float SetMovementSpeedFromSlider(int slider_value);

        /// @brief Set the movement speed multiplier.
        ///
        /// @param [in] speed The movement speed multiplier.
        void SetMovementSpeed(float speed);

        /// @brief Set the control style.
        ///
        /// @param [in] index An index selected in the combo box corresponding to the style.
        bool SetControlStyle(int index);

        /// @brief Sets the vertical axis inversion.
        ///
        /// @param enabled True to invert the axis, false otherwise.
        void SetInvertVertical(bool enabled);

        /// @brief Sets the vertical axis inversion.
        ///
        /// @param enabled True to invert the axis, false otherwise.
        void SetInvertHorizontal(bool enabled);

        /// @brief Toggles the vertical axis inversion.
        void ToggleInvertVertical();

        /// @brief Toggles the horizontal axis inversion.
        void ToggleInvertHorizontal();

        /// @brief Set the orientation shared by BLAS and TLAS cameras.
        void SetOrientation(rra::ViewerIOOrientation orientation);

        /// @brief Set the up axis as X.
        void SetUpAxisAsX();

        /// @brief Set the up axis as Y.
        void SetUpAxisAsY();

        /// @brief Set the up axis as Z.
        void SetUpAxisAsZ();

        /// @brief Set Architecture to Navi2.
        void SetArchitectureToNavi2();

        /// @brief Set Architecture to Navi3.
        void SetArchitectureToNavi3();

        /// @brief Enable the accept first hit flag.
        void EnableRayFlagsAcceptFirstHit();

        /// @brief Disable the accept first hit flag.
        void DisableRayFlagsAcceptFirstHit();

        /// @brief Enable the cull back facing triangles flag.
        void EnableRayCullBackFacingTriangles();

        /// @brief Disable the cull back facing triangles flag.
        void DisableRayCullBackFacingTriangles();

        /// @brief Enable the cull front facing triangles flag.
        void EnableRayCullFrontFacingTriangles();

        /// @brief Disable the cull front facing triangles flag.
        void DisableRayCullFrontFacingTriangles();

        /// @brief Set whether or not to use orthographic projection.
        ///
        /// @param [in] ortho True if orthographic projection is desired.
        void SetOrthographic(bool ortho);

        /// @brief Set camera parameters based on common data.
        ///
        /// @param [in] controller_changed Flag indicating if the camera controller has changed.
        /// @param [in] pane_id            The pane id that the camera parameters were changed on.
        void SetCameraControllerParameters(bool controller_changed, rra::RRAPaneId pane_id);

        /// @brief Set heatmap update callback.
        ///
        /// @param [in] heatmap_update_callback The callback.
        void SetHeatmapUpdateCallback(std::function<void(rra::renderer::HeatmapData)> heatmap_update_callback);

        /// @brief Set the maximum movement speed selectable from the movement speed slider.
        ///
        /// @param maximum_speed The new maximum speed.
        void SetMovementSpeedLimit(float maximum_speed);

        /// @brief Get the maximum movement speed selectable from the movement speed slider.
        ///
        /// @return The maximum speed.
        float GetMovementSpeedLimit();

        /// @brief Get the box sort heuristic name depending on the current configuration.
        ///
        /// @return The box sort heuristic name.
        QString GetBoxSortHeuristicName();

        /// @brief Set whether or not the camera state should persist when switching to and from inspector.
        ///
        /// @param locked If true, camera state will persist.
        void SetCameraLock(bool locked);

        /// @brief Get whether or not the camera state should persist when switching to and from inspector.
        ///
        /// @return Whether or not the camera is locked.
        bool GetCameraLock() const;

        /// @brief Set the parent pane id.
        /// @param [in] pane_id The id to update with.
        void SetParentPaneId(rra::RRAPaneId pane_id);

        /// @brief Get the parent pane id.
        /// @return The parent pane id.
        rra::RRAPaneId GetParentPaneId() const;

    private:
        /// @brief Convert a movement speed slider value to a valid range-clamped movement speed value.
        ///
        /// @param [in] slider_value The slider value.
        ///
        /// @returns A floating point movement speed value corresponding to the selected slider value.
        float MovementSpeedValueFromSlider(int32_t slider_value);

        /// @brief Convert a movement speed value to a range-clamped slider value.
        ///
        /// @param [in] movement_speed The movement speed value.
        ///
        /// @returns A slider integer value corresponding to the selected movement speed value.
        int32_t MovementSpeedValueToSliderValue(float movement_speed);

        /// @brief Struct to describe the camera controls in the UI.
        ///
        /// This is so these parameters can be shared between all instances of the
        /// view pane, so if a UI element is changed on one side pane, then it can be
        /// updated on the other pane(s).
        struct CameraUIControls
        {
            int                      control_style_index;
            rra::ViewerIOOrientation orientation;
        };

        rra::renderer::ViewStateAdapter*                view_state_adapter_      = nullptr;  ///< The adapter used to alter the view state.
        rra::renderer::RenderStateAdapter*              render_state_adapter_    = nullptr;  ///< The adapter used to toggle mesh render states.
        CameraControllers                               camera_controllers_      = {};       ///< The camera controllers object to manage camera controllers.
        ViewerIO*                                       current_controller_      = nullptr;  ///< The camera controller currently in use.
        CameraUIControls                                camera_controls_         = {};       ///< The camera controls shared by all instances of this class.
        std::function<void(rra::renderer::HeatmapData)> heatmap_update_callback_ = nullptr;  ///< The heatmap update callback.
        float                                           movement_speed_minimum_  = {};       ///< The minimum movement speed. Set from settings.
        float                                           movement_speed_maximum_  = {};       ///< The maximum movement speed. Set from settings.
        bool                                            camera_lock_ = false;  ///< Persist camera position when selected dispatch coordinate changes.

        rra::RRAPaneId parent_pane_id_ = rra::RRAPaneId::kPaneIdInvalid;  ///< The parent pane id that this model belongs to.
    };
}  // namespace rra

#endif  // RRA_MODELS_SIDE_PANELS_VIEW_MODEL_H_
