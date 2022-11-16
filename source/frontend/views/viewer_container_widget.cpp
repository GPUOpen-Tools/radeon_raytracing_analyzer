//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the viewer container widget class.
///
/// This class contains regularly-used widgets which are shared between the
/// TLAS and BLAS viewer panes and slot under the list items at the top of
/// the UI
//=============================================================================

#include "views/viewer_container_widget.h"

#include "views/widget_util.h"

#include "public/heatmap.h"

ViewerContainerWidget::ViewerContainerWidget(QWidget* parent)
    : QWidget(parent)
    , ui_(new Ui::ViewerContainerWidget)
{
    ui_->setupUi(this);
    model_ = new rra::ViewerContainerModel();
}

ViewerContainerWidget::~ViewerContainerWidget()
{
    delete model_;
}

static rra::renderer::RendererAdapter* GetAdapterByType(const rra::renderer::RendererAdapterMap& adapters, rra::renderer::RendererAdapterType type)
{
    rra::renderer::RendererAdapter* result = nullptr;

    auto render_state_adapter_iter = adapters.find(type);
    if (render_state_adapter_iter != adapters.end())
    {
        result = render_state_adapter_iter->second;
    }

    return result;
}

void ViewerContainerWidget::SetRendererAdapters(const rra::renderer::RendererAdapterMap& adapters, rra::renderer::BvhTypeFlags bvh_type)
{
    using namespace rra::renderer;

    RendererAdapter* render_state_adapter = GetAdapterByType(adapters, RendererAdapterType::kRendererAdapterTypeRenderState);
    if (render_state_adapter != nullptr)
    {
        std::vector<rra::renderer::GeometryColoringModeInfo> coloring_modes;
        std::vector<rra::renderer::TraversalCounterModeInfo> traversal_modes;
        model_->SetRendererAdapter(render_state_adapter, bvh_type, coloring_modes, traversal_modes);
        UpdateRenderingModes(coloring_modes, traversal_modes);
    }
}

void ViewerContainerWidget::SetupUI(QWidget* parent)
{
    rra::widget_util::InitializeComboBox(parent, ui_->content_bvh_coloring_mode_, {});
    rra::widget_util::InitializeComboBox(parent, ui_->content_heatmap_, {});
    rra::widget_util::InitializeComboBox(parent, ui_->content_geometry_coloring_mode_, {});
    rra::widget_util::InitializeComboBox(parent, ui_->content_traversal_counter_mode_, {});

    connect(ui_->content_bvh_coloring_mode_, &ArrowIconComboBox::SelectionChanged, this, &ViewerContainerWidget::SetBVHColoringMode);
    connect(ui_->content_heatmap_, &ArrowIconComboBox::SelectionChanged, this, &ViewerContainerWidget::SetHeatmapMode);
    connect(ui_->content_traversal_counter_mode_, &ArrowIconComboBox::SelectionChanged, this, &ViewerContainerWidget::SetTraversalCounterMode);
    connect(ui_->content_geometry_coloring_mode_, &ArrowIconComboBox::SelectionChanged, this, &ViewerContainerWidget::SetGeometryColoringMode);
}

void ViewerContainerWidget::ShowColoringMode(bool geometry_mode_enabled)
{
    ui_->content_geometry_coloring_mode_->setVisible(geometry_mode_enabled);
    ui_->content_traversal_counter_mode_->setVisible(!geometry_mode_enabled);
}

void ViewerContainerWidget::SetHeatmapMode()
{
    int row = ui_->content_heatmap_->CurrentRow();
    model_->SetHeatmapData(heatmap_generators_[row].generator_function());
}

void ViewerContainerWidget::SetGeometryColoringMode()
{
    int                                 row        = ui_->content_geometry_coloring_mode_->CurrentRow();
    rra::renderer::GeometryColoringMode mode_value = filtered_color_modes_[row].value;

    model_->SetGeometryColoringMode(mode_value);
}

void ViewerContainerWidget::SetBVHColoringMode()
{
    int                            row        = ui_->content_bvh_coloring_mode_->CurrentRow();
    rra::renderer::BVHColoringMode mode_value = rra::renderer::kAvailableBVHColoringModes[row].value;

    model_->SetBVHColoringMode(static_cast<int>(mode_value));
}

void ViewerContainerWidget::SetTraversalCounterMode()
{
    int                                 row        = ui_->content_traversal_counter_mode_->CurrentRow();
    rra::renderer::TraversalCounterMode mode_value = filtered_traversal_modes_[row].value;

    model_->SetTraversalCounterMode(static_cast<int>(mode_value));
}

void ViewerContainerWidget::UpdateRenderingModes(const std::vector<rra::renderer::GeometryColoringModeInfo>& coloring_modes,
                                                const std::vector<rra::renderer::TraversalCounterModeInfo>& traversal_modes)
{
    // Construct a vector of the name of each coloring mode.
    std::vector<std::string> coloring_mode_names;
    for (rra::renderer::GeometryColoringModeInfo info : coloring_modes)
    {
        filtered_color_modes_.push_back(info);
        coloring_mode_names.push_back(info.name);
    }

    // Populate the coloring modes combo box with the available modes.
    rra::widget_util::RePopulateComboBox(ui_->content_geometry_coloring_mode_, coloring_mode_names);

    // Set a tooltip for each item with a description of the coloring mode.
    for (int mode_index = 0; mode_index < coloring_mode_names.size(); ++mode_index)
    {
        auto coloring_mode_item = ui_->content_geometry_coloring_mode_->FindItem(mode_index);
        coloring_mode_item->setToolTip(coloring_modes[mode_index].description.c_str());
    }

    // Initialize BVH coloring modes.
    // Construct a vector of the name of each coloring mode.
    std::vector<std::string> bvh_coloring_mode_names;
    for (const auto& info : rra::renderer::kAvailableBVHColoringModes)
    {
        bvh_coloring_mode_names.push_back(info.name);
    }

    // Populate the coloring modes combo box with the available modes.
    rra::widget_util::RePopulateComboBox(ui_->content_bvh_coloring_mode_, bvh_coloring_mode_names);

    // Set a tooltip for each item with a description of the bvh coloring mode.
    for (int mode_index = 0; mode_index < rra::renderer::kAvailableBVHColoringModes.size(); ++mode_index)
    {
        auto coloring_mode_item = ui_->content_bvh_coloring_mode_->FindItem(mode_index);
        coloring_mode_item->setToolTip(rra::renderer::kAvailableBVHColoringModes[mode_index].description.c_str());
    }

    // Initialize Traversal counter modes.
    // Construct a vector of the name of each counter mode.
    std::vector<std::string> traversal_counter_mode_names;
    for (const auto& info : traversal_modes)
    {
        filtered_traversal_modes_.push_back(info);
        traversal_counter_mode_names.push_back(info.name);
    }

    // Populate the coloring modes combo box with the available modes.
    rra::widget_util::RePopulateComboBox(ui_->content_traversal_counter_mode_, traversal_counter_mode_names);

    // Set a tooltip for each item with a description of the bvh coloring mode.
    for (int mode_index = 0; mode_index < traversal_counter_mode_names.size(); ++mode_index)
    {
        auto traversal_mode_item = ui_->content_traversal_counter_mode_->FindItem(mode_index);
        traversal_mode_item->setToolTip(traversal_modes[mode_index].description.c_str());
    }

    // Initialize Heatmap modes.
    std::vector<std::string> heatmap_mode_names_;
    heatmap_generators_ = rra::renderer::Heatmap::GetHeatmapGenerators();

    // Construct heatmap name vector.
    for (auto generator : heatmap_generators_)
    {
        heatmap_mode_names_.push_back(generator.name);
    }

    // Populate heatmap dropdown with heatmap names.
    rra::widget_util::RePopulateComboBox(ui_->content_heatmap_, heatmap_mode_names_);

    // Set heatmap tooltips.
    for (int mode_index = 0; mode_index < heatmap_mode_names_.size(); ++mode_index)
    {
        auto heatmap_mode_item = ui_->content_heatmap_->FindItem(mode_index);
        heatmap_mode_item->setToolTip(heatmap_generators_[mode_index].tooltip.c_str());
    }
}
