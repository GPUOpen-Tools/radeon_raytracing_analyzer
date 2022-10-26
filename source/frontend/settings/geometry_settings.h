//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Define the window geometry settings.
//=============================================================================

#ifndef RRA_SETTINGS_GEOMETRY_SETTINGS_H_
#define RRA_SETTINGS_GEOMETRY_SETTINGS_H_

#include <QWidget>

#include "settings/settings.h"

namespace rra
{
    class GeometrySettings
    {
    public:
        /// @brief Constructor.
        GeometrySettings();

        /// @brief Destructor.
        ~GeometrySettings();

        /// @brief Saves a widget's position, size and state in the settings file as a hex string.
        ///
        /// @param [in] widget Pointer to the widget who's geometry is to be saved.
        static void Save(QWidget* widget);

        /// @brief Updates a widget's position, size and state from the settings file.
        ///
        /// @param [in] widget Pointer to the widget who's geometry is to be restored.
        static bool Restore(QWidget* widget);

        /// @brief Adjust a widget's geometry so that it fits on a single monitor.
        ///
        /// @param [in] widget Pointer to the widget who's geometry is to be adjusted.
        static void Adjust(QWidget* widget);
    };
}  // namespace rra

#endif  // RRA_SETTINGS_GEOMETRY_SETTINGS_H_
