//=============================================================================
// Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the Ray history pane.
//=============================================================================

#include "views/ray/ray_history_pane.h"

#include "qt_common/utils/zoom_icon_group_manager.h"

#include <chrono>
#include <algorithm>
#include "managers/message_manager.h"
#include "views/widget_util.h"
#include "constants.h"
#include "public/rra_api_info.h"
#include "util/string_util.h"
#include "public/renderer_types.h"

/// @brief Get the plane that should be shown for a 2D dispatch.
///
/// @param dispatch_id The dispatch ID of a 2D dispatch.
///
/// @return The slice plane.
rra::renderer::SlicePlane GetSlicePlaneFor2DDispatch(uint64_t dispatch_id)
{
    uint32_t x{};
    uint32_t y{};
    uint32_t z{};
    RraRayGetDispatchDimensions(dispatch_id, &x, &y, &z);

    if (x == 1)
    {
        return rra::renderer::kSlicePlaneYZ;
    }
    if (y == 1)
    {
        return rra::renderer::kSlicePlaneXZ;
    }
    if (z == 1)
    {
        return rra::renderer::kSlicePlaneXY;
    }

    RRA_ASSERT_FAIL("Dispatch is not 2D.");
    return {};
}

/// @brief Get the dimension index of a 1D dispatch where the RH data is stored (the dimension greater than 1).
///
/// @param dispatch_id The dispatch ID of a 2D dispatch.
///
/// @return The index of the dimension with the RH data.
uint32_t GetDimensionIndexOf1DDispatch(uint64_t dispatch_id)
{
    uint32_t x{};
    uint32_t y{};
    uint32_t z{};
    RraRayGetDispatchDimensions(dispatch_id, &x, &y, &z);

    if (x > 1)
    {
        return 0;
    }
    else if (y > 1)
    {
        return 1;
    }

    return 2;
}

/// @brief Get the dimension (1D, 2D, or 3D) of a dispatch.
///
/// @param dispatch_id The dispatch ID.
///
/// @return The dimension.
uint32_t GetDispatchDimension(uint64_t dispatch_id)
{
    uint32_t x{};
    uint32_t y{};
    uint32_t z{};
    RraRayGetDispatchDimensions(dispatch_id, &x, &y, &z);

    // Empty dimensions will have value of 1, since for example a 2d dispatch could be (1920, 1080, 1).
    uint32_t empty_dimensions = (x == 1 ? 1 : 0) + (y == 1 ? 1 : 0) + (z == 1 ? 1 : 0);
    return std::max(1u, 3 - empty_dimensions);
}

RayHistoryPane::RayHistoryPane(QWidget* parent)
    : BasePane(parent)
    , ui_(new Ui::RayHistoryPane)
{
    ui_->setupUi(this);
    rra::widget_util::ApplyStandardPaneStyle(this, ui_->main_content_, ui_->main_scroll_area_);

    model_ = new rra::RayHistoryModel(rra::kRayListNumWidgets);

    model_->InitializeTableModel(ui_->ray_table_, 0, rra::kRayListColumnCount);

    auto header = ui_->ray_table_->horizontalHeader();
    connect(header, &QHeaderView::sectionClicked, [=]() { ui_->ray_table_->setSortingEnabled(true); });

    QList<int> splitter_sizes = {1, 3};  // This is a ratio not pixel widths.
    ui_->ray_splitter_->setSizes(splitter_sizes);
    ui_->ray_splitter_->setHandleWidth(1);

    table_delegate_ = new TableItemDelegate();
    ui_->ray_table_->setItemDelegate(table_delegate_);

    QWidget* rh_viewer_pane = new QWidget();
    ray_history_viewer_.setupUi(rh_viewer_pane);
    ray_history_viewer_.rh_traversal_counter_slider_->setCursor(Qt::PointingHandCursor);
    ray_history_viewer_.rh_traversal_counter_slider_->Init();
    rra::widget_util::InitializeComboBox(parent, ray_history_viewer_.heatmap_combo_box_, {});

    ray_history_viewer_.x_wrap_spin_box_->setToolTip("Reshape the x dimension of the 1D dispatch.");
    ray_history_viewer_.y_wrap_spin_box_->setToolTip("Reshape the y dimension of the 1D dispatch.");
    ray_history_viewer_.z_wrap_spin_box_->setToolTip("Reshape the z dimension of the 1D dispatch.");

    // Set the heatmap update callback.
    model_->SetHeatmapUpdateCallback([&](rra::renderer::HeatmapData heatmap_data) {
        auto heatmap_image = QImage(static_cast<int>(rra::kHeatmapResolution), 1, QImage::Format::Format_RGBA8888);

        for (size_t i = 0; i < rra::kHeatmapResolution; i++)
        {
            auto color = heatmap_data.Evaluate(i / (static_cast<float>(rra::kHeatmapResolution - 1)));
            heatmap_image.setPixelColor(QPoint(static_cast<int>(i), 0), QColor::fromRgbF(color.r, color.g, color.b, color.a));
        }

        ray_history_viewer_.rh_traversal_counter_slider_->SetHeatmap(QPixmap::fromImage(heatmap_image));
        ray_history_viewer_.rh_traversal_counter_slider_->repaint();
    });

    ray_history_viewer_.ray_graphics_view_->show();
    ray_history_viewer_.ray_graphics_view_->SetBoxSelectCallback([=](uint32_t min_x, uint32_t min_y, uint32_t max_x, uint32_t max_y) {
        ui_->ray_table_->setSortingEnabled(false);

        GlobalInvocationID min_id{};
        GlobalInvocationID max_id{};
        model_->GetProxyModel()->GetFilterMinAndMax(&min_id, &max_id);
        if (min_x == min_id.x && min_y == min_id.y && max_x == max_id.x && max_y == max_id.y)
        {
            return;
        }

        GlobalInvocationID min{};
        GlobalInvocationID max{};

        switch (model_->GetSlicePlane())
        {
        case rra::renderer::kSlicePlaneXY:
            min = {min_x, min_y, 0};
            max = {max_x, max_y, std::numeric_limits<uint32_t>::max()};
            break;
        case rra::renderer::kSlicePlaneXZ:
            min = {min_x, 0, min_y};
            max = {max_x, std::numeric_limits<uint32_t>::max(), max_y};
            break;
        case rra::renderer::kSlicePlaneYZ:
            min = {0, min_y, min_x};
            max = {std::numeric_limits<uint32_t>::max(), max_y, max_x};
            break;
        }
        model_->GetProxyModel()->SetFilterByInvocationId(min, max);
        model_->GetProxyModel()->SetReshapeDimension(
            ray_history_viewer_.x_wrap_spin_box_->value(), ray_history_viewer_.y_wrap_spin_box_->value(), ray_history_viewer_.z_wrap_spin_box_->value());
        model_->UpdateStatsTable();
        model_->GetProxyModel()->invalidate();
    });

    ray_history_viewer_.ray_graphics_view_->SetPixelSelectCallback([=](uint32_t x, uint32_t y, bool double_click) {
        QModelIndex model_index{};

        if (GetDispatchDimension(dispatch_id_) == 1)
        {
            uint32_t coord_array[3]{};
            coord_array[GetDimensionIndexOf1DDispatch(dispatch_id_)] = Get1DCoordinate(x, y);
            model_index                                              = model_->GetDispatchCoordinateIndex({coord_array[0], coord_array[1], coord_array[2]});
        }
        else
        {
            switch (model_->GetSlicePlane())
            {
            case rra::renderer::kSlicePlaneXY:
                model_index = model_->GetDispatchCoordinateIndex({x, y, model_->GetSliceIndex()});
                break;
            case rra::renderer::kSlicePlaneXZ:
                model_index = model_->GetDispatchCoordinateIndex({x, model_->GetSliceIndex(), y});
                break;
            case rra::renderer::kSlicePlaneYZ:
                model_index = model_->GetDispatchCoordinateIndex({model_->GetSliceIndex(), y, x});
                break;
            }
        }

        if (!model_index.isValid())
        {
            return;
        }

        SelectRayCoordinate();
        ui_->ray_table_->selectionModel()->setCurrentIndex(model_index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui_->ray_table_->scrollTo(model_index, QAbstractItemView::PositionAtTop);

        if (double_click)
        {
            SelectRayAndSwitchPane(model_index);
        }
    });

    ray_history_viewer_.ray_graphics_view_->SetPixelHoverCallback([=](uint32_t x, uint32_t y, bool in_bounds) {
        uint32_t coord_array[3]{};
        coord_array[GetDimensionIndexOf1DDispatch(dispatch_id_)] = Get1DCoordinate(x, y);
        QString coord                                            = "(" + QString::number(x) + ", " + QString::number(y) + ")";

        if (GetDispatchDimension(dispatch_id_) == 1)
        {
            // Connect the heatmap coordinates to the 1D coordinates by the utf8 arrow glyph.
            QString coord_1d = "(" + QString::number(coord_array[0]) + ", " + QString::number(coord_array[1]) + ", " + QString::number(coord_array[2]) + ")";
            coord += u8" \u2192 " + coord_1d;
        }

        ray_history_viewer_.pixel_coordinate_->setText(in_bounds ? coord : "");
    });

    // Extract the color mode names from color_modes_and_names_.
    std::vector<std::string> color_mode_names{};
    color_mode_names.reserve(color_modes_and_names_.size());
    for (const auto& pair : color_modes_and_names_)
    {
        color_mode_names.push_back(pair.second);
    }

    std::vector<std::string> dispatch_plane_options{"XY image with Z index at", "XZ image with Y index at", "YZ image with X index at"};
    rra::widget_util::InitializeComboBox(this, ray_history_viewer_.dispatch_plane_combo_box_, dispatch_plane_options);
    rra::widget_util::InitializeComboBox(this, ray_history_viewer_.color_mode_combo_box_, color_mode_names);

    ui_->ray_history_viewer_container_->addWidget(rh_viewer_pane);

    ui_->dispatch_stats_table_->horizontalHeader()->hide();
    ui_->dispatch_stats_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui_->dispatch_stats_table_->setFocusPolicy(Qt::NoFocus);
    ui_->dispatch_stats_table_->setSelectionMode(QAbstractItemView::NoSelection);
    model_->InitializeStatsTableModel(ui_->dispatch_stats_table_);

    ui_->shader_binding_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui_->shader_binding_table_->setFocusPolicy(Qt::NoFocus);
    ui_->shader_binding_table_->setSelectionMode(QAbstractItemView::NoSelection);
    ui_->shader_binding_table_->setColumnWidth(0, 5);
    model_->InitializeShaderBindingTableModel(ui_->shader_binding_table_);

    connect(ray_history_viewer_.heatmap_combo_box_, &ArrowIconComboBox::SelectionChanged, this, &RayHistoryPane::SetHeatmapSpectrum);
    connect(ray_history_viewer_.color_mode_combo_box_, &ArrowIconComboBox::SelectionChanged, this, &RayHistoryPane::SetColorMode);
    connect(ray_history_viewer_.dispatch_plane_combo_box_, &ArrowIconComboBox::SelectionChanged, this, &RayHistoryPane::SetSlicePlane);
    connect(ui_->dispatch_combo_box_, &ArrowIconComboBox::SelectionChanged, this, &RayHistoryPane::UpdateSelectedDispatch);
    connect(ray_history_viewer_.rh_traversal_counter_slider_, &DoubleSliderHeatmapWidget::SpanChanged, this, &RayHistoryPane::SetTraversalCounterRange);
    connect(ui_->ray_table_, &QAbstractItemView::doubleClicked, this, &RayHistoryPane::SelectRayAndSwitchPane);

    connect(ray_history_viewer_.content_ray_index_, &QSlider::valueChanged, this, &RayHistoryPane::SetCurrentRayIndex);

    connect(ui_->ray_table_->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(SelectRayCoordinate()));
    connect(ray_history_viewer_.dispatch_plane_spin_box_, SIGNAL(valueChanged(int)), this, SLOT(DispatchSliceChanged(int)));
    connect(ray_history_viewer_.x_wrap_spin_box_, SIGNAL(valueChanged(int)), this, SLOT(ReshapeDimensionChanged(int)));
    connect(ray_history_viewer_.y_wrap_spin_box_, SIGNAL(valueChanged(int)), this, SLOT(ReshapeDimensionChanged(int)));
    connect(ray_history_viewer_.z_wrap_spin_box_, SIGNAL(valueChanged(int)), this, SLOT(ReshapeDimensionChanged(int)));

    connect(&rra::MessageManager::Get(), &rra::MessageManager::DispatchSelected, [=](uint32_t dispatch_id) {
        ui_->dispatch_combo_box_->SetSelectedRow(dispatch_id);
    });

    // Set up the zoom buttons.
    ZoomIconManagerConfiguration zoom_config        = {};
    zoom_config.zoom_in_button                      = ray_history_viewer_.zoom_in_button_;
    zoom_config.zoom_in_resource_enabled            = rra::resource::kZoomInEnabled;
    zoom_config.zoom_in_resource_disabled           = rra::resource::kZoomInDisabled;
    zoom_config.zoom_out_button                     = ray_history_viewer_.zoom_out_button_;
    zoom_config.zoom_out_resource_enabled           = rra::resource::kZoomOutEnabled;
    zoom_config.zoom_out_resource_disabled          = rra::resource::kZoomOutDisabled;
    zoom_config.zoom_reset_button                   = ray_history_viewer_.zoom_reset_button_;
    zoom_config.zoom_reset_resource_enabled         = rra::resource::kZoomResetEnabled;
    zoom_config.zoom_reset_resource_disabled        = rra::resource::kZoomResetDisabled;
    zoom_config.zoom_to_selection_button            = ray_history_viewer_.zoom_to_selection_button_;
    zoom_config.zoom_to_selection_resource_enabled  = rra::resource::kZoomToSelectionEnabled;
    zoom_config.zoom_to_selection_resource_disabled = rra::resource::kZoomToSelectionDisabled;

    zoom_icon_manager_ = new ZoomIconGroupManager(zoom_config);

    connect(ray_history_viewer_.zoom_reset_button_, &QPushButton::pressed, ray_history_viewer_.ray_graphics_view_, &RayHistoryGraphicsView::ResetZoom);
    connect(ray_history_viewer_.zoom_in_button_, &QPushButton::pressed, ray_history_viewer_.ray_graphics_view_, &RayHistoryGraphicsView::ZoomIn);
    connect(ray_history_viewer_.zoom_out_button_, &QPushButton::pressed, ray_history_viewer_.ray_graphics_view_, &RayHistoryGraphicsView::ZoomOut);
    connect(
        ray_history_viewer_.zoom_to_selection_button_, &QPushButton::pressed, ray_history_viewer_.ray_graphics_view_, &RayHistoryGraphicsView::ZoomToSelection);
    connect(ray_history_viewer_.ray_graphics_view_, &RayHistoryGraphicsView::UpdateZoomButtons, this, &RayHistoryPane::UpdateZoomButtons);

    connect(&timer_, &QTimer::timeout, this, &RayHistoryPane::TimerUpdate);

    // Hide the shader binding table for now until correct data is parsed from the backend.
    ui_->shader_binding_table_container_->hide();
}

RayHistoryPane::~RayHistoryPane()
{
    delete zoom_icon_manager_;
}

void RayHistoryPane::OnTraceOpen()
{
    timer_.start(10);  // Update every 10 msec.

    // Initialize Heatmap modes.
    std::vector<std::string> heatmap_mode_names;
    heatmap_generators_ = rra::renderer::Heatmap::GetHeatmapGenerators();

    // Construct heatmap name vector.
    for (auto generator : heatmap_generators_)
    {
        heatmap_mode_names.push_back(generator.name);
    }

    rra::widget_util::InitializeComboBox(this, ray_history_viewer_.heatmap_combo_box_, heatmap_mode_names);

    InitializeReshapedDimensions();

    ClearRaySelection();

    // We set the flag to false here to instruct loading the dispatch correctly.
    uint32_t dispatch_count{};
    RraRayGetDispatchCount(&dispatch_count);

    dispatches_loaded_.resize(dispatch_count, false);
    LoadDispatches();

    SetDispatchId(0);

    ray_history_viewer_.ray_graphics_view_->ResetZoom();
}

void RayHistoryPane::OnTraceClose()
{
    show_event_occured_ = false;
    dispatch_id_        = 0;
}

void RayHistoryPane::CreateAndRenderImage()
{
    model_->CreateRayHistoryStatsBuffer(dispatch_id_, &max_statistics_);
    UpdateSliderRange();
    QImage heatmap_image{RenderRayHistoryImage()};
    ray_history_viewer_.ray_graphics_view_->SetHeatmapImage(heatmap_image);
}

void RayHistoryPane::UpdateSliderRange()
{
    ray_history_viewer_.rh_traversal_counter_slider_->setMaximum(GetCurrentColorModeMaxStatistic());
    ray_history_viewer_.rh_traversal_counter_slider_->setMinimum(0);
    ray_history_viewer_.rh_traversal_counter_slider_->SetUpperPosition(GetCurrentColorModeMaxStatistic());
    ray_history_viewer_.rh_traversal_counter_slider_->SetLowerPosition(0);
    ray_history_viewer_.rh_traversal_max_value_->setText(QString::number(GetCurrentColorModeMaxStatistic()));
    ray_history_viewer_.rh_traversal_min_value_->setText(QString::number(0));

    int max_ray_count = max_statistics_.ray_count - 1;
    max_ray_count     = std::max(max_ray_count, 0);
    ray_history_viewer_.content_ray_index_->setMaximum(max_ray_count);
}

void RayHistoryPane::UpdateDispatchSpinBoxRanges()
{
    // Update slice range.
    switch (model_->GetSlicePlane())
    {
    case rra::renderer::kSlicePlaneXY:
        ray_history_viewer_.dispatch_plane_spin_box_->setRange(0, dispatch_reshaped_dimensions_[dispatch_id_].z - 1);
        break;
    case rra::renderer::kSlicePlaneXZ:
        ray_history_viewer_.dispatch_plane_spin_box_->setRange(0, dispatch_reshaped_dimensions_[dispatch_id_].y - 1);
        break;
    case rra::renderer::kSlicePlaneYZ:
        ray_history_viewer_.dispatch_plane_spin_box_->setRange(0, dispatch_reshaped_dimensions_[dispatch_id_].x - 1);
        break;
    }

    // Update reshape range.
    uint32_t x{};
    uint32_t y{};
    uint32_t z{};
    RraRayGetDispatchDimensions(dispatch_id_, &x, &y, &z);

    uint32_t max_dimension = std::max(x, std::max(y, z));
    ray_history_viewer_.x_wrap_spin_box_->blockSignals(true);
    ray_history_viewer_.y_wrap_spin_box_->blockSignals(true);
    ray_history_viewer_.z_wrap_spin_box_->blockSignals(true);
    ray_history_viewer_.x_wrap_spin_box_->setRange(1, max_dimension);
    ray_history_viewer_.y_wrap_spin_box_->setRange(1, max_dimension);
    ray_history_viewer_.z_wrap_spin_box_->setRange(1, max_dimension);
    ray_history_viewer_.x_wrap_spin_box_->blockSignals(false);
    ray_history_viewer_.y_wrap_spin_box_->blockSignals(false);
    ray_history_viewer_.z_wrap_spin_box_->blockSignals(false);
}

void RayHistoryPane::InitializeReshapedDimensions()
{
    uint32_t dispatch_count{};
    RraRayGetDispatchCount(&dispatch_count);

    dispatch_reshaped_dimensions_.clear();
    dispatch_reshaped_dimensions_.resize(dispatch_count, {});
}

uint32_t RayHistoryPane::Get1DCoordinate(uint32_t x, uint32_t y)
{
    uint32_t w{(uint32_t)ray_history_viewer_.x_wrap_spin_box_->value()};
    uint32_t h{(uint32_t)ray_history_viewer_.y_wrap_spin_box_->value()};
    uint32_t z{(uint32_t)ray_history_viewer_.dispatch_plane_spin_box_->value()};

    switch (model_->GetSlicePlane())
    {
    case rra::renderer::kSlicePlaneXY:
        return x + (y * w) + (z * w * h);
    case rra::renderer::kSlicePlaneXZ:
        return x + (z * w) + (y * w * h);
    case rra::renderer::kSlicePlaneYZ:
        return z + (y * w) + (x * w * h);
    }

    return 0xFFFFFFFF;
}

void RayHistoryPane::ClearRaySelection()
{
    ui_->ray_table_->selectionModel()->clear();
}

void RayHistoryPane::TimerUpdate()
{
    uint32_t dispatch_count = 0;
    RraRayGetDispatchCount(&dispatch_count);

    if (dispatch_count == 0)
    {
        timer_.stop();
        return;
    }

    RraDispatchLoadStatus load_status = {};
    RraRayGetDispatchStatus(dispatch_id_, &load_status);

    DispatchType dispatch_type = kRayTracingPipeline;

    RraRayHistoryStats stats = {};

    RraRayGetDispatchType(dispatch_id_, &dispatch_type);
    RraRayGetDispatchStats(dispatch_id_, &stats);

    // Start of with the dispatch region and error label hidden.
    ui_->loading_bar_->SetText("Loading dispatch");
    if (load_status.raw_data_parsed)
    {
        // First phase is complete
        ui_->loading_bar_->SetText("Indexing dispatch");
    }

    // We have indexed everything, time to gather stats.
    // Show the dispatch region as the numbers count up.
    if (load_status.data_indexed)
    {
        ui_->loading_bar_->SetText("Gathering stats");
    }

    bool all_dispatches_loaded    = true;
    bool should_reload_dispatches = false;

    for (uint32_t i = 0; i < dispatch_count; i++)
    {
        RraDispatchLoadStatus individual_load_status = {};
        RraRayGetDispatchStatus(i, &individual_load_status);
        if (!individual_load_status.loading_complete)
        {
            all_dispatches_loaded = false;
        }
        else
        {
            // Update internal status.
            if (!dispatches_loaded_[i])
            {
                uint32_t x{};
                uint32_t y{};
                uint32_t z{};
                RraRayGetDispatchDimensions(i, &x, &y, &z);

                dispatch_reshaped_dimensions_[i].x = x;
                dispatch_reshaped_dimensions_[i].y = y;
                dispatch_reshaped_dimensions_[i].z = z;

                if (i == dispatch_id_)
                {
                    UpdateReshapedDimensions(i);
                    UpdateSelectedDispatch();
                }

                dispatches_loaded_[i]    = true;
                should_reload_dispatches = true;
            }
        }
    }

    if (should_reload_dispatches)
    {
        LoadDispatches();
    }

    if (all_dispatches_loaded)
    {
        timer_.stop();  // We are done with updating.
    }

    // We are done with loading. Hide away!
    if (load_status.loading_complete && !load_status.has_errors)
    {
        ui_->dispatch_valid_switch_->setCurrentIndex(0);
    }

    // If there are errors at any stage we should raise them to the user.
    if (load_status.has_errors)
    {
        ui_->dispatch_valid_switch_->setCurrentIndex(1);
    }
    else
    {
        ui_->loading_bar_->SetFillPercentage(load_status.load_percentage);
    }

    // Repaint some elements.
    ui_->loading_bar_->repaint();
}

void RayHistoryPane::SetDispatchId(uint64_t dispatch_id)
{
    dispatch_id_ = dispatch_id;

    uint32_t dispatch_count = 0;
    RraRayGetDispatchCount(&dispatch_count);
    if (dispatch_count <= dispatch_id)
    {
        return;
    }

    RraDispatchLoadStatus load_status = {};
    RraRayGetDispatchStatus(dispatch_id, &load_status);

    if (load_status.has_errors)
    {
        // In case of invalid dispatch, show an error screen instead of crashing.
        ui_->dispatch_valid_switch_->setCurrentIndex(1);
        return;
    }
    else if (!load_status.loading_complete)
    {
        ui_->dispatch_valid_switch_->setCurrentIndex(2);
        return;
    }

    // Valid dispatch, display the actual data instead of an error screen.
    ui_->dispatch_valid_switch_->setCurrentIndex(0);

    ui_->ray_table_->setSortingEnabled(false);

    if (!show_event_occured_)
    {
        return;
    }

    UpdateDispatchSpinBoxRanges();

    // Load the saved reshaped dispatch size of the dispatch we're switching to.
    // Block signals to avoid rendering heatmap image while setting these values.
    UpdateReshapedDimensions(dispatch_id);

    ray_history_viewer_.dispatch_plane_spin_box_->setValue(0);

    model_->UpdateRayList(dispatch_id);

    CreateAndRenderImage();

    switch (GetDispatchDimension(dispatch_id))
    {
    case 1:
        ray_history_viewer_.reshape_label_->show();
        ray_history_viewer_.x_wrap_spin_box_->show();
        ray_history_viewer_.y_wrap_spin_box_->show();
        ray_history_viewer_.z_wrap_spin_box_->show();
        ray_history_viewer_.rh_viewer_line_separator_0_->show();

        ray_history_viewer_.dispatch_plane_spin_box_->show();
        ray_history_viewer_.dispatch_plane_combo_box_->show();
        ray_history_viewer_.rh_viewer_line_separator_1_->show();
        break;
    case 2:
        // Make sure we show the plane of the two axes with resolution > 1.
        ray_history_viewer_.dispatch_plane_combo_box_->SetSelectedRow((int)GetSlicePlaneFor2DDispatch(dispatch_id));
        ray_history_viewer_.reshape_label_->hide();
        ray_history_viewer_.x_wrap_spin_box_->hide();
        ray_history_viewer_.y_wrap_spin_box_->hide();
        ray_history_viewer_.z_wrap_spin_box_->hide();
        ray_history_viewer_.rh_viewer_line_separator_0_->hide();

        ray_history_viewer_.dispatch_plane_spin_box_->hide();
        ray_history_viewer_.dispatch_plane_combo_box_->hide();
        ray_history_viewer_.rh_viewer_line_separator_1_->hide();
        break;
    case 3:
        ray_history_viewer_.reshape_label_->hide();
        ray_history_viewer_.x_wrap_spin_box_->hide();
        ray_history_viewer_.y_wrap_spin_box_->hide();
        ray_history_viewer_.z_wrap_spin_box_->hide();
        ray_history_viewer_.rh_viewer_line_separator_0_->hide();

        ray_history_viewer_.dispatch_plane_spin_box_->show();
        ray_history_viewer_.dispatch_plane_combo_box_->show();
        ray_history_viewer_.rh_viewer_line_separator_1_->show();
        break;
    }

    ray_history_viewer_.ray_graphics_view_->ClearBoxSelect();
}

void RayHistoryPane::UpdateSelectedDispatch()
{
    uint32_t dispatch_id{(uint32_t)ui_->dispatch_combo_box_->CurrentRow()};
    SetDispatchId(dispatch_id);
}

void RayHistoryPane::SelectRayCoordinate()
{
    QModelIndex index = ui_->ray_table_->currentIndex();

    if (index.isValid())
    {
        uint32_t x{};
        uint32_t y{};
        uint32_t z{};
        uint32_t ray_count{};
        model_->GetRayCoordinate(index, &x, &y, &z, &ray_count);

        uint32_t reshaped_x{x};
        uint32_t reshaped_y{y};
        uint32_t reshaped_z{z};
        if (GetDispatchDimension(dispatch_id_) == 1)
        {
            uint32_t coord_1d = std::vector{x, y, z}[GetDimensionIndexOf1DDispatch(dispatch_id_)];

            uint32_t width{(uint32_t)ray_history_viewer_.x_wrap_spin_box_->value()};
            uint32_t height{(uint32_t)ray_history_viewer_.y_wrap_spin_box_->value()};

            reshaped_x = coord_1d % width;
            reshaped_z = coord_1d / (width * height);
            reshaped_y = (coord_1d - reshaped_z * width * height) / width;
        }

        switch (model_->GetSlicePlane())
        {
        case rra::renderer::kSlicePlaneXY:
            ray_history_viewer_.ray_graphics_view_->SetSelectedPixel(reshaped_x, reshaped_y);
            break;
        case rra::renderer::kSlicePlaneXZ:
            ray_history_viewer_.ray_graphics_view_->SetSelectedPixel(reshaped_x, reshaped_z);
            break;
        case rra::renderer::kSlicePlaneYZ:
            ray_history_viewer_.ray_graphics_view_->SetSelectedPixel(reshaped_z, reshaped_y);
            break;
        }
        emit rra::MessageManager::Get().RayCoordinateSelected(dispatch_id_, x, y, z);
    }
}

void RayHistoryPane::SelectRayAndSwitchPane(const QModelIndex& index)
{
    RRA_UNUSED(index);
    SelectRayCoordinate();

    // Switch to the ray inspector pane.
    emit rra::MessageManager::Get().PaneSwitchRequested(rra::kPaneIdRayInspector);
}

void RayHistoryPane::LoadDispatches()
{
    uint32_t dispatch_count{};
    RraRayGetDispatchCount(&dispatch_count);
    std::vector<std::string> dispatch_options{};
    for (uint32_t id{0}; id < dispatch_count; ++id)
    {
        RraDispatchLoadStatus load_status = {};
        RraRayGetDispatchStatus(id, &load_status);

        std::string trace_rays_str = RraApiInfoIsVulkan() ? "vkCmdTraceRaysKHR" : "DispatchRays";

        if (load_status.loading_complete)
        {
            uint32_t x{};
            uint32_t y{};
            uint32_t z{};
            RraRayGetDispatchDimensions(id, &x, &y, &z);

            dispatch_options.push_back(std::to_string(id) + ": " + trace_rays_str + "(" + std::to_string(x) + ", " + std::to_string(y) + ", " +
                                       std::to_string(z) + ")");
        }
        else
        {
            dispatch_options.push_back(std::to_string(id) + " (Loading)");
        }
    }
    ui_->dispatch_combo_box_->blockSignals(true);
    rra::widget_util::InitializeComboBox(this, ui_->dispatch_combo_box_, dispatch_options);
    ui_->dispatch_combo_box_->SetSelectedRow(dispatch_id_);
    ui_->dispatch_combo_box_->blockSignals(false);
}

void RayHistoryPane::UpdateReshapedDimensions(uint64_t dispatch_id)
{
    // Block signals to avoid rendering heatmap image while setting these values.
    ray_history_viewer_.x_wrap_spin_box_->blockSignals(true);
    ray_history_viewer_.y_wrap_spin_box_->blockSignals(true);
    ray_history_viewer_.z_wrap_spin_box_->blockSignals(true);
    ray_history_viewer_.x_wrap_spin_box_->setValue((int)dispatch_reshaped_dimensions_[dispatch_id].x);
    ray_history_viewer_.y_wrap_spin_box_->setValue((int)dispatch_reshaped_dimensions_[dispatch_id].y);
    ray_history_viewer_.z_wrap_spin_box_->setValue((int)dispatch_reshaped_dimensions_[dispatch_id].z);
    ray_history_viewer_.x_wrap_spin_box_->blockSignals(false);
    ray_history_viewer_.y_wrap_spin_box_->blockSignals(false);
    ray_history_viewer_.z_wrap_spin_box_->blockSignals(false);
    ray_history_viewer_.x_wrap_spin_box_->repaint();
    ray_history_viewer_.y_wrap_spin_box_->repaint();
    ray_history_viewer_.z_wrap_spin_box_->repaint();
}

void RayHistoryPane::SetColorMode()
{
    int row = ray_history_viewer_.color_mode_combo_box_->CurrentRow();
    model_->SetColorMode(color_modes_and_names_[row].first);

    if (color_modes_and_names_[row].first == rra::renderer::RayHistoryColorMode::kRayHistoryColorModeRayDirection)
    {
        ray_history_viewer_.content_ray_index_->show();
        ray_history_viewer_.rh_traversal_ray_index_value_->show();
        ray_history_viewer_.rh_traversal_min_value_->hide();
        ray_history_viewer_.rh_traversal_max_value_->hide();
        ray_history_viewer_.rh_traversal_counter_slider_->hide();
        ray_history_viewer_.heatmap_combo_box_->hide();
    }
    else
    {
        ray_history_viewer_.content_ray_index_->hide();
        ray_history_viewer_.rh_traversal_ray_index_value_->hide();
        ray_history_viewer_.rh_traversal_min_value_->show();
        ray_history_viewer_.rh_traversal_max_value_->show();
        ray_history_viewer_.rh_traversal_counter_slider_->show();
        ray_history_viewer_.heatmap_combo_box_->show();
    }

    if (show_event_occured_)
    {
        UpdateSliderRange();
        QImage heatmap_image{RenderRayHistoryImage()};
        ray_history_viewer_.ray_graphics_view_->SetHeatmapImage(heatmap_image);
    }
}

void RayHistoryPane::SetSlicePlane()
{
    int row = ray_history_viewer_.dispatch_plane_combo_box_->CurrentRow();
    model_->SetSlicePlane((rra::renderer::SlicePlane)row);
    ray_history_viewer_.ray_graphics_view_->HideSelectedPixelIcon();

    if (show_event_occured_)
    {
        UpdateSliderRange();
        QImage heatmap_image{RenderRayHistoryImage()};
        ray_history_viewer_.ray_graphics_view_->SetHeatmapImage(heatmap_image);
        UpdateDispatchSpinBoxRanges();
        ray_history_viewer_.ray_graphics_view_->ClearBoxSelect();
    }
}

void RayHistoryPane::SetTraversalCounterRange(int min_value, int max_value)
{
    if (show_event_occured_)
    {
        ray_history_viewer_.rh_traversal_min_value_->setText(QString::number(min_value));
        ray_history_viewer_.rh_traversal_max_value_->setText(QString::number(max_value));

        QImage heatmap_image{RenderRayHistoryImage()};
        ray_history_viewer_.ray_graphics_view_->SetHeatmapImage(heatmap_image);
    }
}

void RayHistoryPane::SetCurrentRayIndex(int ray_index)
{
    if (show_event_occured_)
    {
        ray_history_viewer_.rh_traversal_ray_index_value_->setText(QString::number(ray_index));

        QImage heatmap_image{RenderRayHistoryImage()};
        ray_history_viewer_.ray_graphics_view_->SetHeatmapImage(heatmap_image);
    }
}

void RayHistoryPane::DispatchSliceChanged(int slice_index)
{
    model_->SetSliceIndex((uint32_t)slice_index);

    QImage heatmap_image{RenderRayHistoryImage()};
    ray_history_viewer_.ray_graphics_view_->SetHeatmapImage(heatmap_image);
}

void RayHistoryPane::ReshapeDimensionChanged(int)
{
    if (show_event_occured_)
    {
        uint32_t           w             = (uint32_t)ray_history_viewer_.x_wrap_spin_box_->value();
        uint32_t           h             = (uint32_t)ray_history_viewer_.y_wrap_spin_box_->value();
        uint32_t           d             = (uint32_t)ray_history_viewer_.z_wrap_spin_box_->value();
        constexpr uint32_t kResolution8k = 7680 * 4320;

        // Save reshaped dispatch size of dispatch which will be remembered even if we switch away from this dispatch then back.
        if (dispatch_id_ != 0xFFFFFFFF)
        {
            dispatch_reshaped_dimensions_[dispatch_id_] = {w, h, d};
        }

        // If the total dimension is greater than 8K resolution, don't render it.
        if (w * h * d > kResolution8k)
        {
            return;
        }

        UpdateDispatchSpinBoxRanges();
        QImage heatmap_image{RenderRayHistoryImage()};
        ray_history_viewer_.ray_graphics_view_->SetHeatmapImage(heatmap_image);
        ray_history_viewer_.ray_graphics_view_->HideSelectedPixelIcon();
        ray_history_viewer_.ray_graphics_view_->ClearBoxSelect();
    }
}

QImage RayHistoryPane::RenderRayHistoryImage()
{
    uint32_t min_val{(uint32_t)ray_history_viewer_.rh_traversal_min_value_->text().toInt()};
    uint32_t max_val{(uint32_t)ray_history_viewer_.rh_traversal_max_value_->text().toInt()};
    uint32_t ray_index = ray_history_viewer_.content_ray_index_->value();

    return model_->RenderRayHistoryImage(min_val,
                                         max_val,
                                         ray_index,
                                         dispatch_reshaped_dimensions_[dispatch_id_].x,
                                         dispatch_reshaped_dimensions_[dispatch_id_].y,
                                         dispatch_reshaped_dimensions_[dispatch_id_].z);
}

uint32_t RayHistoryPane::GetCurrentColorModeMaxStatistic()
{
    switch (model_->GetColorMode())
    {
    case rra::renderer::kRayHistoryColorModeRayCount:
        return max_statistics_.ray_count;
    case rra::renderer::kRayHistoryColorModeTraversalCount:
        return max_statistics_.traversal_count;
    case rra::renderer::kRayHistoryColorModeInstanceIntersectionCount:
        return max_statistics_.instance_intersection_count;
    case rra::renderer::kRayHistoryColorModeAnyHitInvocationCount:
        return max_statistics_.any_hit_invocation_count;
    case rra::renderer::kRayHistoryColorModeRayDirection:
        return max_statistics_.ray_count;
    }

    return 0xFFFFFFFF;
}

void RayHistoryPane::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}

void RayHistoryPane::showEvent(QShowEvent* event)
{
    if (!show_event_occured_)
    {
        // The reshape spin box needs to start with the correct dimensions since SetDispatchId() will save its current state and
        // load the new state. If we don't set it here, it will be saved with reshape dimensions (1, 1, 1).
        if (dispatch_id_ < dispatch_reshaped_dimensions_.size())
        {
            UpdateReshapedDimensions(dispatch_id_);
        }

        show_event_occured_ = true;
    }

    SetDispatchId(dispatch_id_);
    SetColorMode();

    BasePane::showEvent(event);
}

void RayHistoryPane::SetHeatmapSpectrum()
{
    int row = ray_history_viewer_.heatmap_combo_box_->CurrentRow();
    model_->SetHeatmapData(heatmap_generators_[row].generator_function());

    if (show_event_occured_)
    {
        QImage heatmap_image{RenderRayHistoryImage()};
        ray_history_viewer_.ray_graphics_view_->SetHeatmapImage(heatmap_image);
    }
}

void RayHistoryPane::UpdateZoomButtons(bool zoom_in, bool zoom_out, bool zoom_selection, bool reset)
{
    zoom_icon_manager_->SetButtonStates(zoom_in, zoom_out, zoom_selection, reset);
}
