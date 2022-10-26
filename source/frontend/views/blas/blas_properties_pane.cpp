//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the BLAS properties pane.
//=============================================================================

#include "views/blas/blas_properties_pane.h"

#include "managers/message_manager.h"
#include "models/blas/blas_properties_model.h"
#include "views/widget_util.h"

BlasPropertiesPane::BlasPropertiesPane(QWidget* parent)
    : BasePane(parent)
    , ui_(new Ui::BlasPropertiesPane)
    , tlas_index_(0)
{
    ui_->setupUi(this);
    rra::widget_util::ApplyStandardPaneStyle(this, ui_->main_content_, ui_->main_scroll_area_);
    model_ = new rra::BlasPropertiesModel(rra::kBlasPropertiesNumWidgets);

    model_->InitializeModel(ui_->content_node_address_, rra::kBlasPropertiesBaseAddress, "text");

    model_->InitializeModel(ui_->content_num_nodes_, rra::kBlasPropertiesNumNodes, "text");
    model_->InitializeModel(ui_->content_num_box_nodes_, rra::kBlasPropertiesNumBoxNodes, "text");
    model_->InitializeModel(ui_->content_num_box16_nodes_, rra::kBlasPropertiesNumBox16Nodes, "text");
    model_->InitializeModel(ui_->content_num_box32_nodes_, rra::kBlasPropertiesNumBox32Nodes, "text");
    model_->InitializeModel(ui_->content_num_half_box32_nodes_, rra::kBlasPropertiesNumHalfBox32Nodes, "text");
    model_->InitializeModel(ui_->content_num_triangle_nodes_, rra::kBlasPropertiesNumTriangleNodes, "text");
    model_->InitializeModel(ui_->content_num_procedural_nodes_, rra::kBlasPropertiesNumProceduralNodes, "text");
    model_->InitializeModel(ui_->content_num_triangles_, rra::kBlasPropertiesNumTriangles, "text");
    model_->InitializeModel(ui_->content_num_instances_, rra::kBlasPropertiesNumInstances, "text");

    model_->InitializeModel(ui_->content_allow_update_, rra::kBlasPropertiesBuildFlagAllowUpdate, "checked");
    model_->InitializeModel(ui_->content_allow_compaction_, rra::kBlasPropertiesBuildFlagAllowCompaction, "checked");
    model_->InitializeModel(ui_->content_low_memory_, rra::kBlasPropertiesBuildFlagLowMemory, "checked");
    model_->InitializeModel(ui_->content_build_type_, rra::kBlasPropertiesBuildFlagBuildType, "text");

    model_->InitializeModel(ui_->content_memory_, rra::kBlasPropertiesMemory, "text");

    model_->InitializeModel(ui_->content_root_sah_, rra::kBlasPropertiesRootSAH, "text");
    model_->InitializeModel(ui_->content_min_sah_, rra::kBlasPropertiesMinSAH, "text");
    model_->InitializeModel(ui_->content_mean_sah_, rra::kBlasPropertiesMeanSAH, "text");
    model_->InitializeModel(ui_->content_max_depth_, rra::kBlasPropertiesMaxDepth, "text");
    model_->InitializeModel(ui_->content_mean_depth_, rra::kBlasPropertiesAvgDepth, "text");

    connect(&rra::MessageManager::Get(), &rra::MessageManager::BlasSelected, this, &BlasPropertiesPane::SetBlasIndex);
    connect(&rra::MessageManager::Get(), &rra::MessageManager::TlasSelected, this, &BlasPropertiesPane::SetTlasIndex);
}

BlasPropertiesPane::~BlasPropertiesPane()
{
    delete model_;
}

void BlasPropertiesPane::SetBlasIndex(uint64_t blas_index)
{
    model_->Update(tlas_index_, blas_index);
}

void BlasPropertiesPane::SetTlasIndex(uint64_t tlas_index)
{
    if (tlas_index != tlas_index_)
    {
        tlas_index_ = tlas_index;
    }
}
