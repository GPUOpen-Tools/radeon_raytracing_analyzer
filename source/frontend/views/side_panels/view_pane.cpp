//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the View side pane.
//=============================================================================

#include "views/side_panels/view_pane.h"

#include <limits>
#include <sstream>

#include <QClipboard>
#include "constants.h"
#include "managers/message_manager.h"
#include "models/side_panels/view_model.h"
#include "io/viewer_io.h"
#include "views/widget_util.h"
#include "qt_common/utils/qt_util.h"
#include "util/string_util.h"
#include "settings/settings.h"

#include "public/heatmap.h"
#include "public/rra_asic_info.h"

static const int kTraversalCounterDefaultMinValue = 0;
static const int kTraversalCounterDefaultMaxValue = 100;
static const int kControlStyleDefaultIndex        = 0;
static const int kProjectionModeDefaultIndex      = 0;
static const int kHeatmapResolution               = 1000;

constexpr int kPerspectiveIndex  = 0;  ///< Index for perspective option in combo box.
constexpr int kOrthographicIndex = 1;  ///< Index for orthographic option in combo box.

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
    rra::widget_util::InitializeComboBox(this, ui_->content_culling_mode_, model_->GetCullingModes());
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

    ui_->content_show_hide_control_style_hotkeys_->setCursor(Qt::PointingHandCursor);
    ui_->traversal_adapt_to_view_->setCursor(Qt::PointingHandCursor);
    ui_->camera_to_origin_button_->setCursor(Qt::PointingHandCursor);

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
    connect(ui_->content_render_geometry_, &ColoredCheckbox::Clicked, this, &ViewPane::SetRenderGeometry);
    connect(ui_->content_render_bvh_, &ColoredCheckbox::Clicked, this, &ViewPane::SetRenderBVH);
    connect(ui_->content_render_instance_transform_, &ColoredCheckbox::Clicked, this, &ViewPane::SetRenderInstancePretransform);
    connect(ui_->content_wireframe_overlay_, &ColoredCheckbox::Clicked, this, &ViewPane::SetWireframeOverlay);
    connect(ui_->content_culling_mode_, &ArrowIconComboBox::SelectionChanged, this, &ViewPane::SetCullingMode);
    connect(ui_->traversal_counter_slider_, &DoubleSliderHeatmapWidget::SpanChanged, this, &ViewPane::SetTraversalCounterRange);
    connect(ui_->traversal_adapt_to_view_, SIGNAL(clicked(bool)), this, SLOT(AdaptTraversalCounterRangeToView()));
    connect(ui_->camera_to_origin_button_, SIGNAL(clicked(bool)), this, SLOT(MoveCameraToOrigin()));
    connect(ui_->traversal_continuous_update_, &ColoredCheckbox::Clicked, this, &ViewPane::ToggleTraversalCounterContinuousUpdate);

    connect(ui_->content_ray_flags_accept_first_hit_, &ColoredCheckbox::Clicked, this, &ViewPane::ToggleRayFlagsAcceptFirstHit);

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

    connect(ui_->content_field_of_view_, &QSlider::valueChanged, model_, &rra::ViewModel::SetFieldOfView);
    ui_->label_near_plane_->hide();
    ui_->content_near_plane_->hide();
    connect(ui_->content_movement_speed_, &QSlider::valueChanged, model_, &rra::ViewModel::SetMovementSpeed);

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

    // Call SetControlStyle to initialize dynamic settings panel.
    SetControlStyle(ui_->content_control_style_->CurrentRow());
    model_->UpdateControlHotkeys(ui_->camera_hotkeys_widget_);

    // Refresh the UI if any render state has changed externally.
    connect(&rra::MessageManager::Get(), &rra::MessageManager::RenderStateChanged, model_, &rra::ViewModel::Update);

    // Refresh the traversal slider if it has changed externally.
    connect(&rra::MessageManager::Get(), &rra::MessageManager::TraversalSliderChanged, [=](uint32_t min, uint32_t max) {
        ui_->traversal_counter_slider_->SetSpan(min, max);
    });

    UpdateOrientationWidgets();

    // Configure for geometry rendering as default layout.
    ConfigureForGeometryRenderingLayout();

    // Set the heatmap update callback.
    model_->SetHeatmapUpdateCallback([&](rra::renderer::HeatmapData heatmap_data) {
        auto heatmap_image = QImage(static_cast<int>(kHeatmapResolution), 1, QImage::Format::Format_RGBA8888);

        for (size_t i = 0; i < kHeatmapResolution; i++)
        {
            auto color = heatmap_data.Evaluate(i / (static_cast<float>(kHeatmapResolution - 1)));
            heatmap_image.setPixelColor(QPoint(static_cast<int>(i), 0), QColor::fromRgbF(color.r, color.g, color.b, color.a));
        }

        ui_->traversal_counter_slider_->SetHeatmap(QPixmap::fromImage(heatmap_image));
        ui_->traversal_counter_slider_->repaint();
    });
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
    UpdateBoxSortHeuristicLabel();

    ConfigureForGeometryRenderingLayout();
    ui_->content_render_bvh_->setChecked(true);
    ui_->content_render_instance_transform_->setChecked(true);

    model_->SetControlStyle(kControlStyleDefaultIndex);

    reset_camera_orientation_ = true;

    // See if the projection mode needs resetting. Do the actual reset in showEvent
    // since the UI will be set up at that point.
    int projection_mode = ui_->content_projection_mode_->CurrentRow();
    reset_projection_   = projection_mode != kProjectionModeDefaultIndex;

    SetTraversalCounterRange(kTraversalCounterDefaultMinValue, kTraversalCounterDefaultMaxValue);
}

void ViewPane::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);

    if (reset_projection_)
    {
        ui_->content_projection_mode_->SetSelectedRow(kProjectionModeDefaultIndex);
        reset_projection_ = false;
    }

    if (reset_camera_orientation_)
    {
        model_->SetInvertVertical(false);
        model_->SetInvertHorizontal(false);
        SetUpAxisAsY();
        reset_camera_orientation_ = false;
    }

    int control_style_index = model_->GetCurrentControllerIndex();
    if (control_style_index != ui_->content_control_style_->CurrentRow())
    {
        ui_->content_control_style_->SetSelectedRow(control_style_index);
        emit signal_handler.CameraParametersChanged(true);
    }
    else
    {
        emit signal_handler.CameraParametersChanged(false);
    }

    model_->SetMovementSpeedLimit(rra::Settings::Get().GetMovementSpeedLimit());
    ui_->traversal_counter_slider_->setMaximum(rra::Settings::Get().GetTraversalCounterMaximum());
    model_->Update();
    model_->UpdateControlHotkeys(ui_->camera_hotkeys_widget_);

    UpdateOrientationWidgets();
}

rra::ViewModel* ViewPane::GetModel() const
{
    return model_;
}

void ViewPane::SetRenderGeometry()
{
    model_->SetRenderGeometry(ui_->content_render_geometry_->isChecked());
    model_->Update();
}

void ViewPane::SetRenderBVH()
{
    model_->SetRenderBVH(ui_->content_render_bvh_->isChecked());
    model_->Update();
}

void ViewPane::SetRenderInstancePretransform()
{
    model_->SetRenderInstancePretransform(ui_->content_render_instance_transform_->isChecked());
    model_->Update();
}

void ViewPane::SetWireframeOverlay()
{
    model_->SetWireframeOverlay(ui_->content_wireframe_overlay_->isChecked());
}

void ViewPane::SetCullingMode()
{
    model_->SetCullingMode(ui_->content_culling_mode_->CurrentRow());
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
    emit signal_handler.CameraParametersChanged(false);
}

void ViewPane::SetArchitectureToNavi3()
{
    model_->SetArchitectureToNavi3();
    UpdateBoxSortHeuristicLabel();
    emit signal_handler.CameraParametersChanged(false);
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
    emit signal_handler.CameraParametersChanged(false);
}

void ViewPane::MoveCameraToOrigin()
{
    model_->GetCurrentController()->MoveToOrigin();
}

void ViewPane::ToggleTraversalCounterContinuousUpdate()
{
    model_->ToggleTraversalCounterContinuousUpdate([=](uint32_t min, uint32_t max) { ui_->traversal_counter_slider_->SetSpan(min, max); });
    auto should_disable_manual_functions = model_->IsTraversalCounterContinuousUpdateSet();
    ui_->traversal_adapt_to_view_->setDisabled(should_disable_manual_functions);
    ui_->traversal_counter_slider_->setDisabled(should_disable_manual_functions);
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
    for (int i = 0; i < styles.size(); ++i)
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
    emit signal_handler.CameraParametersChanged(changed);
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
            ui_->content_projection_mode_->SetSelectedRow(kPerspectiveIndex);
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
}

void ViewPane::SetProjectionMode(int index)
{
    model_->SetOrthographic(index == 1);
    emit signal_handler.CameraParametersChanged(false);
}

void ViewPane::ToggleVerticalAxisInverted()
{
    model_->ToggleInvertVertical();
    emit signal_handler.CameraParametersChanged(false);
    UpdateOrientationWidgets();
}

void ViewPane::ToggleHorizontalAxisInverted()
{
    model_->ToggleInvertHorizontal();
    emit signal_handler.CameraParametersChanged(false);
    UpdateOrientationWidgets();
}

void ViewPane::SetUpAxisAsX()
{
    model_->SetUpAxisAsX();
    emit signal_handler.CameraParametersChanged(false);
    UpdateOrientationWidgets();
}

void ViewPane::SetUpAxisAsY()
{
    model_->SetUpAxisAsY();
    emit signal_handler.CameraParametersChanged(false);
    UpdateOrientationWidgets();
}

void ViewPane::SetUpAxisAsZ()
{
    model_->SetUpAxisAsZ();
    emit signal_handler.CameraParametersChanged(false);
    UpdateOrientationWidgets();
}

void ViewPane::ConfigureForGeometryRenderingLayout()
{
    model_->SetRenderTraversal(false);
    model_->SetRenderGeometry(true);
    ui_->traversal_counter_controls_container_->hide();
    ui_->content_render_geometry_->show();
    ui_->content_rendering_mode_geometry_->setChecked(true);
    model_->Update();
    emit RenderModeChanged(true);
}

void ViewPane::ConfigureForTraversalRenderingLayout()
{
    model_->SetRenderGeometry(false);
    model_->SetRenderTraversal(true);
    ui_->content_render_geometry_->hide();
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

void ViewPane::NonProceduralWidgetsHidden(bool hidden)
{
    ui_->content_render_geometry_->setHidden(hidden);
    ui_->content_wireframe_overlay_->setHidden(hidden);
}
