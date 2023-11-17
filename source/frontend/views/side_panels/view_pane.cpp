//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the View side pane.
//=============================================================================

#include "views/side_panels/view_pane.h"

#include <limits>
#include <sstream>
#include <algorithm>

#include <QClipboard>
#include "constants.h"
#include "managers/message_manager.h"
#include "managers/pane_manager.h"
#include "models/side_panels/view_model.h"
#include "io/viewer_io.h"
#include "views/widget_util.h"
#include "qt_common/utils/qt_util.h"
#include "util/string_util.h"
#include "settings/settings.h"
#include "views/custom_widgets/slider_style.h"
#include "views/custom_widgets/rgp_histogram_widget.h"
#include "settings/settings.h"
#include "constants.h"

#include "public/heatmap.h"
#include "public/rra_asic_info.h"

static const int kTraversalCounterDefaultMinValue = 0;
static const int kTraversalCounterDefaultMaxValue = 100;

const char* kLockOpenClickableIcon = ":/Resources/assets/third_party/ionicons/lock-open-outline-clickable.svg";
const char* kLockOpenHoverIcon     = ":/Resources/assets/third_party/ionicons/lock-open-outline-hover.svg";

const char* kLockClosedClickableIcon = ":/Resources/assets/third_party/ionicons/lock-closed-outline-clickable.svg";
const char* kLockClosedHoverIcon     = ":/Resources/assets/third_party/ionicons/lock-closed-outline-hover.svg";

const char* kWandClickableIcon = ":/Resources/assets/third_party/ionicons/wand-clickable.svg";
const char* kWandHoverIcon     = ":/Resources/assets/third_party/ionicons/wand-hover.svg";

const char* kKBDMouseClickableIcon = ":/Resources/assets/kbd_mouse_clickable.svg";
const char* kKBDMouseHoverIcon     = ":/Resources/assets/kbd_mouse_hover.svg";

/// @brief The signal handler to broadcast camera changes to all instances of ViewPane objects.
ViewPaneSignalHandler ViewPane::signal_handler;

ViewPane::ViewPane(QWidget* parent)
    : QWidget(parent)
    , ui_(new Ui::ViewPane)
{
    // Initialize the UI.
    ui_->setupUi(this);

    // Create the model.
    model_ = new rra::ViewModel();

    // Initialize any check boxes.
    ui_->content_render_geometry_->Initialize(false, rra::kCheckboxEnableColor);
    ui_->content_render_bvh_->Initialize(false, rra::kCheckboxEnableColor);
    ui_->content_render_instance_transform_->Initialize(false, rra::kCheckboxEnableColor);
    ui_->content_wireframe_overlay_->Initialize(false, rra::kCheckboxEnableColor);

    // Initialize any combo boxes.
    rra::widget_util::InitializeComboBox(this, ui_->content_culling_mode_, model_->GetViewportCullingModes());
    rra::widget_util::InitializeComboBox(this, ui_->content_control_style_, model_->GetControlStyles());
    rra::widget_util::InitializeComboBox(this, ui_->content_projection_mode_, model_->GetProjectionModes());

    // Initialize radio buttons.
    ui_->content_control_style_invert_vertical_->Initialize(false, rra::kCheckboxEnableColor);
    ui_->content_control_style_invert_horizontal_->Initialize(false, rra::kCheckboxEnableColor);
    ui_->content_control_style_up_axis_x_->Initialize(false, rra::kCheckboxEnableColor);
    ui_->content_control_style_up_axis_y_->Initialize(false, rra::kCheckboxEnableColor);
    ui_->content_control_style_up_axis_z_->Initialize(false, rra::kCheckboxEnableColor);

    ui_->content_rendering_mode_geometry_->Initialize(false, rra::kCheckboxEnableColor);
    ui_->content_rendering_mode_traversal_->Initialize(false, rra::kCheckboxEnableColor);

    ui_->traversal_continuous_update_->Initialize(false, rra::kCheckboxEnableColor);

    ui_->content_architecture_navi_2_->Initialize(true, rra::kCheckboxEnableColor);
    ui_->content_architecture_navi_3_->Initialize(false, rra::kCheckboxEnableColor);
    ui_->content_ray_flags_accept_first_hit_->Initialize(false, rra::kCheckboxEnableColor);
    ui_->content_ray_flags_cull_back_facing_triangles_->Initialize(false, rra::kCheckboxEnableColor);
    ui_->content_ray_flags_cull_front_facing_triangles_->Initialize(false, rra::kCheckboxEnableColor);

    // Initialize the traversal counter slider.
    ui_->traversal_counter_slider_->setCursor(Qt::PointingHandCursor);
    ui_->traversal_counter_slider_->Init();

    // Hook up any UI controls that need updating by the model.
    model_->InitializeModel(ui_->content_render_geometry_, rra::kSidePaneViewRenderGeometry, "checked");
    model_->InitializeModel(ui_->content_render_bvh_, rra::kSidePaneViewRenderBVH, "checked");
    model_->InitializeModel(ui_->content_render_instance_transform_, rra::kSidePaneViewRenderInstanceTransforms, "checked");
    model_->InitializeModel(ui_->content_wireframe_overlay_, rra::kSidePaneViewWireframeOverlay, "checked");
    model_->InitializeModel(ui_->content_culling_mode_, rra::kSidePaneViewCullingMode, "currentItem");

    model_->InitializeModel(ui_->content_camera_rotation_row_0_col_0_, rra::kSidePaneCameraRotationRow0Col0, "text");
    model_->InitializeModel(ui_->content_camera_rotation_row_0_col_1_, rra::kSidePaneCameraRotationRow0Col1, "text");
    model_->InitializeModel(ui_->content_camera_rotation_row_0_col_2_, rra::kSidePaneCameraRotationRow0Col2, "text");
    model_->InitializeModel(ui_->content_camera_rotation_row_1_col_0_, rra::kSidePaneCameraRotationRow1Col0, "text");
    model_->InitializeModel(ui_->content_camera_rotation_row_1_col_1_, rra::kSidePaneCameraRotationRow1Col1, "text");
    model_->InitializeModel(ui_->content_camera_rotation_row_1_col_2_, rra::kSidePaneCameraRotationRow1Col2, "text");
    model_->InitializeModel(ui_->content_camera_rotation_row_2_col_0_, rra::kSidePaneCameraRotationRow2Col0, "text");
    model_->InitializeModel(ui_->content_camera_rotation_row_2_col_1_, rra::kSidePaneCameraRotationRow2Col1, "text");
    model_->InitializeModel(ui_->content_camera_rotation_row_2_col_2_, rra::kSidePaneCameraRotationRow2Col2, "text");
    model_->InitializeModel(ui_->content_camera_position_x_, rra::kSidePaneCameraPositionX, "value");
    model_->InitializeModel(ui_->content_camera_position_y_, rra::kSidePaneCameraPositionY, "value");
    model_->InitializeModel(ui_->content_camera_position_z_, rra::kSidePaneCameraPositionZ, "value");

    model_->InitializeModel(ui_->label_fov_number_, rra::kSidePaneViewFieldOfView, "text");
    model_->InitializeModel(ui_->label_movement_speed_number_, rra::kSidePaneViewMovementSpeed, "text");

    ui_->content_camera_position_x_->setMaximum(std::numeric_limits<double>::infinity());
    ui_->content_camera_position_y_->setMaximum(std::numeric_limits<double>::infinity());
    ui_->content_camera_position_z_->setMaximum(std::numeric_limits<double>::infinity());
    ui_->content_camera_position_x_->setMinimum(-std::numeric_limits<double>::infinity());
    ui_->content_camera_position_y_->setMinimum(-std::numeric_limits<double>::infinity());
    ui_->content_camera_position_z_->setMinimum(-std::numeric_limits<double>::infinity());

    ui_->content_camera_rotation_row_0_col_0_->hide();
    ui_->content_camera_rotation_row_0_col_1_->hide();
    ui_->content_camera_rotation_row_0_col_2_->hide();
    ui_->content_camera_rotation_row_1_col_0_->hide();
    ui_->content_camera_rotation_row_1_col_1_->hide();
    ui_->content_camera_rotation_row_1_col_2_->hide();
    ui_->content_camera_rotation_row_2_col_0_->hide();
    ui_->content_camera_rotation_row_2_col_1_->hide();
    ui_->content_camera_rotation_row_2_col_2_->hide();
    ui_->label_camera_rotation_->hide();

    ui_->histogram_content_->SetSelectionMode(RgpHistogramWidget::kHistogramSelectionModeRange);

    ui_->lock_camera_button_->SetNormalIcon(QIcon(kLockOpenClickableIcon));
    ui_->lock_camera_button_->SetHoverIcon(QIcon(kLockOpenHoverIcon));

    ui_->lock_camera_button_->setCursor(Qt::PointingHandCursor);
    ui_->content_show_hide_control_style_hotkeys_->setCursor(Qt::PointingHandCursor);
    ui_->traversal_adapt_to_view_->setCursor(Qt::PointingHandCursor);
    ui_->camera_to_origin_button_->setCursor(Qt::PointingHandCursor);

    ui_->camera_to_origin_button_->setCursor(Qt::PointingHandCursor);

    QIcon refresh_clickable_icon;
    refresh_clickable_icon.addFile(
        QString::fromUtf8(":/Resources/assets/third_party/ionicons/refresh-outline-clickable.svg"), QSize(), QIcon::Normal, QIcon::Off);

    QIcon refresh_hover_icon;
    refresh_hover_icon.addFile(QString::fromUtf8(":/Resources/assets/third_party/ionicons/refresh-outline-hover.svg"), QSize(), QIcon::Normal, QIcon::Off);

    ui_->camera_to_origin_button_->SetNormalIcon(refresh_clickable_icon);
    ui_->camera_to_origin_button_->SetHoverIcon(refresh_hover_icon);

    ui_->copy_camera_transform_button_->hide();
    ui_->paste_camera_transform_button_->hide();

    model_->InitializeModel(ui_->content_field_of_view_, rra::kSidePaneViewFieldOfViewSlider, "value");
    model_->InitializeModel(ui_->content_near_plane_, rra::kSidePaneViewNearPlaneSlider, "value");
    model_->InitializeModel(ui_->content_movement_speed_, rra::kSidePaneViewMovementSpeedSlider, "value");
    model_->InitializeModel(ui_->content_control_style_invert_vertical_, rra::kSidePaneViewInvertVertical, "checked");
    model_->InitializeModel(ui_->content_control_style_invert_horizontal_, rra::kSidePaneViewInvertHorizontal, "checked");

    model_->InitializeModel(ui_->content_control_style_up_axis_x_, rra::kSidePaneViewXUp, "checked");
    model_->InitializeModel(ui_->content_control_style_up_axis_y_, rra::kSidePaneViewYUp, "checked");
    model_->InitializeModel(ui_->content_control_style_up_axis_z_, rra::kSidePaneViewZUp, "checked");

    model_->InitializeModel(ui_->content_rendering_mode_geometry_, rra::kSidePaneViewGeometryMode, "checked");
    model_->InitializeModel(ui_->content_rendering_mode_traversal_, rra::kSidePaneViewTraversalMode, "checked");

    model_->InitializeModel(ui_->traversal_continuous_update_, rra::kSidePaneViewTraversalContinuousUpdate, "unchecked");

    // Set up the connections.
    connect(ui_->content_render_geometry_, &ColoredCheckbox::Clicked, [=]() { this->SetRenderGeometry(true); });
    connect(ui_->content_render_bvh_, &ColoredCheckbox::Clicked, [=]() { this->SetRenderBVH(true); });
    connect(ui_->content_render_instance_transform_, &ColoredCheckbox::Clicked, [=]() { this->SetRenderInstancePretransform(true); });
    connect(ui_->content_wireframe_overlay_, &ColoredCheckbox::Clicked, this, &ViewPane::SetWireframeOverlay);
    connect(ui_->content_culling_mode_, &ArrowIconComboBox::SelectionChanged, this, &ViewPane::SetCullingMode);
    connect(ui_->traversal_counter_slider_, &DoubleSliderHeatmapWidget::SpanChanged, this, &ViewPane::SetTraversalCounterRange);
    connect(ui_->traversal_adapt_to_view_, SIGNAL(clicked(bool)), this, SLOT(AdaptTraversalCounterRangeToView()));
    connect(ui_->camera_to_origin_button_, SIGNAL(clicked(bool)), this, SLOT(MoveCameraToOrigin()));
    connect(ui_->lock_camera_button_, SIGNAL(clicked(bool)), this, SLOT(ToggleCameraLock()));
    connect(ui_->traversal_continuous_update_, &ColoredCheckbox::Clicked, this, &ViewPane::ToggleTraversalCounterContinuousUpdate);

    connect(ui_->content_ray_flags_accept_first_hit_, &ColoredCheckbox::Clicked, this, &ViewPane::ToggleRayFlagsAcceptFirstHit);
    connect(ui_->content_ray_flags_cull_back_facing_triangles_, &ColoredCheckbox::Clicked, this, &ViewPane::ToggleRayFlagsCullBackFacingTriangles);
    connect(ui_->content_ray_flags_cull_front_facing_triangles_, &ColoredCheckbox::Clicked, this, &ViewPane::ToggleRayFlagsCullFrontFacingTriangles);

    connect(ui_->content_camera_position_x_, SIGNAL(valueChanged(double)), this, SLOT(CameraPositionChangedX(double)));
    connect(ui_->content_camera_position_y_, SIGNAL(valueChanged(double)), this, SLOT(CameraPositionChangedY(double)));
    connect(ui_->content_camera_position_z_, SIGNAL(valueChanged(double)), this, SLOT(CameraPositionChangedZ(double)));

    connect(ui_->content_control_style_, &ArrowIconComboBox::SelectionChanged, [=]() { this->SetControlStyle(ui_->content_control_style_->CurrentRow()); });
    connect(
        ui_->content_projection_mode_, &ArrowIconComboBox::SelectionChanged, [=]() { this->SetProjectionMode(ui_->content_projection_mode_->CurrentRow()); });

    connect(&signal_handler, &ViewPaneSignalHandler::CameraParametersChanged, model_, &rra::ViewModel::SetCameraControllerParameters);
    connect(&signal_handler, &ViewPaneSignalHandler::CameraHotkeysChanged, model_, &rra::ViewModel::UpdateControlHotkeys);
    ui_->camera_hotkeys_widget_->hide();
    connect(ui_->content_show_hide_control_style_hotkeys_, SIGNAL(clicked(bool)), this, SLOT(ToggleHotkeyLayout()));

    connect(ui_->content_field_of_view_, &QSlider::valueChanged, this, &ViewPane::SetFieldOfView);
    ui_->label_near_plane_->hide();
    ui_->content_near_plane_->hide();
    connect(ui_->content_movement_speed_, &QSlider::valueChanged, this, &ViewPane::SetMovementSpeed);

    connect(ui_->content_control_style_invert_vertical_, &ColoredCheckbox::Clicked, this, &ViewPane::ToggleVerticalAxisInverted);
    connect(ui_->content_control_style_invert_horizontal_, &ColoredCheckbox::Clicked, this, &ViewPane::ToggleHorizontalAxisInverted);
    connect(ui_->content_control_style_up_axis_x_, &ColoredRadioButton::Clicked, this, &ViewPane::SetUpAxisAsX);
    connect(ui_->content_control_style_up_axis_y_, &ColoredRadioButton::Clicked, this, &ViewPane::SetUpAxisAsY);
    connect(ui_->content_control_style_up_axis_z_, &ColoredRadioButton::Clicked, this, &ViewPane::SetUpAxisAsZ);

    ui_->content_architecture_label_->hide();
    ui_->content_architecture_navi_2_->hide();
    ui_->content_architecture_navi_3_->hide();

    connect(ui_->content_rendering_mode_geometry_, &ColoredRadioButton::Clicked, this, &ViewPane::ConfigureForGeometryRenderingLayout);
    connect(ui_->content_rendering_mode_traversal_, &ColoredRadioButton::Clicked, this, &ViewPane::ConfigureForTraversalRenderingLayout);

    ui_->vertical_layout_traversal_counter_controls_container_->setContentsMargins(0, 0, 0, 0);
    ui_->vertical_layout_traversal_counter_controls_container_->setSpacing(0);

    // Refresh the UI if any render state has changed externally.
    connect(&rra::MessageManager::Get(), &rra::MessageManager::RenderStateChanged, model_, &rra::ViewModel::Update);

    // Refresh the traversal slider if it has changed externally.
    connect(&rra::MessageManager::Get(), &rra::MessageManager::TraversalSliderChanged, [=](uint32_t min, uint32_t max) {
        ui_->traversal_counter_slider_->SetSpan(min, max);
    });

    // Set the heatmap update callback.
    model_->SetHeatmapUpdateCallback([&](rra::renderer::HeatmapData heatmap_data) {
        auto heatmap_image = QImage(static_cast<int>(rra::kHeatmapResolution), 1, QImage::Format::Format_RGBA8888);

        for (size_t i = 0; i < rra::kHeatmapResolution; i++)
        {
            auto color = heatmap_data.Evaluate(i / (static_cast<float>(rra::kHeatmapResolution - 1)));
            heatmap_image.setPixelColor(QPoint(static_cast<int>(i), 0), QColor::fromRgbF(color.r, color.g, color.b, color.a));
        }

        ui_->traversal_counter_slider_->SetHeatmap(QPixmap::fromImage(heatmap_image));
        ui_->traversal_counter_slider_->repaint();
    });

    ui_->content_field_of_view_->setStyle(new AbsoluteSliderPositionStyle(ui_->content_field_of_view_->style()));
    ui_->content_movement_speed_->setStyle(new AbsoluteSliderPositionStyle(ui_->content_movement_speed_->style()));

    ui_->content_show_hide_control_style_hotkeys_->SetNormalIcon(QIcon(kKBDMouseClickableIcon));
    ui_->content_show_hide_control_style_hotkeys_->SetHoverIcon(QIcon(kKBDMouseHoverIcon));
    ui_->content_show_hide_control_style_hotkeys_->setBaseSize(QSize(47, 17));

    ui_->traversal_adapt_to_view_->SetNormalIcon(QIcon(kWandClickableIcon));
    ui_->traversal_adapt_to_view_->SetHoverIcon(QIcon(kWandHoverIcon));
    ui_->traversal_adapt_to_view_->setBaseSize(QSize(23, 23));
}

ViewPane::~ViewPane()
{
    delete model_;
}

void ViewPane::OnTraceOpen()
{
    // Note: The renderer/camera is not fully initialized at this point. To set renderer/camera state
    //       on trace open, set a flag here then do the initialization in showEvent().

    // Reset the UI when a trace is loaded.

    bool is_navi_3 = false;
    RraAsicInfoIsDeviceNavi3(&is_navi_3);
    if (is_navi_3)
    {
        model_->SetArchitectureToNavi3();
    }
    else
    {
        model_->SetArchitectureToNavi2();
    }

    ui_->content_movement_speed_->setMaximum(rra::kMovementSliderMaximum);
    ui_->traversal_counter_slider_->setMinimum(0);
    ui_->traversal_counter_slider_->SetSpan(kTraversalCounterDefaultMinValue, kTraversalCounterDefaultMaxValue);
    ui_->traversal_counter_slider_->setEnabled(true);
    ui_->traversal_adapt_to_view_->setEnabled(true);
    ui_->traversal_continuous_update_->setChecked(false);
    ui_->content_ray_flags_accept_first_hit_->setChecked(false);
    ui_->content_ray_flags_cull_back_facing_triangles_->setChecked(false);
    ui_->content_ray_flags_cull_front_facing_triangles_->setChecked(false);
    UpdateBoxSortHeuristicLabel();

    ui_->content_render_bvh_->setChecked(true);
    ui_->content_render_instance_transform_->setChecked(true);

    model_->SetViewportCullingMode(rra::Settings::Get().GetCullMode());

    SetTraversalCounterRange(kTraversalCounterDefaultMinValue, kTraversalCounterDefaultMaxValue);
}

void HistogramUpdateFunction(Ui::ViewPane* ui, const std::vector<uint32_t>& hist_data, uint32_t buffer_width, uint32_t buffer_height)
{
    const uint32_t bin_count{std::min(100u, (uint32_t)hist_data.size())};
    const uint32_t bin_size{(uint32_t)(hist_data.size() / bin_count)};
    uint32_t       average{0};

    std::vector<RgpHistogramWidget::RgpHistogramData> bin_data{};
    bin_data.reserve(bin_count);

    for (uint32_t i{0}; i < bin_count; ++i)
    {
        uint32_t min_value = i * bin_size;
        uint32_t max_value = (i + 1) * bin_size;
        uint32_t count{0};

        for (uint32_t j{min_value}; j < max_value && j < hist_data.size(); ++j)
        {
            count += hist_data[j];
            average += hist_data[j] * j;
        }

        bin_data.push_back({min_value, max_value, count});
    }

    average /= buffer_width * buffer_height;

    ui->traversal_average_value_->setText(QString("Avg: ") + QString::number(average));
    ui->histogram_content_->SetData(bin_data.data(), (uint32_t)bin_data.size());
    ui->histogram_content_->SetSelectionAttributes(0, INT32_MAX);
}

void ViewPane::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);

    model_->SetHistogramUpdateFunction(
        [=](const std::vector<uint32_t>& hist_data, uint32_t buffer_width, uint32_t buffer_height) {
            HistogramUpdateFunction(ui_, hist_data, buffer_width, buffer_height);
        },
        rra::Settings::Get().GetTraversalCounterMaximum());
    model_->UpdateTraversalCounterMaximumFromSettings();

    model_->SetMovementSpeedLimit(rra::Settings::Get().GetMovementSpeedLimit());
    ui_->traversal_counter_slider_->setMaximum(rra::Settings::Get().GetTraversalCounterMaximum());

    ApplyUIStateFromSettings(parent_pane_id_);
}

void ViewPane::ApplyUIStateFromSettings(rra::RRAPaneId pane)
{
    int  rendering_mode           = rra::Settings::Get().GetRenderingMode(pane);
    bool show_instance_transforms = false;
    bool lock_camera              = false;

    // Geometry settings.
    bool show_geometry = rra::Settings::Get().GetCheckboxSetting(pane, kCheckboxSettingShowGeometry);
    bool show_bvh      = rra::Settings::Get().GetCheckboxSetting(pane, kCheckboxSettingShowAxisAlignedBVH);
    if (pane == rra::kPaneIdTlasViewer)
    {
        show_instance_transforms = rra::Settings::Get().GetCheckboxSetting(pane, kCheckboxSettingShowInstanceTransform);
    }
    else if (pane == rra::kPaneIdRayInspector)
    {
        lock_camera = rra::Settings::Get().GetCheckboxSetting(pane, kCheckboxSettingLockCamera);
    }

    bool show_wireframe = rra::Settings::Get().GetCheckboxSetting(pane, kCheckboxSettingShowWireframe);

    // Ray flags.
    bool accept_first_hit            = rra::Settings::Get().GetCheckboxSetting(pane, kCheckboxSettingAcceptFirstHit);
    bool cull_back_facing_triangles  = rra::Settings::Get().GetCheckboxSetting(pane, kCheckboxSettingCullBackFacingTriangles);
    bool cull_front_facing_triangles = rra::Settings::Get().GetCheckboxSetting(pane, kCheckboxSettingCullFrontFacingTriangles);

    // Set renderer checkbox states.
    ui_->content_render_geometry_->setChecked(show_geometry);
    SetRenderGeometry(false);
    ui_->content_render_bvh_->setChecked(show_bvh);
    SetRenderBVH(false);
    if (pane == rra::kPaneIdTlasViewer)
    {
        ui_->content_render_instance_transform_->setChecked(show_instance_transforms);
        SetRenderInstancePretransform(false);
    }
    else if (pane == rra::kPaneIdRayInspector)
    {
        if (model_->GetCameraLock() != lock_camera)
        {
            ToggleCameraLock();
        }
    }
    ui_->content_wireframe_overlay_->setChecked(show_wireframe);
    SetWireframeOverlay();

    // Set FOV.
    int fov = rra::Settings::Get().GetFieldOfView(pane);
    model_->SetFieldOfView(fov);

    ui_->content_ray_flags_accept_first_hit_->setChecked(accept_first_hit);
    ToggleRayFlagsAcceptFirstHit();
    ui_->content_ray_flags_cull_back_facing_triangles_->setChecked(cull_back_facing_triangles);
    ToggleRayFlagsCullBackFacingTriangles();
    ui_->content_ray_flags_cull_front_facing_triangles_->setChecked(cull_front_facing_triangles);
    ToggleRayFlagsCullFrontFacingTriangles();

    // Set rendering mode. Done after setting the renderer state.
    if (rendering_mode == kRenderingModeGeometry)
    {
        ConfigureForGeometryRenderingLayout();
    }
    else
    {
        ConfigureForTraversalRenderingLayout();
    }

    ui_->content_culling_mode_->SetSelectedRow(rra::Settings::Get().GetCullMode());

    model_->SetInvertVertical(rra::Settings::Get().GetInvertVertical());
    model_->SetInvertHorizontal(rra::Settings::Get().GetInvertHorizontal());

    SetTraversalCounterContinuousUpdate(rra::Settings::Get().GetContinuousUpdateState());

    switch (rra::Settings::Get().GetUpAxis())
    {
    case kUpAxisTypeX:
        SetUpAxisAsX();
        break;
    case kUpAxisTypeY:
        SetUpAxisAsY();
        break;
    case kUpAxisTypeZ:
        SetUpAxisAsZ();
        break;
    case kUpAxisTypeMax:
        break;
    }

    int style = rra::Settings::Get().GetControlStyle(pane);
    ui_->content_control_style_->SetSelectedRow(style);

    ui_->content_projection_mode_->SetSelectedRow(rra::Settings::Get().GetProjectionMode());
    ui_->traversal_continuous_update_->setChecked(rra::Settings::Get().GetContinuousUpdateState());
    model_->Update();
}

rra::ViewModel* ViewPane::GetModel() const
{
    return model_;
}

void ViewPane::SetRenderGeometry(bool update_model)
{
    bool show_geometry = ui_->content_render_geometry_->isChecked();
    model_->SetRenderGeometry(show_geometry);
    if (update_model)
    {
        model_->Update();
    }
    rra::Settings::Get().SetCheckboxSetting(parent_pane_id_, kCheckboxSettingShowGeometry, show_geometry);
}

void ViewPane::SetRenderBVH(bool update_model)
{
    bool render_bvh = ui_->content_render_bvh_->isChecked();
    model_->SetRenderBVH(render_bvh);
    if (update_model)
    {
        model_->Update();
    }
    rra::Settings::Get().SetCheckboxSetting(parent_pane_id_, kCheckboxSettingShowAxisAlignedBVH, render_bvh);
}

void ViewPane::SetRenderInstancePretransform(bool update_model)
{
    bool render_instance_transform = ui_->content_render_instance_transform_->isChecked();
    model_->SetRenderInstancePretransform(render_instance_transform);
    if (update_model)
    {
        model_->Update();
    }
    rra::Settings::Get().SetCheckboxSetting(parent_pane_id_, kCheckboxSettingShowInstanceTransform, render_instance_transform);
}

void ViewPane::SetWireframeOverlay()
{
    bool show_wireframe = ui_->content_wireframe_overlay_->isChecked();
    model_->SetWireframeOverlay(show_wireframe);
    rra::Settings::Get().SetCheckboxSetting(parent_pane_id_, kCheckboxSettingShowWireframe, show_wireframe);
}

void ViewPane::SetCullingMode()
{
    int index = ui_->content_culling_mode_->CurrentRow();
    model_->SetViewportCullingMode(index);
}

void ViewPane::SetTraversalCounterRange(int min_value, int max_value)
{
    model_->SetTraversalCounterRange(min_value, max_value);

    ui_->traversal_min_value_->setText(QString::number(min_value));
    ui_->traversal_max_value_->setText(QString::number(max_value));
}

void ViewPane::AdaptTraversalCounterRangeToView()
{
    model_->AdaptTraversalCounterRangeToView([=](uint32_t min, uint32_t max) { ui_->traversal_counter_slider_->SetSpan(min, max); });
}

void ViewPane::UpdateBoxSortHeuristicLabel()
{
    ui_->content_box_sort_heuristic_value_->setText("Box sort heuristic: " + model_->GetBoxSortHeuristicName());
}

void ViewPane::SetArchitectureToNavi2()
{
    model_->SetArchitectureToNavi2();
    UpdateBoxSortHeuristicLabel();
    emit signal_handler.CameraParametersChanged(false, parent_pane_id_);
}

void ViewPane::SetArchitectureToNavi3()
{
    model_->SetArchitectureToNavi3();
    UpdateBoxSortHeuristicLabel();
    emit signal_handler.CameraParametersChanged(false, parent_pane_id_);
}

void ViewPane::ToggleRayFlagsAcceptFirstHit()
{
    if (ui_->content_ray_flags_accept_first_hit_->isChecked())
    {
        model_->EnableRayFlagsAcceptFirstHit();
    }
    else
    {
        model_->DisableRayFlagsAcceptFirstHit();
    }
    UpdateBoxSortHeuristicLabel();
    rra::Settings::Get().SetCheckboxSetting(parent_pane_id_, kCheckboxSettingAcceptFirstHit, ui_->content_ray_flags_accept_first_hit_->isChecked());
    emit signal_handler.CameraParametersChanged(false, parent_pane_id_);
}

void ViewPane::ToggleRayFlagsCullBackFacingTriangles()
{
    if (ui_->content_ray_flags_cull_back_facing_triangles_->isChecked())
    {
        model_->EnableRayCullBackFacingTriangles();
    }
    else
    {
        model_->DisableRayCullBackFacingTriangles();
    }
    rra::Settings::Get().SetCheckboxSetting(
        parent_pane_id_, kCheckboxSettingCullBackFacingTriangles, ui_->content_ray_flags_cull_back_facing_triangles_->isChecked());
    emit signal_handler.CameraParametersChanged(false, parent_pane_id_);
}

void ViewPane::ToggleRayFlagsCullFrontFacingTriangles()
{
    if (ui_->content_ray_flags_cull_front_facing_triangles_->isChecked())
    {
        model_->EnableRayCullFrontFacingTriangles();
    }
    else
    {
        model_->DisableRayCullFrontFacingTriangles();
    }
    rra::Settings::Get().SetCheckboxSetting(
        parent_pane_id_, kCheckboxSettingCullFrontFacingTriangles, ui_->content_ray_flags_cull_front_facing_triangles_->isChecked());
    emit signal_handler.CameraParametersChanged(false, parent_pane_id_);
}

void ViewPane::MoveCameraToOrigin()
{
    model_->GetCurrentController()->MoveToOrigin();
}

void ViewPane::ToggleCameraLock()
{
    model_->SetCameraLock(!model_->GetCameraLock());
    ui_->lock_camera_button_->SetNormalIcon(QIcon(model_->GetCameraLock() ? kLockClosedClickableIcon : kLockOpenClickableIcon));
    ui_->lock_camera_button_->SetHoverIcon(QIcon(model_->GetCameraLock() ? kLockClosedHoverIcon : kLockOpenHoverIcon));
    rra::Settings::Get().SetCheckboxSetting(parent_pane_id_, kCheckboxSettingLockCamera, model_->GetCameraLock());
}

void ViewPane::ToggleTraversalCounterContinuousUpdate()
{
    SetTraversalCounterContinuousUpdate(!model_->IsTraversalCounterContinuousUpdateSet());
}

void ViewPane::SetTraversalCounterContinuousUpdate(bool continuous_update)
{
    model_->SetTraversalCounterContinuousUpdate(continuous_update, [=](uint32_t min, uint32_t max) { ui_->traversal_counter_slider_->SetSpan(min, max); });
    rra::Settings::Get().SetContinuousUpdateState(continuous_update);
    ui_->traversal_continuous_update_->setChecked(continuous_update);
    ui_->traversal_adapt_to_view_->setDisabled(continuous_update);
    ui_->traversal_counter_slider_->setDisabled(continuous_update);
}

void ViewPane::CheckPasteResult(rra::ViewerIOCameraPasteResult result)
{
    if (result == rra::ViewerIOCameraPasteResult::kFailure)
    {
        std::string error_string = "Pasted string has unrecognized format.";
        QtCommon::QtUtils::ShowMessageBox(this, QMessageBox::Ok, QMessageBox::Information, "Paste Error", QString::fromStdString(error_string));
    }
    else if (result == rra::ViewerIOCameraPasteResult::kOrthographicNotSupported)
    {
        std::string error_string =
            "Orthographic projection is not recommended for this control style.\
            Note that nothing will be rendered if arc radius is 0 while orthographic projection is enabled.";
        QtCommon::QtUtils::ShowMessageBox(this, QMessageBox::Ok, QMessageBox::Information, "Paste Error", QString::fromStdString(error_string));
    }
}

void ViewPane::CopyCameraButtonClicked()
{
    auto        controller     = model_->GetCurrentController();
    QClipboard* clipboard      = QGuiApplication::clipboard();
    QString     readableString = QString::fromStdString(controller->GetReadableString());

    clipboard->setText(readableString);
}

void ViewPane::PasteCameraButtonClicked()
{
    QClipboard* clipboard       = QGuiApplication::clipboard();
    std::string readable_string = clipboard->text().toStdString();

    std::istringstream stream(readable_string);
    std::string        line;
    std::getline(stream, line);

    auto styles = model_->GetControlStyles();
    auto split  = rra::string_util::Split(line, rra::text::kDelimiterBinary);

    if (split.size() != 2)
    {
        CheckPasteResult(rra::ViewerIOCameraPasteResult::kFailure);
        return;
    }

    auto controller_type = rra::string_util::Trim(split[1]);

    // I think it would be better to have an enum defining the indices of control styles
    // so we don't have to loop through them and string compare. Would be O(1) instead of O(n).
    int style_count = static_cast<int>(styles.size());
    for (int i = 0; i < style_count; ++i)
    {
        if (controller_type == styles[i])
        {
            ui_->content_control_style_->SetSelectedRow(i);
            auto controller = model_->GetCurrentController();
            CheckPasteResult(controller->SetStateFromReadableString(readable_string));
            return;
        }
    }

    CheckPasteResult(rra::ViewerIOCameraPasteResult::kFailure);
}

void ViewPane::CameraPositionChangedX(double x_new)
{
    auto  camera    = model_->GetCurrentController()->GetCamera();
    float x_current = camera->GetPosition().x;
    float offset    = x_new - x_current;

    camera->Translate(glm::vec3(offset, 0.0f, 0.0f));
}

void ViewPane::CameraPositionChangedY(double y_new)
{
    auto  camera    = model_->GetCurrentController()->GetCamera();
    float y_current = camera->GetPosition().y;
    float offset    = y_new - y_current;

    camera->Translate(glm::vec3(0.0f, offset, 0.0f));
}

void ViewPane::CameraPositionChangedZ(double z_new)
{
    auto  camera    = model_->GetCurrentController()->GetCamera();
    float z_current = camera->GetPosition().z;
    float offset    = z_new - z_current;

    camera->Translate(glm::vec3(0.0f, 0.0f, offset));
}

void ViewPane::UpdateOrientationWidgets()
{
    auto camera_orientation = model_->GetCameraControllerOrientation();
    ui_->content_control_style_up_axis_x_->setChecked(camera_orientation.up_axis == rra::ViewerIOUpAxis::kUpAxisX);
    ui_->content_control_style_up_axis_y_->setChecked(camera_orientation.up_axis == rra::ViewerIOUpAxis::kUpAxisY);
    ui_->content_control_style_up_axis_z_->setChecked(camera_orientation.up_axis == rra::ViewerIOUpAxis::kUpAxisZ);
    ui_->content_control_style_invert_horizontal_->setChecked(camera_orientation.flip_horizontal);
    ui_->content_control_style_invert_vertical_->setChecked(camera_orientation.flip_vertical);
}

void ViewPane::SetControlStyle(int index)
{
    bool changed = model_->SetControlStyle(index);
    emit signal_handler.CameraParametersChanged(changed, parent_pane_id_);
    emit signal_handler.CameraHotkeysChanged(ui_->camera_hotkeys_widget_);
    UpdateOrientationWidgets();
    emit ControlStyleChanged();

    // If control style does not support orthographic projection, set it to false and disable that option.
    auto controller = model_->GetCurrentController();
    if (controller)
    {
        if (controller->SupportsOrthographicProjection())
        {
            ui_->content_projection_mode_->setHidden(false);
        }
        else
        {
            ui_->content_projection_mode_->SetSelectedRow(kProjectionModePerspective);
            ui_->content_projection_mode_->setHidden(true);
            controller->GetCamera()->SetOrthographic(false);
        }

        bool hide_up_axis_ui = !controller->SupportsUpAxis();
        ui_->content_control_style_up_axis_label_->setHidden(hide_up_axis_ui);
        ui_->content_control_style_up_axis_x_->setHidden(hide_up_axis_ui);
        ui_->content_control_style_up_axis_y_->setHidden(hide_up_axis_ui);
        ui_->content_control_style_up_axis_z_->setHidden(hide_up_axis_ui);
        ui_->content_control_style_invert_vertical_->setHidden(hide_up_axis_ui);
        ui_->content_control_style_invert_horizontal_->setText(hide_up_axis_ui ? "Invert" : "Invert horizontal");

        controller->ControlStyleChanged();
    }

    model_->UpdateControlHotkeys(ui_->camera_hotkeys_widget_);

    if (index != ui_->content_control_style_->CurrentRow())
    {
        ui_->content_control_style_->SetSelectedRow(index);
        emit signal_handler.CameraParametersChanged(true, parent_pane_id_);
    }
    else
    {
        emit signal_handler.CameraParametersChanged(false, parent_pane_id_);
    }
}

void ViewPane::SetProjectionMode(int index)
{
    model_->SetOrthographic(index == 1);
    rra::Settings::Get().SetProjectionMode(static_cast<ProjectionMode>(index));
    emit signal_handler.CameraParametersChanged(false, parent_pane_id_);
}

void ViewPane::ToggleVerticalAxisInverted()
{
    model_->ToggleInvertVertical();
    emit signal_handler.CameraParametersChanged(false, parent_pane_id_);
    UpdateOrientationWidgets();
}

void ViewPane::ToggleHorizontalAxisInverted()
{
    model_->ToggleInvertHorizontal();
    emit signal_handler.CameraParametersChanged(false, parent_pane_id_);
    UpdateOrientationWidgets();
}

void ViewPane::SetUpAxisAsX()
{
    model_->SetUpAxisAsX();
    emit signal_handler.CameraParametersChanged(false, parent_pane_id_);
    UpdateOrientationWidgets();
    rra::Settings::Get().SetUpAxis(kUpAxisTypeX);
}

void ViewPane::SetUpAxisAsY()
{
    model_->SetUpAxisAsY();
    emit signal_handler.CameraParametersChanged(false, parent_pane_id_);
    UpdateOrientationWidgets();
    rra::Settings::Get().SetUpAxis(kUpAxisTypeY);
}

void ViewPane::SetUpAxisAsZ()
{
    model_->SetUpAxisAsZ();
    emit signal_handler.CameraParametersChanged(false, parent_pane_id_);
    UpdateOrientationWidgets();
    rra::Settings::Get().SetUpAxis(kUpAxisTypeZ);
}

void ViewPane::ConfigureForGeometryRenderingLayout()
{
    rra::Settings::Get().SetRenderingMode(parent_pane_id_, kRenderingModeGeometry);
    model_->SetRenderTraversal(false);
    ui_->traversal_counter_controls_container_->hide();
    ui_->content_render_geometry_->show();
    ui_->content_culling_mode_->show();
    ui_->content_rendering_mode_geometry_->setChecked(true);
    model_->Update();
    emit RenderModeChanged(true);
}

void ViewPane::ConfigureForTraversalRenderingLayout()
{
    rra::Settings::Get().SetRenderingMode(parent_pane_id_, kRenderingModeTraversal);
    model_->SetRenderTraversal(true);
    ui_->content_render_geometry_->hide();
    ui_->content_culling_mode_->hide();
    ui_->traversal_counter_controls_container_->show();
    ui_->content_rendering_mode_traversal_->setChecked(true);
    model_->Update();
    emit RenderModeChanged(false);
}

void ViewPane::ToggleHotkeyLayout()
{
    if (ui_->camera_hotkeys_widget_->isVisible())
    {
        qDeleteAll(ui_->camera_hotkeys_widget_->children());
        ui_->camera_hotkeys_widget_->setLayout(new QHBoxLayout());
        ui_->camera_hotkeys_widget_->hide();
    }
    else
    {
        ui_->camera_hotkeys_widget_->show();
        model_->UpdateControlHotkeys(ui_->camera_hotkeys_widget_);
    }
}

void ViewPane::HideTLASWidgets()
{
    ui_->content_render_instance_transform_->hide();
}

void ViewPane::HideRAYWidgets()
{
    ui_->lock_camera_button_->hide();
}

void ViewPane::NonProceduralWidgetsHidden(bool hidden)
{
    ui_->content_render_geometry_->setHidden(hidden);
    ui_->content_wireframe_overlay_->setHidden(hidden);
}

void ViewPane::SetParentPaneId(rra::RRAPaneId pane_id)
{
    parent_pane_id_ = pane_id;
    model_->SetParentPaneId(pane_id);
    ApplyUIStateFromSettings(parent_pane_id_);
}

void ViewPane::SetFieldOfView(int slider_value)
{
    float fov = model_->SetFieldOfViewFromSlider(slider_value);
    rra::Settings::Get().SetFieldOfView(parent_pane_id_, static_cast<int>(fov));
}

void ViewPane::SetMovementSpeed(int slider_value)
{
    float speed = model_->SetMovementSpeedFromSlider(slider_value);
    rra::Settings::Get().SetMovementSpeed(parent_pane_id_, static_cast<int>(speed));
}
