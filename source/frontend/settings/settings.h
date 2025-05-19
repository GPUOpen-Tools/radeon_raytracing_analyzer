//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Define the settings and information about recently opened traces.
//=============================================================================

#ifndef RRA_SETTINGS_SETTINGS_H_
#define RRA_SETTINGS_SETTINGS_H_

#include <QMap>
#include <QVector>

#include "qt_common/utils/color_palette.h"
#include "qt_common/utils/common_definitions.h"

#include "public/renderer_types.h"

#include "constants.h"
#include "managers/pane_manager.h"

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

enum CullModeType
{
    kCullModeTypeNone,   ///< No face culling.
    kCullModeTypeFront,  ///< Front-face triangle culling.
    kCullModeTypeBack,   ///< Back-face triangle culling.

    kCullModeTypeMax,  ///< The maximum value of the CullModeType enums (not a valid value)
};

enum ControlStyleType
{
    kControlStyleTypeCAD,       ///< CAD control style.
    kControlStyleTypeFPS,       ///< FPS control style.
    kControlStyleTypeAxisFree,  ///< Axis-free control style.

    kControlStyleTypeMax,  ///< The maximum value of the ControlStyleType enums (not a valid value)
};

enum UpAxisType
{
    kUpAxisTypeX,  ///< Indicates the x-axis is the up axis.
    kUpAxisTypeY,  ///< Indicates the y-axis is the up axis.
    kUpAxisTypeZ,  ///< Indicates the z-axis is the up axis.

    kUpAxisTypeMax,  ///< The maximum value of the UpAxisType enums (not a valid value)
};

enum HeatmapColorType
{
    kHeatmapColorTypeTemperature,  ///< The temperature heatmap color.
    kHeatmapColorTypeSpectrum,     ///< The spectrum heatmap color.
    kHeatmapColorTypeGrayscale,    ///< The grayscale heatmap color.
    kHeatmapColorTypeViridis,      ///< The viridis heatmap color.
    kHeatmapColorTypePlasma,       ///< The plasma heatmap color.

    kHeatmapColorTypeMax,  ///< The maximum value of the HeatmapColorType enums (not a valid value)
};

/// @brief Coloring mode types set in the UI.
enum ColoringMode
{
    kColoringModeBVHColor,
    kColoringModeGeometryColor,
    kColoringModeHeatmapColor,
    kColoringModeTraversalCounterColor,
};

enum RenderingMode
{
    kRenderingModeGeometry,
    kRenderingModeTraversal,
};

enum ProjectionMode
{
    kProjectionModePerspective,
    kProjectionModeOrthographic,
};

/// @brief Checkbox setting types set in the UI.
enum CheckboxSetting
{
    kCheckboxSettingShowGeometry,
    kCheckboxSettingLockCamera,
    kCheckboxSettingShowAxisAlignedBVH,
    kCheckboxSettingShowInstanceTransform,
    kCheckboxSettingShowWireframe,
    kCheckboxSettingAcceptFirstHit,
    kCheckboxSettingCullBackFacingTriangles,
    kCheckboxSettingCullFrontFacingTriangles,
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
    kSettingGeneralCameraControlSync,
    kSettingGeneralTreeviewNodeID,
    kSettingGeneralTraversalCounterMaximum,
    kSettingGeneralMovementSpeedLimit,
    kSettingGeneralFrustumCullRatio,
    kSettingGeneralDecimalPrecision,
    kSettingGeneralPersistentUIState,
    kSettingGeneralDriverOverridesAllowNotifications,

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
    kSettingThemesAndColorsBackgroundLight1,
    kSettingThemesAndColorsBackgroundLight2,
    kSettingThemesAndColorsBackgroundDark1,
    kSettingThemesAndColorsBackgroundDark2,
    kSettingThemesAndColorsColorThemeMode,
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
    kSettingThemesAndColorsInvocationRaygen,
    kSettingThemesAndColorsInvocationClosestHit,
    kSettingThemesAndColorsInvocationAnyHit,
    kSettingThemesAndColorsInvocationIntersection,
    kSettingThemesAndColorsInvocationMiss,
    kSettingThemesAndColorsInvocationCallable,
    kSettingThemesAndColorsSelectedRayColor,
    kSettingThemesAndColorsRayColor,
    kSettingThemesAndColorsShadowRayColor,
    kSettingThemesAndColorsZeroMaskRayColor,

    // Persistent UI state.
    kSettingPersistenceCullMode,
    kSettingPersistenceUpAxis,
    kSettingPersistenceInvertVertical,
    kSettingPersistenceInvertHorizontal,
    kSettingPersistenceContinuousUpdate,
    kSettingPersistenceProjectionMode,

    // Persistent UI state for each view pane.
    kSettingPersistenceTLASControlStyle,
    kSettingPersistenceTLASBVHColoringMode,
    kSettingPersistenceTLASGeometryColoringMode,
    kSettingPersistenceTLASHeatmapColor,
    kSettingPersistenceTLASTraversalCounterMode,
    kSettingPersistenceTLASRenderingMode,
    kSettingPersistenceTLASShowGeometry,
    kSettingPersistenceTLASShowAxisAlignedBVH,
    kSettingPersistenceTLASShowInstanceTransform,  // TLAS only.
    kSettingPersistenceTLASShowWireframe,
    kSettingPersistenceTLASAcceptFirstHit,
    kSettingPersistenceTLASCullBackFacingTriangles,
    kSettingPersistenceTLASCullFrontFacingTriangles,
    kSettingPersistenceTLASFieldOfView,
    kSettingPersistenceTLASMovementSpeed,

    kSettingPersistenceBLASControlStyle,
    kSettingPersistenceBLASBVHColoringMode,
    kSettingPersistenceBLASGeometryColoringMode,
    kSettingPersistenceBLASHeatmapColor,
    kSettingPersistenceBLASTraversalCounterMode,
    kSettingPersistenceBLASRenderingMode,
    kSettingPersistenceBLASShowGeometry,
    kSettingPersistenceBLASShowAxisAlignedBVH,
    kSettingPersistenceBLASShowWireframe,
    kSettingPersistenceBLASAcceptFirstHit,
    kSettingPersistenceBLASCullBackFacingTriangles,
    kSettingPersistenceBLASCullFrontFacingTriangles,
    kSettingPersistenceBLASFieldOfView,
    kSettingPersistenceBLASMovementSpeed,

    kSettingPersistenceInspectorControlStyle,
    kSettingPersistenceInspectorBVHColoringMode,
    kSettingPersistenceInspectorGeometryColoringMode,
    kSettingPersistenceInspectorHeatmapColor,
    kSettingPersistenceInspectorTraversalCounterMode,
    kSettingPersistenceInspectorRenderingMode,
    kSettingPersistenceInspectorShowGeometry,
    kSettingPersistenceInspectorLockCamera,
    kSettingPersistenceInspectorShowAxisAlignedBVH,
    kSettingPersistenceInspectorShowWireframe,
    kSettingPersistenceInspectorAcceptFirstHit,
    kSettingPersistenceInspectorCullBackFacingTriangles,
    kSettingPersistenceInspectorCullFrontFacingTriangles,
    kSettingPersistenceInspectorFieldOfView,
    kSettingPersistenceInspectorMovementSpeed,

    kSettingCount,
};

typedef std::unordered_map<rra::RRAPaneId, std::unordered_map<int, SettingID> > SettingLookups;
typedef std::unordered_map<rra::RRAPaneId, SettingID>                           Pane2SettingMap;

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

        /// @brief Set the value of kSettingGeneralPersistentUIState in the settings.
        ///
        /// @param [in] value The new value of kSettingGeneralPersistentUIState.
        void SetPersistentUIState(const bool value);

        /// @brief Get the value of kSettingGeneralCheckForUpdatesOnStartup in the settings.
        ///
        /// @return The value of kSettingGeneralCheckForUpdatesOnStartup.
        bool GetCheckForUpdatesOnStartup() const;

        /// @brief Get the value of kSettingGeneralResetOnStyleChange.
        ///
        /// @return The value of kSettingGeneralResetOnStyleChange.
        bool GetCameraResetOnStyleChange() const;

        /// @brief Get the value of kSettingGeneralCameraResetOnStyleChange.
        ///
        /// @return The value of kSettingGeneralCameraResetOnStyleChange.
        bool GetCameraControlSync() const;

        /// @brief Get the value of kSettingGeneralPersistentUIState.
        ///
        /// @return The value of kSettingGeneralPersistentUIState.
        bool GetPersistentUIState() const;

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

        // Viewer persistent settings. -------------------------------------

        /// @brief Get the value of the continuous update state from the settings.
        ///
        /// @return The continuous update state.
        bool GetContinuousUpdateState() const;

        /// @brief Set the value of the continuous update state in the settings.
        ///
        /// @param [in] statue The continuous update state to save to the settings.
        void SetContinuousUpdateState(bool state);

        /// @brief Get the value of the kSettingGeneralCullMode in the settings.
        ///
        /// @return The triangle face culling mode.
        CullModeType GetCullMode() const;

        /// @brief Set the value of the kSettingGeneralCullMode in the settings.
        ///
        /// @param [in] cull_mode The triangle face culling mode.
        void SetCullMode(CullModeType cull_mode);

        /// @brief Get the value of the control style from the settings.
        ///
        /// @param [in] pane The pane to get the control style for.
        ///
        /// @return The control style type.
        int GetControlStyle(rra::RRAPaneId pane) const;

        /// @brief Set the value of thecontrol style in the settings.
        ///
        /// @param [in] pane               The pane to set the control style for.
        /// @param [in] control_style_type The control style type.
        void SetControlStyle(rra::RRAPaneId pane, ControlStyleType control_style_type);

        /// @brief Get the value of the projection mode from the settings.
        ///
        /// @return The projection mode.
        ProjectionMode GetProjectionMode() const;

        /// @brief Set the value of the projection mode in the settings.
        ///
        /// @param [in] projection_mode The projection mode to save to the settings.
        void SetProjectionMode(ProjectionMode projection_mode);

        /// @brief Get the value of the kSettingGeneralUpAxis in the settings.
        ///
        /// @return The up axis type.
        UpAxisType GetUpAxis() const;

        /// @brief Set the value of the kSettingGeneralUpAxis in the settings.
        ///
        /// @param [in] up_axis The up axis type.
        void SetUpAxis(UpAxisType up_axis);

        /// @brief Get the value of the kSettingGeneralInvertVertical in the settings.
        ///
        /// @return True if vertical inversion is enabled.
        bool GetInvertVertical() const;

        /// @brief Set the value of the kSettingGeneralInvertVertical in the settings.
        ///
        /// @param [in] invert_vertical True if vertical inversion is enabled.
        void SetInvertVertical(bool invert_vertical);

        /// @brief Get the value of the kSettingGeneralInvertHorizontal in the settings.
        ///
        /// @return True if horizontal inversion is enabled.
        bool GetInvertHorizontal() const;

        /// @brief Set the value of the kSettingGeneralInvertHorizontal in the settings.
        ///
        /// @param [in] invert_horizontal True if horizontal inversion is enabled.
        void SetInvertHorizontal(bool invert_horizontal);

        /// @brief Get the coloring mode from the settings.
        ///
        /// @param [in] pane       The pane to get the coloring mode for.
        /// @param [in] color_mode The type of coloring mode requested.
        ///
        /// @return The coloring mode value.
        int GetColoringMode(rra::RRAPaneId pane, ColoringMode color_mode) const;

        /// @brief Set the coloring mode in the settings.
        ///
        /// @param [in] pane       The pane to apply the coloring mode to.
        /// @param [in] color_mode The type of coloring mode.
        /// @param [in] value      The value to store in the settings.
        void SetColoringMode(rra::RRAPaneId pane, ColoringMode color_mode, int value);

        /// @brief Get the rendering mode (geometry or traversal) from the settings.
        ///
        /// @param [in] pane The pane to get the rendering mode for.
        ///
        /// @return The rendering mode.
        int GetRenderingMode(rra::RRAPaneId pane) const;

        /// @brief Set the rendering mode (geometry or traversal) in the settings.
        ///
        /// @param [in] pane           The pane to apply the rendering mode to.
        /// @param [in] rendering_mode The rendering mode to store in the settings.
        void SetRenderingMode(rra::RRAPaneId pane, RenderingMode rendering_mode);

        /// @brief Get a checkbox state from the settings.
        ///
        /// Allows multiple checkbox states per pane to be accessed via a single function.
        ///
        /// @param [in] pane    The pane to apply the checkbox state mode to.
        /// @param [in] setting The checkbox setting to get.
        ///
        /// @return The checkbox state from the settings.
        bool GetCheckboxSetting(rra::RRAPaneId pane, CheckboxSetting setting) const;

        /// @brief Apply a checkbox state to the settings.
        ///
        /// @param [in] pane    The pane where the checkbox state originates.
        /// @param [in] setting The checkbox setting to set.
        /// @param [in] value   The checkbox state to store in the settings.
        void SetCheckboxSetting(rra::RRAPaneId pane, CheckboxSetting setting, bool value);

        /// @brief Get the field of view from a specified viewer pane.
        ///
        /// @param [in] pane The pane to get the field of view setting for.
        ///
        /// @return The field of view.
        int GetFieldOfView(rra::RRAPaneId pane) const;

        /// @brief Set the field of view in a specified viewer pane.
        ///
        /// @param [in] pane  The pane to set the field of view setting from.
        /// @param [in] value The value to save in the settings.
        void SetFieldOfView(rra::RRAPaneId pane, int value);

        /// @brief Get the movement speed from a specified viewer pane.
        /// @param [in] pane The pane to get the movement speed setting for.
        ///
        /// @return The movement speed.
        int GetMovementSpeed(rra::RRAPaneId pane) const;

        /// @brief Set the movement speed in a specified viewer pane.
        ///
        /// @param [in] pane  The pane to set the movement speed setting from.
        /// @param [in] value The value to save in the settings.
        void SetMovementSpeed(rra::RRAPaneId pane, int value);

        /// @brief Reset the persistent UI state back to the default values.
        ///
        /// @param pane The ID of the Pane to reset the UI for.
        void SetPersistentUIToDefault(RRAPaneId pane);

        /// @brief Reset the persistent UI state back to the default values.
        ///
        /// Sets the UI state common between panes.
        void SetPersistentUIToDefault();

        /// @brief Gets the current color theme mode.
        ///
        /// @return the value of the current color theme.
        int GetColorTheme();

        /// @brief Sets the color theme mode.
        ///
        /// @param [in] the value of the color theme.
        void SetColorTheme(int value);

        /// @brief Set the value of kSettingGeneralDriverOverridesAllowNotifications in the settings.
        ///
        /// @param [in] value The new value of kSettingGeneralDriverOverridesAllowNotifications.
        void SetDriverOverridesAllowNotifications(const bool value);

        /// @brief Get the value of kSettingGeneralDriverOverridesAllowNotifications in the settings.
        ///
        /// @return The value of kSettingGeneralDriverOverridesAllowNotifications.
        bool GetDriverOverridesAllowNotifications() const;

    private:
        /// @brief Get the settingID from a lookup map based on the pane and an index.
        ///
        /// Used to return multiple settings from a single function.
        ///
        /// @param [in] lut   The lookup table to use.
        /// @param [in] pane  The Id of the pane to use for the lookup.
        /// @param [in] index The setting type to use.
        SettingID GetSettingIndex(const SettingLookups& lut, rra::RRAPaneId pane, int index) const;

        /// @brief Get the settingID from a lookup map based on the pane and an index.
        ///
        /// Used to return multiple settings from a single function.
        ///
        /// @param [in] lut   The lookup table to use.
        /// @param [in] pane  The Id of the pane to use for the lookup.
        SettingID GetSettingIndex(const Pane2SettingMap& lut, rra::RRAPaneId pane) const;

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

