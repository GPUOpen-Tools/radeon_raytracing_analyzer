//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the View State Adapter interface. This type can
///         be used by frontend code to query and alter the scene view state.
//=============================================================================

#ifndef RRA_RENDERER_VIEW_STATE_ADAPTER_H_
#define RRA_RENDERER_VIEW_STATE_ADAPTER_H_

#include "public/camera.h"
#include "public/renderer_adapter.h"

namespace rra
{
    namespace renderer
    {
        /// @brief The ViewStateAdapter class declaration.
        class ViewStateAdapter : public RendererAdapter
        {
        public:
            /// @brief Constructor.
            ///
            /// @param [in] camera The target camera instance.
            ViewStateAdapter(Camera* camera);

            /// @brief Destructor.
            virtual ~ViewStateAdapter() = default;

            /// @brief Get the camera field of view in degrees.
            ///
            /// @returns The camera field of view.
            float GetFieldOfView() const;

            /// @brief Set the camera field of view.
            ///
            /// @param [in] field_of_view The field of view in degrees.
            void SetFieldOfView(float field_of_view);

            /// @brief Get the camera near plane multiplier.
            ///
            /// @returns The camera near plane multiplier.
            float GetNearPlaneMultiplier() const;

            /// @brief Set the camera near plane multiplier.
            ///
            /// @param [in] near_plane_multiplier The camera near plane multiplier.
            void SetNearPlaneMultiplier(float near_plane_multiplier);

            /// @brief Get the camera movement speed.
            ///
            /// @returns The camera movement speed.
            float GetMovementSpeed() const;

            /// @brief Set the camera movement speed multiplier.
            ///
            /// @param [in] movement_speed The new camera movement speed multiplier.
            void SetMovementSpeed(float movement_speed);

            /// @brief Set the camera controller.
            ///
            /// @param [in] camera_controller The new camera controller for the camera.
            void SetCameraController(rra::renderer::CameraController* camera_controller);

        private:
            Camera* camera = nullptr;  ///< The renderer to alter the render state for.
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VIEW_STATE_ADAPTER_H_
