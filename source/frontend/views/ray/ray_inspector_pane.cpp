//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the Ray inspector pane.
//=============================================================================

#include "views/ray/ray_inspector_pane.h"

#include "qt_common/utils/qt_util.h"

#include "constants.h"
#include "managers/message_manager.h"
#include "models/ray/ray_inspector_ray_tree_item.h"
#include "models/ray/ray_inspector_ray_tree_model.h"
#include "settings/settings.h"
#include "util/string_util.h"
#include "views/widget_util.h"

RayInspectorPane::RayInspectorPane(QWidget* parent)
    : BasePane(parent)
    , ui_(new Ui::RayInspectorPane)
{
    ui_->setupUi(this);
    ui_->side_panel_container_->GetViewPane()->SetParentPaneId(rra::kPaneIdRayInspector);
    ui_->viewer_container_widget_->SetupUI(this, rra::kPaneIdRayInspector);

    rra::widget_util::ApplyStandardPaneStyle(ui_->main_scroll_area_);

    model_ = new rra::RayInspectorModel(rra::kRayInspectorRayListNumWidgets);
    model_->InitializeTreeModel(ui_->ray_tree_);

    table_delegate_ = new RayInspectorRayTreeItemDelegate(this);
    ui_->ray_tree_->setItemDelegate(table_delegate_);
    ui_->ray_tree_->setMinimumWidth(390);
    ui_->ray_tree_->setSortingEnabled(false);
    ui_->ray_tree_->SetFocusOnSelectedRayCallback([&]() { FocusOnSelectedRay(); });
    ui_->ray_tree_->SetResetSceneCallback([&]() {
        UpdateCameraController();
        if (last_camera_controller_)
        {
            last_camera_controller_->ResetCameraPosition();
        }
    });

    QList<int> splitter_sizes = {1, 10000};
    ui_->ray_splitter_->setSizes(splitter_sizes);
    ui_->ray_splitter_->setStretchFactor(0, 1);
    ui_->ray_splitter_->setStretchFactor(1, 10000);

    connect(&rra::MessageManager::Get(), &rra::MessageManager::RayCoordinateSelected, [=](uint32_t dispatch_id, uint32_t x, uint32_t y, uint32_t z) {
        this->SetRayCoordinate(dispatch_id, x, y, z);
    });

    connect(ui_->ray_tree_->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(SelectRay()));
    connect(ui_->ray_tree_, &QAbstractItemView::doubleClicked, [&]() { FocusOnSelectedRay(); });

    connect(ui_->side_panel_container_->GetViewPane(), &ViewPane::RenderModeChanged, [=](bool geometry_mode) {
        ui_->viewer_container_widget_->ShowColoringMode(geometry_mode);
    });

    // Reset the UI state. When the 'reset' button is clicked, it broadcasts a message from the message manager. Any objects interested in this
    // message can then act upon it.
    connect(&rra::MessageManager::Get(), &rra::MessageManager::ResetUIState, [=](rra::RRAPaneId pane) {
        if (pane == rra::kPaneIdRayInspector)
        {
            // Only reset UI if a ray is selected.
            if (model_->GetRay(0))
            {
                ui_->side_panel_container_->GetViewPane()->ApplyUIStateFromSettings(rra::kPaneIdRayInspector);
                ui_->viewer_container_widget_->ApplyUIStateFromSettings(rra::kPaneIdRayInspector);
            }
        }
    });

    connect(ui_->content_ray_acceleration_structure_address_, &ScaledPushButton::clicked, this, &RayInspectorPane::GotoTlasPaneFromSelectedRay);
    connect(ui_->content_ray_result_instance_index_, &ScaledPushButton::clicked, this, &RayInspectorPane::GotoTlasPaneFromSelectedRayWithInstance);

    connect(ui_->content_origin_, &ScaledPushButton::clicked, this, &RayInspectorPane::FocusOnRayOrigin);

    connect(ui_->content_ray_result_hit_distance_, &ScaledPushButton::clicked, this, &RayInspectorPane::FocusOnHitLocation);
    connect(ui_->selected_ray_focus_, &ScaledPushButton::clicked, this, &RayInspectorPane::FocusOnSelectedRay);
    ui_->selected_ray_focus_->setCursor(QCursor(Qt::PointingHandCursor));

    connect(ui_->increment_ray_, &ScaledPushButton::clicked, this, [=]() { emit rra::MessageManager::Get().RayStepSelected(1); });
    connect(ui_->decrement_ray_, &ScaledPushButton::clicked, this, [=]() { emit rra::MessageManager::Get().RayStepSelected(-1); });

    ui_->content_ray_acceleration_structure_address_->SetLinkStyleSheet();
    ui_->content_origin_->SetLinkStyleSheet();
    ui_->content_ray_result_hit_distance_->SetLinkStyleSheet();
    ui_->content_ray_result_instance_index_->SetLinkStyleSheet();

    // Save selected control style to settings.
    connect(ui_->side_panel_container_->GetViewPane(), &ViewPane::ControlStyleChanged, this, [&]() {
        if (renderer_interface_ == nullptr)
        {
            return;
        }
        rra::renderer::Camera& camera = renderer_interface_->GetCamera();

        auto camera_controller = static_cast<rra::ViewerIO*>(camera.GetCameraController());
        if (camera_controller)
        {
            rra::Settings::Get().SetControlStyle(rra::kPaneIdRayInspector, (ControlStyleType)camera_controller->GetComboBoxIndex());
        }
    });

    // Hide any widgets that are only applicable for the TLAS viewer.
    ui_->side_panel_container_->GetViewPane()->HideTLASWidgets();

    flag_table_delegate_ = new FlagTableItemDelegate();
    model_->InitializeFlagsTableModel(ui_->flags_table_);
    ui_->flags_table_->setItemDelegate(flag_table_delegate_);
    ui_->flags_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui_->flags_table_->setFocusPolicy(Qt::NoFocus);
    ui_->flags_table_->setSelectionMode(QAbstractItemView::NoSelection);

    // Set the size of the legends.
    const QSize legend_size = {20, 20};
    ui_->ray_color_legend_->setFixedSize(legend_size);
    ui_->selected_ray_color_legend_->setFixedSize(legend_size);
    ui_->accept_first_hit_ray_legend_->setFixedSize(legend_size);
    ui_->zero_mask_ray_legend_->setFixedSize(legend_size);

    UpdateColoringLegend();

    ui_->content_token_dump_->hide();

    QIcon clickable_icon;

    if (QtCommon::QtUtils::ColorTheme::Get().GetColorTheme() == ColorThemeType::kColorThemeTypeDark)
    {
        clickable_icon.addFile(
            QString::fromUtf8(":/Resources/assets/third_party/ionicons/scan-outline-clickable-dark-theme.svg"), QSize(), QIcon::Normal, QIcon::Off);
    }
    else
    {
        clickable_icon.addFile(QString::fromUtf8(":/Resources/assets/third_party/ionicons/scan-outline-clickable.svg"), QSize(), QIcon::Normal, QIcon::Off);
    }

    QIcon hover_icon;
    hover_icon.addFile(QString::fromUtf8(":/Resources/assets/third_party/ionicons/scan-outline-hover.svg"), QSize(), QIcon::Normal, QIcon::Off);

    ui_->selected_ray_focus_->SetNormalIcon(clickable_icon);
    ui_->selected_ray_focus_->SetHoverIcon(hover_icon);

    connect(&QtCommon::QtUtils::ColorTheme::Get(), &QtCommon::QtUtils::ColorTheme::ColorThemeUpdated, this, &RayInspectorPane::OnColorThemeUpdated);

    ui_->selected_ray_focus_->setBaseSize(QSize(25, 25));
}

RayInspectorPane::~RayInspectorPane()
{
    delete table_delegate_;

    // Destroy the renderer interface if it was created.
    if (renderer_interface_ != nullptr)
    {
        renderer_interface_->Shutdown();
        delete renderer_interface_;
    }

    delete model_;
    delete ui_;
}

void RayInspectorPane::SelectRay()
{
    QModelIndex index = ui_->ray_tree_->currentIndex();
    if (!index.isValid())
    {
        return;
    }

    QVariant variant_data = index.data(Qt::UserRole);
    auto     ray_index    = variant_data.toInt();

    model_->SelectRayIndex(ray_index);
    UpdateRayValues();
    if (renderer_interface_ != nullptr)
    {
        renderer_interface_->MarkAsDirty();
    }
}

void RayInspectorPane::UpdateRayValues()
{
    UpdateRayResultView();

    auto selected_index = model_->GetSelectedRayIndex();
    auto opt_ray        = model_->GetRay(selected_index);

    if (!opt_ray)
    {
        return;
    }

    auto& ray = opt_ray.value();

    int decimal_precision = rra::Settings::Get().GetDecimalPrecision();

    ui_->viewer_container_widget_->UpdateMaskWidget(ray.cull_mask);

    QString origin{"(" + QString::number(ray.origin[0], rra::kQtFloatFormat, decimal_precision) + ", " +
                   QString::number(ray.origin[1], rra::kQtFloatFormat, decimal_precision) + ", " +
                   QString::number(ray.origin[2], rra::kQtFloatFormat, decimal_precision) + ")"};

    QString origin_tooltip{"(" + QString::number(ray.origin[0], rra::kQtFloatFormat, rra::kQtTooltipFloatPrecision) + ", " +
                           QString::number(ray.origin[1], rra::kQtFloatFormat, rra::kQtTooltipFloatPrecision) + ", " +
                           QString::number(ray.origin[2], rra::kQtFloatFormat, rra::kQtTooltipFloatPrecision) + ")"};

    QString direction{"(" + QString::number(ray.direction[0], rra::kQtFloatFormat, decimal_precision) + ", " +
                      QString::number(ray.direction[1], rra::kQtFloatFormat, decimal_precision) + ", " +
                      QString::number(ray.direction[2], rra::kQtFloatFormat, decimal_precision) + ")"};

    QString direction_tooltip{"(" + QString::number(ray.direction[0], rra::kQtFloatFormat, rra::kQtTooltipFloatPrecision) + ", " +
                              QString::number(ray.direction[1], rra::kQtFloatFormat, rra::kQtTooltipFloatPrecision) + ", " +
                              QString::number(ray.direction[2], rra::kQtFloatFormat, rra::kQtTooltipFloatPrecision) + ")"};

    ui_->content_ray_acceleration_structure_address_->setText(QString("0x") + QString("%1").arg(ray.tlas_address, 0, 16));
    ui_->content_ray_acceleration_structure_address_->setCursor(QCursor(Qt::PointingHandCursor));
    ui_->content_cull_mask_->setText("0x" + QString("%1").arg(ray.cull_mask, 0, 16));
    ui_->content_sbt_record_offset_->setText(rra::string_util::LocalizedValue(ray.sbt_record_offset));
    ui_->content_sbt_record_stride_->setText(rra::string_util::LocalizedValue(ray.sbt_record_stride));
    ui_->content_miss_index_->setText(rra::string_util::LocalizedValue(ray.miss_index));
    ui_->content_tmin_->setText(QString::number(ray.t_min, rra::kQtFloatFormat, decimal_precision));
    ui_->content_tmin_->setToolTip(QString::number(ray.t_min, rra::kQtFloatFormat, rra::kQtTooltipFloatPrecision));
    ui_->content_origin_->setText(origin);
    ui_->content_origin_->setCursor(QCursor(Qt::PointingHandCursor));
    ui_->content_origin_->setToolTip(origin_tooltip);

    QString tmax_string           = QString::number(ray.t_max, rra::kQtFloatFormat, rra::kQtTooltipFloatPrecision);
    QString formatted_tmax_string = QString::number(tmax_string.toDouble()).remove("+");

    ui_->content_tmax_->setText(formatted_tmax_string);
    ui_->content_tmax_->setToolTip(tmax_string);

    ui_->content_direction_->setText(direction);
    ui_->content_direction_->setToolTip(direction_tooltip);

    ui_->selected_ray_label_->setText("Selected ray");
    ui_->ray_details_container_->show();

    model_->PopulateScene(renderer_interface_);
}

void RayInspectorPane::UpdateRayResultView()
{
    auto selected_index = model_->GetSelectedRayIndex();
    auto opt_ray        = model_->GetRay(selected_index);
    auto opt_result     = model_->GetRayResult(selected_index);

    if (!opt_ray || !opt_result)
    {
        return;
    }

    auto& ray    = opt_ray.value();
    auto& result = opt_result.value();

    RRA_UNUSED(ray);
    RRA_UNUSED(result);

    int decimal_precision = rra::Settings::Get().GetDecimalPrecision();

    bool hit = result.hit_t >= 0.0;
    if (hit)
    {
        ui_->content_ray_result_hit_distance_->setText(QString::number(result.hit_t, rra::kQtFloatFormat, decimal_precision));
        ui_->content_ray_result_hit_distance_->setCursor(QCursor(Qt::PointingHandCursor));
        ui_->content_ray_result_hit_distance_->setToolTip(QString::number(result.hit_t, rra::kQtFloatFormat, rra::kQtTooltipFloatPrecision));

        ui_->content_ray_result_instance_index_->setText(QString::number(result.instance_index));
        ui_->content_ray_result_instance_index_->setCursor(QCursor(Qt::PointingHandCursor));
        ui_->content_ray_result_instance_index_->setToolTip("View instance in TLAS viewer");

        ui_->content_ray_result_geometry_index_->setText(QString::number(result.geometry_index));
        //ui_->content_ray_result_geometry_index_->setCursor(QCursor(Qt::PointingHandCursor));
        //ui_->content_ray_result_geometry_index_->setToolTip("View geometry and primitive in BLAS viewer");

        ui_->content_ray_result_primitive_index_->setText(QString::number(result.primitive_index));
        //ui_->content_ray_result_primitive_index_->setCursor(QCursor(Qt::PointingHandCursor));
        //ui_->content_ray_result_primitive_index_->setToolTip("View geometry and primitive in BLAS viewer");

        ui_->selected_ray_result_label_->setText("Ray result");
        ui_->ray_result_container_->show();

        switch (result.any_hit_result)
        {
        case kAnyHitResultNoAnyHit:
            ui_->label_ray_result_any_hit_status_->hide();
            ui_->content_ray_result_any_hit_status_->hide();
            break;
        case kAnyHitResultAccept:
            ui_->label_ray_result_any_hit_status_->show();
            ui_->content_ray_result_any_hit_status_->show();
            ui_->content_ray_result_any_hit_status_->setText("Accept");
            break;
        case kAnyHitResultReject:
            ui_->label_ray_result_any_hit_status_->show();
            ui_->content_ray_result_any_hit_status_->show();
            ui_->content_ray_result_any_hit_status_->setText("Reject");
            break;
        }
    }
    else
    {
        ui_->selected_ray_result_label_->setText("Ray result (miss)");

        ui_->content_ray_result_hit_distance_->setText("-");
        ui_->content_ray_result_hit_distance_->setCursor(QCursor(Qt::ArrowCursor));
        ui_->content_ray_result_hit_distance_->setToolTip("-");

        ui_->content_ray_result_instance_index_->setText("-");
        ui_->content_ray_result_instance_index_->setCursor(QCursor(Qt::ArrowCursor));
        ui_->content_ray_result_instance_index_->setToolTip("-");

        ui_->content_ray_result_geometry_index_->setText("-");
        ui_->content_ray_result_primitive_index_->setText("-");
    }
}

void RayInspectorPane::DeselectRay()
{
    ui_->selected_ray_label_->setText("No ray selected");
    ui_->ray_details_container_->hide();
    ui_->ray_tree_->clearSelection();
}

void RayInspectorPane::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}

void RayInspectorPane::showEvent(QShowEvent* event)
{
    UpdateColoringLegend();

    auto opt_ray = model_->GetRay(0);
    if (opt_ray)
    {
        // There are rays, so show ray inspector pane.
        ui_->ray_valid_switch_->setCurrentIndex(0);

        if (renderer_widget_ != nullptr)
        {
            if (renderer_interface_)
            {
                renderer_interface_->MarkAsDirty();
            }

            // The viewer pane is being shown. Get the widget to start drawing new frames.
            renderer_widget_->ContinueFrames();
            renderer_widget_->UpdateSwapchainSize();
        }

        ui_->side_panel_container_->GetViewPane()->SetControlStyle(rra::Settings::Get().GetControlStyle(rra::kPaneIdRayInspector));
    }
    else
    {
        // No ray(s) selected, show an empty page.
        ui_->ray_valid_switch_->setCurrentIndex(1);
    }

    rra::RayInspectorRayTreeProxyModel* proxy_model = model_->GetProxyModel();
    if (proxy_model)
    {
        proxy_model->invalidate();
        proxy_model->sort(0);
    }
    ui_->ray_tree_->expandAll();
    ui_->ray_tree_->repaint();

    BasePane::showEvent(event);
}

void RayInspectorPane::OnTraceOpen()
{
    model_->ResetModelValues();

    InitializeRendererWidget(ui_->tlas_scene_, ui_->side_panel_container_);

    ui_->viewer_container_widget_->ShowColoringMode(true);

    model_->UpdateTlasMap();

    model_->ClearKey();

    // Set the heatmap to grayscale.
    ui_->viewer_container_widget_->SetHeatmap(HeatmapColorType::kHeatmapColorTypeGrayscale);

    UpdateCameraController();
    if (last_camera_controller_)
    {
        last_camera_controller_->ResetCameraPosition();
    }
}

void RayInspectorPane::OnTraceClose()
{
    // Stop the renderer widget from drawing new frames.
    HaltRendererWidget();

    if (renderer_interface_)
    {
        // The trace has been closed so release the renderer memory.
        renderer_interface_->Shutdown();
        delete renderer_interface_;
        renderer_interface_ = nullptr;

        renderer_widget_->SetRendererInterface(renderer_interface_);
    }
}

void RayInspectorPane::hideEvent(QHideEvent* event)
{
    if (renderer_widget_ != nullptr)
    {
        // The viewer pane is being hidden, so stop the renderer from drawing new frames.
        renderer_widget_->PauseFrames();
    }

    QWidget::hideEvent(event);
}

void RayInspectorPane::SetRayCoordinate(uint32_t dispatch_id, uint32_t x, uint32_t y, uint32_t z)
{
    QString s = "Rays at dispatch ";
    s += QString::number(dispatch_id) + "\nCoordinates " + QString::number(x) + ", " + QString::number(y) + ", " + QString::number(z) + "";
    ui_->dispatch_indices_label_->setText(s);
    model_->SetKey({dispatch_id, x, y, z});
    UpdateRayValues();

    auto opt_ray = model_->GetRay(0);
    if (opt_ray)
    {
        // There are rays, so show ray inspector pane.
        ui_->ray_valid_switch_->setCurrentIndex(0);
    }
    else
    {
        // No ray(s) selected, show an empty page.
        ui_->ray_valid_switch_->setCurrentIndex(1);
    }

    if (!ui_->side_panel_container_->GetViewPane()->GetModel()->GetCameraLock())
    {
        FocusOnSelectedRay();
    }
    else
    {
        model_->BurstUpdateCamera();
    }

    if (renderer_interface_)
    {
        renderer_interface_->MarkAsDirty();
    }

    ui_->ray_tree_->expandAll();
}

void RayInspectorPane::InitializeRendererWidget(RendererWidget* renderer_widget, SidePaneContainer* side_panel_container)
{
    RRA_ASSERT(renderer_widget != nullptr);
    if (renderer_widget != nullptr)
    {
        // Create a renderer instance that the widget will use to draw the scene geometry.
        rra::renderer::RendererAdapterMap renderer_adapter_map;
        RRA_ASSERT(renderer_interface_ == nullptr);
        renderer_interface_ = rra::renderer::RendererFactory::CreateRenderer(renderer_adapter_map);

        RRA_ASSERT(renderer_interface_ != nullptr);
        if (renderer_interface_ != nullptr)
        {
            // Provide the renderer adapter map to the side panel container.
            side_panel_container->SetRendererAdapters(renderer_adapter_map);

            // Provide the renderer adapter map to the viewer container.
            ui_->viewer_container_widget_->SetRendererAdapters(renderer_adapter_map, rra::renderer::BvhTypeFlags::TopLevel);
            ui_->viewer_container_widget_->DisableMaskWidgetInteraction();

            // Provide the renderer adapter map to the model.
            model_->SetRendererAdapters(renderer_adapter_map);

            // Keep track of the widget being initialized, because it will need to be shut down later.
            renderer_widget_ = renderer_widget;

            // Attach the renderer to the widget.
            renderer_widget->SetRendererInterface(renderer_interface_);

            // The scene renderer widget will attempt to initialize itself the first time it is shown in the UI.
            // When initialization is complete, this signal will indicate that the widget is ready to draw the scene.
            connect(renderer_widget, &RendererWidget::DeviceInitialized, this, [renderer_widget] { renderer_widget->Run(); });

            // Connect the renderer widget mouse and keyboard handler slots.
            connect(renderer_widget, &RendererWidget::MouseMoved, this, &RayInspectorPane::MouseMoved);
            connect(renderer_widget, &RendererWidget::MousePressed, this, &RayInspectorPane::MousePressed);
            connect(renderer_widget, &RendererWidget::MouseDoubleClicked, this, &RayInspectorPane::MouseDoubleClicked);
            connect(renderer_widget, &RendererWidget::MouseReleased, this, &RayInspectorPane::MouseReleased);
            connect(renderer_widget, &RendererWidget::MouseWheelMoved, this, &RayInspectorPane::MouseWheelMoved);
            connect(renderer_widget, &RendererWidget::KeyPressed, this, &RayInspectorPane::KeyPressed);
            connect(renderer_widget, &RendererWidget::KeyReleased, this, &RayInspectorPane::KeyReleased);
            connect(renderer_widget, &RendererWidget::FocusOut, this, &RayInspectorPane::FocusOut);
            connect(renderer_widget, &RendererWidget::FocusIn, this, &RayInspectorPane::FocusIn);
        }
        else
        {
            renderer_widget->SetRendererInterface(nullptr);
        }
    }
}

void RayInspectorPane::HaltRendererWidget()
{
    if (renderer_widget_ != nullptr)
    {
        renderer_widget_->PauseFrames();

        // disonnect the renderer widget mouse and keyboard handler slots.
        disconnect(renderer_widget_, &RendererWidget::MouseMoved, this, &RayInspectorPane::MouseMoved);
        disconnect(renderer_widget_, &RendererWidget::MousePressed, this, &RayInspectorPane::MousePressed);
        disconnect(renderer_widget_, &RendererWidget::MouseDoubleClicked, this, &RayInspectorPane::MouseDoubleClicked);
        disconnect(renderer_widget_, &RendererWidget::MouseReleased, this, &RayInspectorPane::MouseReleased);
        disconnect(renderer_widget_, &RendererWidget::MouseWheelMoved, this, &RayInspectorPane::MouseWheelMoved);
        disconnect(renderer_widget_, &RendererWidget::KeyPressed, this, &RayInspectorPane::KeyPressed);
        disconnect(renderer_widget_, &RendererWidget::KeyReleased, this, &RayInspectorPane::KeyReleased);
        disconnect(renderer_widget_, &RendererWidget::FocusOut, this, &RayInspectorPane::FocusOut);
    }
}

void RayInspectorPane::FocusOnSelectedRay()
{
    UpdateCameraController();
    model_->BurstResetCamera();
    if (renderer_interface_)
    {
        renderer_interface_->MarkAsDirty();
    }
}

void RayInspectorPane::UpdateCameraController()
{
    if (renderer_interface_ == nullptr)
    {
        return;
    }
    rra::renderer::Camera& camera = renderer_interface_->GetCamera();

    // Can assume the controller is going to be a ViewerIO.
    auto camera_controller = static_cast<rra::ViewerIO*>(camera.GetCameraController());

    // If setting UI on trace load, row count will be 0, so no need to set camera controller.
    if (camera_controller)
    {
        camera_controller->SetViewerCallbacks(model_->GetViewerCallbacks());
    }

    if (camera_controller != last_camera_controller_)
    {
        last_camera_controller_ = camera_controller;
    }
}

static QColor InspectorGetQColorFromGLM(glm::vec4 color)
{
    QColor q_color;
    q_color.setRed(color.r * 255.0f);
    q_color.setGreen(color.g * 255.0f);
    q_color.setBlue(color.b * 255.0f);
    q_color.setAlpha(color.a * 255.0f);
    return q_color;
}

void RayInspectorPane::UpdateColoringLegend()
{
    auto& node_colors = rra::GetSceneNodeColors();

    ui_->ray_color_legend_->SetColor(InspectorGetQColorFromGLM(node_colors.ray_color));
    ui_->selected_ray_color_legend_->SetColor(InspectorGetQColorFromGLM(node_colors.selected_ray_color));
    ui_->accept_first_hit_ray_legend_->SetColor(InspectorGetQColorFromGLM(node_colors.shadow_ray_color));
    ui_->zero_mask_ray_legend_->SetColor(InspectorGetQColorFromGLM(node_colors.zero_mask_ray_color));
}

void RayInspectorPane::MousePressed(QMouseEvent* mouse_event)
{
    UpdateCameraController();
    rra::renderer::Camera& camera = renderer_interface_->GetCamera();

    // Can assume the controller is going to be a ViewerIO.
    auto camera_controller = static_cast<rra::ViewerIO*>(camera.GetCameraController());

    if (camera_controller)
    {
        camera_controller->MousePressed(mouse_event);
    }

    mouse_start_dragging_pos_ = mouse_event->pos();
}

void RayInspectorPane::MouseDoubleClicked(QMouseEvent* mouse_event)
{
    UpdateCameraController();
    rra::renderer::Camera& camera = renderer_interface_->GetCamera();

    // Can assume the controller is going to be a ViewerIO.
    auto camera_controller = static_cast<rra::ViewerIO*>(camera.GetCameraController());

    if (camera_controller)
    {
        glm::vec2 window_size = {renderer_widget_->width(), renderer_widget_->height()};
        glm::vec2 hit_coords  = {mouse_event->pos().x(), mouse_event->pos().y()};
        auto      gizmo_hit   = rra::renderer::CheckOrientationGizmoHit(camera.GetRotationMatrix(), camera.GetAspectRatio(), hit_coords / window_size);

        // Only cast ray if gizmo was not clicked on.
        bool cast_ray = true;

        if (gizmo_hit == rra::renderer::OrientationGizmoHitType::kBackground)
        {
            cast_ray = false;
        }
        else if (gizmo_hit != rra::renderer::OrientationGizmoHitType::kNone)
        {
            // Rotate view if gizmo label is clicked.
            glm::vec3 camera_forward = GetForwardFromGizmoHit(gizmo_hit);
            camera_controller->SetRotationFromForward(camera_forward);
            cast_ray = false;
        }

        rra::SceneCollectionModelClosestHit closest_hit =
            camera_controller->MouseDoubleClicked(mouse_event, {renderer_widget_->width(), renderer_widget_->height()}, &camera, cast_ray);
        if (closest_hit.distance > 0.0)
        {
            //SelectLeafNode(closest_hit.blas_index, true);
        }
    }
}

/// @brief Simple function to find the closest distance on rays.
/// @param origin_a Origin of ray a
/// @param dir_a    Direction of ray a
/// @param origin_b Origin of ray b
/// @param dir_b    Direction of ray b
/// @return distances from the origins of each rays.
static std::pair<float, float> FindClosestDistanceOnRays(glm::vec3 origin_a, glm::vec3 dir_a, glm::vec3 origin_b, glm::vec3 dir_b)
{
    dir_a = glm::normalize(dir_a);
    dir_b = glm::normalize(dir_b);

    auto ob_diff_oa  = origin_b - origin_a;
    auto dot_da_db   = glm::dot(dir_a, dir_b);
    auto denominator = -(1.0f - (dot_da_db * dot_da_db));

    if (denominator == 0.0f)
    {
        return {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
    }

    float closest_distance_on_a = (glm::dot(ob_diff_oa, dir_b) * dot_da_db - glm::dot(ob_diff_oa, dir_a)) / denominator;
    float closest_distance_on_b = (glm::dot(-ob_diff_oa, dir_a) * dot_da_db - glm::dot(-ob_diff_oa, dir_b)) / denominator;

    return {closest_distance_on_a, closest_distance_on_b};
}

std::optional<uint32_t> RayInspectorPane::CheckRayClick(rra::renderer::Camera& camera, glm::vec2 hit_coords)
{
    // Map coords to screen space.
    hit_coords *= 2.0f;
    hit_coords -= 1.0f;
    hit_coords.y = -hit_coords.y;

    auto camera_ray = camera.CastRay(hit_coords);
    auto ray_count  = model_->GetRayCount();

    const float distance_threshold = 0.02f;

    float                   closest_ray_distance = distance_threshold;
    std::optional<uint32_t> closest_ray_index    = std::nullopt;

    // Get the currently selected tlas to discriminate for invisible rays.
    uint64_t current_tlas               = 0;
    auto     opt_currently_selected_ray = model_->GetRay(model_->GetSelectedRayIndex());
    if (opt_currently_selected_ray)
    {
        current_tlas = opt_currently_selected_ray.value().tlas_address;
    }

    for (uint32_t i = 0; i < ray_count; i++)
    {
        auto opt_ray    = model_->GetRay(i);
        auto opt_result = model_->GetRayResult(i);

        if (!opt_ray || !opt_result)
        {
            continue;
        }

        auto& ray    = opt_ray.value();
        auto& result = opt_result.value();

        auto ray_origin    = glm::vec3(ray.origin[0], ray.origin[1], ray.origin[2]);
        auto ray_direction = glm::vec3(ray.direction[0], ray.direction[1], ray.direction[2]);

        auto closest_distance_on_ray = FindClosestDistanceOnRays(camera_ray.origin, camera_ray.direction, ray_origin, ray_direction).second;

        glm::vec3 closest_point = ray_origin + glm::normalize(ray_direction) * closest_distance_on_ray;
        auto      mapped_point  = camera.GetViewProjection() * glm::vec4(closest_point, 1.0);
        mapped_point /= mapped_point.w;

        glm::vec2 screen_space_coords = glm::vec2(mapped_point.x, mapped_point.y);

        auto screen_space_distance     = glm::distance(screen_space_coords, hit_coords);
        bool behind_hit_t              = result.hit_t >= 0.0 ? result.hit_t * glm::length(ray_direction) > closest_distance_on_ray : true;
        bool within_distance_threshold = screen_space_distance < distance_threshold;
        bool behind_t_max              = (ray.t_max * glm::length(ray_direction)) > closest_distance_on_ray;
        bool distance_is_valid         = closest_distance_on_ray > 0.0;
        bool same_tlas                 = ray.tlas_address == current_tlas;

        if (same_tlas && distance_is_valid && within_distance_threshold && behind_t_max && behind_hit_t)
        {
            if (screen_space_distance < closest_ray_distance)
            {
                closest_ray_distance = screen_space_distance;
                closest_ray_index    = i;
            }
        }
    }

    return closest_ray_index;
}

// Helps iterate over tree model.
static void Iterate(const QModelIndex& index, const QAbstractItemModel* model, const std::function<void(const QModelIndex&)>& step_function)
{
    if (index.isValid())
    {
        step_function(index);
    }
    if ((index.flags() & Qt::ItemNeverHasChildren) || !model->hasChildren(index))
    {
        return;
    }
    auto rows = model->rowCount(index);
    auto cols = model->columnCount(index);
    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            Iterate(model->index(i, j, index), model, step_function);
        }
    }
}

void RayInspectorPane::MouseReleased(QMouseEvent* mouse_event)
{
    UpdateCameraController();
    rra::renderer::Camera& camera = renderer_interface_->GetCamera();

    // Can assume the controller is going to be a ViewerIO.
    auto camera_controller = static_cast<rra::ViewerIO*>(camera.GetCameraController());

    if (camera_controller)
    {
        glm::vec2 window_size = {renderer_widget_->width(), renderer_widget_->height()};
        glm::vec2 hit_coords  = {mouse_event->pos().x(), mouse_event->pos().y()};
        auto      gizmo_hit   = rra::renderer::CheckOrientationGizmoHit(camera.GetRotationMatrix(), camera.GetAspectRatio(), hit_coords / window_size);

        // Only cast ray if gizmo was not clicked on.
        bool cast_ray = true;

        if (gizmo_hit == rra::renderer::OrientationGizmoHitType::kBackground)
        {
            cast_ray = false;
        }
        else if (gizmo_hit != rra::renderer::OrientationGizmoHitType::kNone)
        {
            // Rotate view if gizmo label is clicked.
            glm::vec3 camera_forward = GetForwardFromGizmoHit(gizmo_hit);
            camera_controller->SetRotationFromForward(camera_forward);
            cast_ray = false;
        }

        camera_controller->MouseReleased(mouse_event, {renderer_widget_->width(), renderer_widget_->height()}, &camera, cast_ray);

        if (mouse_event->button() == Qt::LeftButton)
        {
            auto opt_ray_index = CheckRayClick(camera, hit_coords / window_size);
            if (opt_ray_index)
            {
                auto ray_index = opt_ray_index.value();
                model_->SelectRayIndex(ray_index);

                // Search for model index and set to tree.
                Iterate(ui_->ray_tree_->rootIndex(), ui_->ray_tree_->model(), [&](const QModelIndex& model_index) {
                    if (model_index.data(Qt::UserRole).toInt() == int(ray_index))
                    {
                        ui_->ray_tree_->setCurrentIndex(model_index);
                    }
                });
            }
        }
    }

    QApplication::restoreOverrideCursor();
    cursor_overridden_ = false;
}

bool RayInspectorPane::WrapMouseMovement(QPoint pos)
{
    glm::ivec2 window_size = {renderer_widget_->width(), renderer_widget_->height()};

    QPoint global_pos          = renderer_widget_->mapToGlobal(pos);
    QPoint global_top_left     = renderer_widget_->mapToGlobal(QPoint(0, 0));
    QPoint global_bottom_right = renderer_widget_->mapToGlobal(QPoint(window_size.x, window_size.y));

    bool wrapped = false;

    if (pos.x() > window_size.x)
    {
        QCursor::setPos(global_top_left.x(), global_pos.y());
        wrapped = true;
    }

    if (pos.x() < 0)
    {
        QCursor::setPos(global_bottom_right.x(), global_pos.y());
        wrapped = true;
    }

    if (pos.y() > window_size.y)
    {
        QCursor::setPos(global_pos.x(), global_top_left.y());
        wrapped = true;
    }

    if (pos.y() < 0)
    {
        QCursor::setPos(global_pos.x(), global_bottom_right.y());
        wrapped = true;
    }

    return wrapped;
}

/// @brief Check whether p1 and p2 are at least dist distance apart.
///
/// @param p1 First point.
/// @param p2 Second point.
/// @param dist Distance to compare against.
/// @returns True if the distance is greater than dist.
static bool DistanceGreaterThanRayInspector(const QPoint& p1, const QPoint& p2, float dist)
{
    int diff_x   = p2.x() - p1.x();
    int diff_y   = p2.y() - p1.y();
    int sqr_dist = diff_x * diff_x + diff_y * diff_y;
    return (float)sqr_dist > dist * dist;
}

void RayInspectorPane::MouseMoved(QMouseEvent* mouse_event)
{
    UpdateCameraController();
    rra::renderer::Camera& camera = renderer_interface_->GetCamera();

    // Can assume the controller is going to be a ViewerIO.
    auto camera_controller = static_cast<rra::ViewerIO*>(camera.GetCameraController());

    if (camera_controller)
    {
        glm::vec2 window_size = {renderer_widget_->width(), renderer_widget_->height()};
        glm::vec2 hit_coords  = {mouse_event->pos().x(), mouse_event->pos().y()};
        auto      gizmo_hit   = rra::renderer::CheckOrientationGizmoHit(camera.GetRotationMatrix(), camera.GetAspectRatio(), hit_coords / window_size);

        rra::renderer::SetOrientationGizmoSelected(gizmo_hit);

        if (renderer_interface_ && gizmo_hit != last_gizmo_hit_)
        {
            renderer_interface_->MarkAsDirty();
            last_gizmo_hit_ = gizmo_hit;
        }

        // The mouse event's position can only exit the range of [0, window_size] if any mouse buttons are held down.
        // So we do not need to explicity check that.
        if (WrapMouseMovement(mouse_event->pos()))
        {
            camera_controller->InvalidateLastMousePosition();
        }

        bool mouse_button_pressed{mouse_event->buttons().testFlag(Qt::MouseButton::LeftButton) ||
                                  mouse_event->buttons().testFlag(Qt::MouseButton::RightButton) ||
                                  mouse_event->buttons().testFlag(Qt::MouseButton::MiddleButton)};

        if (!cursor_overridden_ && mouse_button_pressed &&
            DistanceGreaterThanRayInspector(mouse_start_dragging_pos_, mouse_event->pos(), camera_controller->GetMouseMoveDelta()))
        {
            QGuiApplication::setOverrideCursor(hand_cursor_);
            cursor_overridden_ = true;
        }

        camera_controller->MouseMoved(QCursor::pos());
    }
}

void RayInspectorPane::MouseWheelMoved(QWheelEvent* wheel_event)
{
    UpdateCameraController();

    rra::renderer::Camera& camera = renderer_interface_->GetCamera();

    // Can assume the controller is going to be a ViewerIO.
    auto camera_controller = static_cast<rra::ViewerIO*>(camera.GetCameraController());

    if (camera_controller)
    {
        camera_controller->MouseWheelMoved(wheel_event);
    }
}

void RayInspectorPane::KeyPressed(QKeyEvent* key_event)
{
    UpdateCameraController();
    rra::renderer::Camera& camera = renderer_interface_->GetCamera();

    // Can assume the controller is going to be a ViewerIO.
    auto camera_controller = static_cast<rra::ViewerIO*>(camera.GetCameraController());

    Qt::KeyboardModifiers modifiers = key_event->modifiers();
    if (modifiers & Qt::CTRL)
    {
        ctrl_key_down_ = true;
    }

    int key = key_event->key();
    switch (key)
    {
    case Qt::Key_C:
        model_->AdaptTraversalCounterRangeToView();
        break;

    case Qt::Key_V:
        model_->ToggleInstanceTransformWireframe();
        break;

    case Qt::Key_B:
        model_->ToggleBVHWireframe();
        break;

    case Qt::Key_N:
        if (ctrl_key_down_)
        {
            emit rra::MessageManager::Get().RayStepSelected(1);
        }
        else
        {
            model_->ToggleRenderGeometry();
        }
        break;

    case Qt::Key_P:
        if (ctrl_key_down_)
        {
            emit rra::MessageManager::Get().RayStepSelected(-1);
        }
        break;

    case Qt::Key_M:
        model_->ToggleMeshWireframe();
        break;

    case Qt::Key_F:
        // Prevent focus through controller.
        break;

    default:
        // We don't care about whatever key was pressed.
        if (camera_controller)
        {
            camera_controller->KeyPressed(static_cast<Qt::Key>(key));
        }
        break;
    }
}

void RayInspectorPane::KeyReleased(QKeyEvent* key_event)
{
    UpdateCameraController();
    if (last_camera_controller_)
    {
        // Invalidate F key to override focus behavior.
        last_camera_controller_->KeyReleased(Qt::Key_F);
    }

    auto viewer_callbacks = model_->GetViewerCallbacks();

    Qt::KeyboardModifiers modifiers = key_event->modifiers();
    if (modifiers & Qt::CTRL)
    {
        ctrl_key_down_ = false;
    }

    int key = key_event->key();
    switch (key)
    {
    case Qt::Key_F:
        FocusOnSelectedRay();
        break;
    default:
        // We don't care about whatever key was pressed.
        if (last_camera_controller_)
        {
            last_camera_controller_->KeyReleased(static_cast<Qt::Key>(key));
        }
        break;
    }
}

void RayInspectorPane::FocusOut()
{
    UpdateCameraController();
    if (renderer_interface_)
    {
        rra::renderer::Camera& camera = renderer_interface_->GetCamera();

        // Can assume the controller is going to be a ViewerIO.
        auto camera_controller = static_cast<rra::ViewerIO*>(camera.GetCameraController());

        if (camera_controller)
        {
            camera_controller->Reset();
        }
    }
}

void RayInspectorPane::FocusIn()
{
    UpdateCameraController();
    if (renderer_interface_)
    {
        rra::renderer::Camera& camera = renderer_interface_->GetCamera();

        // Can assume the controller is going to be a ViewerIO.
        auto camera_controller = static_cast<rra::ViewerIO*>(camera.GetCameraController());

        if (camera_controller)
        {
            camera_controller->Reset();
        }
    }
}

void RayInspectorPane::GotoTlasPaneFromSelectedRay()
{
    auto opt_tlas_index = model_->GetSelectedTlasIndex();
    if (!opt_tlas_index)
    {
        return;
    }

    emit rra::MessageManager::Get().TlasSelected(opt_tlas_index.value());
    emit rra::MessageManager::Get().PaneSwitchRequested(rra::kPaneIdTlasViewer);

    UpdateCameraController();

    if (!last_camera_controller_)
    {
        return;
    }

    auto camera = last_camera_controller_->GetCamera();
    if (!camera)
    {
        return;
    }

    emit rra::MessageManager::Get().TlasAssumeCamera(
        camera -> GetPosition(), camera->GetForward(), camera->GetUp(), camera->GetFieldOfView(), camera->GetMovementSpeed());
}

void RayInspectorPane::GotoTlasPaneFromSelectedRayWithInstance()
{
    auto opt_ray_result = model_->GetRayResult(model_->GetSelectedRayIndex());
    if (!opt_ray_result)
    {
        return;
    }

    auto opt_tlas_index = model_->GetSelectedTlasIndex();
    if (!opt_tlas_index)
    {
        return;
    }

    auto& ray_result = opt_ray_result.value();
    if (ray_result.hit_t < 0)
    {
        return;
    }

    emit rra::MessageManager::Get().TlasSelected(opt_tlas_index.value());
    emit rra::MessageManager::Get().PaneSwitchRequested(rra::kPaneIdTlasViewer);

    emit rra::MessageManager::Get().InspectorInstanceSelected(ray_result.instance_index);

    UpdateCameraController();
    if (!last_camera_controller_)
    {
        return;
    }

    auto camera = last_camera_controller_->GetCamera();
    if (!camera)
    {
        return;
    }

    emit rra::MessageManager::Get().TlasAssumeCamera(
        camera -> GetPosition(), camera->GetForward(), camera->GetUp(), camera->GetFieldOfView(), camera->GetMovementSpeed());
}

void RayInspectorPane::FocusOnHitLocation()
{
    auto selected_ray_index = model_->GetSelectedRayIndex();
    auto opt_ray            = model_->GetRay(selected_ray_index);
    auto opt_ray_result     = model_->GetRayResult(selected_ray_index);

    if (!opt_ray || !opt_ray_result)
    {
        return;
    }

    auto& ray        = opt_ray.value();
    auto& ray_result = opt_ray_result.value();

    if (ray_result.hit_t < 0.0)
    {
        return;
    }

    UpdateCameraController();
    if (!last_camera_controller_)
    {
        return;
    }

    auto camera = last_camera_controller_->GetCamera();
    if (!camera)
    {
        return;
    }

    auto camera_pos = camera->GetPosition();
    auto hit_location =
        glm::vec3(ray.origin[0], ray.origin[1], ray.origin[2]) + glm::vec3(ray.direction[0], ray.direction[1], ray.direction[2]) * ray_result.hit_t;

    last_camera_controller_->FitCameraParams(camera_pos, glm::normalize(hit_location - camera_pos), camera->GetUp());
}

void RayInspectorPane::FocusOnRayOrigin()
{
    auto selected_ray_index = model_->GetSelectedRayIndex();
    auto opt_ray            = model_->GetRay(selected_ray_index);

    if (!opt_ray)
    {
        return;
    }

    auto& ray = opt_ray.value();

    UpdateCameraController();
    if (!last_camera_controller_)
    {
        return;
    }

    auto camera = last_camera_controller_->GetCamera();
    if (!camera)
    {
        return;
    }

    auto camera_pos = camera->GetPosition();
    auto origin     = glm::vec3(ray.origin[0], ray.origin[1], ray.origin[2]);

    last_camera_controller_->FitCameraParams(camera_pos, glm::normalize(origin - camera_pos), camera->GetUp());
}

bool RayInspectorPane::event(QEvent* event)
{
    auto key_event = static_cast<QKeyEvent*>(event);

    switch (event->type())
    {
    case QEvent::KeyPress:
        KeyPressed(key_event);
        break;

    case QEvent::KeyRelease:
        KeyReleased(key_event);
        break;
    default:
        break;
    }

    return QWidget::event(event);
}

void RayInspectorPane::OnColorThemeUpdated()
{
    if (QtCommon::QtUtils::ColorTheme::Get().GetColorTheme() == ColorThemeType::kColorThemeTypeDark)
    {
        ui_->selected_ray_focus_->SetNormalIcon(QIcon(":/Resources/assets/third_party/ionicons/scan-outline-clickable-dark-theme.svg"));
    }
    else
    {
        ui_->selected_ray_focus_->SetNormalIcon(QIcon(":/Resources/assets/third_party/ionicons/scan-outline-clickable.svg"));
    }
}


