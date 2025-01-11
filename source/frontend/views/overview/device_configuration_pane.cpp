//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the Device configuration pane.
//=============================================================================

#include "views/overview/device_configuration_pane.h"

#include "qt_common/utils/qt_util.h"

#include "managers/message_manager.h"
#include "views/widget_util.h"

DeviceConfigurationPane::DeviceConfigurationPane(QWidget* parent)
    : BasePane(parent)
    , ui_(new Ui::DeviceConfigurationPane)
{
    ui_->setupUi(this);

    model_ = new rra::DeviceConfigurationModel();

    model_->InitializeModel(ui_->content_processor_brand_, rra::kDeviceConfigurationCPUName, "text");
    model_->InitializeModel(ui_->content_processor_speed_, rra::kDeviceConfigurationCPUSpeed, "text");
    model_->InitializeModel(ui_->content_physical_cores_, rra::kDeviceConfigurationCPUPhysicalCores, "text");
    model_->InitializeModel(ui_->content_logical_cores_, rra::kDeviceConfigurationCPULogicalCores, "text");
    model_->InitializeModel(ui_->content_system_memory_, rra::kDeviceConfigurationSystemMemorySize, "text");

    model_->InitializeModel(ui_->content_api_name_, rra::kDeviceConfigurationApiName, "text");
    model_->InitializeModel(ui_->content_raytracing_version_, rra::kDeviceConfigurationRaytracingVersion, "text");
    model_->InitializeModel(ui_->content_device_name_, rra::kDeviceConfigurationDeviceName, "text");
    model_->InitializeModel(ui_->content_device_id_, rra::kDeviceConfigurationDeviceID, "text");
    model_->InitializeModel(ui_->content_memory_size_, rra::kDeviceConfigurationMemorySize, "text");
    model_->InitializeModel(ui_->content_shader_core_clock_frequency_, rra::kDeviceConfigurationShaderCoreClockFrequency, "text");
    model_->InitializeModel(ui_->content_memory_clock_frequency_, rra::kDeviceConfigurationMemoryClockFrequency, "text");
    model_->InitializeModel(ui_->content_local_memory_bandwidth_, rra::kDeviceConfigurationLocalMemoryBandwidth, "text");
    model_->InitializeModel(ui_->content_local_memory_type_, rra::kDeviceConfigurationLocalMemoryType, "text");
    model_->InitializeModel(ui_->content_local_memory_bus_width_, rra::kDeviceConfigurationLocalMemoryBusWidth, "text");

    model_->InitializeModel(ui_->content_driver_packaging_version_, rra::kDeviceConfigurationDriverPackagingVersion, "text");
    model_->InitializeModel(ui_->content_driver_software_version_, rra::kDeviceConfigurationDriverSoftwareVersion, "text");

    ui_->label_raytracing_version_->hide();
    ui_->content_raytracing_version_->hide();

    if (QtCommon::QtUtils::ColorTheme::Get().GetColorTheme() == ColorThemeType::kColorThemeTypeDark)
    {
        ui_->label_amd_logo_->setStyleSheet("QLabel#label_amd_logo_ { image: url(:/Resources/assets/amd_logo_white.svg); }");
    }

    connect(&QtCommon::QtUtils::ColorTheme::Get(), &QtCommon::QtUtils::ColorTheme::ColorThemeUpdated, this, &DeviceConfigurationPane::OnColorThemeUpdated);
}

DeviceConfigurationPane::~DeviceConfigurationPane()
{
    delete ui_;
    delete model_;
}

void DeviceConfigurationPane::showEvent(QShowEvent* event)
{
    Refresh();
    QWidget::showEvent(event);

    bool visible = model_->SystemInfoAvailable();

    ui_->label_title_system_->setVisible(visible);
    ui_->label_processor_brand_->setVisible(visible);
    ui_->content_processor_brand_->setVisible(visible);
    ui_->label_processor_speed_->setVisible(visible);
    ui_->content_processor_speed_->setVisible(visible);
    ui_->label_physical_cores_->setVisible(visible);
    ui_->content_physical_cores_->setVisible(visible);
    ui_->label_logical_cores_->setVisible(visible);
    ui_->content_logical_cores_->setVisible(visible);
    ui_->label_system_memory_->setVisible(visible);
    ui_->content_system_memory_->setVisible(visible);

    ui_->label_driver_information_->setVisible(visible);
    ui_->label_driver_packaging_version_->setVisible(visible);
    ui_->content_driver_packaging_version_->setVisible(visible);

    // Detect if the loaded scene was taken on Windows
    if (model_->IsDriverSoftwareVersionNeeded())
    {
        // Driver software version is Windows only.
        ui_->label_driver_software_version_->setVisible(visible);
        ui_->content_driver_software_version_->setVisible(visible);
    }
    else
    {
        ui_->label_driver_software_version_->setVisible(false);
        ui_->content_driver_software_version_->setVisible(false);
    }
}

void DeviceConfigurationPane::OnColorThemeUpdated()
{
    if (QtCommon::QtUtils::ColorTheme::Get().GetColorTheme() == ColorThemeType::kColorThemeTypeDark)
    {
        ui_->label_amd_logo_->setStyleSheet("QLabel#label_amd_logo_ { image: url(:/Resources/assets/amd_logo_white.svg); }");
    }
    else
    {
        ui_->label_amd_logo_->setStyleSheet("QLabel#label_amd_logo_ { image: url(:/Resources/assets/amd_logo.svg); }");
    }
}

void DeviceConfigurationPane::Refresh()
{
    model_->Update();
}

void DeviceConfigurationPane::Reset()
{
    model_->ResetModelValues();
}
