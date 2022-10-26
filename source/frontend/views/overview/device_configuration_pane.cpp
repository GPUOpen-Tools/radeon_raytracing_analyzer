//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
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

    // Set white background for this pane.
    rra::widget_util::SetWidgetBackgroundColor(this, Qt::white);

    model_ = new rra::DeviceConfigurationModel();

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

    ui_->label_raytracing_version_->hide();
    ui_->content_raytracing_version_->hide();
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
}

void DeviceConfigurationPane::Refresh()
{
    model_->Update();
}

void DeviceConfigurationPane::Reset()
{
    model_->ResetModelValues();
}
