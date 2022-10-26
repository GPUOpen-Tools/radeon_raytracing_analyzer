//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the camera class.
//=============================================================================

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "public/camera.h"

#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtx/euler_angles.hpp>
#include "public/renderer_types.h"

// windows.h defined near and far for some reason.
#undef far
#undef near

namespace rra
{
    namespace renderer
    {
        void CameraController::UpdateCamera(Camera* camera)
        {
            camera_ = camera;
        }

        Camera* CameraController::GetCamera()
        {
            return camera_;
        }

        const Camera* CameraController::GetCamera() const
        {
            return camera_;
        }

        glm::mat4 Camera::GetViewMatrix() const
        {
            // First rotate camera then translate it.
            glm::mat4 camera_transform = glm::translate(glm::mat4(1.0f), position_) * rotation_;
            // View matrix is inverse since instead of moving camera, we move the whole world.
            return glm::inverse(camera_transform);
        }

        glm::mat3 Camera::GetReflectedRotationMatrix(bool flip_x, bool flip_y, bool flip_z) const
        {
            float x = flip_x ? -1.0f : 1.0f;
            float y = flip_y ? -1.0f : 1.0f;
            float z = flip_z ? -1.0f : 1.0f;
            return glm::mat3(glm::scale(glm::mat4(1.0f), glm::vec3(x, y, z))) * glm::mat3(rotation_);
        }

        glm::mat3 Camera::GetRotationMatrix() const
        {
            return rotation_;
        }

        void Camera::SetRotationMatrix(glm::mat3 rotation_matrix)
        {
            rotation_ = rotation_matrix;
            UpdatePosition();
        }

        void Camera::UpdateProjectionMatrix()
        {
            if (use_ortho_)
            {
                float scale_factor = 0.01f;
                ortho_scale_       = scale_factor * arc_radius_ * field_of_view_;

                float top    = ortho_scale_;
                float right  = ortho_scale_ * aspect_ratio_;
                float left   = -right;
                float bottom = -top;
                projection_  = glm::ortho(left, right, bottom, top, -0.5f * far_, far_);
            }
            else
            {
                projection_ = glm::infinitePerspective(glm::radians(field_of_view_), aspect_ratio_, GetNearClip());
            }

            updated_ = true;
        }

        void Camera::SetOrthographic(bool ortho)
        {
            use_ortho_ = ortho;

            UpdateProjectionMatrix();
        }

        glm::vec3 Camera::GetArcCenterPosition() const
        {
            return arc_center_position_;
        }

        void Camera::UpdatePosition()
        {
            // Unit vector pointing the direction the camera is facing.
            // kUnitZ is negative since when camera has identity rotation,
            // z-axis points opposite direction of camera.
            glm::vec3 facing = glm::mat3(rotation_) * -kUnitZ;
            position_        = arc_center_position_ - facing * arc_radius_;
            updated_         = true;
        }

        void Camera::SetArcCenterPosition(glm::vec3 arc_center_position)
        {
            arc_center_position_ = arc_center_position;
            UpdatePosition();
        }

        float Camera::GetFieldOfView() const
        {
            return field_of_view_;
        }

        void Camera::SetFieldOfView(float field_of_view)
        {
            field_of_view_ = field_of_view;

            UpdateProjectionMatrix();
        }

        float Camera::GetNearClip() const
        {
            return near_multiplier_ * near_scale_;
        }

        void Camera::SetNearClipScale(float near_clip_scale)
        {
            near_scale_ = near_clip_scale;

            UpdateProjectionMatrix();
        }

        void Camera::SetNearClipMultiplier(float near_clip_multiplier)
        {
            near_multiplier_ = near_clip_multiplier;

            UpdateProjectionMatrix();
        }

        float Camera::GetNearClipMultiplier() const
        {
            return near_multiplier_;
        }

        float Camera::GetFarClip() const
        {
            return far_;
        }

        void Camera::SetFarClip(float far_clip)
        {
            far_ = far_clip;

            UpdateProjectionMatrix();
        }

        FrustumInfo Camera::GetFrustumInfo() const
        {
            FrustumInfo info{};
            info.camera_fov             = GetFieldOfView();
            info.camera_position        = GetPosition();
            info.camera_view_projection = GetViewProjection();
            info.fov_threshold_ratio    = kFovCullingRatio;
            return info;
        }

        void Camera::SetMovementSpeed(float movement_speed)
        {
            movement_speed_scroll_multiplier_ = movement_speed;
        }

        float Camera::GetMovementSpeed() const
        {
            return movement_speed_scroll_multiplier_;
        }

        float Camera::GetArcRadius() const
        {
            return arc_radius_;
        }

        void Camera::SetArcRadius(float arc_radius)
        {
            arc_radius_ = arc_radius;
            UpdatePosition();
        }

        void Camera::SetPerspective(float field_of_view, float near, float far)
        {
            field_of_view_ = field_of_view;
            near_scale_    = near / near_multiplier_;
            far_           = far;

            UpdateProjectionMatrix();
        }

        void Camera::UpdateAspectRatio(float aspect)
        {
            aspect_ratio_ = aspect;

            UpdateProjectionMatrix();
        }

        float Camera::GetAspectRatio() const
        {
            return aspect_ratio_;
        }

        glm::vec3 Camera::GetUp() const
        {
            return glm::normalize(glm::mat3(rotation_) * kUnitY);
        }

        glm::vec3 Camera::GetForward() const
        {
            return glm::normalize(glm::mat3(rotation_) * -kUnitZ);
        }

        void Camera::Translate(glm::vec3 offset)
        {
            arc_center_position_ += offset;
            UpdatePosition();
        }

        void Camera::MoveForward(float units)
        {
            glm::vec3 transformed_forward = glm::mat3(rotation_) * -kUnitZ;
            transformed_forward           = glm::normalize(transformed_forward) * units * GetMovementSpeed();

            Translate(transformed_forward);
        }

        void Camera::MoveUp(float units)
        {
            glm::vec3 transformed_up = glm::mat3(rotation_) * kUnitY;
            transformed_up           = glm::normalize(transformed_up) * units * GetMovementSpeed();

            Translate(transformed_up);
        }

        void Camera::MoveRight(float units)
        {
            glm::vec3 transformed_right = glm::mat3(rotation_) * kUnitX;
            transformed_right           = glm::normalize(transformed_right) * units * GetMovementSpeed();

            Translate(transformed_right);
        }

        void Camera::SetEulerRotation(glm::vec3 rotation)
        {
            rotation  = glm::radians(rotation);
            rotation_ = glm::eulerAngleZYX(rotation.z, rotation.y, rotation.x);
            UpdatePosition();
        }

        void Camera::Pitch(float angle)
        {
            rotation_ = glm::rotate(rotation_, angle, glm::mat3(rotation_) / glm::vec3(1.0f, 0.0f, 0.0f));
            UpdatePosition();
        }

        void Camera::Yaw(float angle)
        {
            rotation_ = glm::rotate(rotation_, angle, glm::mat3(rotation_) / glm::vec3(0.0f, 1.0f, 0.0f));
            UpdatePosition();
        }

        void Camera::Roll(float angle)
        {
            rotation_ = glm::rotate(rotation_, angle, glm::mat3(rotation_) / glm::vec3(0.0f, 0.0f, 1.0f));
            UpdatePosition();
        }

        glm::vec3 Camera::GetPosition() const
        {
            return position_;
        }

        glm::mat4 Camera::GetViewProjection() const
        {
            // Matrix multiplication is the other way around.
            // The final location of a vertex in the world will fall into projection in the last step.
            return projection_ * GetViewMatrix();
        }

        float Camera::GetOrthoScale() const
        {
            return ortho_scale_;
        }

        bool Camera::Orthographic() const
        {
            return use_ortho_;
        }

        CameraRay Camera::CastRay(glm::vec2 normalized_coords) const
        {
            CameraRay ray = {};

            if (use_ortho_)
            {
                float     offset_x = normalized_coords.x * ortho_scale_ * aspect_ratio_;
                float     offset_y = normalized_coords.y * ortho_scale_;
                glm::vec3 offset   = glm::mat3(rotation_) * glm::vec3(offset_x, offset_y, far_);
                ray.origin         = GetPosition() + offset;

                ray.direction = glm::mat3(rotation_) * glm::vec3(0.0f, 0.0f, -1.0f);
            }
            else
            {
                // Ray origin.
                ray.origin = GetPosition();

                // Ray direction. Uses a reconstructed perspective to avoid near-far plane issues.
                glm::mat4 bound_perspective     = glm::perspective(glm::radians(GetFieldOfView()), GetAspectRatio(), 0.1f, 1.0f);
                glm::vec4 perspective_direction = glm::inverse(bound_perspective) * glm::vec4(normalized_coords, 1.0f, 1.0f);
                perspective_direction           = perspective_direction / perspective_direction.w;
                ray.direction                   = glm::mat3(rotation_) * glm::normalize(glm::vec3(perspective_direction));
            }

            return ray;
        }

        void Camera::SetCameraController(CameraController* controller)
        {
            controller_ = controller;
            controller->UpdateCamera(this);
            updated_ = true;
        }

        CameraController* Camera::GetCameraController() const
        {
            return controller_;
        }

        void Camera::ProcessInputs()
        {
            if (controller_)
            {
                controller_->ProcessUserInputs();
            }
        }

    }  // namespace renderer
}  // namespace rra
