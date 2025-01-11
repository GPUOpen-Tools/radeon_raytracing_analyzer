//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definitions to list camera controllers.
//=============================================================================

#include "camera_controllers.h"

#include "cad_camera_controller.h"
#include "fps_camera_controller.h"
#include "axis_free_camera_controller.h"

namespace rra
{
    static const std::string kControlStyleSuffix = " control style";

    CameraControllers::CameraControllers()
    {
        ViewerIO* controller = nullptr;

        controller                                                = new CADController();
        controllers_[controller->GetName() + kControlStyleSuffix] = controller;

        controller                                                = new FPSController();
        controllers_[controller->GetName() + kControlStyleSuffix] = controller;

        controller                                                = new AxisFreeController();
        controllers_[controller->GetName() + kControlStyleSuffix] = controller;
    }

    CameraControllers::~CameraControllers()
    {
        for (auto& i : controllers_)
        {
            delete i.second;
        }
    }

    std::vector<std::string> CameraControllers::GetControllerNames() const
    {
        std::vector<std::string> names;
        names.resize(controllers_.size());
        for (auto& i : controllers_)
        {
            names[i.second->GetComboBoxIndex()] = i.first;
        }
        return names;
    }

    ViewerIO* CameraControllers::GetControllerByName(const std::string& controller_name)
    {
        return controllers_[controller_name];
    }
}  // namespace rra
