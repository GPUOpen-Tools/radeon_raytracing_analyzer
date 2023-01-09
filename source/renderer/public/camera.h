//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the camera class.
//=============================================================================

#ifndef RRA_RENDERER_CAMERA_H_
#define RRA_RENDERER_CAMERA_H_

#include <glm/glm/glm.hpp>
#include <glm/glm/gtx/transform.hpp>
#include <map>

#include "public/renderer_types.h"

namespace rra
{
    namespace renderer
    {
        static const glm::vec3 kUnitX = glm::vec3(1.0f, 0.0f, 0.0f);  ///< The X unit vector.
        static const glm::vec3 kUnitY = glm::vec3(0.0f, 1.0f, 0.0f);  ///< The Y unit vector.
        static const glm::vec3 kUnitZ = glm::vec3(0.0f, 0.0f, 1.0f);  ///< The Z unit vector.

        static const float kDefaultCameraFieldOfView       = 45.0f;   ///< The default FOV in degrees.
        static const float kDefaultCameraNearPlaneDistance = 0.1f;    ///< The default near plane distance.
        static const float kDefaultCameraFarPlaneDistance  = 100.0f;  ///< The default far plane distance.

        static const float kFovCullingRatio =
            0.0001f;  ///< Cull the instance if the max dim of the bounding volume is less than camera fov multiplied by this ratio.

        /// Forward decleration for the controller.
        class Camera;

        /// @brief The camera controller interface to control the camera.
        class CameraController
        {
        public:
            /// @brief virtual destructor.
            virtual ~CameraController()
            {
            }

            /// @brief Process the user inputs to manipulate the camera.
            virtual void ProcessUserInputs() = 0;

            /// @brief Update the current camera.
            ///
            /// @param [in] camera The camera to update.
            void UpdateCamera(Camera* camera);

            /// @brief Get the camera associated with the CameraController
            Camera* GetCamera();

            /// @brief Get the camera associated with the CameraController
            const Camera* GetCamera() const;

        private:
            Camera* camera_ = nullptr;  ///< A pointer to the camera that is in use.
        };

        /// @brief A structure to represent a ray.
        struct CameraRay
        {
            glm::vec3 origin    = {};
            glm::vec3 direction = {};
        };

        /// @brief The camera object manages computing the view and projection matrices used to render a scene.
        class Camera
        {
        public:
            /// @brief Get the arc center position of the camera.
            ///
            /// @returns The arc center position camera position.
            glm::vec3 GetArcCenterPosition() const;

            /// @brief Set the arc center position of the camera.
            ///
            /// @param [in] arc_center_position The arc center positionm to set.
            void SetArcCenterPosition(glm::vec3 arc_center_position);

            /// @brief Get the field of view.
            ///
            /// @returns The field of view.
            float GetFieldOfView() const;

            /// @brief Set the field of view.
            ///
            /// @param [in] field_of_view The new field of view.
            void SetFieldOfView(float field_of_view);

            /// @brief Get the near clip plane distance.
            ///
            /// @returns The near clip plane distance.
            float GetNearClip() const;

            /// @brief Set the near plane distance.
            ///
            /// @param [in] near_clip The near clip plane distance.
            void SetNearClipScale(float near_clip_scale);

            /// @brief Set the near plane distance.
            ///
            /// @param [in] near_clip The near clip plane distance.
            void SetNearClipMultiplier(float near_clip_multiplier);

            /// @brief Get the near clip plane multiplier.
            ///
            /// @returns The near clip plane multiplier.
            float GetNearClipMultiplier() const;

            /// @brief Get the far clip plane distance.
            ///
            /// @returns The far clip plane distance.
            float GetFarClip() const;

            /// @brief Set the far plane distance.
            ///
            /// @param [in] far_clip The far clip plane distance.
            void SetFarClip(float far_clip);

            /// @brief Set the movement speed.
            ///
            /// @param [in] movement_speed The new movement speed.
            void SetMovementSpeed(float movement_speed);

            /// @brief Get the movement speed.
            ///
            /// @returns The computed movement speed. (multiplier * scale)
            float GetMovementSpeed() const;

            /// @brief Get the arc radius.
            ///
            /// @returns The arc radius of the camera.
            float GetArcRadius() const;

            /// @brief Set the arc radius.
            ///
            /// @param [in] The arc radius of the camera to set.
            void SetArcRadius(float arc_radius);

            /// @brief Configure the perspective projection matrix.
            ///
            /// @param [in] field_of_view The field of view specified in degrees.
            /// @param [in] near The near plane distance.
            /// @param [in] far The far plane distance.
            void SetPerspective(float field_of_view, float near, float far);

            /// @brief Update the aspect ratio.
            ///
            /// @param [in] aspect The aspect ratio.
            void UpdateAspectRatio(float aspect);

            /// @brief Get the aspect ratio.
            ///
            /// @returns The aspect ratio.
            float GetAspectRatio() const;

            /// @brief Get the up vector.
            ///
            /// @returns The up vector.
            glm::vec3 GetUp() const;

            /// @brief Get the forward vector.
            ///
            /// @returns The forward vector.
            glm::vec3 GetForward() const;

            /// @brief Move the camera by the given offset.
            ///
            /// @param [in] offset The offset vector to move the camera with.
            void Translate(glm::vec3 offset);

            /// @brief Move the camera along its forward vector by a specified number of units.
            ///
            /// @param [in] units The number of units to move the camera.
            void MoveForward(float units);

            /// @brief Move the camera along its up vector by a specified number of units.
            ///
            /// @param [in] units The number of units to move the camera.
            void MoveUp(float units);

            /// @brief Move the camera along its right vector by a specified number of units.
            ///
            /// @param [in] units The number of units to move the camera.
            void MoveRight(float units);

            /// @brief Set the rotation of the camera using euler angles.
            ///
            /// @param [in] rotation The camera rotation specified using euler angles.
            void SetEulerRotation(glm::vec3 rotation);

            /// @brief Set the controller of this camera.
            ///
            /// @param [in] camera_controller The controller to bind to this camera.
            void SetCameraController(CameraController* controller);

            /// @brief Get the camera controller.
            ///
            /// @returns A camera controller.
            CameraController* GetCameraController() const;

            /// @brief Process user inputs through the controller.
            void ProcessInputs();

            /// @brief Get the rotation matrix reflected about specified axes.
            ///
            /// @param [in] flip_x Whether to reflect about the x-axis.
            /// @param [in] flip_y Whether to reflect about the y-axis.
            /// @param [in] flip_z Whether to reflect about the z-axis.
            /// @returns The rotation matrix reflected about the specified axes.
            glm::mat3 GetReflectedRotationMatrix(bool flip_x, bool flip_y, bool flip_z) const;

            /// @brief Get the rotation matrix.
            ///
            /// @returns A rotation matrix.
            glm::mat3 GetRotationMatrix() const;

            /// @brief Set the rotation matrix.
            ///
            /// @param [in] rotation_matrix The rotation matrix to set.
            void SetRotationMatrix(glm::mat3 rotation_matrix);

            /// @brief Get position of the camera.
            ///
            /// @returns The camera position.
            glm::vec3 GetPosition() const;

            /// @brief Get view*projection matrix.
            ///
            /// @returns The view*projection matrix.
            glm::mat4 GetViewProjection() const;

            /// @brief Get the orthographic scale of the camera.
            /// @returns Orthographic scale of the camera.
            float GetOrthoScale() const;

            /// @brief Query if the camera is set to orthographic mode.
            /// @returns True if orthographic, false otherwise.
            bool Orthographic() const;

            /// @brief Generates ray data using normalized screen coordinates.
            ///
            /// @param [in] normalized_coords The coordinates on the screen space.
            ///
            /// @returns A structure representing an origin and a direction.
            CameraRay CastRay(glm::vec2 normalized_coords) const;

            /// @brief Get the view matrix.
            ///
            /// @returns The camera's view matrix.
            glm::mat4 GetViewMatrix() const;

            /// @brief Set whether the camera should use orthographic or perspective projection.
            void SetOrthographic(bool ortho);

            /// @brief Get frustum parameters from camera.
            ///
            /// @returns Frustum parameters.
            FrustumInfo GetFrustumInfo() const;

            bool      updated_    = false;  ///< The flag indicating that the view matrix must be recomputed.
            glm::mat4 projection_ = {};     ///< The projection matrix.

        private:
            /// @brief Recompute the projection matrix.
            void UpdateProjectionMatrix();

            /// @brief Recompute the camera position.
            void UpdatePosition();

            glm::mat4 rotation_                         = glm::mat4(1.0f);  ///< The rotation matrix.
            glm::vec3 position_                         = {};               ///< The camera position.
            glm::vec3 arc_center_position_              = {};               ///< The arc center position.
            float     field_of_view_                    = {};               ///< The field of view, specified in degrees.
            float     aspect_ratio_                     = {};               ///< The aspect ratio.
            float     near_scale_                       = 1.0f;             ///< The near plane scale.
            float     near_multiplier_                  = 0.05f;            ///< The near plane multiplier.
            float     far_                              = {};               ///< The far plane distance.
            float     arc_radius_                       = 0.1f;             ///< Arc radius for the arc mode.
            float     movement_speed_scroll_multiplier_ = {};               ///< The movement speed multiplier.
            float     ortho_scale_                      = {};               ///< How wide of an area the orthographic projection displays.
            bool      use_ortho_                        = false;            ///< Whether or not to use orthographic projection mode.

            CameraController* controller_ = nullptr;  ///< The controller that controls this camera.
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_CAMERA_H_
