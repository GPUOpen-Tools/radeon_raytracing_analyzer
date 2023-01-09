//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the CAD style camera controller.
//=============================================================================

#include "cad_camera_controller.h"
#include "public/rra_macro.h"

#include <QMouseEvent>

#include "models/side_panels/view_model.h"

namespace rra
{
    static const std::string kCADName      = "CAD";
    static const float       kArcRadiusMin = 0.00001f;

    CADController::CADController()
    {
        last_mouse_position_ = kInvalidPosition;
        elapsed_time_start_  = std::chrono::steady_clock::now();
    }

    CADController::~CADController()
    {
    }

    const std::string& CADController::GetName() const
    {
        return kCADName;
    }

    void CADController::Reset()
    {
        for (auto& i : key_states_)
        {
            i.second = false;
        }
        elapsed_time_start_ = std::chrono::steady_clock::now();
    }

    void CADController::MouseMoved(QPoint pos)
    {
        const QPoint& current_position = pos;
        if (last_mouse_position_ == kInvalidPosition)
        {
            last_mouse_position_ = current_position;
        }

        const QPoint drag_offset = current_position - last_mouse_position_;

        last_mouse_position_ = current_position;

        if (last_mouse_button_pressed_ == Qt::MouseButton::LeftButton)
        {
            euler_angles_ += GetCameraOrientation().GetControlMapping() * glm::vec3(-drag_offset.y(), -drag_offset.x(), 0) * 0.2f;
            updated_ = true;
        }
        else if (last_mouse_button_pressed_ == Qt::MouseButton::RightButton || last_mouse_button_pressed_ == Qt::MouseButton::MiddleButton)
        {
            pan_distance_ += glm::vec3(drag_offset.x() * -0.1f * arc_radius_, drag_offset.y() * 0.1 * arc_radius_, 0.0f) / last_camera_movement_speed_;
            updated_ = true;
        }
    }

    void CADController::MouseWheelMoved(QWheelEvent* wheel_event)
    {
        // Get the amount of mouse wheel moved. Inverted to match zoom out = wheel down.
        float mouse_wheel_delta = -wheel_event->angleDelta().y();

        // Multiply by an offset amount. Most mouse wheels rotate about 15.0 degree intervals.
        // Offset by a value so large that the exponential multiplication against arc_radius moves by a sensible amount.
        float offset = glm::max(1e3f + mouse_wheel_delta, 0.0f) / 1e3f;

        // Set the new value, check against epsilon.
        arc_radius_ = glm::max(arc_radius_ * offset, kArcRadiusMin);

        // Mark as updated.
        updated_ = true;
    }

    void CADController::KeyPressed(Qt::Key key)
    {
        key_states_[key] = true;
    }

    void CADController::KeyReleased(Qt::Key key)
    {
        key_states_[key] = false;
        key_releases_.push_back(key);
    }

    void CADController::ResetCameraPosition()
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
            euler_angles_  = GetCameraOrientation().GetDefaultEuler();
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

    void CADController::ResetArcRadius()
    {
        auto  cam        = GetCamera();
        float new_radius = 1.0f;
        cam->SetArcCenterPosition(cam->GetPosition() + new_radius * cam->GetForward());
        arc_radius_ = new_radius;
    }

    float CADController::FocusCameraOnVolume(renderer::Camera* camera, BoundingVolumeExtents volume)
    {
        glm::vec3 min = {volume.min_x, volume.min_y, volume.min_z};
        glm::vec3 max = {volume.max_x, volume.max_y, volume.max_z};

        float radius   = std::max(std::max(std::abs(max.x - min.x), std::abs(max.y - min.y)), std::abs(max.z - min.z)) * 0.5f;
        float distance = radius / static_cast<float>(tan(glm::radians(camera->GetFieldOfView() * 0.5f)));

        // Point the camera directly at the center of the bounding volume extents.
        glm::vec3 center = min + (max - min) / 2.0f;
        camera->SetArcCenterPosition(center);

        arc_radius_ = 1.5f * distance;

        updated_ = true;
        return radius;
    }

    void CADController::UpdateControlHotkeys(QWidget* widget)
    {
        std::vector<std::pair<std::string, std::string>> controls;

        controls.push_back({"Rotate camera", "Mouse Left Button"});
        controls.push_back({"Pan camera", "Mouse Right Button"});
        controls.push_back({"Zoom", "Mouse Wheel"});
        controls.push_back({"Move Forward, Left, Back, Right", "W, A, S, D"});
        controls.push_back({"Move Up, Down", "E and Q"});
        controls.push_back({"Look Up, Left, Down, Right", "Arrow Keys"});
        controls.push_back({"10x Slower, Faster Movement", "Ctrl, Shift"});
        controls.push_back({"Reset camera", "R"});
        controls.push_back({"Focus on selection", "F"});
        controls.push_back({"Update up axis", "U"});

        CreateControlHotkeysLayout(widget, controls);
    }

    void CADController::SetRotationFromForward(glm::vec3 forward)
    {
        auto orientation = GetCameraOrientation();
        auto camera      = GetCamera();
        if (camera)
        {
            euler_angles_ = orientation.GetEulerByForward(forward);
            updated_      = true;
        }
    }

    bool CADController::SupportsOrthographicProjection() const
    {
        return true;
    }

    bool CADController::SupportsUpAxis() const
    {
        return true;
    }

    uint32_t CADController::GetComboBoxIndex() const
    {
        return 0;
    }

    void CADController::ProcessUserInputs()
    {
        auto camera           = GetCamera();
        auto elapsed_time_end = std::chrono::steady_clock::now();
        auto delta_time       = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed_time_end - elapsed_time_start_).count() / 1e9f;
        delta_time            = glm::min(delta_time, 0.1F);
        elapsed_time_start_   = std::chrono::steady_clock::now();

        last_camera_movement_speed_ = camera->GetMovementSpeed();

        if (camera == nullptr)
        {
            return;
        }

        glm::vec3 translate_offset = pan_distance_;
        float     speed_modifier   = 1.0f;

        glm::vec3 rotate_offset = {};

        float zoom_offset = 0.0f;
        float ortho_zoom  = 0.0f;

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
                if (GetCamera()->Orthographic())
                {
                    ortho_zoom -= delta_time * GetCamera()->GetMovementSpeed();
                    updated_ = true;
                }
                else
                {
                    translate_offset += renderer::kUnitZ;
                }
                break;
            case Qt::Key_S:
                if (GetCamera()->Orthographic())
                {
                    ortho_zoom += delta_time * GetCamera()->GetMovementSpeed();
                    updated_ = true;
                }
                else
                {
                    translate_offset -= renderer::kUnitZ;
                }
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
                rotate_offset.y -= kLookRate * delta_time;
                break;
            case Qt::Key_Right:
                rotate_offset.y += kLookRate * delta_time;
                break;
            case Qt::Key_Up:
                rotate_offset.x -= kLookRate * delta_time;
                break;
            case Qt::Key_Down:
                rotate_offset.x += kLookRate * delta_time;
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

        // If orthographic mode is enabled, W and S keys zoom in and out.
        arc_radius_ += ortho_zoom * speed_modifier;
        arc_radius_ = std::max(kArcRadiusMin, arc_radius_);

        // Move the camera if the offset is non-zero.
        if (glm::length(translate_offset) > epsilon)
        {
            camera->MoveRight(translate_offset.x * delta_time * speed_modifier);
            camera->MoveUp(translate_offset.y * delta_time * speed_modifier);
            camera->MoveForward(translate_offset.z * delta_time * speed_modifier);
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
            arc_radius_ = arc_radius_ + (zoom_offset * arc_radius_) * delta_time;
            camera->SetArcRadius(arc_radius_);

            auto camera_orientation = GetCameraOrientation();
            euler_angles_ += camera_orientation.GetControlMapping() * rotate_offset;
            euler_angles_  = camera_orientation.Confine(euler_angles_);
            auto out_euler = camera_orientation.MapEuler(euler_angles_);
            camera->SetEulerRotation(out_euler);
            camera->SetRotationMatrix(camera_orientation.GetReflectionMatrix() * camera->GetRotationMatrix());
        }

        if (updated_ && viewer_model_)
        {
            view_model_->UpdateCameraTransformUI();
        }

        updated_      = false;
        pan_distance_ = {};
    }
}  // namespace rra