//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the Summary pane.
//=============================================================================

#include "views/overview/summary_pane.h"

#include "qt_common/utils/qt_util.h"

#include "public/rra_assert.h"
#include "public/rra_tlas.h"

#include "constants.h"
#include "managers/message_manager.h"
#include "settings/settings.h"
#include "views/widget_util.h"

#include "tlas_pane.h"
#include "dispatch_pane.h"
#include "util/string_util.h"
#include <public/rra_ray_history.h>

SummaryPane::SummaryPane(QWidget* parent)
    : BasePane(parent)
    , ui_(new Ui::SummaryPane)
    , tlas_index_(0)
{
    ui_->setupUi(this);
    rra::widget_util::ApplyStandardPaneStyle(ui_->main_scroll_area_);

    ui_->global_stats_table_->horizontalHeader()->setStretchLastSection(false);
    ui_->global_stats_table_->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);

    model_ = new rra::SummaryModel();

    // Initialize tables.
    ui_->global_stats_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui_->global_stats_table_->setFocusPolicy(Qt::NoFocus);
    ui_->global_stats_table_->setSelectionMode(QAbstractItemView::NoSelection);
    model_->InitializeStatsTableModel(ui_->global_stats_table_);

    connect(ui_->search_box_, &QLineEdit::textChanged, model_, &rra::SummaryModel::SearchTextChanged);
    connect(&rra::MessageManager::Get(), &rra::MessageManager::TlasSelected, this, &SummaryPane::SetTlasIndex);

    // Search box is temporarily hidden until it has functionality for the TLAS list.
    ui_->search_box_->hide();
}

SummaryPane::~SummaryPane()
{
    delete ui_;
    delete model_;
}

void SummaryPane::UpdateTlasMap()
{
    tlas_address_to_index_.clear();

    uint64_t tlas_count = 0;
    RraBvhGetTlasCount(&tlas_count);

    for (uint64_t i = 0; i < tlas_count; i++)
    {
        uint64_t address;
        RraTlasGetBaseAddress(i, &address);
        tlas_address_to_index_[address] = i;
    }
}

void SummaryPane::AddDispatchPanes()
{
    uint32_t num_dispatches = model_->GetDispatchCount();
    if (num_dispatches > 0)
    {
        UpdateTlasMap();
        for (uint64_t index = 0; index < num_dispatches; index++)
        {
            DispatchPane* dispatch_pane = new DispatchPane();
            dispatch_pane->SetDispatchId(index);
            dispatch_pane->SetSummaryPane(this);
            dispatch_pane->SetTlasMap(&tlas_address_to_index_);

            widget_deletion_queue_.push_back(dispatch_pane);

            ui_->dispatch_pane_list_->addWidget(dispatch_pane, 0, Qt::AlignLeft | Qt::AlignTop);
        }
    }

    // Show/hide the dispatches depending if ray history information is available.
    ui_->dispatch_contents_->setVisible(num_dispatches > 0);
}

void SummaryPane::AddTlasPanes()
{
    const auto& tlas_stats = model_->GetTlasStatistics();
    ui_->total_memory_content_->setText(rra::string_util::LocalizedValueMemory(model_->GetTotalTraceMemory(), false, true));

    ui_->tlas_pane_list_->setSpacing(10);
    ui_->tlas_overview_scroll_area_->setWidgetResizable(true);

    for (const rra::TlasListStatistics& tlas : tlas_stats)
    {
        TlasPane* tlas_pane = new TlasPane();
        tlas_pane->SetTlasStats(this, tlas, model_->IsTlasEmpty(tlas.tlas_index), model_->RebraidingEnabled());
        widget_deletion_queue_.push_back(tlas_pane);

        ui_->tlas_pane_list_->addWidget(tlas_pane);
    }
}

void SummaryPane::OnTraceOpen()
{
    model_->Update();

    AddDispatchPanes();

    AddTlasPanes();

    tlas_index_ = 0;
    ui_->search_box_->setText("");

    BasePane::OnTraceOpen();
}

void SummaryPane::OnTraceClose()
{
    for (QWidget* widget : widget_deletion_queue_)
    {
        ui_->tlas_pane_list_->removeWidget(widget);
        delete widget;
    }
    widget_deletion_queue_.clear();
}

void SummaryPane::showEvent(QShowEvent* event)
{
    ui_->search_box_->setText("");
    QWidget::showEvent(event);
}

void SummaryPane::Reset()
{
    model_->ResetModelValues();
}

void SummaryPane::SelectTlas(uint32_t tlas_index, const bool navigate_to_pane)
{
    if (tlas_index != UINT32_MAX)
    {
        // Select the selected TLAS in the TLAS pane.
        tlas_index_ = tlas_index;
        emit rra::MessageManager::Get().TlasSelected(tlas_index_);

        if (navigate_to_pane)
        {
            // Switch to the BLAS pane.
            emit rra::MessageManager::Get().PaneSwitchRequested(rra::kPaneIdTlasViewer);
        }
    }
}

void SummaryPane::SetTlasIndex(uint64_t tlas_index)
{
    if (tlas_index != tlas_index_)
    {
        tlas_index_ = tlas_index;
    }
}
