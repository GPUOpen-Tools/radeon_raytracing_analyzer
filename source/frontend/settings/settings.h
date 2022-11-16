//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Define the settings and information about recently opened traces.
//=============================================================================

#ifndef RRA_SETTINGS_SETTINGS_H_
#define RRA_SETTINGS_SETTINGS_H_

#include <QVector>
#include <QMap>

#include "qt_common/utils/color_palette.h"
#include "qt_common/utils/common_definitions.h"

#include "constants.h"

/// @brief A struct for a setting key-value pair.
struct Setting
{
    QString name;   ///< Name of the setting.
    QString value;  ///< Value of the setting.
};

enum TreeviewNodeIDType
{
    kTreeviewNodeIDTypeOffset,          ///< Display Treeview Node ID's as offsets.
    kTreeviewNodeIDTypeVirtualAddress,  ///< Display Treeview Node ID's as GPU virtual addresses.

    kTreeviewNodeIDTypeMax,  ///< The maximum value of the TreeviewNodeIDType enums (not a valid value)
};

/// @brief Enum of all settings.
enum SettingID
{
    kSettingMainWindowGeometryData,
    kSettingMainWindowWidth,
    kSettingMainWindowHeight,
    kSettingMainWindowXpos,
    kSettingMainWindowYpos,

    kSettingLastFileOpenLocation,
#ifdef BETA_LICENSE
    kSettingLicenseAgreementVersion,
#endif  // BETA_LICENSE
    kSettingGeneralCheckForUpdatesOnStartup,
    kSettingGeneralCameraResetOnStyleChange,
    kSettingGeneralTreeviewNodeID,
    kSettingGeneralTraversalCounterMaximum,
    kSettingGeneralMovementSpeedLimit,
    kSettingGeneralFrustumCullRatio,
    kSettingGeneralDecimalPrecision,

    kSettingThemesAndColorsPalette,

    kSettingThemesAndColorsBoundingVolumeBox16,
    kSettingThemesAndColorsBoundingVolumeBox32,
    kSettingThemesAndColorsBoundingVolumeInstance,
    kSettingThemesAndColorsBoundingVolumeTriangle,
    kSettingThemesAndColorsBoundingVolumeProcedural,
    kSettingThemesAndColorsBoundingVolumeSelected,
    kSettingThemesAndColorsWireframeNormal,
    kSettingThemesAndColorsWireframeSelected,
    kSettingThemesAndColorsGeometrySelected,
    kSettingThemesAndColorsBackground1,
    kSettingThemesAndColorsBackground2,
    kSettingThemesAndColorsNonOpaque,
    kSettingThemesAndColorsOpaque,
    kSettingThemesAndColorsPositive,
    kSettingThemesAndColorsNegative,
    kSettingThemesAndColorsBuildAlgorithmNone,
    kSettingThemesAndColorsBuildAlgorithmFastBuild,
    kSettingThemesAndColorsBuildAlgorithmFastTrace,
    kSettingThemesAndColorsBuildAlgorithmBoth,
    kSettingThemesAndColorsInstanceOpaqueNone,
    kSettingThemesAndColorsInstanceOpaqueForceOpaque,
    kSettingThemesAndColorsInstanceOpaqueForceNoOpaque,
    kSettingThemesAndColorsInstanceOpaqueBoth,

    kSettingCount,
};

typedef QMap<SettingID, Setting> SettingsMap;

namespace rra
{
    /// @brief Support for the settings.
    class Settings
    {
    public:
        /// @brief Get the single settings object.
        ///
        /// @return a reference to the Settings.
        static Settings& Get();

        /// @brief Constructor.
        Settings();

        /// @brief Destructor.
        ~Settings();

        /// @brief Get file path to the settings.
        ///
        /// Find the 'Temp' folder on the local OS and create a subfolder (on linux, create .RRA folder).
        ///
        /// @return The name and location of the xml file.
        QString GetSettingsFileLocation() const;

        /// @brief Apply default settings and then override them if found on disk.
        ///
        /// @return true if settings were read from file, and false otherwise.
        bool LoadSettings();

        /// @brief Save the settings (and list of recent files) to disk.
        void SaveSettings() const;

        /// @brief Add a setting to our active map if it is recognized.
        ///
        /// @param [in] name  The name of the setting to add.
        /// @param [in] value The value of the setting.
        void AddPotentialSetting(const QString& name, const QString& value);

        /// @brief Add a recent file to the settings.
        ///
        /// @param [in] recent_file The recent file to add.
        void AddRecentFile(const RecentFileData& recent_file);

        /// @brief Update the recent files list.
        ///
        /// Called when loading a new trace file. If the file already exists in the recent files list,
        /// bump it to the top. If it doesn't exist then add it to the list.
        ///
        /// @param [in] trace_file_name  The trace file name to add/remove.
        /// @param [in] create_time      A string containing the trace creation time.
        void TraceLoaded(const QString& trace_file_name, const QString& create_time);

        /// @brief Remove a file from the recent files list.
        ///
        /// @param [in] trace_name The name of the file to remove.
        void RemoveRecentFile(const QString& trace_name);

        /// @brief Checks if a file is in the recent files list.
        ///
        /// @param [in] trace_name The name of the trace that is being searched for.
        ///
        /// @return True if the trace is in the recent files list, false if it is not.
        bool InRecentFilesList(const QString& trace_name);

        /// @brief Get a reference to the settings.
        ///
        /// @return A reference to the settings.
        const QMap<SettingID, Setting>& GetSettings() const;

        /// @brief Get a reference to the recent files list.
        ///
        /// @return A reference to the recent files list.
        const QVector<RecentFileData>& RecentFiles() const;

        /// @brief Get a setting as a string value.
        ///
        /// @param [in] setting_id The identifier for this setting.
        ///
        /// @return The string value for the setting specified.
        QString GetStringValue(const SettingID setting_id) const;

        /// @brief Set a setting as an string value.
        ///
        /// @param [in] setting_id The identifier for this setting.
        /// @param [in] value      The new value of the setting.
        void SetStringValue(SettingID setting_id, const QString& value);

        /// @brief Get window width from the settings.
        ///
        /// @return Main window width.
        int GetWindowWidth() const;

        /// @brief Get window height from the settings.
        ///
        /// @return Main window height.
        int GetWindowHeight() const;

        /// @brief Get window X screen position from the settings.
        ///
        /// @return Main window x position.
        int GetWindowXPos() const;

        /// @brief Get window Y screen position from the settings.
        ///
        /// @return Main window y position.
        int GetWindowYPos() const;

        /// @brief Get last file open location from the settings.
        ///
        /// @return Path to last opened file dir.
        QString GetLastFileOpenLocation() const;
#ifdef BETA_LICENSE
        /// @brief Get LicenseAgreementVersion from the settings.
        ///
        /// @return The value of LicenseAgreementVersion.
        const QString& GetLicenseAgreementVersion();

        /// @brief Set the value of LicenseAgreementVersion in the settings.
        ///
        /// @param [in] value The new value of LicenseAgreementVersion.
        void SetLicenseAgreementVersion(const QString& value);
#endif  // BETA_LICENSE

        /// @brief Sets the size of the window (width and height) in the settings.
        ///
        /// @param [in] width  The new width.
        /// @param [in] height The new height.
        void SetWindowSize(const int width, const int height);

        /// @brief Sets the position of the window on the screen in the settings.
        ///
        /// @param [in] x_pos The new X position.
        /// @param [in] y_pos The new Y Position.
        void SetWindowPos(const int x_pos, const int y_pos);

        /// @brief Set last file open location in the settings.
        ///
        /// @param [in] last_file_open_location  path + filename.
        void SetLastFileOpenLocation(const QString& last_file_open_location);

        /// @brief Set the value of kSettingGeneralCheckForUpdatesOnStartup in the settings.
        ///
        /// @param [in] value The new value of kSettingGeneralCheckForUpdatesOnStartup.
        void SetCheckForUpdatesOnStartup(const bool value);

        /// @brief Set the value of kSettingGeneralCameraResetOnStyleChange in the settings.
        /// 
        /// @param [in] value The new value of kSettingGeneralCameraResetOnStyleChange.
        void SetCameraResetOnStyleChange(const bool value);

        /// @brief Get the value of kSettingGeneralCheckForUpdatesOnStartup in the settings.
        ///
        /// @return The value of kSettingGeneralCheckForUpdatesOnStartup.
        bool GetCheckForUpdatesOnStartup() const;

        /// @brief Get the value of kSettingGeneralResetOnStyleChange.
        /// 
        /// @return The value of kSettingGeneralResetOnStyleChange.
        bool GetCameraResetOnStyleChange() const;

        /// @brief Get the value of the kSettingGeneralTreeviewNodeID in the settings.
        ///
        /// This corresponds to the Node ID type used in the Viewer pane treeviews.
        ///
        /// @return The node ID type.
        TreeviewNodeIDType GetTreeviewNodeIdType() const;

        /// @brief Set the value of the kSettingGeneralTreeviewNodeID in the settings.
        ///
        /// @param [in] node_id_type The node ID type.
        void SetTreeviewNodeIdType(TreeviewNodeIDType node_id_type);

        /// @brief Get the color palette from the settings.
        ///
        /// @return The current color palette.
        const ColorPalette& GetColorPalette() const;

        /// @brief Get the value of a palette id from the settings.
        ///
        /// @param [in] setting_id The id of the palette setting to query.
        ///
        /// @return The palette id value for the setting_id.
        int GetPaletteId(SettingID setting_id);

        /// @brief Set the value of a palette id in the settings.
        ///
        /// @param [in] setting_id The id of the setting to change.
        /// @param [in] value      The new palette id value of this item.
        void SetPaletteId(SettingID setting_id, const int value);

        /// @brief Cache the color palette.
        ///
        /// Creating a temporary ColorPalette object with a palette string for each palette query can be time consuming.
        void CachePalette();

        /// @brief Set the color palette.
        ///
        /// @param [in] value The new color palette value.
        void SetColorPalette(const ColorPalette& value);

        /// @brief Restore all color settings to their default value.
        void RestoreDefaultColors();

        /// @brief Restore all palette settings to their default value.
        void RestoreDefaultPalette();

        /// @brief Get a setting as a QColor object.
        ///
        /// @param [in] setting_id The identifier for this setting.
        ///
        /// @return The color value for the setting specified.
        QColor GetColorValue(SettingID setting_id) const;

        /// @brief Set the maximum slider value for the traversal counter.
        ///
        /// @param new_max The new maximum traversal counter slider value.
        void SetTraversalCounterMaximum(int new_max);

        /// @brief Set the maximum slider value for camera movement speed.
        ///
        /// @param new_speed The new maximum movement speed slider value.
        void SetMovementSpeedLimit(int new_speed);

        /// @brief Set the frustum cull ratio.
        ///
        /// @param new_ratio The new frustum cull ratio.
        void SetFrustumCullRatio(float new_ratio);

        /// @brief  Set the decimal precision.
        /// 
        /// @param new_precision The new decimal precision.
        void SetDecimalPrecision(int new_precision);

        /// @brief Get the maximum slider value for the traveral counter.
        ///
        /// @return The maximum traversal counter slider value.
        int GetTraversalCounterMaximum();

        /// @brief Get the maximum slider value for camera movement speed.
        ///
        /// @return The new maximum movement speed slider value.
        int GetMovementSpeedLimit();

        /// @brief Get the frustum cull ratio.
        ///
        /// @return The frustum cull ratio.
        float GetFrustumCullRatio();

        /// @brief Get the decimal precision.
        /// 
        /// @return The decimal precision.
        int GetDecimalPrecision();

    private:
        /// @brief Set the value of checkbox's state in the settings.
        ///
        /// @param [in] setting_id The setting id of checkbox.
        /// @param [in] value      The new value of checkbox state.
        void SetCheckBoxStatus(const SettingID setting_id, const bool value);

        /// @brief Get checkbox state from the settings.
        ///
        /// @param [in] setting_id The setting id of checkbox.
        ///
        /// @return The value of checkbox state.
        bool GetCheckBoxStatus(const SettingID setting_id) const;

        /// @brief Initialize our table with default settings.
        void InitDefaultSettings();

        /// @brief Store an active setting.
        ///
        /// @param [in] id      The identifier for this setting.
        /// @param [in] setting The setting containing name and value.
        void AddActiveSetting(SettingID id, const Setting& setting);

        /// @brief Get a setting as a boolean value.
        ///
        /// @param [in] setting_id The identifier for this setting.
        ///
        /// @return The boolean value for the setting specified.
        bool GetBoolValue(SettingID setting_id) const;

        /// @brief Get a setting as an integer value.
        ///
        /// @param [in] setting_id The identifier for this setting.
        ///
        /// @return The integer value for the setting specified.
        int GetIntValue(SettingID setting_id) const;

        /// @brief Get a setting as an float value.
        ///
        /// @param [in] setting_id The identifier for this setting.
        ///
        /// @return The float value for the setting specified.
        float GetFloatValue(SettingID setting_id) const;

        /// @brief Set a setting as a boolean value.
        ///
        /// @param [in] setting_id The identifier for this setting.
        /// @param [in] value      The new value of the setting.
        void SetBoolValue(SettingID setting_id, const bool value);

        /// @brief Set a setting as an integer value.
        ///
        /// @param [in] setting_id The identifier for this setting.
        /// @param [in] value      The new value of the setting.
        void SetIntValue(SettingID setting_id, const int value);

        /// @brief Set a setting as an float value.
        ///
        /// @param [in] setting_id The identifier for this setting.
        /// @param [in] value      The new value of the setting.
        void SetFloatValue(SettingID setting_id, const float value);

        /// @brief Restore a setting to its default value.
        ///
        /// @param [in] setting_id The identifier for this setting.
        void SetToDefaultValue(SettingID setting_id);

        /// @brief Remove a file from the recent files list.
        ///
        /// @param [in] file_name The name of the file to remove.
        void RemoveRecentFile(const char* file_name);

        QVector<RecentFileData> recent_files_;      ///< Vector of recently opened files.
        SettingsMap             active_settings_;   ///< Map containing active settings.
        SettingsMap             default_settings_;  ///< Map containing default settings.
        ColorPalette*           color_palette_;     ///< The currently cached color palette.
    };
}  // namespace rra

#endif  // RRA_SETTINGS_SETTINGS_H_
