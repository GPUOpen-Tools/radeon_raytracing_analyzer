//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the Renderer Adapter type.
//=============================================================================

#include "public/view_state_adapter.h"

namespace rra
{
    namespace renderer
    {
        ViewStateAdapter::ViewStateAdapter(Camera* camera)
            : camera(camera)
        {
        }

        float ViewStateAdapter::GetFieldOfView() const
        {
            return camera->GetFieldOfView();
        }

        void ViewStateAdapter::SetFieldOfView(float field_of_view)
        {
            camera->SetFieldOfView(field_of_view);
        }

        float ViewStateAdapter::GetNearPlaneMultiplier() const
        {
            return camera->GetNearClipMultiplier();
        }

        void ViewStateAdapter::SetNearPlaneMultiplier(float near_plane_multiplier)
        {
            camera->SetNearClipMultiplier(near_plane_multiplier);
        }

        float ViewStateAdapter::GetMovementSpeed() const
        {
            return camera->GetMovementSpeed();
        }

        void ViewStateAdapter::SetMovementSpeed(float movement_speed)
        {
            camera->SetMovementSpeed(movement_speed);
        }

        void ViewStateAdapter::SetCameraController(rra::renderer::CameraController* camera_controller)
        {
            camera->SetCameraController(camera_controller);
        }
    }  // namespace renderer
}  // namespace rra
