//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the Side pane container class.
//=============================================================================

#include "views/side_panels/side_pane_container.h"

#include "managers/message_manager.h"
#include "models/acceleration_structure_tree_view_item.h"
#include "models/blas/blas_viewer_model.h"
#include "views/side_panels/view_pane.h"
#include "views/widget_util.h"

#include "public/renderer_adapter.h"

/// @brief Enum of the side panel ID's.
enum PaneIndex
{
    kPaneIndexView,
};

static const QString kTextHideControls = "Hide controls";
static const QString kTextShowControls = "Show controls";

SidePaneContainer::SidePaneContainer(QWidget* parent)
    : QWidget(parent)
    , ui_(new Ui::SidePaneContainer)
    , side_pane_index_(kPaneIndexView)
{
    ui_->setupUi(this);

    connect(ui_->view_button_, &QPushButton::clicked, [=]() { UpdateSidePane(kPaneIndexView); });
    ui_->side_panel_scroll_area_->show();
    ui_->view_button_->setText(kTextHideControls);

    // Add side panes to the stacked widget.
    // NOTE: Ownership of the created panes is transferred to the stacked widget, so no need to delete
    // them later.
    view_pane_ = new ViewPane(this);
    ui_->side_panel_stack_->addWidget(view_pane_);
}

SidePaneContainer::~SidePaneContainer()
{
}

void SidePaneContainer::SetRendererAdapters(const rra::renderer::RendererAdapterMap& adapters)
{
    using namespace rra::renderer;

    rra::ViewModel* view_pane_model = view_pane_->GetModel();
    view_pane_model->SetRendererAdapters(adapters);
}

void SidePaneContainer::UpdateSidePane(int pane_index)
{
    ui_->side_panel_stack_->setCurrentIndex(pane_index);

    if (!ui_->side_panel_scroll_area_->isVisible())
    {
        ui_->side_panel_scroll_area_->show();
        ui_->view_button_->setText(kTextHideControls);
    }

    else if (pane_index == side_pane_index_)
    {
        ui_->side_panel_scroll_area_->hide();
        ui_->view_button_->setText(kTextShowControls);
    }

    side_pane_index_ = pane_index;
}

void SidePaneContainer::OnTraceOpen()
{
    view_pane_->OnTraceOpen();
}

ViewPane* SidePaneContainer::GetViewPane() const
{
    return view_pane_;
}

void SidePaneContainer::MarkAsBLAS()
{
    view_pane_->HideTLASWidgets();
}

void SidePaneContainer::MarkProceduralGeometry(bool is_procedural)
{
    view_pane_->NonProceduralWidgetsHidden(is_procedural);
}
