//=============================================================================
/// Copyright (c) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Implementation of a ray inspector tree view.
///
/// Based on a scaled treeview, it adds a right-click context menu.
//=============================================================================

#include "views/ray/ray_inspector_tree_view.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>

#include "io/viewer_io.h"

RayInspectorTreeView::RayInspectorTreeView(QWidget* parent)
    : ScaledTreeView(parent)
{
}

RayInspectorTreeView::~RayInspectorTreeView()
{
}

void RayInspectorTreeView::SetFocusOnSelectedRayCallback(std::function<void()> focus_on_selected_ray)
{
    focus_on_selected_ray_ = focus_on_selected_ray;
}

void RayInspectorTreeView::SetResetSceneCallback(std::function<void()> reset_scene)
{
    reset_scene_ = reset_scene;
}

void RayInspectorTreeView::SetViewerModel(rra::RayInspectorModel* model, uint64_t current_bvh_index)
{
    model_             = model;
    current_bvh_index_ = current_bvh_index;
}

bool RayInspectorTreeView::event(QEvent* event)
{
    QKeyEvent* key_event{};

    switch (event->type())
    {
    case QEvent::Enter:
        break;
    case QEvent::KeyPress:
        key_event = static_cast<QKeyEvent*>(event);
        if (key_event->key() == Qt::Key_F)
        {
            if (focus_on_selected_ray_)
            {
                focus_on_selected_ray_();
            }
        }
        if (key_event->key() == Qt::Key_R)
        {
            if (reset_scene_)
            {
                reset_scene_();
            }
        }
        break;
    case QEvent::KeyRelease:
        break;
    default:
        break;
    }

    return ScaledTreeView::event(event);
}

