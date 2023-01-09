//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the TLAS properties pane.
//=============================================================================

#include "views/tlas/tlas_properties_pane.h"

#include "managers/message_manager.h"
#include "models/tlas/tlas_properties_model.h"
#include "views/widget_util.h"

TlasPropertiesPane::TlasPropertiesPane(QWidget* parent)
    : BasePane(parent)
    , ui_(new Ui::TlasPropertiesPane)
{
    ui_->setupUi(this);
    rra::widget_util::ApplyStandardPaneStyle(this, ui_->main_content_, ui_->main_scroll_area_);
    model_ = new rra::TlasPropertiesModel(rra::kTlasPropertiesNumWidgets);

    model_->InitializeModel(ui_->content_node_address_, rra::kTlasPropertiesBaseAddress, "text");

    model_->InitializeModel(ui_->content_num_nodes_, rra::kTlasPropertiesNumNodes, "text");
    model_->InitializeModel(ui_->content_num_box_nodes_, rra::kTlasPropertiesNumBoxNodes, "text");
    model_->InitializeModel(ui_->content_num_box16_nodes_, rra::kTlasPropertiesNumBox16Nodes, "text");
    model_->InitializeModel(ui_->content_num_box32_nodes_, rra::kTlasPropertiesNumBox32Nodes, "text");
    model_->InitializeModel(ui_->content_num_instances_, rra::kTlasPropertiesNumInstanceNodes, "text");
    model_->InitializeModel(ui_->content_num_blases_, rra::kTlasPropertiesNumBlases, "text");
    model_->InitializeModel(ui_->content_num_triangles_, rra::kTlasPropertiesNumTriangles, "text");

    model_->InitializeModel(ui_->content_allow_update_, rra::kTlasPropertiesBuildFlagAllowUpdate, "checked");
    model_->InitializeModel(ui_->content_allow_compaction_, rra::kTlasPropertiesBuildFlagAllowCompaction, "checked");
    model_->InitializeModel(ui_->content_low_memory_, rra::kTlasPropertiesBuildFlagLowMemory, "checked");
    model_->InitializeModel(ui_->content_build_type_, rra::kTlasPropertiesBuildFlagBuildType, "text");

    model_->InitializeModel(ui_->content_memory_tlas_, rra::kTlasPropertiesMemoryTlas, "text");
    model_->InitializeModel(ui_->content_memory_total_, rra::kTlasPropertiesMemoryTotal, "text");

    connect(&rra::MessageManager::Get(), &rra::MessageManager::TlasSelected, this, &TlasPropertiesPane::SetTlasIndex);
}

TlasPropertiesPane::~TlasPropertiesPane()
{
    delete model_;
}

void TlasPropertiesPane::SetTlasIndex(uint64_t tlas_index)
{
    model_->Update(tlas_index);
}
