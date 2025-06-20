//=============================================================================
/// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Implementation of an acceleration structure tree view.
///
/// Based on a scaled treeview, it adds a right-click context menu.
//=============================================================================

#include "views/acceleration_structure_tree_view.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>

#include "io/viewer_io.h"

AccelerationStructureTreeView::AccelerationStructureTreeView(QWidget* parent)
    : ScaledTreeView(parent)
{
}

AccelerationStructureTreeView::~AccelerationStructureTreeView()
{
}

void AccelerationStructureTreeView::SetViewerModel(rra::AccelerationStructureViewerModel* model, uint64_t current_bvh_index)
{
    model_             = model;
    current_bvh_index_ = current_bvh_index;
}

void AccelerationStructureTreeView::contextMenuEvent(QContextMenuEvent* event)
{
    if (model_ == nullptr)
    {
        return;
    }

    const QModelIndexList& list = selectedIndexes();
    if (list.size() >= 1)
    {
        const QModelIndex& model_index = list[0];
        model_->SetSelectedNodeIndex(model_index);
    }

    QMenu menu;
    menu.setStyle(QApplication::style());

    rra::SceneContextMenuRequest request = {};
    request.location                     = rra::SceneContextMenuLocation::kSceneContextMenuLocationTreeView;

    auto context_options   = model_->GetSceneContextOptions(current_bvh_index_, request);
    auto camera_controller = model_->GetCameraController();

    context_options["Focus on selection"] = [=]() { camera_controller->FocusOnSelection(); };

    // Keep track of the actions so they can be deleted later.
    auto                  num_actions = context_options.size();
    std::vector<QAction*> actions;
    actions.reserve(num_actions);

    for (const auto& option_action : context_options)
    {
        auto action = new QAction(QString::fromStdString(option_action.first));
        menu.addAction(action);
        actions.push_back(action);
    }

    const QPoint pos    = event->globalPos();
    QAction*     action = menu.exec(pos);

    if (action != nullptr)
    {
        context_options[action->text().toStdString()]();
    }

    // Delete the actions.
    for (auto action_iter : actions)
    {
        delete action_iter;
    }
}

bool AccelerationStructureTreeView::event(QEvent* event)
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
            model_->GetCameraController()->FocusOnSelection();
        }
        break;
    case QEvent::KeyRelease:
        break;
    default:
        break;
    }

    return ScaledTreeView::event(event);
}

