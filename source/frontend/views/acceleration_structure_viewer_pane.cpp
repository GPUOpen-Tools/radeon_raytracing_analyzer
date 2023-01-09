//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of an acceleration structure viewer pane base class.
//=============================================================================

#include "views/acceleration_structure_viewer_pane.h"

#include "public/rra_assert.h"
#include "public/camera.h"
#include "public/renderer_adapter.h"

#include "models/acceleration_structure_tree_view_item.h"
#include "models/acceleration_structure_viewer_model.h"
#include "views/widget_util.h"
#include "settings/settings.h"

#include "io/viewer_io.h"

#include <QMenu>
#include <QContextMenuEvent>
#include <QCursor>

AccelerationStructureViewerPane::AccelerationStructureViewerPane(QWidget* parent)
    : BasePane(parent)
    , model_(nullptr)
    , acceleration_structure_combo_box_(nullptr)
    , renderer_interface_(nullptr)
    , renderer_widget_(nullptr)
    , trace_loaded_(false)
{
    hand_cursor_.setShape(Qt::CursorShape::ClosedHandCursor);
}

AccelerationStructureViewerPane::~AccelerationStructureViewerPane()
{
    delete model_;

    // Destroy the renderer interface if it was created.
    if (renderer_interface_ != nullptr)
    {
        renderer_interface_->Shutdown();
        delete renderer_interface_;
    }
}

void AccelerationStructureViewerPane::SetTableParams(ScaledTableView* table_view)
{
    table_view->horizontalHeader()->setStretchLastSection(false);
    table_view->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    table_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_view->setFocusPolicy(Qt::NoFocus);
    table_view->setSelectionMode(QAbstractItemView::NoSelection);
    table_view->GetHeaderView()->setSectionResizeMode(QHeaderView::Stretch);
}

void AccelerationStructureViewerPane::keyPressEvent(QKeyEvent* event)
{
    RRA_ASSERT(model_ != nullptr);
    uint64_t bvh_index = model_->FindAccelerationStructureIndex(acceleration_structure_combo_box_);

    switch (event->key())
    {
    case Qt::Key_Escape:
        model_->ClearSelection(bvh_index);
        HandleTreeNodeSelected(QModelIndex());
        break;
    default:
        break;
    }

    BasePane::keyPressEvent(event);
}

void AccelerationStructureViewerPane::SelectedTreeNodeChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected);
    const QModelIndexList& selected_indices = selected.indexes();
    if (selected_indices.size() > 0)
    {
        const QModelIndex& selected_model_index = selected_indices[0];
        HandleTreeNodeSelected(selected_model_index);
    }
    else
    {
        // Provide an invalid index, which will deselect everything.
        HandleTreeNodeSelected(QModelIndex());
    }
}

void AccelerationStructureViewerPane::PopulateAccelerationStructure()
{
    RRA_ASSERT(acceleration_structure_combo_box_ != nullptr);
    RRA_ASSERT(model_ != nullptr);

    // Set up the combo box showing the available acceleration structures.
    // When row(0) is selected below, the slot will be called and that will call
    // SetupAccelerationStructure(), so no need to call this function explicitly here.
    rra::widget_util::InitSingleSelectComboBox(this, acceleration_structure_combo_box_, "", false);
    model_->SetupAccelerationStructureList(acceleration_structure_combo_box_);
    acceleration_structure_combo_box_->SetMaximumHeight(400);

    trace_loaded_ = true;
}

void AccelerationStructureViewerPane::showEvent(QShowEvent* event)
{
    if (trace_loaded_ == false)
    {
        PopulateAccelerationStructure();
    }

    // Refresh the selected model index (represents the address/node) since the display
    // type may have changed in the settings.
    int selected_row = acceleration_structure_combo_box_->CurrentRow();
    int row          = model_->FindRowFromAccelerationStructureIndex(last_selected_as_id_);
    if (selected_row != row)
    {
        acceleration_structure_combo_box_->SetSelectedRow(row);
    }
    uint64_t bvh_index = model_->FindAccelerationStructureIndex(acceleration_structure_combo_box_);

    if (bvh_index != UINT64_MAX)
    {
        // If we switch panes while holding shift we don't want to keep multiselecting.
        model_->SetMultiSelect(false);
        model_->RefreshUI(bvh_index);
    }

    if (renderer_widget_ != nullptr)
    {
        // The viewer pane is being shown. Get the widget to start drawing new frames.
        renderer_widget_->ContinueFrames();
        renderer_widget_->UpdateSwapchainSize();
        if (renderer_interface_)
        {
            renderer_interface_->MarkAsDirty();
        }
    }

    QWidget::showEvent(event);
}

void AccelerationStructureViewerPane::hideEvent(QHideEvent* event)
{
    if (renderer_widget_ != nullptr)
    {
        // The viewer pane is being hidden, so stop the renderer from drawing new frames.
        renderer_widget_->PauseFrames();
    }

    QWidget::hideEvent(event);
}

void AccelerationStructureViewerPane::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}

void AccelerationStructureViewerPane::HandleTreeNodeSelected(const QModelIndex& model_index)
{
    RRA_ASSERT(model_ != nullptr);
    uint64_t bvh_index = model_->FindAccelerationStructureIndex(acceleration_structure_combo_box_);

    model_->UpdateLastSelectedNodeIsLeaf(model_index, bvh_index);

    if (model_index.isValid())
    {
        if (bvh_index != UINT64_MAX)
        {
            // Do a complete refresh of the UI. Refresh at the is independent of TLAS/BLAS views.

            // We only update the scene selection if the renderer is not in focus, which means it was
            // clicked from the UI.
            if (!renderer_widget_->GetRendererIsFocused())
            {
                model_->SetSceneSelection(model_index, bvh_index);
            }
            model_->SetSelectedNodeIndex(model_index);
            model_->RefreshUI(bvh_index);

            renderer_interface_->GetCamera().updated_ = true;
        }
    }
    else
    {
        model_->ResetModelValues(false);

        if (bvh_index != UINT64_MAX)
        {
            model_->RefreshUI(bvh_index);

            renderer_interface_->GetCamera().updated_ = true;
        }
    }
    UpdateWidgets(model_index);
}

void AccelerationStructureViewerPane::OnTraceClose()
{
    trace_loaded_ = false;

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
    ResetUI(true);
}

void AccelerationStructureViewerPane::Reset()
{
    ResetUI(true);
    trace_loaded_ = false;
}

bool AccelerationStructureViewerPane::eventFilter(QObject* object, QEvent* event)
{
    QTreeView* tree = qobject_cast<QTreeView*>(object);

    if (tree && event->type() == QEvent::Resize)
    {
        // When tree view is resized we want to set the minimum width slightly less
        // than width of the left pane so each item in tree always a large space
        // that is clickable.
        QResizeEvent* resize_event    = static_cast<QResizeEvent*>(event);
        int           left_pane_width = resize_event->size().width() - 40;
        tree->header()->setMinimumSectionSize(left_pane_width);
        tree->resizeColumnToContents(0);
    }
    return false;
}

void AccelerationStructureViewerPane::ResetUI(bool reset_scene)
{
    RRA_ASSERT(model_ != nullptr);
    model_->ResetModelValues(reset_scene);
    UpdateWidgets(QModelIndex());
}

uint64_t AccelerationStructureViewerPane::UpdateSelectedBvh()
{
    ResetUI(false);
    RRA_ASSERT(model_ != nullptr);

    return SetupAccelerationStructure();
}

uint64_t AccelerationStructureViewerPane::SetupAccelerationStructure()
{
    uint64_t acceleration_structure_index = model_->FindAccelerationStructureIndex(acceleration_structure_combo_box_);
    if (acceleration_structure_index != UINT64_MAX)
    {
        // Update the model to display the BVH at the selected index.
        model_->PopulateScene(renderer_interface_, acceleration_structure_index);

        // Update the treeview to display the BVH nodes for the selected index.
        model_->PopulateTreeView(acceleration_structure_index);

        // Reset camera position.
        ResetCamera();
    }

    return acceleration_structure_index;
}

void AccelerationStructureViewerPane::InitializeRendererWidget(RendererWidget*             renderer_widget,
                                                               SidePaneContainer*          side_panel_container,
                                                               ViewerContainerWidget*      viewer_container_widget,
                                                               rra::renderer::BvhTypeFlags bvh_type)
{
    RRA_ASSERT(renderer_widget != nullptr);
    if (renderer_widget != nullptr)
    {
        // Create a renderer instance that the widget will use to draw the scene geometry.
        rra::renderer::RendererAdapterMap renderer_adapter_map;
        renderer_interface_ = rra::renderer::RendererFactory::CreateRenderer(renderer_adapter_map);

        RRA_ASSERT(renderer_interface_ != nullptr);
        if (renderer_interface_ != nullptr)
        {
            // Provide the renderer adapter map to the side panel container.
            side_panel_container->SetRendererAdapters(renderer_adapter_map);

            // Provide the renderer adapter map to the viewer container.
            viewer_container_widget->SetRendererAdapters(renderer_adapter_map, bvh_type);

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
            connect(renderer_widget, &RendererWidget::MouseMoved, this, &AccelerationStructureViewerPane::MouseMoved);
            connect(renderer_widget, &RendererWidget::MousePressed, this, &AccelerationStructureViewerPane::MousePressed);
            connect(renderer_widget, &RendererWidget::MouseDoubleClicked, this, &AccelerationStructureViewerPane::MouseDoubleClicked);
            connect(renderer_widget, &RendererWidget::MouseReleased, this, &AccelerationStructureViewerPane::MouseReleased);
            connect(renderer_widget, &RendererWidget::MouseWheelMoved, this, &AccelerationStructureViewerPane::MouseWheelMoved);
            connect(renderer_widget, &RendererWidget::KeyPressed, this, &AccelerationStructureViewerPane::KeyPressed);
            connect(renderer_widget, &RendererWidget::KeyReleased, this, &AccelerationStructureViewerPane::KeyReleased);
            connect(renderer_widget, &RendererWidget::FocusOut, this, &AccelerationStructureViewerPane::FocusOut);
            connect(renderer_widget, &RendererWidget::FocusIn, this, &AccelerationStructureViewerPane::FocusIn);
        }
    }
}

void AccelerationStructureViewerPane::HaltRendererWidget()
{
    if (renderer_widget_ != nullptr)
    {
        renderer_widget_->PauseFrames();

        // disonnect the renderer widget mouse and keyboard handler slots.
        disconnect(renderer_widget_, &RendererWidget::MouseMoved, this, &AccelerationStructureViewerPane::MouseMoved);
        disconnect(renderer_widget_, &RendererWidget::MousePressed, this, &AccelerationStructureViewerPane::MousePressed);
        disconnect(renderer_widget_, &RendererWidget::MouseDoubleClicked, this, &AccelerationStructureViewerPane::MouseDoubleClicked);
        disconnect(renderer_widget_, &RendererWidget::MouseReleased, this, &AccelerationStructureViewerPane::MouseReleased);
        disconnect(renderer_widget_, &RendererWidget::MouseWheelMoved, this, &AccelerationStructureViewerPane::MouseWheelMoved);
        disconnect(renderer_widget_, &RendererWidget::KeyPressed, this, &AccelerationStructureViewerPane::KeyPressed);
        disconnect(renderer_widget_, &RendererWidget::KeyReleased, this, &AccelerationStructureViewerPane::KeyReleased);
        disconnect(renderer_widget_, &RendererWidget::FocusOut, this, &AccelerationStructureViewerPane::FocusOut);
    }
}

/// @brief A recursive helper function used to expand any tree items necessary in order to show the selected index.
///
/// @param [in] tree_view The tree view to expand elements in.
/// @param [in] index The tree model index to show in the tree view.
void ExpandToShowIndex(QTreeView* tree_view, const QModelIndex& index)
{
    if (index.parent().isValid())
    {
        ExpandToShowIndex(tree_view, index.parent());
    }

    tree_view->expand(index);
}

void AccelerationStructureViewerPane::SelectTreeItem(QTreeView* tree_view, const QModelIndex& selected_index)
{
    QItemSelectionModel* selection_model = tree_view->selectionModel();
    if (selected_index.isValid())
    {
        // Make sure that all nodes up to the root are expanded.
        ExpandToShowIndex(tree_view, selected_index);

        // Scroll to the item to be selected for cases where it's not visible.
        tree_view->scrollTo(selected_index);

        // Select the row in the tree view.
        selection_model->select(selected_index, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
    }
    else
    {
        // Clear the selection.
        selection_model->select(selected_index, QItemSelectionModel::Rows | QItemSelectionModel::Clear);
    }
}

void AccelerationStructureViewerPane::UpdateSceneSelection(QTreeView* tree_view)
{
    auto        scene          = model_->GetSceneCollectionModel()->GetSceneByIndex(last_selected_as_id_);
    QModelIndex selected_index = QModelIndex();
    if (scene && scene->HasSelection())
    {
        selected_index = model_->GetModelIndexForNode(scene->GetMostRecentSelectedNodeId());
    }

    SelectTreeItem(tree_view, selected_index);

    HandleTreeNodeSelected(selected_index);
}

void AccelerationStructureViewerPane::UpdateCameraController()
{
    if (renderer_interface_ == nullptr)
    {
        return;
    }
    rra::renderer::Camera& camera = renderer_interface_->GetCamera();

    // Can assume the controller is going to be a ViewerIO.
    auto camera_controller = static_cast<rra::ViewerIO*>(camera.GetCameraController());

    // If setting UI on trace load, row count will be 0, so no need to set camera controller.
    if (camera_controller && (acceleration_structure_combo_box_->RowCount() > 0))
    {
        uint64_t selected_bvh_index = model_->FindAccelerationStructureIndex(acceleration_structure_combo_box_);
        if (selected_bvh_index != UINT64_MAX)
        {
            camera_controller->SetViewerModel(model_, selected_bvh_index);
        }
        else
        {
            camera_controller->SetViewerModel(nullptr, UINT64_MAX);
        }

        if (camera_controller != last_camera_controller_)
        {
            last_camera_controller_ = camera_controller;
            model_->SetCameraController(camera_controller);

            if (rra::Settings::Get().GetCameraResetOnStyleChange())
            {
                ResetCamera();
            }
            else
            {
                camera_controller->ResetArcRadius();
            }
        }
    }
}

void AccelerationStructureViewerPane::ResetCamera()
{
    UpdateCameraController();
    rra::renderer::Camera& camera            = renderer_interface_->GetCamera();
    auto                   camera_controller = static_cast<rra::ViewerIO*>(camera.GetCameraController());

    if (camera_controller)
    {
        camera_controller->ResetCameraPosition();
    }
}

void AccelerationStructureViewerPane::MousePressed(QMouseEvent* mouse_event)
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

void AccelerationStructureViewerPane::MouseDoubleClicked(QMouseEvent* mouse_event)
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
            SelectLeafNode(closest_hit.blas_index, true);
        }
    }
}

void AccelerationStructureViewerPane::MouseReleased(QMouseEvent* mouse_event)
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
            camera_controller->MouseReleased(mouse_event, {renderer_widget_->width(), renderer_widget_->height()}, &camera, cast_ray);
        if (closest_hit.distance > 0.0)
        {
            SelectLeafNode(closest_hit.blas_index, false);
        }
    }

    QApplication::restoreOverrideCursor();
    cursor_overridden_ = false;
}

void AccelerationStructureViewerPane::SelectLeafNode(const uint64_t blas_index, const bool navigate_to_pane)
{
    Q_UNUSED(blas_index);
    Q_UNUSED(navigate_to_pane);
}

bool AccelerationStructureViewerPane::WrapMouseMovement(QPoint pos)
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
bool DistanceGreaterThan(const QPoint& p1, const QPoint& p2, float dist)
{
    int diff_x   = p2.x() - p1.x();
    int diff_y   = p2.y() - p1.y();
    int sqr_dist = diff_x * diff_x + diff_y * diff_y;
    return (float)sqr_dist > dist * dist;
}

void AccelerationStructureViewerPane::MouseMoved(QMouseEvent* mouse_event)
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
            DistanceGreaterThan(mouse_start_dragging_pos_, mouse_event->pos(), camera_controller->GetMouseMoveDelta()))
        {
            QGuiApplication::setOverrideCursor(hand_cursor_);
            cursor_overridden_ = true;
        }

        camera_controller->MouseMoved(QCursor::pos());
    }
}

void AccelerationStructureViewerPane::MouseWheelMoved(QWheelEvent* wheel_event)
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

void AccelerationStructureViewerPane::KeyPressed(QKeyEvent* key_event)
{
    UpdateCameraController();
    rra::renderer::Camera& camera = renderer_interface_->GetCamera();

    // Can assume the controller is going to be a ViewerIO.
    auto camera_controller = static_cast<rra::ViewerIO*>(camera.GetCameraController());

    if (camera_controller)
    {
        camera_controller->KeyPressed(static_cast<Qt::Key>(key_event->key()));
    }

    switch (key_event->key())
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
        model_->ToggleRenderGeometry();
        break;

    case Qt::Key_M:
        model_->ToggleMeshWireframe();
        break;

    case Qt::Key_H:
        model_->HideSelectedNodes(last_camera_controller_->GetSceneIndex());
        break;

    case Qt::Key_J:
        model_->ShowAllNodes(last_camera_controller_->GetSceneIndex());
        break;

    case Qt::Key_Control:
        model_->SetMultiSelect(true);

    default:
        // We don't care about whatever key was pressed.
        break;
    }
}

void AccelerationStructureViewerPane::KeyReleased(QKeyEvent* key_event)
{
    UpdateCameraController();
    rra::renderer::Camera& camera = renderer_interface_->GetCamera();

    // Can assume the controller is going to be a ViewerIO.
    auto camera_controller = static_cast<rra::ViewerIO*>(camera.GetCameraController());

    if (camera_controller)
    {
        camera_controller->KeyReleased(static_cast<Qt::Key>(key_event->key()));
    }

    switch (key_event->key())
    {
    case Qt::Key_Control:
        model_->SetMultiSelect(false);

    default:
        // We don't care about whatever key was pressed.
        break;
    }
}

void AccelerationStructureViewerPane::FocusOut()
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

void AccelerationStructureViewerPane::FocusIn()
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
