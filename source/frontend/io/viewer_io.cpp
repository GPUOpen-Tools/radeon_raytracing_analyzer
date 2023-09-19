//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of qt based user input controller interface.
//=============================================================================

#include "viewer_io.h"
#include "models/side_panels/view_model.h"

#include <sstream>

#include <QHBoxLayout>
#include <QMenu>
#include <QMouseEvent>
#include <QApplication>
#include <algorithm>
#include "util/string_util.h"
#include "constants.h"
#include "qt_common/custom_widgets/scaled_label.h"

namespace rra
{
    glm::vec3 ViewerIOOrientation::Confine(glm::vec3 euler) const
    {
        float lower_bound = -90.0f + glm::epsilon<float>();
        float upper_bound = 90.0f - glm::epsilon<float>();

        switch (up_axis)
        {
        case ViewerIOUpAxis::kUpAxisX:
            euler.z = 90.0f;
            lower_bound += 180.0f;
            upper_bound += 180.0f;
            break;
        case ViewerIOUpAxis::kUpAxisY:
            euler.z = 0.0f;
            break;
        case ViewerIOUpAxis::kUpAxisZ:
            euler.z = 0.0f;
            lower_bound += 90.0f;
            upper_bound += 90.0f;
            break;
        }

        lower_bound += !flip_vertical ? 0.0f : 180.0f;
        upper_bound += !flip_vertical ? 0.0f : 180.0f;
        euler.x = glm::min(glm::max(euler.x, lower_bound), upper_bound);

        return euler;
    }

    glm::vec3 ViewerIOOrientation::GetEulerByForward(glm::vec3 forward) const
    {
        glm::vec3 euler = {};

        float flip = !flip_vertical ? 0.0f : glm::pi<float>();
        float sign = !flip_vertical ? 1.0f : -1.0f;

        switch (up_axis)
        {
        case ViewerIOUpAxis::kUpAxisX:
            euler.x = glm::pi<float>() + sign * glm::atan(forward.x, glm::sqrt(forward.y * forward.y + forward.z * forward.z)) + flip;
            euler.y = glm::atan(forward.y, forward.z) + (!flip_horizontal ? flip : -flip - glm::pi<float>());
            euler.z = 90.0f;
            break;
        case ViewerIOUpAxis::kUpAxisY:
            euler.x = sign * glm::atan(forward.y, glm::sqrt(forward.z * forward.z + forward.x * forward.x)) + flip;
            euler.y = glm::atan(forward.z, -forward.x) + glm::half_pi<float>() + flip;
            euler.z = 0.0f;
            break;
        case ViewerIOUpAxis::kUpAxisZ:
            euler.x = glm::half_pi<float>() + sign * glm::atan(forward.z, glm::sqrt(forward.x * forward.x + forward.y * forward.y)) + flip;
            euler.y = -glm::atan(forward.y, -forward.x) + glm::half_pi<float>() + flip;
            euler.z = 0.0f;
            break;
        }

        euler.y *= (!flip_horizontal ? 1.0f : -1.0f);

        return glm::degrees(euler);
    }

    glm::vec3 ViewerIOOrientation::MapEuler(glm::vec3 euler) const
    {
        glm::vec3 output = {};

        switch (up_axis)
        {
        case ViewerIOUpAxis::kUpAxisX:
            output.x = euler.x;
            output.y = euler.y;
            output.z = euler.z;
            break;
        case ViewerIOUpAxis::kUpAxisY:
            output.x = euler.x;
            output.y = euler.y;
            output.z = euler.z;
            break;
        case ViewerIOUpAxis::kUpAxisZ:
            output.x = euler.x;
            output.y = euler.z;
            output.z = euler.y;
            break;
        }
        return output;
    }

    glm::mat3 ViewerIOOrientation::GetReflectionMatrix() const
    {
        bool flip_x = false;
        bool flip_y = false;
        bool flip_z = false;

        // The selected up axis defines which axis we should treat as up and horizontal
        // so we handle each case individually.
        if (up_axis == ViewerIOUpAxis::kUpAxisX)
        {
            flip_z = flip_horizontal;
        }
        else if (up_axis == ViewerIOUpAxis::kUpAxisY)
        {
            flip_x = flip_horizontal;
        }
        else if (up_axis == ViewerIOUpAxis::kUpAxisZ)
        {
            flip_x = flip_horizontal;
        }

        float x = flip_x ? -1.0f : 1.0f;
        float y = flip_y ? -1.0f : 1.0f;
        float z = flip_z ? -1.0f : 1.0f;

        return glm::mat3(glm::scale(glm::mat4(1.0f), glm::vec3(x, y, z)));
    }

    glm::vec3 ViewerIOOrientation::GetDefaultEuler() const
    {
        float sign = 0.0f;
        if (flip_vertical)
        {
            sign = 180.0f;
        }

        glm::vec3 euler = {};
        if (up_axis == ViewerIOUpAxis::kUpAxisX)
        {
            euler = {sign + 180.0f, 180.0f, 90.0f};
        }
        else if (up_axis == ViewerIOUpAxis::kUpAxisY)
        {
            euler = {sign, 0.0f, 0.0f};
        }
        else if (up_axis == ViewerIOUpAxis::kUpAxisZ)
        {
            euler = {90.0f + sign, 90.0f, 0.0f};
        }

        euler += glm::vec3(-45.0f, 45.0f, 0.0f);

        if (flip_horizontal)
        {
            if (up_axis == ViewerIOUpAxis::kUpAxisX)
            {
                euler.y += 90.0f;
            }
            else if (up_axis == ViewerIOUpAxis::kUpAxisY)
            {
                euler.y -= 90.0f;
            }
            else if (up_axis == ViewerIOUpAxis::kUpAxisZ)
            {
                euler.y += 90.0f;
            }
        }

        return euler;
    }

    glm::vec3 ViewerIOOrientation::GetControlMapping() const
    {
        glm::vec3 control_mapping = {1.0f, 1.0f, 0.0f};

        if (flip_vertical)
        {
            control_mapping.y *= -1.0f;
        }

        if (up_axis == ViewerIOUpAxis::kUpAxisX)
        {
            control_mapping.y *= -1.0f;
        }

        return control_mapping;
    }

    std::string ViewerIO::GetReadableString() const
    {
        auto      camera = GetCamera();
        glm::vec3 pos    = camera->GetArcCenterPosition();

        std::string result = "Control style" + rra::text::kDelimiterBinary + rra::text::kDelimiter + GetName() + " control style\n";
        result += "Euler angles" + rra::text::kDelimiterBinary + rra::text::kDelimiter;
        result += std::to_string(euler_angles_.x) + rra::text::kDelimiter + std::to_string(euler_angles_.y) + rra::text::kDelimiter +
                  std::to_string(euler_angles_.z) + '\n';
        result += "Arc center position" + rra::text::kDelimiterBinary + rra::text::kDelimiter;
        result += std::to_string(pos.x) + rra::text::kDelimiter + std::to_string(pos.y) + rra::text::kDelimiter + std::to_string(pos.z) + '\n';
        result += "Arc radius" + rra::text::kDelimiterBinary + rra::text::kDelimiter + std::to_string(arc_radius_) + '\n';
        result += "Up axis" + rra::text::kDelimiterBinary + rra::text::kDelimiter;

        switch (camera_orientation_.up_axis)
        {
        case ViewerIOUpAxis::kUpAxisX:
            result += "X\n";
            break;
        case ViewerIOUpAxis::kUpAxisY:
            result += "Y\n";
            break;
        case ViewerIOUpAxis::kUpAxisZ:
            result += "Z\n";
            break;
        }

        result += "Horizontal invert" + rra::text::kDelimiterBinary + rra::text::kDelimiter;
        result += camera_orientation_.flip_horizontal ? "true\n" : "false\n";
        result += "Vertical invert" + rra::text::kDelimiterBinary + rra::text::kDelimiter;
        result += camera_orientation_.flip_vertical ? "true\n" : "false\n";
        result += "Field of view" + rra::text::kDelimiterBinary + rra::text::kDelimiter + std::to_string(camera->GetFieldOfView()) + '\n';
        result += "Orthographic" + rra::text::kDelimiterBinary + rra::text::kDelimiter;
        result += GetCamera()->Orthographic() ? "true\n" : "false\n";

        return result;
    }

    ViewerIOCameraPasteResult ViewerIO::SetStateFromReadableString(const std::string& readable_string)
    {
        // Stores the internal state of ViewerIO objects that is needed to serialize/deserialize.
        // We don't change the controller's state as we read the string since we may return an
        // error code and don't want to leave the camera in a partially changed state.
        // So temporarily store the values in this struct until we know there are no errors.
        struct
        {
            glm::vec3           euler_angles{};
            glm::vec3           arc_center_position{};
            float               arc_radius{};
            ViewerIOOrientation orientation{};
            float               fov{};
            bool                orthographic{};
        } state;

        std::istringstream        stream(readable_string);
        std::string               line;
        ViewerIOCameraPasteResult result = ViewerIOCameraPasteResult::kSuccess;

        while (std::getline(stream, line))
        {
            std::vector<std::string> split = string_util::Split(line, rra::text::kDelimiterBinary);
            std::string&             name  = split[0];
            std::string              val   = string_util::Trim(split[1]);

            if (name == "Control style")
            {
                continue;
            }
            else if (name == "Euler angles")
            {
                std::istringstream ss(val);

                if (!(ss >> state.euler_angles.x >> state.euler_angles.y >> state.euler_angles.z))
                {
                    return ViewerIOCameraPasteResult::kFailure;
                }
            }
            else if (name == "Arc center position")
            {
                std::istringstream ss(val);

                if (!(ss >> state.arc_center_position.x >> state.arc_center_position.y >> state.arc_center_position.z))
                {
                    return ViewerIOCameraPasteResult::kFailure;
                }
            }
            else if (name == "Arc radius")
            {
                std::istringstream ss(val);

                if (!(ss >> state.arc_radius))
                {
                    return ViewerIOCameraPasteResult::kFailure;
                }
            }
            else if (name == "Up axis")
            {
                if (val == "X")
                {
                    state.orientation.up_axis = ViewerIOUpAxis::kUpAxisX;
                }
                else if (val == "Y")
                {
                    state.orientation.up_axis = ViewerIOUpAxis::kUpAxisY;
                }
                else if (val == "Z")
                {
                    state.orientation.up_axis = ViewerIOUpAxis::kUpAxisZ;
                }
                else
                {
                    return ViewerIOCameraPasteResult::kFailure;
                }
            }
            else if (name == "Horizontal invert")
            {
                if (val == "true")
                {
                    state.orientation.flip_horizontal = true;
                }
                else if (val == "false")
                {
                    state.orientation.flip_horizontal = false;
                }
                else
                {
                    return ViewerIOCameraPasteResult::kFailure;
                }
            }
            else if (name == "Vertical invert")
            {
                if (val == "true")
                {
                    state.orientation.flip_vertical = true;
                }
                else if (val == "false")
                {
                    state.orientation.flip_vertical = false;
                }
                else
                {
                    return ViewerIOCameraPasteResult::kFailure;
                }
            }
            else if (name == "Field of view")
            {
                std::istringstream ss(val);

                if (!(ss >> state.fov))
                {
                    return ViewerIOCameraPasteResult::kFailure;
                }
            }
            else if (name == "Orthographic")
            {
                if (val == "true")
                {
                    if (!SupportsOrthographicProjection())
                    {
                        // This means copied data was manually edited to enable orthographic projection
                        // in a control style that doesn't support it.
                        result = ViewerIOCameraPasteResult::kOrthographicNotSupported;
                    }
                    state.orthographic = true;
                }
                else if (val == "false")
                {
                    state.orthographic = false;
                }
                else
                {
                    return ViewerIOCameraPasteResult::kFailure;
                }
            }
            else
            {
                return ViewerIOCameraPasteResult::kFailure;
            }
        }

        auto camera = GetCamera();

        // Now actually change state
        euler_angles_ = state.euler_angles;
        camera->SetArcCenterPosition(state.arc_center_position);
        arc_radius_ = state.arc_radius;
        SetCameraOrientation(state.orientation);
        view_model_->SetOrientation(state.orientation);
        camera->SetFieldOfView(state.fov);
        camera->SetOrthographic(state.orthographic);

        updated_ = true;
        view_model_->Update();

        return result;
    }

    void ViewerIO::SetViewerCallbacks(ViewerIOCallbacks callbacks)
    {
        viewer_callbacks_ = callbacks;
    }

    void ViewerIO::SetViewModel(ViewModel* view_model)
    {
        view_model_ = view_model;
    }

    void ViewerIO::SetViewModelUpByCardinalMax(glm::vec3 up)
    {
        auto abs_up = glm::abs(up);
        if (view_model_)
        {
            ViewerIOOrientation orientation = GetCameraOrientation();

            if (abs_up.x > abs_up.y && abs_up.x > abs_up.z)
            {
                orientation.up_axis       = ViewerIOUpAxis::kUpAxisX;
                orientation.flip_vertical = up.x < 0.0f ? true : false;
            }
            else if (abs_up.y > abs_up.x && abs_up.y > abs_up.z)
            {
                orientation.up_axis       = ViewerIOUpAxis::kUpAxisY;
                orientation.flip_vertical = up.y < 0.0f ? true : false;
            }
            else if (abs_up.z > abs_up.x && abs_up.z > abs_up.y)
            {
                orientation.up_axis       = ViewerIOUpAxis::kUpAxisZ;
                orientation.flip_vertical = up.z < 0.0f ? true : false;
            }

            view_model_->SetOrientation(orientation);
            view_model_->SetCameraControllerParameters(false, view_model_->GetParentPaneId());
            UpdateViewModel();
        }
    }

    void ViewerIO::UpdateViewModel()
    {
        if (view_model_)
        {
            view_model_->Update();
        }
    }

    void ViewerIO::SetCameraOrientation(ViewerIOOrientation camera_orientation)
    {
        camera_orientation_ = camera_orientation;
        updated_            = true;
    }

    ViewerIOOrientation ViewerIO::GetCameraOrientation() const
    {
        return camera_orientation_;
    }

    void ViewerIO::HandleContextMenu(QMouseEvent* mouse_event, glm::vec2 window_size, const renderer::Camera* camera)
    {
        if (!viewer_callbacks_.get_context_options)
        {
            return;
        }

        QMenu menu;
        menu.setStyle(QApplication::style());

        auto      mouse_pos = mouse_event->pos();
        glm::vec2 coords    = {mouse_pos.x(), mouse_pos.y()};

        // Normalize coords to -1 to 1 space.
        coords /= window_size;
        coords *= 2.0f;
        coords -= 1.0f;

        // Flip y to match screen space.
        coords.y *= -1.0f;

        auto ray = camera->CastRay(coords);

        SceneContextMenuRequest request = {};
        request.location                = SceneContextMenuLocation::kSceneContextMenuLocationViewer;
        request.origin                  = ray.origin;
        request.direction               = ray.direction;

        auto context_options = viewer_callbacks_.get_context_options(request);

        context_options["Focus on selection"] = [&]() { should_focus_on_selection_ = true; };

        for (const auto& option_action : context_options)
        {
            auto action = new QAction(QString::fromStdString(option_action.first));
            menu.addAction(action);
        }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QAction* action = menu.exec(mouse_event->globalPos());
#else
        QAction* action = menu.exec(mouse_event->globalPosition().toPoint());
#endif

        if (action != nullptr)
        {
            context_options[action->text().toStdString()]();
        }
    }

    void ViewerIO::FitCameraParams(glm::vec3 position, glm::vec3 forward, glm::vec3 up)
    {
        RRA_UNUSED(up);
        auto orientation = GetCameraOrientation();
        auto camera      = GetCamera();
        if (camera)
        {
            euler_angles_ = orientation.GetEulerByForward(forward);
            camera->SetArcRadius(arc_radius_);
            camera->SetArcCenterPosition(position + forward * camera->GetArcRadius());
        }

        updated_ = true;

        if (view_model_)
        {
            view_model_->Update();
        }
    }

    void ViewerIO::InvalidateLastMousePosition()
    {
        last_mouse_position_ = kInvalidPosition;
    }

    float ViewerIO::GetMouseMoveDelta() const
    {
        return mouse_move_delta_;
    }

    void ViewerIO::MoveToOrigin()
    {
        auto camera = GetCamera();
        ResetArcRadius();
        camera->SetArcRadius(arc_radius_);
        camera->Translate(-camera->GetPosition());
    }

    void ViewerIO::FocusOnSelection()
    {
        should_focus_on_selection_ = true;
    }

    void ViewerIO::ControlStyleChanged()
    {
        // By default, do nothing.
    }

    bool ViewerIO::MouseMovedWithinDelta() const
    {
        glm::vec2 press_pos   = glm::vec2((float)mouse_press_position_.x(), (float)mouse_press_position_.y());
        glm::vec2 release_pos = glm::vec2((float)last_mouse_position_.x(), (float)last_mouse_position_.y());

        return glm::distance(press_pos, release_pos) < mouse_move_delta_;
    }

    void ViewerIO::ResetKeyStates()
    {
        for (auto& i : key_states_)
        {
            i.second = false;
        }
    }

    void ViewerIO::MousePressed(QMouseEvent* mouse_event)
    {
        if (last_mouse_button_pressed_ == Qt::MouseButton::NoButton)
        {
            last_mouse_button_pressed_ = mouse_event->button();
        }

        mouse_press_position_ = last_mouse_position_;
    }

    void ViewerIO::CalculateClosestHit(QMouseEvent*                    mouse_event,
                                       glm::vec2                       window_size,
                                       const renderer::Camera*         camera,
                                       SceneCollectionModelClosestHit& closest_hit) const
    {
        if (!viewer_callbacks_.select_from_scene)
        {
            return;
        }

        auto      mouse_pos = mouse_event->pos();
        glm::vec2 coords    = {mouse_pos.x(), mouse_pos.y()};

        // Normalize coords to -1 to 1 space.
        coords /= window_size;
        coords *= 2.0f;
        coords -= 1.0f;

        closest_hit = viewer_callbacks_.select_from_scene(camera, coords);
    }

    SceneCollectionModelClosestHit ViewerIO::MouseDoubleClicked(QMouseEvent* mouse_event, glm::vec2 window_size, const renderer::Camera* camera, bool cast_ray)
    {
        SceneCollectionModelClosestHit closest_hit = {};

        // If the mouse button was a left release, find a closest hit.
        if (cast_ray && mouse_event->button() == Qt::LeftButton)
        {
            CalculateClosestHit(mouse_event, window_size, camera, closest_hit);
        }

        // Forget about the last mouse button after a press is released.
        last_mouse_button_pressed_ = Qt::MouseButton::NoButton;

        return closest_hit;
    }

    SceneCollectionModelClosestHit ViewerIO::MouseReleased(QMouseEvent* mouse_event, glm::vec2 window_size, const renderer::Camera* camera, bool cast_ray)
    {
        SceneCollectionModelClosestHit closest_hit = {};

        // If the mouse button was a left release, find a closest hit.
        if (cast_ray && mouse_event->button() == Qt::LeftButton && MouseMovedWithinDelta())
        {
            CalculateClosestHit(mouse_event, window_size, camera, closest_hit);
        }

        if (mouse_event->button() == Qt::RightButton && MouseMovedWithinDelta())
        {
            HandleContextMenu(mouse_event, window_size, camera);
        }

        // Forget about the last mouse button after a press is released.
        last_mouse_button_pressed_ = Qt::MouseButton::NoButton;

        return closest_hit;
    }

    void ViewerIO::CreateControlHotkeysLayout(QWidget* widget, const std::vector<std::pair<std::string, std::string>>& control_strings)
    {
        auto top_layout = new QHBoxLayout();
        top_layout->setSpacing(2);
        top_layout->setContentsMargins(0, 0, 5, 0);
        auto left_column = new QVBoxLayout();
        left_column->setSpacing(2);
        left_column->setContentsMargins(0, 0, 0, 0);
        auto right_column = new QVBoxLayout();
        right_column->setSpacing(2);
        right_column->setContentsMargins(0, 0, 0, 0);

        top_layout->addLayout(left_column);
        top_layout->addLayout(right_column);

        for (auto& i : control_strings)
        {
            auto left  = new ScaledLabel(widget);
            auto right = new ScaledLabel(widget);

            left->setText(QString::fromStdString(i.first));
            right->setText(QString::fromStdString(i.second));
            right->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

            left_column->addWidget(left);
            right_column->addWidget(right);
        }

        widget->setLayout(top_layout);
    }

}  // namespace rra