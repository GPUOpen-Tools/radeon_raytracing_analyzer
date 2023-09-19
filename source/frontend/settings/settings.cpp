//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the settings.
//=============================================================================

#include "settings/settings.h"

#include <chrono>
#include <ctime>
#include <QFile>

#include "public/rra_assert.h"

#include "constants.h"
#include "settings/settings_reader.h"
#include "settings/settings_writer.h"
#include "util/file_util.h"

#include "models/scene.h"

namespace rra
{
    // Look up maps. The first level is a lookup from PaneID to the Panes' associated data. The second is a lookup from
    // A generic ID to its pane-specific ID.
    static SettingLookups pane_color_maps = {
        {rra::kPaneIdTlasViewer,
         {{kColoringModeBVHColor, kSettingPersistenceTLASBVHColoringMode},
          {kColoringModeGeometryColor, kSettingPersistenceTLASGeometryColoringMode},
          {kColoringModeHeatmapColor, kSettingPersistenceTLASHeatmapColor},
          {kColoringModeTraversalCounterColor, kSettingPersistenceTLASTraversalCounterMode}}},

        {rra::kPaneIdBlasViewer,
         {{kColoringModeBVHColor, kSettingPersistenceBLASBVHColoringMode},
          {kColoringModeGeometryColor, kSettingPersistenceBLASGeometryColoringMode},
          {kColoringModeHeatmapColor, kSettingPersistenceBLASHeatmapColor},
          {kColoringModeTraversalCounterColor, kSettingPersistenceBLASTraversalCounterMode}}},

        {rra::kPaneIdRayInspector,
         {{kColoringModeBVHColor, kSettingPersistenceInspectorBVHColoringMode},
          {kColoringModeGeometryColor, kSettingPersistenceInspectorGeometryColoringMode},
          {kColoringModeHeatmapColor, kSettingPersistenceInspectorHeatmapColor},
          {kColoringModeTraversalCounterColor, kSettingPersistenceInspectorTraversalCounterMode}}},
    };

    static Pane2SettingMap pane_2_rendering_mode = {
        {rra::kPaneIdTlasViewer, kSettingPersistenceTLASRenderingMode},
        {rra::kPaneIdBlasViewer, kSettingPersistenceBLASRenderingMode},
        {rra::kPaneIdRayInspector, kSettingPersistenceInspectorRenderingMode},
    };

    static Pane2SettingMap pane_2_control_style = {
        {rra::kPaneIdTlasViewer, kSettingPersistenceTLASControlStyle},
        {rra::kPaneIdBlasViewer, kSettingPersistenceBLASControlStyle},
        {rra::kPaneIdRayInspector, kSettingPersistenceInspectorControlStyle},
    };

    static SettingLookups pane_checkbox_maps = {
        {rra::kPaneIdTlasViewer,
         {{kCheckboxSettingShowGeometry, kSettingPersistenceTLASShowGeometry},
          {kCheckboxSettingShowAxisAlignedBVH, kSettingPersistenceTLASShowAxisAlignedBVH},
          {kCheckboxSettingShowInstanceTransform, kSettingPersistenceTLASShowInstanceTransform},
          {kCheckboxSettingShowWireframe, kSettingPersistenceTLASShowWireframe},
          {kCheckboxSettingAcceptFirstHit, kSettingPersistenceTLASAcceptFirstHit},
          {kCheckboxSettingCullBackFacingTriangles, kSettingPersistenceTLASCullBackFacingTriangles},
          {kCheckboxSettingCullFrontFacingTriangles, kSettingPersistenceTLASCullFrontFacingTriangles}}},

        {rra::kPaneIdBlasViewer,
         {{kCheckboxSettingShowGeometry, kSettingPersistenceBLASShowGeometry},
          {kCheckboxSettingShowAxisAlignedBVH, kSettingPersistenceBLASShowAxisAlignedBVH},
          {kCheckboxSettingShowWireframe, kSettingPersistenceBLASShowWireframe},
          {kCheckboxSettingAcceptFirstHit, kSettingPersistenceBLASAcceptFirstHit},
          {kCheckboxSettingCullBackFacingTriangles, kSettingPersistenceBLASCullBackFacingTriangles},
          {kCheckboxSettingCullFrontFacingTriangles, kSettingPersistenceBLASCullFrontFacingTriangles}}},

        {rra::kPaneIdRayInspector,
         {{kCheckboxSettingShowGeometry, kSettingPersistenceInspectorShowGeometry},
          {kCheckboxSettingLockCamera, kSettingPersistenceInspectorLockCamera},
          {kCheckboxSettingShowAxisAlignedBVH, kSettingPersistenceInspectorShowAxisAlignedBVH},
          {kCheckboxSettingShowWireframe, kSettingPersistenceInspectorShowWireframe},
          {kCheckboxSettingAcceptFirstHit, kSettingPersistenceInspectorAcceptFirstHit},
          {kCheckboxSettingCullBackFacingTriangles, kSettingPersistenceInspectorCullBackFacingTriangles},
          {kCheckboxSettingCullFrontFacingTriangles, kSettingPersistenceInspectorCullFrontFacingTriangles}}},
    };

    static Pane2SettingMap pane_2_fov = {
        {rra::kPaneIdTlasViewer, kSettingPersistenceTLASFieldOfView},
        {rra::kPaneIdBlasViewer, kSettingPersistenceBLASFieldOfView},
        {rra::kPaneIdRayInspector, kSettingPersistenceInspectorFieldOfView},
    };

    static Pane2SettingMap pane_2_movement_speed = {
        {rra::kPaneIdTlasViewer, kSettingPersistenceTLASMovementSpeed},
        {rra::kPaneIdBlasViewer, kSettingPersistenceBLASMovementSpeed},
        {rra::kPaneIdRayInspector, kSettingPersistenceInspectorMovementSpeed},
    };

    // Single instance of the Settings.
    static Settings settings;

    Settings& Settings::Get()
    {
        return settings;
    }

    Settings::Settings()
    {
        InitDefaultSettings();
    }

    Settings::~Settings()
    {
    }

    void Settings::AddRecentFile(const RecentFileData& recent_file)
    {
        recent_files_.push_back(recent_file);
    }

    void Settings::TraceLoaded(const QString& trace_file_name, const QString& create_time)
    {
        RecentFileData trace_file;

        trace_file.path = trace_file_name;
        trace_file.keywords.clear();

        trace_file.created = create_time;

        std::chrono::system_clock::time_point tp  = std::chrono::system_clock::now();
        std::time_t                           now = std::chrono::system_clock::to_time_t(tp);
        trace_file.accessed                       = QString::number(now);

        // If the file loaded is from the recent files list, remove it.
        RemoveRecentFile(trace_file_name);

        // Add the loaded file to the top of the recent file list.
        recent_files_.insert(recent_files_.begin(), trace_file);
    }

    void Settings::RemoveRecentFile(const char* file_name)
    {
        const int num_recent_files = this->recent_files_.size();
        for (int loop = 0; loop < num_recent_files; loop++)
        {
            if (file_name != nullptr && recent_files_[loop].path.compare(file_name) == 0)
            {
                recent_files_.remove(loop);
                break;
            }
        }
    }

    void Settings::RemoveRecentFile(const QString& trace_name)
    {
        const int num_recent_files = this->recent_files_.size();
        for (int loop = 0; loop < num_recent_files; loop++)
        {
            if (recent_files_[loop].path.compare(trace_name) == 0)
            {
                recent_files_.remove(loop);
                break;
            }
        }
    }

    bool Settings::InRecentFilesList(const QString& trace_name)
    {
        const int num_recent_files = this->recent_files_.size();
        for (int loop = 0; loop < num_recent_files; loop++)
        {
            if (recent_files_[loop].path.compare(trace_name) == 0)
            {
                return true;
            }
        }
        return false;
    }

    void Settings::AddPotentialSetting(const QString& name, const QString& value)
    {
        for (SettingsMap::iterator i = default_settings_.begin(); i != default_settings_.end(); ++i)
        {
            if (i.value().name.compare(name) == 0)
            {
                AddActiveSetting(i.key(), {name, value});
                break;
            }
        }
    }

    QString Settings::GetSettingsFileLocation() const
    {
        QString xml_file = "";

        // Get file location
        xml_file = file_util::GetFileLocation();

        // Add the file name
        xml_file.append("/RraSettings.xml");

        return xml_file;
    }

    bool Settings::LoadSettings()
    {
        // Begin by applying the defaults
        for (SettingsMap::iterator i = default_settings_.begin(); i != default_settings_.end(); ++i)
        {
            AddPotentialSetting(i.value().name, i.value().value);
        }

        QFile file(GetSettingsFileLocation());

        bool read_settings_file = file.open(QFile::ReadOnly | QFile::Text);

        // Override the defaults
        if (read_settings_file)
        {
            SettingsReader xml_reader(&Settings::Get());

            read_settings_file = xml_reader.Read(&file);

            file.close();

            // Make sure the XML parse worked
            Q_ASSERT(read_settings_file == true);
        }

        // If there is not file or if the parsing of an existing file failed, save a new file
        if (!read_settings_file)
        {
            SaveSettings();

            read_settings_file = false;
        }

        SetColorPalette(ColorPalette(active_settings_[kSettingThemesAndColorsPalette].value));

        return read_settings_file;
    }

    /// @brief Turn Qt color to glm.
    glm::vec4 QColorToGLM(QColor color)
    {
        return glm::vec4(color.red(), color.green(), color.blue(), 255.0f) / 255.0f;
    }

    void Settings::SaveSettings() const
    {
        QFile file(GetSettingsFileLocation());
        bool  success = file.open(QFile::WriteOnly | QFile::Text);

        if (success)
        {
            SettingsWriter xml_writer(&Settings::Get());
            success = xml_writer.Write(&file);
            file.close();

            RRA_ASSERT(success == true);
        }

        rra::SceneNodeColors scene_palette;
        scene_palette.box16_node_color                      = QColorToGLM(GetColorValue(kSettingThemesAndColorsBoundingVolumeBox16));
        scene_palette.box32_node_color                      = QColorToGLM(GetColorValue(kSettingThemesAndColorsBoundingVolumeBox32));
        scene_palette.instance_node_color                   = QColorToGLM(GetColorValue(kSettingThemesAndColorsBoundingVolumeInstance));
        scene_palette.procedural_node_color                 = QColorToGLM(GetColorValue(kSettingThemesAndColorsBoundingVolumeProcedural));
        scene_palette.triangle_node_color                   = QColorToGLM(GetColorValue(kSettingThemesAndColorsBoundingVolumeTriangle));
        scene_palette.selected_node_color                   = QColorToGLM(GetColorValue(kSettingThemesAndColorsBoundingVolumeSelected));
        scene_palette.wireframe_normal_color                = QColorToGLM(GetColorValue(kSettingThemesAndColorsWireframeNormal));
        scene_palette.wireframe_selected_color              = QColorToGLM(GetColorValue(kSettingThemesAndColorsWireframeSelected));
        scene_palette.selected_geometry_color               = QColorToGLM(GetColorValue(kSettingThemesAndColorsGeometrySelected));
        scene_palette.background1_color                     = QColorToGLM(GetColorValue(kSettingThemesAndColorsBackground1));
        scene_palette.background2_color                     = QColorToGLM(GetColorValue(kSettingThemesAndColorsBackground2));
        scene_palette.transparent_color                     = QColorToGLM(GetColorValue(kSettingThemesAndColorsNonOpaque));
        scene_palette.opaque_color                          = QColorToGLM(GetColorValue(kSettingThemesAndColorsOpaque));
        scene_palette.positive_color                        = QColorToGLM(GetColorValue(kSettingThemesAndColorsPositive));
        scene_palette.negative_color                        = QColorToGLM(GetColorValue(kSettingThemesAndColorsNegative));
        scene_palette.build_algorithm_none_color            = QColorToGLM(GetColorValue(kSettingThemesAndColorsBuildAlgorithmNone));
        scene_palette.build_algorithm_fast_build_color      = QColorToGLM(GetColorValue(kSettingThemesAndColorsBuildAlgorithmFastBuild));
        scene_palette.build_algorithm_fast_trace_color      = QColorToGLM(GetColorValue(kSettingThemesAndColorsBuildAlgorithmFastTrace));
        scene_palette.build_algorithm_both_color            = QColorToGLM(GetColorValue(kSettingThemesAndColorsBuildAlgorithmBoth));
        scene_palette.instance_opaque_none_color            = QColorToGLM(GetColorValue(kSettingThemesAndColorsInstanceOpaqueNone));
        scene_palette.instance_opaque_force_opaque_color    = QColorToGLM(GetColorValue(kSettingThemesAndColorsInstanceOpaqueForceOpaque));
        scene_palette.instance_opaque_force_no_opaque_color = QColorToGLM(GetColorValue(kSettingThemesAndColorsInstanceOpaqueForceNoOpaque));
        scene_palette.instance_opaque_force_both_color      = QColorToGLM(GetColorValue(kSettingThemesAndColorsInstanceOpaqueBoth));
        scene_palette.selected_ray_color                    = QColorToGLM(GetColorValue(kSettingThemesAndColorsSelectedRayColor));
        scene_palette.ray_color                             = QColorToGLM(GetColorValue(kSettingThemesAndColorsRayColor));
        scene_palette.shadow_ray_color                      = QColorToGLM(GetColorValue(kSettingThemesAndColorsShadowRayColor));
        scene_palette.zero_mask_ray_color                   = QColorToGLM(GetColorValue(kSettingThemesAndColorsZeroMaskRayColor));

        rra::SetSceneNodeColors(scene_palette);
    }

    void Settings::InitDefaultSettings()
    {
        default_settings_[kSettingMainWindowGeometryData] = {"WindowGeometryData", ""};
        default_settings_[kSettingMainWindowWidth]        = {"WindowWidth", "0"};
        default_settings_[kSettingMainWindowHeight]       = {"WindowHeight", "0"};
        default_settings_[kSettingMainWindowXpos]         = {"WindowXPos", "100"};
        default_settings_[kSettingMainWindowYpos]         = {"WindowYPos", "100"};
        default_settings_[kSettingLastFileOpenLocation]   = {"LastFileOpenLocation", ""};
#ifdef BETA_LICENSE
        default_settings_[kSettingLicenseAgreementVersion] = {"LicenseAgreementVersion", "0.0.0.0"};
#endif  // BETA_LICENSE
        default_settings_[kSettingGeneralCheckForUpdatesOnStartup] = {"CheckForUpdatesOnStartup", "False"};
        default_settings_[kSettingGeneralCameraResetOnStyleChange] = {"CameraResetOnStyleChange", "True"};
        default_settings_[kSettingGeneralCameraControlSync]        = {"CameraControlSync", "False"};
        default_settings_[kSettingGeneralPersistentUIState]        = {"PersistentUIState", "True"};
        default_settings_[kSettingGeneralTreeviewNodeID]           = {"TreeviewNodeID", "0"};
        default_settings_[kSettingGeneralTraversalCounterMaximum]  = {"TraversalCounterMaximum", "1000"};
        default_settings_[kSettingGeneralMovementSpeedLimit]       = {"MovementSpeedLimit", "10000"};
        default_settings_[kSettingGeneralFrustumCullRatio]         = {"FrustumCullRatio", "0.0005"};
        default_settings_[kSettingGeneralDecimalPrecision]         = {"DecimalPrecision", "2"};

        default_settings_[kSettingThemesAndColorsPalette] = {"ColorPalette",
                                                             "#FFFFBA02,#FFFF8B00,#FFF76210,#FFE17F35,#FFDA3B01,#FFEF6950,#FFD03438,#FFFF4343,"
                                                             "#FFFF6062,#FFE81123,#FFEA015D,#FFC40052,#FFFF0080,#FFFF97FF,#FFFF4CFF,#FFDC00DD,"
                                                             "#FF0278D8,#FF0063B1,#FF8E8CD7,#FF6B69D6,#FF7F00FF,#FF00EAFF,#FFAF47C2,#FF871797,"
                                                             "#FF000000,#FFC3C3C3,#FFFFFFFF,#FF00172E,#FFC0D1D5,#FFF0FFFF,#FF00CC69,#FF10883E"};

        default_settings_[kSettingThemesAndColorsBoundingVolumeBox16]         = {"BoundingVolumeBox16Color", "23"};
        default_settings_[kSettingThemesAndColorsBoundingVolumeBox32]         = {"BoundingVolumeBox32Color", "15"};
        default_settings_[kSettingThemesAndColorsBoundingVolumeInstance]      = {"BoundingVolumeInstanceColor", "30"};
        default_settings_[kSettingThemesAndColorsBoundingVolumeTriangle]      = {"BoundingVolumeTriangleColor", "16"};
        default_settings_[kSettingThemesAndColorsBoundingVolumeProcedural]    = {"BoundingVolumeProceduralColor", "9"};
        default_settings_[kSettingThemesAndColorsBoundingVolumeSelected]      = {"BoundingVolumeSelectedColor", "0"};
        default_settings_[kSettingThemesAndColorsWireframeNormal]             = {"WireframeNormalColor", "27"};
        default_settings_[kSettingThemesAndColorsWireframeSelected]           = {"WireframeSelectedColor", "29"};
        default_settings_[kSettingThemesAndColorsGeometrySelected]            = {"GeometrySelectedColor", "28"};
        default_settings_[kSettingThemesAndColorsBackground1]                 = {"Background1Color", "25"};
        default_settings_[kSettingThemesAndColorsBackground2]                 = {"Background2Color", "26"};
        default_settings_[kSettingThemesAndColorsNonOpaque]                   = {"NonOpaqueColor", "29"};
        default_settings_[kSettingThemesAndColorsOpaque]                      = {"OpaqueColor", "30"};
        default_settings_[kSettingThemesAndColorsPositive]                    = {"PositiveColor", "30"};
        default_settings_[kSettingThemesAndColorsNegative]                    = {"NegativeColor", "29"};
        default_settings_[kSettingThemesAndColorsBuildAlgorithmNone]          = {"BuildAlgorithmNone", "25"};
        default_settings_[kSettingThemesAndColorsBuildAlgorithmFastBuild]     = {"BuildAlgorithmFastBuild", "20"};
        default_settings_[kSettingThemesAndColorsBuildAlgorithmFastTrace]     = {"BuildAlgorithmFastTrace", "30"};
        default_settings_[kSettingThemesAndColorsBuildAlgorithmBoth]          = {"BuildAlgorithmBoth", "9"};
        default_settings_[kSettingThemesAndColorsInstanceOpaqueNone]          = {"InstanceOpaqueNone", "25"};
        default_settings_[kSettingThemesAndColorsInstanceOpaqueForceOpaque]   = {"InstanceOpaqueForceOpaque", "30"};
        default_settings_[kSettingThemesAndColorsInstanceOpaqueForceNoOpaque] = {"InstanceOpaqueForceNoOpaque", "20"};
        default_settings_[kSettingThemesAndColorsInstanceOpaqueBoth]          = {"InstanceOpaqueBoth", "9"};
        default_settings_[kSettingThemesAndColorsInvocationRaygen]            = {"InvocationRaygen", "16"};
        default_settings_[kSettingThemesAndColorsInvocationClosestHit]        = {"InvocationClosestHit", "30"};
        default_settings_[kSettingThemesAndColorsInvocationAnyHit]            = {"InvocationAnyHit", "1"};
        default_settings_[kSettingThemesAndColorsInvocationIntersection]      = {"InvocationIntersection", "9"};
        default_settings_[kSettingThemesAndColorsInvocationMiss]              = {"InvocationMiss", "18"};
        default_settings_[kSettingThemesAndColorsInvocationCallable]          = {"InvocationCallable", "0"};
        default_settings_[kSettingThemesAndColorsSelectedRayColor]            = {"SelectedRay", "0"};
        default_settings_[kSettingThemesAndColorsRayColor]                    = {"Ray", "31"};
        default_settings_[kSettingThemesAndColorsShadowRayColor]              = {"ShadowRay", "18"};
        default_settings_[kSettingThemesAndColorsZeroMaskRayColor]            = {"ZeroMaskRay", "9"};

        color_palette_ = new ColorPalette(default_settings_[kSettingThemesAndColorsPalette].value);

        // UI Persistent state.
        default_settings_[kSettingPersistenceCullMode]         = {"CullMode", "0"};
        default_settings_[kSettingPersistenceUpAxis]           = {"UpAxis", "1"};
        default_settings_[kSettingPersistenceInvertVertical]   = {"InvertVertical", "False"};
        default_settings_[kSettingPersistenceInvertHorizontal] = {"InvertHorizontal", "False"};
        default_settings_[kSettingPersistenceProjectionMode]   = {"Perspective", "0"};
        default_settings_[kSettingPersistenceContinuousUpdate] = {"ContinuousUpdate", "False"};

        default_settings_[kSettingPersistenceTLASControlStyle]             = {"TLASControlStyle", "1"};
        default_settings_[kSettingPersistenceTLASBVHColoringMode]          = {"TLASBVHColoringMode", "0"};
        default_settings_[kSettingPersistenceTLASGeometryColoringMode]     = {"TLASGeometryColoringMode", "0"};
        default_settings_[kSettingPersistenceTLASHeatmapColor]             = {"TLASHeatmapColor", "0"};
        default_settings_[kSettingPersistenceTLASTraversalCounterMode]     = {"TLASTraversalCounterMode", "0"};
        default_settings_[kSettingPersistenceTLASRenderingMode]            = {"TLASRenderingMode", "0"};
        default_settings_[kSettingPersistenceTLASShowGeometry]             = {"TLASShowGeometry", "True"};
        default_settings_[kSettingPersistenceTLASShowAxisAlignedBVH]       = {"TLASShowAxisAlignedBVH", "True"};
        default_settings_[kSettingPersistenceTLASShowInstanceTransform]    = {"TLASShowInstanceTransform", "True"};
        default_settings_[kSettingPersistenceTLASShowWireframe]            = {"TLASShowWireframe", "True"};
        default_settings_[kSettingPersistenceTLASAcceptFirstHit]           = {"TLASAcceptFirstHit", "False"};
        default_settings_[kSettingPersistenceTLASCullBackFacingTriangles]  = {"TLASCullBackFacingTriangles", "False"};
        default_settings_[kSettingPersistenceTLASCullFrontFacingTriangles] = {"TLASCullFrontFacingTriangles", "False"};
        default_settings_[kSettingPersistenceTLASFieldOfView]              = {"TLASFieldOfView", "75"};
        default_settings_[kSettingPersistenceTLASMovementSpeed]            = {"TLASMovementSpeed", "3"};

        default_settings_[kSettingPersistenceBLASControlStyle]             = {"BLASControlStyle", "0"};
        default_settings_[kSettingPersistenceBLASBVHColoringMode]          = {"BLASBVHColoringMode", "0"};
        default_settings_[kSettingPersistenceBLASGeometryColoringMode]     = {"BLASGeometryColoringMode", "0"};
        default_settings_[kSettingPersistenceBLASHeatmapColor]             = {"BLASHeatmapColor", "0"};
        default_settings_[kSettingPersistenceBLASTraversalCounterMode]     = {"BLASTraversalCounterMode", "0"};
        default_settings_[kSettingPersistenceBLASRenderingMode]            = {"BLASRenderingMode", "0"};
        default_settings_[kSettingPersistenceBLASShowGeometry]             = {"BLASShowGeometry", "True"};
        default_settings_[kSettingPersistenceBLASShowAxisAlignedBVH]       = {"BLASShowAxisAlignedBVH", "True"};
        default_settings_[kSettingPersistenceBLASShowWireframe]            = {"BLASShowWireframe", "True"};
        default_settings_[kSettingPersistenceBLASAcceptFirstHit]           = {"BLASAcceptFirstHit", "False"};
        default_settings_[kSettingPersistenceBLASCullBackFacingTriangles]  = {"BLASCullBackFacingTriangles", "False"};
        default_settings_[kSettingPersistenceBLASCullFrontFacingTriangles] = {"BLASCullFrontFacingTriangles", "False"};
        default_settings_[kSettingPersistenceBLASFieldOfView]              = {"BLASFieldOfView", "75"};
        default_settings_[kSettingPersistenceBLASMovementSpeed]            = {"BLASMovementSpeed", "3"};

        default_settings_[kSettingPersistenceInspectorControlStyle]             = {"InspectorControlStyle", "1"};
        default_settings_[kSettingPersistenceInspectorBVHColoringMode]          = {"InspectorBVHColoringMode", "0"};
        default_settings_[kSettingPersistenceInspectorGeometryColoringMode]     = {"InspectorGeometryColoringMode", "0"};
        default_settings_[kSettingPersistenceInspectorHeatmapColor]             = {"InspectorHeatmapColor", "2"};
        default_settings_[kSettingPersistenceInspectorTraversalCounterMode]     = {"InspectorTraversalCounterMode", "0"};
        default_settings_[kSettingPersistenceInspectorRenderingMode]            = {"InspectorRenderingMode", "0"};
        default_settings_[kSettingPersistenceInspectorShowGeometry]             = {"InspectorShowGeometry", "True"};
        default_settings_[kSettingPersistenceInspectorLockCamera]               = {"InspectorLockCamera", "False"};
        default_settings_[kSettingPersistenceInspectorShowAxisAlignedBVH]       = {"InspectorShowAxisAlignedBVH", "True"};
        default_settings_[kSettingPersistenceInspectorShowWireframe]            = {"InspectorShowWireframe", "True"};
        default_settings_[kSettingPersistenceInspectorAcceptFirstHit]           = {"InspectorAcceptFirstHit", "False"};
        default_settings_[kSettingPersistenceInspectorCullBackFacingTriangles]  = {"InspectorCullBackFacingTriangles", "False"};
        default_settings_[kSettingPersistenceInspectorCullFrontFacingTriangles] = {"InspectorCullFrontFacingTriangles", "False"};
        default_settings_[kSettingPersistenceInspectorFieldOfView]              = {"InspectorFieldOfView", "75"};
        default_settings_[kSettingPersistenceInspectorMovementSpeed]            = {"InspectorMovementSpeed", "3"};
    }

    void Settings::AddActiveSetting(SettingID setting_id, const Setting& setting)
    {
        active_settings_[setting_id] = setting;
    }

    const QMap<SettingID, Setting>& Settings::GetSettings() const
    {
        return active_settings_;
    }

    const QVector<RecentFileData>& Settings::RecentFiles() const
    {
        return recent_files_;
    }

    QString Settings::GetStringValue(const SettingID setting_id) const
    {
        return active_settings_[setting_id].value;
    }

    bool Settings::GetBoolValue(SettingID setting_id) const
    {
        return (active_settings_[setting_id].value.compare("True") == 0) ? true : false;
    }

    int Settings::GetIntValue(SettingID setting_id) const
    {
        return active_settings_[setting_id].value.toInt();
    }

    float Settings::GetFloatValue(SettingID setting_id) const
    {
        return active_settings_[setting_id].value.toFloat();
    }

    void Settings::SetStringValue(SettingID setting_id, const QString& value)
    {
        AddPotentialSetting(default_settings_[setting_id].name, value);
    }

    void Settings::SetToDefaultValue(SettingID setting_id)
    {
        active_settings_[setting_id].value = default_settings_[setting_id].value;
    }

    void Settings::SetPersistentUIToDefault(RRAPaneId pane)
    {
        // If the cameras are synced, the TLAS value is used as the default setting when reset.
        if (GetCameraControlSync())
        {
            SetToDefaultValue(kSettingPersistenceTLASControlStyle);
        }

        switch (pane)
        {
        case rra::kPaneIdTlasViewer:
            SetToDefaultValue(kSettingPersistenceTLASControlStyle);
            SetToDefaultValue(kSettingPersistenceTLASBVHColoringMode);
            SetToDefaultValue(kSettingPersistenceTLASGeometryColoringMode);
            SetToDefaultValue(kSettingPersistenceTLASHeatmapColor);
            SetToDefaultValue(kSettingPersistenceTLASTraversalCounterMode);

            SetToDefaultValue(kSettingPersistenceTLASRenderingMode);
            SetToDefaultValue(kSettingPersistenceTLASShowGeometry);
            SetToDefaultValue(kSettingPersistenceTLASShowAxisAlignedBVH);
            SetToDefaultValue(kSettingPersistenceTLASShowInstanceTransform);
            SetToDefaultValue(kSettingPersistenceTLASShowWireframe);

            SetToDefaultValue(kSettingPersistenceTLASAcceptFirstHit);
            SetToDefaultValue(kSettingPersistenceTLASCullBackFacingTriangles);
            SetToDefaultValue(kSettingPersistenceTLASCullFrontFacingTriangles);

            SetToDefaultValue(kSettingPersistenceTLASFieldOfView);
            SetToDefaultValue(kSettingPersistenceTLASMovementSpeed);
            break;

        case rra::kPaneIdBlasViewer:
            SetToDefaultValue(kSettingPersistenceBLASControlStyle);
            SetToDefaultValue(kSettingPersistenceBLASBVHColoringMode);
            SetToDefaultValue(kSettingPersistenceBLASGeometryColoringMode);
            SetToDefaultValue(kSettingPersistenceBLASHeatmapColor);
            SetToDefaultValue(kSettingPersistenceBLASTraversalCounterMode);

            SetToDefaultValue(kSettingPersistenceBLASRenderingMode);
            SetToDefaultValue(kSettingPersistenceBLASShowGeometry);
            SetToDefaultValue(kSettingPersistenceBLASShowAxisAlignedBVH);
            SetToDefaultValue(kSettingPersistenceBLASShowWireframe);

            SetToDefaultValue(kSettingPersistenceBLASAcceptFirstHit);
            SetToDefaultValue(kSettingPersistenceBLASCullBackFacingTriangles);
            SetToDefaultValue(kSettingPersistenceBLASCullFrontFacingTriangles);

            SetToDefaultValue(kSettingPersistenceBLASFieldOfView);
            SetToDefaultValue(kSettingPersistenceBLASMovementSpeed);
            break;

        case rra::kPaneIdRayInspector:
            SetToDefaultValue(kSettingPersistenceInspectorControlStyle);
            SetToDefaultValue(kSettingPersistenceInspectorBVHColoringMode);
            SetToDefaultValue(kSettingPersistenceInspectorGeometryColoringMode);
            SetToDefaultValue(kSettingPersistenceInspectorHeatmapColor);
            SetToDefaultValue(kSettingPersistenceInspectorTraversalCounterMode);

            SetToDefaultValue(kSettingPersistenceInspectorRenderingMode);
            SetToDefaultValue(kSettingPersistenceInspectorShowGeometry);
            SetToDefaultValue(kSettingPersistenceInspectorLockCamera);
            SetToDefaultValue(kSettingPersistenceInspectorShowAxisAlignedBVH);
            SetToDefaultValue(kSettingPersistenceInspectorShowWireframe);

            SetToDefaultValue(kSettingPersistenceInspectorAcceptFirstHit);
            SetToDefaultValue(kSettingPersistenceInspectorCullBackFacingTriangles);
            SetToDefaultValue(kSettingPersistenceInspectorCullFrontFacingTriangles);

            SetToDefaultValue(kSettingPersistenceInspectorFieldOfView);
            SetToDefaultValue(kSettingPersistenceInspectorMovementSpeed);
            break;

        default:
            RRA_ASSERT_MESSAGE(false, "Invalid pane");
            return;
        }
    }

    void Settings::SetPersistentUIToDefault()
    {
        // Shared currently.
        SetToDefaultValue(kSettingPersistenceCullMode);
        SetToDefaultValue(kSettingPersistenceUpAxis);
        SetToDefaultValue(kSettingPersistenceInvertVertical);
        SetToDefaultValue(kSettingPersistenceInvertHorizontal);
        SetToDefaultValue(kSettingPersistenceProjectionMode);
        SetToDefaultValue(kSettingPersistenceContinuousUpdate);
    }

    void Settings::SetBoolValue(SettingID setting_id, const bool value)
    {
        if (value)
        {
            AddPotentialSetting(default_settings_[setting_id].name, "True");
        }
        else
        {
            AddPotentialSetting(default_settings_[setting_id].name, "False");
        }
    }

    void Settings::SetIntValue(SettingID setting_id, const int value)
    {
        AddPotentialSetting(default_settings_[setting_id].name, QString::number(value));
    }

    void Settings::SetFloatValue(SettingID setting_id, const float value)
    {
        AddPotentialSetting(default_settings_[setting_id].name, QString::number(value));
    }

    int Settings::GetWindowWidth() const
    {
        return GetIntValue(kSettingMainWindowWidth);
    }

    int Settings::GetWindowHeight() const
    {
        return GetIntValue(kSettingMainWindowHeight);
    }

    int Settings::GetWindowXPos() const
    {
        return GetIntValue(kSettingMainWindowXpos);
    }

    int Settings::GetWindowYPos() const
    {
        return GetIntValue(kSettingMainWindowYpos);
    }

    QString Settings::GetLastFileOpenLocation() const
    {
        return active_settings_[kSettingLastFileOpenLocation].value;
    }

#ifdef BETA_LICENSE
    const QString& Settings::GetLicenseAgreementVersion()
    {
        return active_settings_[kSettingLicenseAgreementVersion].value;
    }
#endif  // BETA_LICENSE

    void Settings::SetLastFileOpenLocation(const QString& last_file_open_location)
    {
        AddPotentialSetting("LastFileOpenLocation", last_file_open_location);
        SaveSettings();
    }

#ifdef BETA_LICENSE
    void Settings::SetLicenseAgreementVersion(const QString& value)
    {
        AddPotentialSetting("LicenseAgreementVersion", value);
        SaveSettings();
    }
#endif  // BETA_LICENSE

    void Settings::SetWindowSize(const int width, const int height)
    {
        AddPotentialSetting(default_settings_[kSettingMainWindowWidth].name, QString::number(width));
        AddPotentialSetting(default_settings_[kSettingMainWindowHeight].name, QString::number(height));
        SaveSettings();
    }

    void Settings::SetWindowPos(const int x_pos, const int y_pos)
    {
        AddPotentialSetting(default_settings_[kSettingMainWindowXpos].name, QString::number(x_pos));
        AddPotentialSetting(default_settings_[kSettingMainWindowYpos].name, QString::number(y_pos));
        SaveSettings();
    }

    void Settings::SetCheckForUpdatesOnStartup(const bool value)
    {
        SetBoolValue(kSettingGeneralCheckForUpdatesOnStartup, value);
        SaveSettings();
    }

    void Settings::SetCameraResetOnStyleChange(const bool value)
    {
        SetBoolValue(kSettingGeneralCameraResetOnStyleChange, value);
        SaveSettings();
    }

    void Settings::SetPersistentUIState(const bool value)
    {
        SetBoolValue(kSettingGeneralPersistentUIState, value);
        SaveSettings();
    }

    void Settings::SetCheckBoxStatus(const SettingID setting_id, const bool value)
    {
        SetBoolValue(setting_id, value);
        SaveSettings();
    }

    bool Settings::GetCheckBoxStatus(const SettingID setting_id) const
    {
        return GetBoolValue(setting_id);
    }

    bool Settings::GetCheckForUpdatesOnStartup() const
    {
        return GetBoolValue(kSettingGeneralCheckForUpdatesOnStartup);
    }

    bool Settings::GetCameraResetOnStyleChange() const
    {
        return GetBoolValue(kSettingGeneralCameraResetOnStyleChange);
    }

    bool Settings::GetCameraControlSync() const
    {
        // This feature has been sunset, not necessary after persistent settings patch.
        return false;
    }

    bool Settings::GetPersistentUIState() const
    {
        return GetBoolValue(kSettingGeneralPersistentUIState);
    }

    TreeviewNodeIDType Settings::GetTreeviewNodeIdType() const
    {
        return static_cast<TreeviewNodeIDType>(GetIntValue(kSettingGeneralTreeviewNodeID));
    }

    void Settings::SetTreeviewNodeIdType(TreeviewNodeIDType node_id_type)
    {
        SetIntValue(kSettingGeneralTreeviewNodeID, node_id_type);
        SaveSettings();
    }

    const ColorPalette& Settings::GetColorPalette() const
    {
        return *color_palette_;
    }

    int Settings::GetPaletteId(SettingID setting_id)
    {
        return GetIntValue(setting_id);
    }

    void Settings::SetPaletteId(SettingID setting_id, const int value)
    {
        SetIntValue(setting_id, value);
        SaveSettings();
    }

    void Settings::CachePalette()
    {
        if (color_palette_)
        {
            delete color_palette_;
            color_palette_ = new ColorPalette(active_settings_[kSettingThemesAndColorsPalette].value);
        }
    }

    void Settings::SetColorPalette(const ColorPalette& value)
    {
        active_settings_[kSettingThemesAndColorsPalette].value = value.GetString();
        CachePalette();
        SaveSettings();
    }

    void Settings::RestoreDefaultColors()
    {
        SetToDefaultValue(kSettingThemesAndColorsBoundingVolumeBox16);
        SetToDefaultValue(kSettingThemesAndColorsBoundingVolumeBox32);
        SetToDefaultValue(kSettingThemesAndColorsBoundingVolumeInstance);
        SetToDefaultValue(kSettingThemesAndColorsBoundingVolumeTriangle);
        SetToDefaultValue(kSettingThemesAndColorsBoundingVolumeProcedural);
        SetToDefaultValue(kSettingThemesAndColorsBoundingVolumeSelected);
        SetToDefaultValue(kSettingThemesAndColorsWireframeNormal);
        SetToDefaultValue(kSettingThemesAndColorsWireframeSelected);
        SetToDefaultValue(kSettingThemesAndColorsGeometrySelected);
        SetToDefaultValue(kSettingThemesAndColorsBackground1);
        SetToDefaultValue(kSettingThemesAndColorsBackground2);
        SetToDefaultValue(kSettingThemesAndColorsNonOpaque);
        SetToDefaultValue(kSettingThemesAndColorsOpaque);
        SetToDefaultValue(kSettingThemesAndColorsPositive);
        SetToDefaultValue(kSettingThemesAndColorsNegative);
        SetToDefaultValue(kSettingThemesAndColorsBuildAlgorithmNone);
        SetToDefaultValue(kSettingThemesAndColorsBuildAlgorithmFastBuild);
        SetToDefaultValue(kSettingThemesAndColorsBuildAlgorithmFastTrace);
        SetToDefaultValue(kSettingThemesAndColorsBuildAlgorithmBoth);
        SetToDefaultValue(kSettingThemesAndColorsInstanceOpaqueNone);
        SetToDefaultValue(kSettingThemesAndColorsInstanceOpaqueForceOpaque);
        SetToDefaultValue(kSettingThemesAndColorsInstanceOpaqueForceNoOpaque);
        SetToDefaultValue(kSettingThemesAndColorsInstanceOpaqueBoth);
        SetToDefaultValue(kSettingThemesAndColorsInvocationRaygen);
        SetToDefaultValue(kSettingThemesAndColorsInvocationClosestHit);
        SetToDefaultValue(kSettingThemesAndColorsInvocationAnyHit);
        SetToDefaultValue(kSettingThemesAndColorsInvocationIntersection);
        SetToDefaultValue(kSettingThemesAndColorsInvocationMiss);
        SetToDefaultValue(kSettingThemesAndColorsSelectedRayColor);
        SetToDefaultValue(kSettingThemesAndColorsRayColor);
        SetToDefaultValue(kSettingThemesAndColorsShadowRayColor);
        SetToDefaultValue(kSettingThemesAndColorsZeroMaskRayColor);

        SaveSettings();
    }

    void Settings::RestoreDefaultPalette()
    {
        SetToDefaultValue(kSettingThemesAndColorsPalette);
        CachePalette();
        SaveSettings();
    }

    QColor Settings::GetColorValue(SettingID setting_id) const
    {
        const ColorPalette& palette    = GetColorPalette();
        int                 palette_id = GetIntValue(setting_id);

        return palette.GetColor(palette_id);
    }

    void Settings::SetTraversalCounterMaximum(int new_max)
    {
        SetIntValue(kSettingGeneralTraversalCounterMaximum, new_max);
        SaveSettings();
    }

    void Settings::SetMovementSpeedLimit(int new_speed)
    {
        SetIntValue(kSettingGeneralMovementSpeedLimit, new_speed);
        SaveSettings();
    }

    void Settings::SetFrustumCullRatio(float new_ratio)
    {
        SetFloatValue(kSettingGeneralFrustumCullRatio, new_ratio);
        SaveSettings();
    }

    void Settings::SetDecimalPrecision(int new_precision)
    {
        SetIntValue(kSettingGeneralDecimalPrecision, new_precision);
        SaveSettings();
    }

    int Settings::GetTraversalCounterMaximum()
    {
        return GetIntValue(kSettingGeneralTraversalCounterMaximum);
    }

    int Settings::GetMovementSpeedLimit()
    {
        return GetIntValue(kSettingGeneralMovementSpeedLimit);
    }

    float Settings::GetFrustumCullRatio()
    {
        return GetFloatValue(kSettingGeneralFrustumCullRatio);
    }

    int Settings::GetDecimalPrecision()
    {
        return GetIntValue(kSettingGeneralDecimalPrecision);
    }

    // Viewer persistent settings. -------------------------------------

    SettingID Settings::GetSettingIndex(const SettingLookups& lut, rra::RRAPaneId pane, int index) const
    {
        const auto& it = lut.find(pane);
        if (it != lut.end())
        {
            const auto& settings_map = (*it).second;
            const auto& it2          = settings_map.find(index);
            if (it2 != settings_map.end())
            {
                return (*it2).second;
            }
        }
        RRA_ASSERT(false);
        return kSettingCount;
    }

    SettingID Settings::GetSettingIndex(const Pane2SettingMap& lut, rra::RRAPaneId pane) const
    {
        const auto& it = lut.find(pane);
        if (it != lut.end())
        {
            return (*it).second;
        }
        RRA_ASSERT(false);
        return kSettingCount;
    }

    bool Settings::GetContinuousUpdateState() const
    {
        return GetBoolValue(kSettingPersistenceContinuousUpdate);
    }

    void Settings::SetContinuousUpdateState(bool state)
    {
        SetBoolValue(kSettingPersistenceContinuousUpdate, state);
        SaveSettings();
    }

    CullModeType Settings::GetCullMode() const
    {
        return static_cast<CullModeType>(GetIntValue(kSettingPersistenceCullMode));
    }

    void Settings::SetCullMode(CullModeType cull_mode)
    {
        SetIntValue(kSettingPersistenceCullMode, cull_mode);
        SaveSettings();
    }

    int Settings::GetControlStyle(rra::RRAPaneId pane) const
    {
        // If camera controls are synced, we use TLAS settings for each pane.
        SettingID setting_id = GetCameraControlSync() ? kSettingPersistenceTLASControlStyle : GetSettingIndex(pane_2_control_style, pane);
        if (setting_id != kSettingCount)
        {
            return GetIntValue(setting_id);
        }
        RRA_ASSERT(false);
        return 0;
    }

    void Settings::SetControlStyle(rra::RRAPaneId pane, ControlStyleType value)
    {
        // If camera controls are synced, we use TLAS settings for each pane.
        SettingID setting_id = GetCameraControlSync() ? kSettingPersistenceTLASControlStyle : GetSettingIndex(pane_2_control_style, pane);
        if (setting_id != kSettingCount)
        {
            SetIntValue(setting_id, value);
            SaveSettings();
        }
    }

    ProjectionMode Settings::GetProjectionMode() const
    {
        return static_cast<ProjectionMode>(GetIntValue(kSettingPersistenceProjectionMode));
    }

    void Settings::SetProjectionMode(ProjectionMode projection_mode)
    {
        SetIntValue(kSettingPersistenceProjectionMode, projection_mode);
        SaveSettings();
    }

    UpAxisType Settings::GetUpAxis() const
    {
        return static_cast<UpAxisType>(GetIntValue(kSettingPersistenceUpAxis));
    }

    void Settings::SetUpAxis(UpAxisType up_axis)
    {
        SetIntValue(kSettingPersistenceUpAxis, up_axis);
        SaveSettings();
    }

    bool Settings::GetInvertVertical() const
    {
        return GetBoolValue(kSettingPersistenceInvertVertical);
    }

    void Settings::SetInvertVertical(bool invert_vertical)
    {
        SetBoolValue(kSettingPersistenceInvertVertical, invert_vertical);
        SaveSettings();
    }

    bool Settings::GetInvertHorizontal() const
    {
        return GetBoolValue(kSettingPersistenceInvertHorizontal);
    }

    void Settings::SetInvertHorizontal(bool invert_horizontal)
    {
        SetBoolValue(kSettingPersistenceInvertHorizontal, invert_horizontal);
        SaveSettings();
    }

    int Settings::GetColoringMode(rra::RRAPaneId pane, ColoringMode color_mode) const
    {
        SettingID index = GetSettingIndex(pane_color_maps, pane, color_mode);
        if (index != kSettingCount)
        {
            return GetIntValue(index);
        }
        RRA_ASSERT(false);
        return 0;
    }

    void Settings::SetColoringMode(rra::RRAPaneId pane, ColoringMode color_mode, int value)
    {
        SettingID index = GetSettingIndex(pane_color_maps, pane, color_mode);
        if (index != kSettingCount)
        {
            SetIntValue(index, value);
            SaveSettings();
        }
    }

    int Settings::GetRenderingMode(rra::RRAPaneId pane) const
    {
        SettingID index = GetSettingIndex(pane_2_rendering_mode, pane);
        if (index != kSettingCount)
        {
            return GetIntValue(index);
        }
        RRA_ASSERT(false);
        return 0;
    }

    void Settings::SetRenderingMode(rra::RRAPaneId pane, RenderingMode value)
    {
        SettingID index = GetSettingIndex(pane_2_rendering_mode, pane);
        if (index != kSettingCount)
        {
            SetIntValue(index, value);
            SaveSettings();
        }
    }

    bool Settings::GetCheckboxSetting(rra::RRAPaneId pane, CheckboxSetting setting) const
    {
        SettingID index = GetSettingIndex(pane_checkbox_maps, pane, setting);
        if (index != kSettingCount)
        {
            return GetBoolValue(index);
        }
        RRA_ASSERT(false);
        return true;
    }

    void Settings::SetCheckboxSetting(rra::RRAPaneId pane, CheckboxSetting setting, bool value)
    {
        SettingID index = GetSettingIndex(pane_checkbox_maps, pane, setting);
        if (index != kSettingCount)
        {
            SetBoolValue(index, value);
            SaveSettings();
        }
    }

    int Settings::GetFieldOfView(rra::RRAPaneId pane) const
    {
        SettingID index = GetSettingIndex(pane_2_fov, pane);
        if (index != kSettingCount)
        {
            return GetIntValue(index);
        }
        RRA_ASSERT(false);
        return 0;
    }

    void Settings::SetFieldOfView(rra::RRAPaneId pane, int value)
    {
        SettingID index = GetSettingIndex(pane_2_fov, pane);
        if (index != kSettingCount)
        {
            SetIntValue(index, value);
            SaveSettings();
        }
    }

    int Settings::GetMovementSpeed(rra::RRAPaneId pane) const
    {
        SettingID index = GetSettingIndex(pane_2_movement_speed, pane);
        if (index != kSettingCount)
        {
            return GetIntValue(index);
        }
        RRA_ASSERT(false);
        return 0;
    }

    void Settings::SetMovementSpeed(rra::RRAPaneId pane, int value)
    {
        SettingID index = GetSettingIndex(pane_2_movement_speed, pane);
        if (index != kSettingCount)
        {
            SetIntValue(index, value);
            SaveSettings();
        }
    }

}  // namespace rra