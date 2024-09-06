//=============================================================================
// Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the dispatch pane.
//=============================================================================

#include "dispatch_pane.h"

#include "qt_common/utils/qt_util.h"

#include "managers/message_manager.h"

#include <string>
#include "settings/settings.h"
#include "util/string_util.h"
#include "views/widget_util.h"
#include "constants.h"

#include "public/rra_api_info.h"
#include "public/rra_ray_history.h"

#include <sstream>

static const int kValueFontSize = 14;
static const int kTextFontSize  = 10;
static const int kDonutWidth    = 15;
static const int kDonutSize     = 130;

DispatchPane::DispatchPane(QWidget* parent)
    : QWidget(parent)
    , ui_(new Ui_DispatchPane)
{
    ui_->setupUi(this);
    ui_->donut_->SetNumSegments(kNumInvocations);
    ui_->donut_->SetArcWidth(kDonutWidth);
    ui_->donut_->SetSize(kDonutSize);
    ui_->donut_->SetFontSizes(kValueFontSize, kTextFontSize);
    ui_->donut_->SetTextLineTwo("Total");

    connect(&timer_, &QTimer::timeout, this, &DispatchPane::Update);

    timer_.start(10);  // Update every 10 msec.
}

DispatchPane::~DispatchPane()
{
    disconnect(ui_->dispatch_title_, &ScaledPushButton::released, this, &DispatchPane::NavigateToRayHistory);
}

void DispatchPane::showEvent(QShowEvent* event)
{
    // Set the legend and donut colors in case they have been changed in the settings|themes and colors pane.
    ui_->raygen_legend_->SetColor(rra::Settings::Get().GetColorValue(kSettingThemesAndColorsInvocationRaygen));
    ui_->closest_hit_legend_->SetColor(rra::Settings::Get().GetColorValue(kSettingThemesAndColorsInvocationClosestHit));
    ui_->any_hit_legend_->SetColor(rra::Settings::Get().GetColorValue(kSettingThemesAndColorsInvocationAnyHit));
    ui_->intersection_legend_->SetColor(rra::Settings::Get().GetColorValue(kSettingThemesAndColorsInvocationIntersection));
    ui_->miss_legend_->SetColor(rra::Settings::Get().GetColorValue(kSettingThemesAndColorsInvocationMiss));

    ui_->donut_->SetIndexColor(kInvocationRaygen, rra::Settings::Get().GetColorValue(kSettingThemesAndColorsInvocationRaygen));
    ui_->donut_->SetIndexColor(kInvocationClosestHit, rra::Settings::Get().GetColorValue(kSettingThemesAndColorsInvocationClosestHit));
    ui_->donut_->SetIndexColor(kInvocationAnyHit, rra::Settings::Get().GetColorValue(kSettingThemesAndColorsInvocationAnyHit));
    ui_->donut_->SetIndexColor(kInvocationIntersection, rra::Settings::Get().GetColorValue(kSettingThemesAndColorsInvocationIntersection));
    ui_->donut_->SetIndexColor(kInvocationMiss, rra::Settings::Get().GetColorValue(kSettingThemesAndColorsInvocationMiss));

    QWidget::showEvent(event);
}

void DispatchPane::paintEvent(QPaintEvent* event)
{
    const QPalette& palette = QtCommon::QtUtils::ColorTheme::Get().GetCurrentPalette();
    const QColor background_color = palette.color(QPalette::AlternateBase);
    QPainter painter(this);
    painter.fillRect(rect(), background_color);
    QWidget::paintEvent(event);
}

void DispatchPane::SetDispatchId(uint32_t dispatch_id)
{
    dispatch_id_ = dispatch_id;
}

void DispatchPane::SetDispatchDimensions(const QString& dispatch_name, int width, int height, int depth)
{
    ui_->dispatch_title_->setText(dispatch_name + "(" + QString::number(width) + ", " + QString::number(height) + ", " + QString::number(depth) + ") ");
}

void DispatchPane::SetTraversalParameters(uint64_t ray_count, uint64_t loop_iteration_count, uint64_t instance_intersection_count, float rays_per_pixel)
{
    int decimal_precision = rra::Settings::Get().GetDecimalPrecision();

    ui_->ray_count_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(ray_count)));
    ui_->loop_iteration_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(loop_iteration_count)));
    ui_->instance_intersection_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(instance_intersection_count)));
    ui_->rays_per_pixel_content_->setText(QString::number(rays_per_pixel, rra::kQtFloatFormat, decimal_precision));
    ui_->rays_per_pixel_content_->setToolTip(QString::number(rays_per_pixel, rra::kQtFloatFormat, rra::kQtTooltipFloatPrecision));
}

void DispatchPane::SetInvocationParameters(DispatchType dispatch_type,
                                           uint64_t     raygen_count,
                                           uint64_t     closest_hit_count,
                                           uint64_t     any_hit_count,
                                           uint64_t     intersection_count,
                                           uint64_t     miss_count)
{
    if (dispatch_type == kCompute || dispatch_type == kGraphics)
    {
        ConfigureForComputeOrGraphicsDispatch();
        raygen_count = 0;
    }
    else if (dispatch_type == kRayTracingPipeline)
    {
        ConfigureForRaytracingPipeline();
    }

    ui_->raygen_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(raygen_count)));
    ui_->closest_hit_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(closest_hit_count)));
    ui_->any_hit_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(any_hit_count)));
    ui_->intersection_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(intersection_count)));
    ui_->miss_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(miss_count)));

    ui_->donut_->SetIndexValue(kInvocationRaygen, raygen_count);
    ui_->donut_->SetIndexValue(kInvocationClosestHit, closest_hit_count);
    ui_->donut_->SetIndexValue(kInvocationAnyHit, any_hit_count);
    ui_->donut_->SetIndexValue(kInvocationIntersection, intersection_count);
    ui_->donut_->SetIndexValue(kInvocationMiss, miss_count);

    uint32_t total = raygen_count + closest_hit_count + any_hit_count + intersection_count + miss_count;
    ui_->donut_->SetTextLineOne(rra::string_util::LocalizedValue(total));
}

void DispatchPane::ConfigureForRaytracingPipeline()
{
    ui_->shader_invocations_label_->setText("Shader invocations");

    ui_->raygen_content_->show();
    ui_->raygen_label_->show();
    ui_->raygen_legend_->show();

    ui_->any_hit_content_->show();
    ui_->any_hit_label_->show();
    ui_->any_hit_legend_->show();

    ui_->closest_hit_label_->setText("Closest hit");
    ui_->intersection_label_->setText("Intersection");
}

void DispatchPane::ConfigureForComputeOrGraphicsDispatch()
{
    ui_->shader_invocations_label_->setText("Query results");

    ui_->raygen_content_->hide();
    ui_->raygen_label_->hide();
    ui_->raygen_legend_->hide();

    ui_->any_hit_content_->hide();
    ui_->any_hit_label_->hide();
    ui_->any_hit_legend_->hide();

    ui_->closest_hit_label_->setText("Triangle hit");
    ui_->intersection_label_->setText("Procedural hit");
}

void DispatchPane::Update()
{
    RraDispatchLoadStatus load_status = {};
    RraRayGetDispatchStatus(dispatch_id_, &load_status);

    DispatchType dispatch_type = kRayTracingPipeline;

    RraRayHistoryStats stats = {};

    RraRayGetDispatchType(dispatch_id_, &dispatch_type);
    RraRayGetDispatchStats(dispatch_id_, &stats);

    SetTraversalParameters(stats.ray_count, stats.loop_iteration_count, stats.instance_intersection_count, stats.ray_count / (float)stats.pixel_count);
    SetInvocationParameters(dispatch_type, stats.raygen_count, stats.closest_hit_count, stats.any_hit_count, stats.intersection_count, stats.miss_count);

    // Start of with the dispatch region and error label hidden.
    ui_->dispatch_region_->hide();
    ui_->error_label_->hide();
    ui_->loading_bar_->SetText("Decompressing data");

    if (load_status.data_decompressed)
    {
        // First phase is complete
        ui_->loading_bar_->SetText("Loading dispatch");
    }

    if (load_status.raw_data_parsed)
    {
        // Second phase is complete
        ui_->loading_bar_->SetText("Indexing dispatch");
    }

    // We have indexed everything, time to gather stats.
    // Show the dispatch region as the numbers count up.
    if (load_status.data_indexed)
    {
        ui_->loading_bar_->SetText("Gathering stats");
        ui_->dispatch_region_->show();
    }

    // We are done with loading. Hide away!
    if (load_status.loading_complete && !load_status.has_errors)
    {
        ui_->loading_bar_->hide();
        ui_->loading_bar_bottom_spacer_->changeSize(0, 0);
    }

    // If there are errors at any stage we should raise them to the user.
    if (load_status.has_errors)
    {
        ui_->dispatch_region_->hide();
        ui_->error_label_->show();
        ui_->loading_bar_->MarkError();
        if (load_status.incomplete_data)
        {
            ui_->error_label_->setText(
                "This dispatch has incomplete data.\nPlease increase the buffer size in Radeon Developer Panel,\nlower the screen resolution or reduce the "
                "size of the window.");
        }
        else
        {
            ui_->error_label_->setText("This dispatch could not be loaded due to malformed data.");
        }

        timer_.stop();  // We are done with updating.
    }
    else
    {
        ui_->loading_bar_->SetFillPercentage(load_status.load_percentage);
    }

    bool is_rt_pipeline = dispatch_type == kRayTracingPipeline;

    std::string trace_rays_str =
        RraApiInfoIsVulkan() ? (is_rt_pipeline ? "vkCmdTraceRaysKHR" : "vkCmdDispatch") : (is_rt_pipeline ? "DispatchRays" : "Dispatch");

    char user_marker_string_buffer[512];
    RraRayGetDispatchUserMarkerString(dispatch_id_, user_marker_string_buffer, 512);
    if (strlen(user_marker_string_buffer) > 0)
    {
        std::string user_marker_string{user_marker_string_buffer};
        size_t      delimiter_idx     = user_marker_string.find_last_of('/') + 1;
        std::string user_marker_stack = user_marker_string.substr(0, delimiter_idx);
        std::string user_marker       = user_marker_string.substr(delimiter_idx);

        // Hide stack label if stack is only 1 deep.
        if (!user_marker_stack.empty())
        {
            ui_->user_marker_stack_->setText(user_marker_stack.c_str());
            ui_->user_marker_stack_->setToolTip(user_marker_stack.c_str());
            ui_->user_marker_stack_->show();
        }
        else
        {
            ui_->user_marker_stack_->hide();
        }
        ui_->user_marker_->setText(user_marker.c_str());
        ui_->user_marker_->setToolTip(user_marker.c_str());
        ui_->user_marker_->show();
    }
    else
    {
        ui_->user_marker_stack_->hide();
        ui_->user_marker_->hide();
    }

    trace_rays_str = std::to_string(dispatch_id_) + ": " + trace_rays_str;

    uint32_t width  = 0;
    uint32_t height = 0;
    uint32_t depth  = 0;
    RraRayGetDispatchDimensions(dispatch_id_, &width, &height, &depth);

    SetDispatchDimensions(trace_rays_str.c_str(), width, height, depth);

    // Repaint some elements.
    ui_->loading_bar_->repaint();
    ui_->donut_->repaint();

    if (load_status.loading_complete && !load_status.has_errors)
    {
        ui_->dispatch_title_->setCursor(Qt::PointingHandCursor);
        connect(ui_->dispatch_title_, &ScaledPushButton::released, this, &DispatchPane::NavigateToRayHistory);

        ui_->dispatch_title_->SetLinkStyleSheet();

        timer_.stop();  // We are done with updating.
    }

    ui_->dispatch_title_->setStyleSheet(ui_->dispatch_title_->styleSheet().append("ScaledPushButton#dispatch_title_ {font: 20pt;}"));
}

void DispatchPane::NavigateToRayHistory()
{
    // Select the dispatch in the Ray history pane.
    emit rra::MessageManager::Get().DispatchSelected(dispatch_id_);

    // Switch to the Ray history pane.
    emit rra::MessageManager::Get().PaneSwitchRequested(rra::kPaneIdRayHistory);
}
