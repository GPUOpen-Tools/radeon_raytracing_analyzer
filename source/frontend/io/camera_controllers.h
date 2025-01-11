//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration to list camera controllers.
//=============================================================================

#ifndef RRA_CAMERA_CONTROLLERS_CAMERA_CONTROLLERS_H_
#define RRA_CAMERA_CONTROLLERS_CAMERA_CONTROLLERS_H_

#include "viewer_io.h"
#include <map>

namespace rra
{
    /// @brief A class to magane camera controllers.
    class CameraControllers
    {
    public:
        /// @brief Constructor.
        CameraControllers();

        /// @brief Destructor.
        ~CameraControllers();

        /// @brief Get the controller names.
        ///
        /// @returns A list of controller names.
        std::vector<std::string> GetControllerNames() const;

        /// @brief Get a controller by its name.
        ///
        /// @param [in] controller_name The controller's name.
        ///
        /// @returns the controller with the matching name.
        ViewerIO* GetControllerByName(const std::string& controller_name);

    private:
        std::map<std::string, ViewerIO*> controllers_;  ///< The camera controllers.
    };
}  // namespace rra

#endif
