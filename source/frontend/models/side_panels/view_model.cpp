//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the View side pane model.
//=============================================================================

#include "models/side_panels/view_model.h"

#include <QVariant>
#include <cmath>

#include "constants.h"
#include "public/rra_print.h"

namespace rra
{
    // @brief List of culling modes.
    static std::vector<std::string> kCullingModes = {"No culling", "Cull frontface", "Cull backface"};

    // Temporary for now. These will come from the view.
    static std::vector<std::string> kProjectionTypes = {"Perspective"};

    /// @brief The camera controls shared between all instances of the ViewModel class.
    ViewModel::CameraUIControls ViewModel::camera_controls_ = {};

    /// @brief The SliderRangeConverter namespace includes static helper functions used to map
    /// floating point values of a known min/max range to and from a consistent slider range.
    namespace SliderRangeConverter
    {
        static const int32_t kSliderMinimum = 0;    ///< The minimum value a slider will reach.
        static const int32_t kSliderMaximum = 100;  ///< The maximum value a slider will reach.

        static const float kFieldOfViewMinimum = 30;  ///< The minimum field of view value in degrees.
        static const float kFieldOfViewMaximum = 90;  ///< The maximum field of view value in degrees.

        static const float kNearPlaneMinimum = 0.001f;  ///< The minimum near plane distance.
        static const float kNearPlaneMaximum = 1.0f;    ///< The maximum near plane distance.

        /// @brief Map a slider value (always between 0 and 100 inclusive) to a provided min/max float range.
        ///
        /// @param [in] slider_value The slider value.
        /// @param [in] min The minimum value to clamp the output range within.
        /// @param [in] max The maximum value to clamp the output range within.
        ///
        /// @returns The float value clamped within the specified range.
        float MapFromSliderValue(int32_t slider_value, float min, float max)
        {
            float coefficient = (slider_value - kSliderMinimum) / static_cast<float>(kSliderMaximum - kSliderMinimum);
            return min + (max - min) * coefficient;
        }

        /// @brief Map a value within the provided range to an integer that can be assigned to a slider within the GUI.
        ///
        /// @param [in] value The value to map to a slider value.
        /// @param [in] min The minimum range value.
        /// @param [in] max The maximum range value.
        ///
        /// @returns The slider integer value.
        int32_t MapToSliderValue(float value, float min, float max)
        {
            float coefficient = (value - min) / (max - min);
            return kSliderMinimum + (kSliderMaximum - kSliderMinimum) * coefficient;
        }

        /// @brief Convert a field of view slider value to a valid range-clamped FOV value.
        ///
        /// @param [in] slider_value The slider value.
        ///
        /// @returns A floating point FOV value corresponding to the selected slider value.
        float FieldOfViewFromSlider(int32_t slider_value)
        {
            return MapFromSliderValue(slider_value, kFieldOfViewMinimum, kFieldOfViewMaximum);
        }

        /// @brief Convert a field of view value to a range-clamped slider value.
        ///
        /// @param [in] field_of_view The field of view value.
        ///
        /// @returns A slider integer value corresponding to the selected FOV value.
        int32_t FieldOfViewValueToSliderValue(float field_of_view)
        {
            return MapToSliderValue(field_of_view, kFieldOfViewMinimum, kFieldOfViewMaximum);
        }

        /// @brief Convert a near plane distance slider value to a valid range-clamped near plane distance value.
        ///
        /// @param [in] slider_value The slider value.
        ///
        /// @returns A floating point near plane distance value corresponding to the selected slider value.
        float NearPlaneValueFromSlider(int32_t slider_value)
        {
            return MapFromSliderValue(slider_value, kNearPlaneMinimum, kNearPlaneMaximum);
        }

        /// @brief Convert a near plane distance value to a range-clamped slider value.
        ///
        /// @param [in] near_plane The near plane distance value.
        ///
        /// @returns A slider integer value corresponding to the selected near plane distance value.
        int32_t NearPlaneValueToSliderValue(float near_plane)
        {
            return MapToSliderValue(near_plane, kNearPlaneMinimum, kNearPlaneMaximum);
        }
    };  // namespace SliderRangeConverter

    float ViewModel::MovementSpeedValueFromSlider(int32_t slider_value)
    {
        float normalized_slider_value = slider_value / (float)kMovementSliderMaximum;

        float exponent = -std::log(movement_speed_minimum_ / movement_speed_maximum_) * (normalized_slider_value - 1.0f);
        return movement_speed_maximum_ * std::exp(exponent);
    }

    int32_t ViewModel::MovementSpeedValueToSliderValue(float movement_speed)
    {
        float numerator   = std::log(movement_speed / movement_speed_maximum_);
        float denominator = -std::log(movement_speed_minimum_ / movement_speed_maximum_);

        // Limit slider to its maximum length (in the case where it's been reduced in the settings).
        float normalized_slider_value = std::min((numerator / denominator) + 1.0f, 1.0f);
        return (int32_t)(normalized_slider_value * kMovementSliderMaximum);
    }

    float ViewModel::GetMovementSpeedLimit()
    {
        return movement_speed_maximum_;
    }

    QString ViewModel::GetBoxSortHeuristicName()
    {
        if (render_state_adapter_)
        {
            if (render_state_adapter_->GetBoxSortHeuristic() == kBoxSortHeuristicClosest)
            {
                return "closest";
            }
            else if (render_state_adapter_->GetBoxSortHeuristic() == kBoxSortHeuristicMidPoint)
            {
                return "middle point";
            }
            else if (render_state_adapter_->GetBoxSortHeuristic() == kBoxSortHeuristicLargest)
            {
                return "largest";
            }
        }

        return "-";
    }

    ViewModel::ViewModel()
        : SidePanelModel(kSidePaneViewNumWidgets)
    {
        for (const auto& name : camera_controllers_.GetControllerNames())
        {
            camera_controllers_.GetControllerByName(name)->SetViewModel(this);
        }
    }

    void ViewModel::SetRendererAdapters(const rra::renderer::RendererAdapterMap& adapters)
    {
        using namespace rra::renderer;

        view_state_adapter_   = GetAdapter<ViewStateAdapter*>(adapters, RendererAdapterType::kRendererAdapterTypeView);
        render_state_adapter_ = GetAdapter<RenderStateAdapter*>(adapters, RendererAdapterType::kRendererAdapterTypeRenderState);
        render_state_adapter_->AddHeatmapUpdateCallback(heatmap_update_callback_);
        render_state_adapter_->SetArchitectureToNavi3();

        view_state_adapter_->SetCameraController(camera_controllers_.GetControllerByName(camera_controllers_.GetControllerNames()[0]));
        Update();
    }

    void ViewModel::Update()
    {
        if (render_state_adapter_ != nullptr)
        {
            SetModelData(kSidePaneViewRenderInstanceTransforms, render_state_adapter_->GetRenderInstancePretransform());
            SetModelData(kSidePaneViewRenderBVH, render_state_adapter_->GetRenderBoundingVolumes());
            SetModelData(kSidePaneViewRenderGeometry, render_state_adapter_->GetRenderGeometry());
            SetModelData(kSidePaneViewWireframeOverlay, render_state_adapter_->GetRenderWireframe());
            SetModelData(kSidePaneViewCullingMode, render_state_adapter_->GetCullingMode());
        }

        if (view_state_adapter_ != nullptr && current_controller_ != nullptr)
        {
            auto orientation = GetCameraControllerOrientation();

            // Set "invert y" UI.
            SetModelData(kSidePaneViewInvertVertical, orientation.flip_vertical);
            SetModelData(kSidePaneViewInvertHorizontal, orientation.flip_horizontal);

            // Set up axis UI.
            SetModelData(kSidePaneViewXUp, orientation.up_axis == rra::ViewerIOUpAxis::kUpAxisX);
            SetModelData(kSidePaneViewYUp, orientation.up_axis == rra::ViewerIOUpAxis::kUpAxisY);
            SetModelData(kSidePaneViewZUp, orientation.up_axis == rra::ViewerIOUpAxis::kUpAxisZ);

            // Set UI sliders.
            // For some reason QT does not properly update the ui without these two lines.
            auto fov = view_state_adapter_->GetFieldOfView();
            SetModelData(kSidePaneViewFieldOfViewSlider, 0);

            SetModelData(kSidePaneViewFieldOfViewSlider, SliderRangeConverter::FieldOfViewValueToSliderValue(fov));
            SetModelData(kSidePaneViewNearPlaneSlider, SliderRangeConverter::NearPlaneValueToSliderValue(view_state_adapter_->GetNearPlaneMultiplier()));
            float current_movement_speed = view_state_adapter_->GetMovementSpeed();
            if (current_movement_speed > movement_speed_maximum_)
            {
                view_state_adapter_->SetMovementSpeed(movement_speed_maximum_);
            }

            float slider_value = MovementSpeedValueToSliderValue(view_state_adapter_->GetMovementSpeed());
            SetModelData(kSidePaneViewMovementSpeedSlider, slider_value);
            SetModelData(kSidePaneViewMovementSpeed, QString::number(MovementSpeedValueFromSlider(slider_value), kQtFloatFormat, 0));
        }

        UpdateCameraTransformUI();
    }

    void ViewModel::UpdateCameraTransformUI()
    {
        if (current_controller_ != nullptr)
        {
            auto camera = current_controller_->GetCamera();
            if (camera != nullptr)
            {
                auto rotation_matrix = camera->GetRotationMatrix();
                SetModelData(kSidePaneCameraRotationRow0Col0, QString::number(rotation_matrix[0][0], kQtFloatFormat, kQtTooltipFloatPrecision));
                SetModelData(kSidePaneCameraRotationRow0Col1, QString::number(rotation_matrix[1][0], kQtFloatFormat, kQtTooltipFloatPrecision));
                SetModelData(kSidePaneCameraRotationRow0Col2, QString::number(rotation_matrix[2][0], kQtFloatFormat, kQtTooltipFloatPrecision));
                SetModelData(kSidePaneCameraRotationRow1Col0, QString::number(rotation_matrix[0][1], kQtFloatFormat, kQtTooltipFloatPrecision));
                SetModelData(kSidePaneCameraRotationRow1Col1, QString::number(rotation_matrix[1][1], kQtFloatFormat, kQtTooltipFloatPrecision));
                SetModelData(kSidePaneCameraRotationRow1Col2, QString::number(rotation_matrix[2][1], kQtFloatFormat, kQtTooltipFloatPrecision));
                SetModelData(kSidePaneCameraRotationRow2Col0, QString::number(rotation_matrix[0][2], kQtFloatFormat, kQtTooltipFloatPrecision));
                SetModelData(kSidePaneCameraRotationRow2Col1, QString::number(rotation_matrix[1][2], kQtFloatFormat, kQtTooltipFloatPrecision));
                SetModelData(kSidePaneCameraRotationRow2Col2, QString::number(rotation_matrix[2][2], kQtFloatFormat, kQtTooltipFloatPrecision));

                auto position = camera->GetPosition();
                SetModelData(kSidePaneCameraPositionX, position.x);
                SetModelData(kSidePaneCameraPositionY, position.y);
                SetModelData(kSidePaneCameraPositionZ, position.z);
            }
        }
    }

    std::vector<std::string>& ViewModel::GetCullingModes() const
    {
        return kCullingModes;
    }

    void ViewModel::SetRenderGeometry(bool enabled)
    {
        if (render_state_adapter_ != nullptr)
        {
            render_state_adapter_->SetRenderGeometry(enabled);
        }
    }

    void ViewModel::SetRenderBVH(bool enabled)
    {
        if (render_state_adapter_ != nullptr)
        {
            render_state_adapter_->SetRenderBoundingVolumes(enabled);
        }
    }

    void ViewModel::SetRenderInstancePretransform(bool enabled)
    {
        if (render_state_adapter_ != nullptr)
        {
            render_state_adapter_->SetRenderInstancePretransform(enabled);
        }
    }

    void ViewModel::SetWireframeOverlay(bool enabled)
    {
        if (render_state_adapter_ != nullptr)
        {
            render_state_adapter_->SetRenderWireframe(enabled);
        }
    }

    void ViewModel::SetCullingMode(int index)
    {
        if (render_state_adapter_ != nullptr)
        {
            render_state_adapter_->SetCullingMode(index);
        }
    }

    void ViewModel::SetTraversalCounterRange(uint32_t min_value, uint32_t max_value)
    {
        if (render_state_adapter_ != nullptr)
        {
            render_state_adapter_->SetTraversalCounterRange(min_value, max_value);
        }
    }

    void ViewModel::SetRenderTraversal(bool enabled)
    {
        if (render_state_adapter_ != nullptr)
        {
            render_state_adapter_->SetRenderTraversal(enabled);
        }
    }

    void ViewModel::AdaptTraversalCounterRangeToView(std::function<void(uint32_t min, uint32_t max)> update_function)
    {
        if (render_state_adapter_ != nullptr)
        {
            render_state_adapter_->AdaptTraversalCounterRangeToView([=](uint32_t min, uint32_t max) {
                update_function(min, max);
                Update();
            });
        }
    }

    void ViewModel::ToggleTraversalCounterContinuousUpdate(std::function<void(uint32_t min, uint32_t max)> update_function)
    {
        if (render_state_adapter_ != nullptr)
        {
            if (render_state_adapter_->IsTraversalCounterContinuousUpdateFunctionSet())
            {
                render_state_adapter_->SetTraversalCounterContinuousUpdateFunction(nullptr);
                Update();
            }
            else
            {
                render_state_adapter_->SetTraversalCounterContinuousUpdateFunction([=](uint32_t min, uint32_t max) {
                    update_function(min, max);
                    Update();
                });
            }
        }
    }

    bool ViewModel::IsTraversalCounterContinuousUpdateSet()
    {
        if (render_state_adapter_ != nullptr)
        {
            return render_state_adapter_->IsTraversalCounterContinuousUpdateFunctionSet();
        }
        else
        {
            return false;
        }
    }

    std::vector<std::string>& ViewModel::GetProjectionTypes() const
    {
        return kProjectionTypes;
    }

    std::vector<std::string> ViewModel::GetControlStyles() const
    {
        return camera_controllers_.GetControllerNames();
    }

    std::vector<std::string> ViewModel::GetProjectionModes() const
    {
        return {"Perspective projection", "Orthographic projection"};
    }

    int ViewModel::GetCurrentControllerIndex() const
    {
        return camera_controls_.control_style_index;
    }

    ViewerIO* ViewModel::GetCurrentController()
    {
        return current_controller_;
    }

    void ViewModel::UpdateControlHotkeys(QWidget* hotkeys_widget)
    {
        if (current_controller_ != nullptr)
        {
            qDeleteAll(hotkeys_widget->children());
            current_controller_->UpdateControlHotkeys(hotkeys_widget);
        }
    }

    void ViewModel::SetFieldOfView(int value)
    {
        if (view_state_adapter_ != nullptr)
        {
            float fov = SliderRangeConverter::FieldOfViewFromSlider(value);
            view_state_adapter_->SetFieldOfView(fov);
            SetModelData(kSidePaneViewFieldOfView, QString::number(fov, kQtFloatFormat, 0));
        }
    }

    void ViewModel::SetNearPlane(int value)
    {
        if (view_state_adapter_ != nullptr)
        {
            view_state_adapter_->SetNearPlaneMultiplier(SliderRangeConverter::NearPlaneValueFromSlider(value));
        }
    }

    void ViewModel::SetMovementSpeed(int value)
    {
        if (view_state_adapter_ != nullptr)
        {
            float speed = MovementSpeedValueFromSlider(value);
            view_state_adapter_->SetMovementSpeed(speed);
            SetModelData(kSidePaneViewMovementSpeed, QString::number(speed, kQtFloatFormat, 0));
        }
    }

    void ViewModel::SetCameraControllerParameters(bool controller_changed)
    {
        if (view_state_adapter_ != nullptr)
        {
            // Check if not the same camera.
            if (controller_changed || current_controller_ == nullptr)
            {
                current_controller_ = camera_controllers_.GetControllerByName(camera_controllers_.GetControllerNames()[camera_controls_.control_style_index]);
                view_state_adapter_->SetCameraController(current_controller_);
            }
            auto camera = current_controller_->GetCamera();

            if (camera)
            {
                auto origin  = camera->GetPosition();
                auto forward = camera->GetForward();
                auto up      = camera->GetUp();
                current_controller_->SetCameraOrientation(camera_controls_.orientation);
                current_controller_->FitCameraParams(origin, forward, up);
            }
            else
            {
                current_controller_->SetCameraOrientation(camera_controls_.orientation);
            }
        }
    }

    void ViewModel::SetHeatmapUpdateCallback(std::function<void(rra::renderer::HeatmapData)> heatmap_update_callback)
    {
        heatmap_update_callback_ = heatmap_update_callback;
    }

    void ViewModel::SetMovementSpeedLimit(float maximum_speed)
    {
        movement_speed_maximum_ = maximum_speed;
        movement_speed_minimum_ = kMinimumMovementSpeedMultiplier * maximum_speed;
    }

    bool ViewModel::SetControlStyle(int index)
    {
        int old_index                        = camera_controls_.control_style_index;
        camera_controls_.control_style_index = index;
        return index != old_index;
    }

    void ViewModel::SetInvertVertical(bool enabled)
    {
        camera_controls_.orientation.flip_vertical = enabled;
    }

    void ViewModel::SetInvertHorizontal(bool enabled)
    {
        camera_controls_.orientation.flip_horizontal = enabled;
    }

    void ViewModel::ToggleInvertVertical()
    {
        camera_controls_.orientation.flip_vertical = !camera_controls_.orientation.flip_vertical;
    }

    void ViewModel::ToggleInvertHorizontal()
    {
        camera_controls_.orientation.flip_horizontal = !camera_controls_.orientation.flip_horizontal;
    }

    void ViewModel::SetOrientation(rra::ViewerIOOrientation orientation)
    {
        camera_controls_.orientation = orientation;
    }

    void ViewModel::SetUpAxisAsX()
    {
        camera_controls_.orientation.up_axis = ViewerIOUpAxis::kUpAxisX;
    }

    void ViewModel::SetUpAxisAsY()
    {
        camera_controls_.orientation.up_axis = ViewerIOUpAxis::kUpAxisY;
    }

    void ViewModel::SetUpAxisAsZ()
    {
        camera_controls_.orientation.up_axis = ViewerIOUpAxis::kUpAxisZ;
    }

    void ViewModel::SetArchitectureToNavi2()
    {
        if (render_state_adapter_)
        {
            render_state_adapter_->SetArchitectureToNavi2();
        }
    }

    void ViewModel::SetArchitectureToNavi3()
    {
        if (render_state_adapter_)
        {
            render_state_adapter_->SetArchitectureToNavi3();
        }
    }

    void ViewModel::EnableRayFlagsAcceptFirstHit()
    {
        render_state_adapter_->SetRayFlagAcceptFirstHit(true);
    }

    void ViewModel::DisableRayFlagsAcceptFirstHit()
    {
        render_state_adapter_->SetRayFlagAcceptFirstHit(false);
    }

    void ViewModel::SetOrthographic(bool ortho)
    {
        // If setting UI on trace load, current_controller_ will be null, so no need to update.
        if (current_controller_)
        {
            auto cam = current_controller_->GetCamera();
            cam->SetOrthographic(ortho);
        }
    }

    const ViewerIOOrientation& ViewModel::GetCameraControllerOrientation() const
    {
        return camera_controls_.orientation;
    }

}  // namespace rra
