//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
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
#include "managers/pane_manager.h"

#include "public/heatmap.h"
#include "public/renderer_types.h"
#include "settings/settings.h"

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
        UpdateRenderingModes(coloring_modes, traversal_modes, bvh_type);
    }
}

void ViewerContainerWidget::ApplyUIStateFromSettings(rra::RRAPaneId pane)
{
    int bvh_index       = rra::Settings::Get().GetColoringMode(pane, kColoringModeBVHColor);
    int geometry_index  = rra::Settings::Get().GetColoringMode(pane, kColoringModeGeometryColor);
    int heatmap_index   = rra::Settings::Get().GetColoringMode(pane, kColoringModeHeatmapColor);
    int traversal_index = rra::Settings::Get().GetColoringMode(pane, kColoringModeTraversalCounterColor);

    ui_->content_bvh_coloring_mode_->SetSelectedRow(bvh_index);
    rra::renderer::BVHColoringMode bvh_mode_value = rra::renderer::kAvailableBVHColoringModes[bvh_index].value;
    model_->SetBVHColoringMode(static_cast<int>(bvh_mode_value));

    ui_->content_geometry_coloring_mode_->SetSelectedRow(geometry_index);
    rra::renderer::GeometryColoringMode geometry_mode_value = filtered_color_modes_[geometry_index].value;
    model_->SetGeometryColoringMode(geometry_mode_value);

    ui_->content_heatmap_->SetSelectedRow(heatmap_index);
    model_->SetHeatmapData(heatmap_generators_[heatmap_index].generator_function());

    ui_->content_traversal_counter_mode_->SetSelectedRow(traversal_index);
    rra::renderer::TraversalCounterMode traversal_mode_value = filtered_traversal_modes_[traversal_index].value;
    model_->SetTraversalCounterMode(static_cast<int>(traversal_mode_value));

    ui_->content_instance_mask_hex_input_->setValue(0xFF);
}

void ViewerContainerWidget::SetupUI(QWidget* parent, rra::RRAPaneId pane)
{
    parent_pane_id_ = pane;
    rra::widget_util::InitializeComboBox(parent, ui_->content_bvh_coloring_mode_, {});
    rra::widget_util::InitializeComboBox(parent, ui_->content_heatmap_, {});
    rra::widget_util::InitializeComboBox(parent, ui_->content_geometry_coloring_mode_, {});
    rra::widget_util::InitializeComboBox(parent, ui_->content_traversal_counter_mode_, {});

    connect(ui_->content_bvh_coloring_mode_, &ArrowIconComboBox::SelectionChanged, this, &ViewerContainerWidget::SetBVHColoringMode);
    connect(ui_->content_heatmap_, &ArrowIconComboBox::SelectionChanged, this, &ViewerContainerWidget::SetHeatmapMode);
    connect(ui_->content_traversal_counter_mode_, &ArrowIconComboBox::SelectionChanged, this, &ViewerContainerWidget::SetTraversalCounterMode);
    connect(ui_->content_geometry_coloring_mode_, &ArrowIconComboBox::SelectionChanged, this, &ViewerContainerWidget::SetGeometryColoringMode);

    ui_->content_instance_mask_hex_input_->setMinimum(0x0);
    ui_->content_instance_mask_hex_input_->setMaximum(0xFF);
    ui_->content_instance_mask_hex_input_->setValue(0xFF);
    ui_->content_instance_mask_hex_input_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui_->content_instance_mask_hex_input_->setDisplayIntegerBase(16);
    ui_->content_instance_mask_hex_input_->setFixedWidth(30);

    // Force uppercase hexadecimal.
    QFont font = ui_->content_instance_mask_hex_input_->font();
    font.setCapitalization(QFont::AllUppercase);
    ui_->content_instance_mask_hex_input_->setFont(font);

    connect(ui_->content_instance_mask_bit_7_, &BinaryCheckbox::Clicked, this, [=]() { ViewerContainerWidget::BinaryCheckboxClicked(0b10000000); });
    connect(ui_->content_instance_mask_bit_6_, &BinaryCheckbox::Clicked, this, [=]() { ViewerContainerWidget::BinaryCheckboxClicked(0b01000000); });
    connect(ui_->content_instance_mask_bit_5_, &BinaryCheckbox::Clicked, this, [=]() { ViewerContainerWidget::BinaryCheckboxClicked(0b00100000); });
    connect(ui_->content_instance_mask_bit_4_, &BinaryCheckbox::Clicked, this, [=]() { ViewerContainerWidget::BinaryCheckboxClicked(0b00010000); });
    connect(ui_->content_instance_mask_bit_3_, &BinaryCheckbox::Clicked, this, [=]() { ViewerContainerWidget::BinaryCheckboxClicked(0b00001000); });
    connect(ui_->content_instance_mask_bit_2_, &BinaryCheckbox::Clicked, this, [=]() { ViewerContainerWidget::BinaryCheckboxClicked(0b00000100); });
    connect(ui_->content_instance_mask_bit_1_, &BinaryCheckbox::Clicked, this, [=]() { ViewerContainerWidget::BinaryCheckboxClicked(0b00000010); });
    connect(ui_->content_instance_mask_bit_0_, &BinaryCheckbox::Clicked, this, [=]() { ViewerContainerWidget::BinaryCheckboxClicked(0b00000001); });

    connect(ui_->content_instance_mask_hex_input_, SIGNAL(valueChanged(int)), this, SLOT(InstanceMaskHexChanged(int)));

    ui_->content_instance_mask_bit_7_->Initialize(true, rra::kCheckboxEnableColor);
    ui_->content_instance_mask_bit_6_->Initialize(true, rra::kCheckboxEnableColor);
    ui_->content_instance_mask_bit_5_->Initialize(true, rra::kCheckboxEnableColor);
    ui_->content_instance_mask_bit_4_->Initialize(true, rra::kCheckboxEnableColor);
    ui_->content_instance_mask_bit_3_->Initialize(true, rra::kCheckboxEnableColor);
    ui_->content_instance_mask_bit_2_->Initialize(true, rra::kCheckboxEnableColor);
    ui_->content_instance_mask_bit_1_->Initialize(true, rra::kCheckboxEnableColor);
    ui_->content_instance_mask_bit_0_->Initialize(true, rra::kCheckboxEnableColor);
}

void ViewerContainerWidget::ShowColoringMode(bool geometry_mode_enabled)
{
    ui_->content_geometry_coloring_mode_->setVisible(geometry_mode_enabled);
    ui_->content_traversal_counter_mode_->setVisible(!geometry_mode_enabled);
}

void ViewerContainerWidget::SetScene(rra::Scene* scene)
{
    model_->SetScene(scene);
}

void ViewerContainerWidget::BinaryCheckboxClicked(uint32_t mask)
{
    EvaluateInstanceMaskWarning(mask);

    uint32_t filter = model_->GetInstanceMaskFilter();
    filter ^= mask;
    model_->SetInstanceMaskFilter(filter);
    ui_->content_instance_mask_hex_input_->setValue(filter);
}

void ViewerContainerWidget::InstanceMaskHexChanged(int mask)
{
    uint32_t filter = (uint32_t)mask;
    model_->SetInstanceMaskFilter(filter);
    EvaluateInstanceMaskWarning(mask);

    ui_->content_instance_mask_bit_7_->setChecked(filter & 0b10000000);
    ui_->content_instance_mask_bit_6_->setChecked(filter & 0b01000000);
    ui_->content_instance_mask_bit_5_->setChecked(filter & 0b00100000);
    ui_->content_instance_mask_bit_4_->setChecked(filter & 0b00010000);
    ui_->content_instance_mask_bit_3_->setChecked(filter & 0b00001000);
    ui_->content_instance_mask_bit_2_->setChecked(filter & 0b00000100);
    ui_->content_instance_mask_bit_1_->setChecked(filter & 0b00000010);
    ui_->content_instance_mask_bit_0_->setChecked(filter & 0b00000001);
}

void ViewerContainerWidget::DisableMaskWidgetInteraction()
{
    ui_->content_instance_mask_bit_7_->DisableInteraction();
    ui_->content_instance_mask_bit_6_->DisableInteraction();
    ui_->content_instance_mask_bit_5_->DisableInteraction();
    ui_->content_instance_mask_bit_4_->DisableInteraction();
    ui_->content_instance_mask_bit_3_->DisableInteraction();
    ui_->content_instance_mask_bit_2_->DisableInteraction();
    ui_->content_instance_mask_bit_1_->DisableInteraction();
    ui_->content_instance_mask_bit_0_->DisableInteraction();

    ui_->content_instance_mask_hex_input_->setReadOnly(true);
    ui_->content_instance_mask_hex_input_->setEnabled(false);
    instance_mask_interaction_disabled_ = true;
}

void ViewerContainerWidget::UpdateMaskWidget(int mask)
{
    model_->SetInstanceMaskFilter(mask);
    EvaluateInstanceMaskWarning(mask);

    uint32_t filter = (uint32_t)mask;
    ui_->content_instance_mask_hex_input_->setValue(filter);
    ui_->content_instance_mask_bit_7_->setChecked(filter & 0b10000000);
    ui_->content_instance_mask_bit_6_->setChecked(filter & 0b01000000);
    ui_->content_instance_mask_bit_5_->setChecked(filter & 0b00100000);
    ui_->content_instance_mask_bit_4_->setChecked(filter & 0b00010000);
    ui_->content_instance_mask_bit_3_->setChecked(filter & 0b00001000);
    ui_->content_instance_mask_bit_2_->setChecked(filter & 0b00000100);
    ui_->content_instance_mask_bit_1_->setChecked(filter & 0b00000010);
    ui_->content_instance_mask_bit_0_->setChecked(filter & 0b00000001);
}

void ViewerContainerWidget::SetHeatmap(HeatmapColorType heatmap_type)
{
    ui_->content_heatmap_->SetSelectedRow(static_cast<int>(heatmap_type));
}

void ViewerContainerWidget::SetHeatmapMode()
{
    int row = ui_->content_heatmap_->CurrentRow();
    model_->SetHeatmapData(heatmap_generators_[row].generator_function());
    rra::Settings::Get().SetColoringMode(parent_pane_id_, kColoringModeHeatmapColor, row);
}

void ViewerContainerWidget::SetGeometryColoringMode()
{
    int                                 row        = ui_->content_geometry_coloring_mode_->CurrentRow();
    rra::renderer::GeometryColoringMode mode_value = filtered_color_modes_[row].value;
    model_->SetGeometryColoringMode(mode_value);
    rra::Settings::Get().SetColoringMode(parent_pane_id_, kColoringModeGeometryColor, row);
}

void ViewerContainerWidget::SetBVHColoringMode()
{
    int                            row        = ui_->content_bvh_coloring_mode_->CurrentRow();
    rra::renderer::BVHColoringMode mode_value = rra::renderer::kAvailableBVHColoringModes[row].value;
    model_->SetBVHColoringMode(static_cast<int>(mode_value));
    rra::Settings::Get().SetColoringMode(parent_pane_id_, kColoringModeBVHColor, static_cast<int>(mode_value));
}

void ViewerContainerWidget::SetTraversalCounterMode()
{
    int                                 row        = ui_->content_traversal_counter_mode_->CurrentRow();
    rra::renderer::TraversalCounterMode mode_value = filtered_traversal_modes_[row].value;
    model_->SetTraversalCounterMode(static_cast<int>(mode_value));
    rra::Settings::Get().SetColoringMode(parent_pane_id_, kColoringModeTraversalCounterColor, row);
}

void ViewerContainerWidget::UpdateRenderingModes(const std::vector<rra::renderer::GeometryColoringModeInfo>& coloring_modes,
                                                 const std::vector<rra::renderer::TraversalCounterModeInfo>& traversal_modes,
                                                 rra::renderer::BvhTypeFlags                                 bvh_type)
{
    // Construct a vector of the name of each coloring mode.
    std::vector<std::string> coloring_mode_names;
    for (rra::renderer::GeometryColoringModeInfo info : coloring_modes)
    {
        filtered_color_modes_.push_back(info);
        coloring_mode_names.push_back(info.name);
    }
    bvh_type_ = bvh_type;

    // Populate the coloring modes combo box with the available modes.
    rra::widget_util::RePopulateComboBox(ui_->content_geometry_coloring_mode_, coloring_mode_names);
    ui_->content_geometry_coloring_mode_->SetSelectedRow(rra::Settings::Get().GetColoringMode(parent_pane_id_, kColoringModeGeometryColor));

    if (bvh_type == rra::renderer::BvhTypeFlags::TopLevel)
    {
        ui_->instance_mask_filter_group_->show();
    }
    else
    {
        ui_->instance_mask_filter_group_->hide();
    }
    SetGeometryColoringMode();

    // Set a tooltip for each item with a description of the coloring mode.
    int name_count = static_cast<int>(coloring_mode_names.size());
    for (int mode_index = 0; mode_index < name_count; ++mode_index)
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
    ui_->content_bvh_coloring_mode_->SetSelectedRow(rra::Settings::Get().GetColoringMode(parent_pane_id_, kColoringModeBVHColor));
    SetBVHColoringMode();

    // Set a tooltip for each item with a description of the bvh coloring mode.
    int color_mode_count = static_cast<int>(rra::renderer::kAvailableBVHColoringModes.size());
    for (int mode_index = 0; mode_index < color_mode_count; ++mode_index)
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
    ui_->content_traversal_counter_mode_->SetSelectedRow(rra::Settings::Get().GetColoringMode(parent_pane_id_, kColoringModeTraversalCounterColor));
    SetTraversalCounterMode();

    // Set a tooltip for each item with a description of the bvh coloring mode.
    int counter_mode_count = static_cast<int>(traversal_counter_mode_names.size());
    for (int mode_index = 0; mode_index < counter_mode_count; ++mode_index)
    {
        auto traversal_mode_item = ui_->content_traversal_counter_mode_->FindItem(mode_index);
        traversal_mode_item->setToolTip(traversal_modes[mode_index].description.c_str());
    }

    // Initialize Heatmap modes.
    std::vector<std::string> heatmap_mode_names;
    heatmap_generators_ = rra::renderer::Heatmap::GetHeatmapGenerators();

    // Construct heatmap name vector.
    for (auto generator : heatmap_generators_)
    {
        heatmap_mode_names.push_back(generator.name);
    }

    // Populate heatmap dropdown with heatmap names.
    rra::widget_util::RePopulateComboBox(ui_->content_heatmap_, heatmap_mode_names);
    ui_->content_heatmap_->SetSelectedRow(rra::Settings::Get().GetColoringMode(parent_pane_id_, kColoringModeHeatmapColor));
    SetHeatmapMode();

    // Set heatmap tooltips.
    int heatmap_color_count = static_cast<int>(heatmap_mode_names.size());
    for (int mode_index = 0; mode_index < heatmap_color_count; ++mode_index)
    {
        auto heatmap_mode_item = ui_->content_heatmap_->FindItem(mode_index);
        heatmap_mode_item->setToolTip(heatmap_generators_[mode_index].tooltip.c_str());
    }
}

void ViewerContainerWidget::EvaluateInstanceMaskWarning(int mask)
{
    if (mask == 0)
    {
        ui_->content_instance_mask_bit_7_->EnableWarningOverride();
        ui_->content_instance_mask_bit_6_->EnableWarningOverride();
        ui_->content_instance_mask_bit_5_->EnableWarningOverride();
        ui_->content_instance_mask_bit_4_->EnableWarningOverride();
        ui_->content_instance_mask_bit_3_->EnableWarningOverride();
        ui_->content_instance_mask_bit_2_->EnableWarningOverride();
        ui_->content_instance_mask_bit_1_->EnableWarningOverride();
        ui_->content_instance_mask_bit_0_->EnableWarningOverride();
    }
    else
    {
        ui_->content_instance_mask_bit_7_->DisableWarningOverride();
        ui_->content_instance_mask_bit_6_->DisableWarningOverride();
        ui_->content_instance_mask_bit_5_->DisableWarningOverride();
        ui_->content_instance_mask_bit_4_->DisableWarningOverride();
        ui_->content_instance_mask_bit_3_->DisableWarningOverride();
        ui_->content_instance_mask_bit_2_->DisableWarningOverride();
        ui_->content_instance_mask_bit_1_->DisableWarningOverride();
        ui_->content_instance_mask_bit_0_->DisableWarningOverride();
    }

    ui_->content_instance_mask_bit_7_->repaint();
    ui_->content_instance_mask_bit_6_->repaint();
    ui_->content_instance_mask_bit_5_->repaint();
    ui_->content_instance_mask_bit_4_->repaint();
    ui_->content_instance_mask_bit_3_->repaint();
    ui_->content_instance_mask_bit_2_->repaint();
    ui_->content_instance_mask_bit_1_->repaint();
    ui_->content_instance_mask_bit_0_->repaint();
}