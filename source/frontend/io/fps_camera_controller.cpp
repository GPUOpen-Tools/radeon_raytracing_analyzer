//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the fps style camera controller.
//=============================================================================

#include "fps_camera_controller.h"
#include "public/rra_macro.h"

#include <QMouseEvent>

#include "models/side_panels/view_model.h"

namespace rra
{
    static const std::string kFPSName              = "FPS";
    static const float       kMouseWheelMultiplier = 1e-3f;

    FPSController::FPSController()
    {
        last_mouse_position_ = kInvalidPosition;
        elapsed_time_start_  = std::chrono::steady_clock::now();
    }

    FPSController::~FPSController()
    {
    }

    const std::string& FPSController::GetName() const
    {
        return kFPSName;
    }

    void FPSController::Reset()
    {
        for (auto& i : key_states_)
        {
            i.second = false;
        }
        elapsed_time_start_ = std::chrono::steady_clock::now();
    }

    void FPSController::MouseMoved(QPoint pos)
    {
        const QPoint& current_position = pos;
        if (last_mouse_position_ == kInvalidPosition)
        {
            last_mouse_position_ = current_position;
        }

        const QPoint drag_offset = current_position - last_mouse_position_;

        last_mouse_position_    = current_position;
        auto camera_orientation = GetCameraOrientation();

        if (last_mouse_button_pressed_ == Qt::MouseButton::MiddleButton)
        {
            pan_distance_ += glm::vec3(drag_offset.x() * -0.1f, drag_offset.y() * -0.1f, 0.0f);
            updated_ = true;
        }
        else if (last_mouse_button_pressed_ == Qt::MouseButton::RightButton)
        {
            euler_angles_ += camera_orientation.GetControlMapping() * glm::vec3(-drag_offset.y(), -drag_offset.x(), 0) * 0.2f;
            updated_ = true;
        }
    }

    void FPSController::MouseWheelMoved(QWheelEvent* wheel_event)
    {
        // Get the amount of mouse wheel moved.
        float mouse_wheel_delta = wheel_event->angleDelta().y();

        // Add up the multiplier.
        movement_speed_scroll_multiplier_ += mouse_wheel_delta * kMouseWheelMultiplier;

        // Mark as updated.
        updated_ = true;
    }

    void FPSController::KeyPressed(Qt::Key key)
    {
        key_states_[key] = true;
    }

    void FPSController::KeyReleased(Qt::Key key)
    {
        key_states_[key] = false;
        key_releases_.push_back(key);
    }

    void FPSController::ResetCameraPosition()
    {
        auto camera = GetCamera();

        if (!viewer_model_)
        {
            return;
        }

        BoundingVolumeExtents volume;
        auto                  has_volume = viewer_model_->GetSceneCollectionModel()->GetSceneBounds(viewer_bvh_index_, volume);
        if (has_volume)
        {
            float diagonal = glm::length(glm::vec3{volume.max_x - volume.min_x, volume.max_y - volume.min_y, volume.max_z - volume.min_z});
            euler_angles_ = GetCameraOrientation().GetDefaultEuler();
            arc_radius_   = 0.0f;
            camera->SetEulerRotation(GetCameraOrientation().MapEuler(euler_angles_));
            camera->SetRotationMatrix(GetCameraOrientation().GetReflectionMatrix() * camera->GetRotationMatrix());
            camera->SetFieldOfView(renderer::kDefaultCameraFieldOfView);
            camera->SetMovementSpeed(kDefaultSpeedDiagonalMultiplier * diagonal);
            auto radius = FocusCameraOnVolume(camera, volume);
            camera->SetFarClip(radius * kViewerIOFarPlaneMultiplier);
            updated_ = true;
        }

        if (view_model_)
        {
            view_model_->Update();
        }
    }

    void FPSController::ResetArcRadius()
    {
        arc_radius_ = 0.0f;
    }

    float FPSController::FocusCameraOnVolume(renderer::Camera* camera, BoundingVolumeExtents volume)
    {
        glm::vec3 min = {volume.min_x, volume.min_y, volume.min_z};
        glm::vec3 max = {volume.max_x, volume.max_y, volume.max_z};

        float radius   = std::max(std::max(std::abs(max.x - min.x), std::abs(max.y - min.y)), std::abs(max.z - min.z)) * 0.5f;
        float distance = radius / static_cast<float>(tan(glm::radians(camera->GetFieldOfView() * 0.5f)));

        glm::vec3 looking_direction = camera->CastRay({0.0f, 0.0f}).direction;

        // Point the camera directly at the center of the bounding volume extents.
        glm::vec3 center = min + (max - min) / 2.0f;
        camera->SetArcCenterPosition(center - looking_direction * distance * 1.5f);

        updated_ = true;
        return radius;
    }

    void FPSController::UpdateControlHotkeys(QWidget* widget)
    {
        std::vector<std::pair<std::string, std::string>> controls;

        controls.push_back({"Rotate camera", "Mouse Right Button"});
        controls.push_back({"Pan camera", "Mouse Middle Button"});
        controls.push_back({"Move Forward, Left, Back, Right", "W, A, S, D"});
        controls.push_back({"Move Up, Down", "E and Q"});
        controls.push_back({"Look Up, Left, Down, Right", "Arrow Keys"});
        controls.push_back({"10x Slower, Faster Movement", "Ctrl, Shift"});
        controls.push_back({"Reset camera", "R"});
        controls.push_back({"Focus on selection", "F"});
        controls.push_back({"Update up axis", "U"});

        CreateControlHotkeysLayout(widget, controls);
    }

    void FPSController::SetRotationFromForward(glm::vec3 forward)
    {
        auto orientation = GetCameraOrientation();
        auto camera      = GetCamera();
        if (camera)
        {
            euler_angles_ = orientation.GetEulerByForward(forward);
            updated_      = true;
        }
    }

    bool FPSController::SupportsOrthographicProjection() const
    {
        return false;
    }

    bool FPSController::SupportsUpAxis() const
    {
        return true;
    }

    uint32_t FPSController::GetComboBoxIndex() const
    {
        return 1;
    }

    void FPSController::ProcessUserInputs()
    {
        auto camera           = GetCamera();
        auto elapsed_time_end = std::chrono::steady_clock::now();
        auto elapsed_seconds  = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed_time_end - elapsed_time_start_).count() / 1e9f;
        elapsed_seconds       = glm::min(elapsed_seconds, 0.1F);
        elapsed_time_start_   = std::chrono::steady_clock::now();

        if (camera == nullptr)
        {
            return;
        }

        float multiplier = elapsed_seconds;

        glm::vec3 translate_offset = {pan_distance_.x, pan_distance_.y, 0.0f};
        float     speed_modifier   = 1.0f;

        glm::vec3 rotate_offset = {};

        float zoom_offset = 0.0f;

        static const float kLookRate = 90.0f;  // The rotation rate in degrees per second.

        for (const auto& key_state : key_states_)
        {
            if (!key_state.second)
            {
                continue;
            }

            switch (key_state.first)
            {
            case Qt::Key_W:
                translate_offset += renderer::kUnitZ;
                break;
            case Qt::Key_S:
                translate_offset -= renderer::kUnitZ;
                break;
            case Qt::Key_A:
                translate_offset -= renderer::kUnitX;
                break;
            case Qt::Key_D:
                translate_offset += renderer::kUnitX;
                break;
            case Qt::Key_Q:
                translate_offset -= renderer::kUnitY;
                break;
            case Qt::Key_E:
                translate_offset += renderer::kUnitY;
                break;
            case Qt::Key_Left:
                rotate_offset.y -= kLookRate * multiplier;
                break;
            case Qt::Key_Right:
                rotate_offset.y += kLookRate * multiplier;
                break;
            case Qt::Key_Up:
                rotate_offset.x -= kLookRate * multiplier;
                break;
            case Qt::Key_Down:
                rotate_offset.x += kLookRate * multiplier;
                break;
            case Qt::Key_PageUp:
                zoom_offset -= 1.0f;
                break;
            case Qt::Key_PageDown:
                zoom_offset += 1.0f;
                break;
            case Qt::Key_Shift:
                speed_modifier = 10.0f;
                break;
            case Qt::Key_Control:
                speed_modifier = 0.1f;
                break;
            case Qt::Key_R:
                ResetCameraPosition();
                break;
            case Qt::Key_F:
                should_focus_on_selection_ = true;
                break;
            case Qt::Key_U:
                SetViewModelUpByCardinalMax(camera->GetUp());
                break;
            default:
                break;
            }
        }

        // Process by released keys.
        for (auto& released_key : key_releases_)
        {
            switch (released_key)
            {
            case Qt::Key_U:
                SetViewModelUpByCardinalMax(camera->GetUp());
                break;

            default:
                break;
            }
        }
        key_releases_.clear();

        // Handle camera if the focus flag is set.
        if (should_focus_on_selection_ && viewer_model_)
        {
            BoundingVolumeExtents volume;
            auto                  has_volume = viewer_model_->GetSceneCollectionModel()->GetSceneSelectionBounds(viewer_bvh_index_, volume);
            if (has_volume)
            {
                FocusCameraOnVolume(camera, volume);
            }
            should_focus_on_selection_ = false;
        }

        static const float epsilon = 0.00001f;

        // Move the camera if the offset is non-zero.
        if (glm::length(translate_offset) > epsilon)
        {
            camera->MoveRight(translate_offset.x * multiplier * speed_modifier);
            camera->MoveUp(translate_offset.y * multiplier * speed_modifier);
            camera->MoveForward(translate_offset.z * multiplier * speed_modifier);
        }

        if (fabs(zoom_offset) > epsilon)
        {
            updated_ = true;
        }

        if (glm::length(rotate_offset) > epsilon)
        {
            updated_ = true;
        }

        if (!updated_)
        {
            updated_ = camera->updated_;
        }

        if (updated_)
        {
            arc_radius_ = arc_radius_ + (zoom_offset * arc_radius_) * multiplier;
            camera->SetArcRadius(arc_radius_);

            auto camera_orientation = GetCameraOrientation();
            euler_angles_ += camera_orientation.GetControlMapping() * rotate_offset;
            euler_angles_  = camera_orientation.Confine(euler_angles_);
            auto out_euler = camera_orientation.MapEuler(euler_angles_);
            camera->SetEulerRotation(out_euler);
            camera->SetRotationMatrix(camera_orientation.GetReflectionMatrix() * camera->GetRotationMatrix());

            auto      ray          = camera->CastRay({0.0f, 0.0f});
            glm::vec3 forward_unit = ray.direction;

            if (movement_speed_scroll_multiplier_ != 1.0f)
            {
                camera->SetMovementSpeed(camera->GetMovementSpeed() * movement_speed_scroll_multiplier_);
                UpdateViewModel();
            }

            glm::vec3 normalized_forward = glm::normalize(forward_unit);
            // Check for nan to prevent weird circumstances that make the scene disappear.
            // A simple way of checking for nan is to compare the variable with itself. Nan is not equal to nan.
            if (normalized_forward == normalized_forward)
            {
                glm::vec3 fps_forward = pan_distance_.z * normalized_forward;
                camera->SetArcCenterPosition(camera->GetArcCenterPosition() + fps_forward * camera->GetMovementSpeed());
            }
        }

        if (updated_ && viewer_model_)
        {
            view_model_->UpdateCameraTransformUI();
        }

        updated_                          = false;
        pan_distance_                     = {};
        movement_speed_scroll_multiplier_ = 1.0f;
    }
}  // namespace rra