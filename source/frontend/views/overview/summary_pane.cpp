//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the Summary pane.
//=============================================================================

#include "views/overview/summary_pane.h"

#include "qt_common/utils/qt_util.h"

#include "public/rra_assert.h"

#include "constants.h"
#include "managers/message_manager.h"
#include "views/widget_util.h"

#include "utils/scaling_manager.h"
#include "ui_tlas_pane.h"
#include "util/string_util.h"

SummaryPane::SummaryPane(QWidget* parent)
    : BasePane(parent)
    , ui_(new Ui::SummaryPane)
    , tlas_index_(0)
{
    ui_->setupUi(this);
    rra::widget_util::ApplyStandardPaneStyle(this, ui_->main_content_, ui_->main_scroll_area_);

    // Set white background for this pane.
    rra::widget_util::SetWidgetBackgroundColor(this, Qt::white);

    ui_->global_stats_table_->horizontalHeader()->setStretchLastSection(false);
    ui_->global_stats_table_->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);

    model_ = new rra::SummaryModel();

    // Initialize tables.
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

void SummaryPane::AddTlasPanes()
{
    const auto& tlas_stats = model_->GetTlasStatistics();
    ui_->total_memory_content_->setText(rra::string_util::LocalizedValueMemory(model_->GetTotalTraceMemory(), false, true));

    Ui_TlasOverviewPane tlas_overview_pane{};
    ui_->tlas_pane_list_->setSpacing(30);
    ui_->tlas_overview_scroll_area_->setWidgetResizable(true);

    for (const rra::TlasListStatistics& tlas : tlas_stats)
    {
        QWidget* tlas_pane = new QWidget();
        tlas_pane_widgets_.push_back(tlas_pane);
        tlas_overview_pane.setupUi(tlas_pane);

        QString tlas_memory{rra::string_util::LocalizedValueMemory(static_cast<double>(tlas.memory), false, true)};
        QString total_memory{rra::string_util::LocalizedValueMemory(static_cast<double>(tlas.effective_memory), false, true)};
        bool    allow_update     = tlas.build_flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        bool    allow_compaction = tlas.build_flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
        bool    low_memory       = tlas.build_flags & VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR;

        tlas_overview_pane.tlas_title_->setText(QString("TLAS index %1").arg(tlas.tlas_index));
        tlas_overview_pane.tlas_title_address_->setText(QString("0x%1").arg(tlas.address, 0, 16));
        tlas_overview_pane.total_triangles_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(tlas.total_triangle_count)));
        tlas_overview_pane.unique_triangles_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(tlas.unique_triangle_count)));
        tlas_overview_pane.instance_count_->setText(QString("%1 instances").arg(rra::string_util::LocalizedValue(tlas.instance_count)));
        tlas_overview_pane.blas_count_->setText(QString("%1 BLASes").arg(rra::string_util::LocalizedValue(tlas.blas_count)));
        tlas_overview_pane.tlas_memory_size_->setText(QString("TLAS (%1)").arg(tlas_memory));
        tlas_overview_pane.total_memory_size_->setText(QString("Total (%1)").arg(total_memory));
        tlas_overview_pane.allow_update_content_->setChecked(allow_update);
        tlas_overview_pane.allow_compaction_content_->setChecked(allow_compaction);
        tlas_overview_pane.low_memory_content_->setChecked(low_memory);
        tlas_overview_pane.build_type_content_->setText(rra::string_util::GetBuildTypeString(tlas.build_flags));
        tlas_overview_pane.number_of_nodes_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(tlas.node_count)));
        tlas_overview_pane.number_of_box_nodes_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(tlas.box_node_count)));
        tlas_overview_pane.number_of_box16_nodes_label_->hide();
        tlas_overview_pane.number_of_box16_nodes_content_->hide();
        tlas_overview_pane.number_of_box32_nodes_label_->hide();
        tlas_overview_pane.number_of_box32_nodes_content_->hide();
        tlas_overview_pane.number_of_instance_nodes_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(tlas.instance_count)));
        tlas_overview_pane.number_of_inactive_instances_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(tlas.inactive_instance_count)));

        if (!model_->IsTlasEmpty(tlas.tlas_index))
        {
            tlas_overview_pane.tlas_title_->setCursor(Qt::PointingHandCursor);
            tlas_overview_pane.tlas_title_address_->setCursor(Qt::PointingHandCursor);
            connect(tlas_overview_pane.tlas_title_, &QPushButton::clicked, this, [=]() { this->SelectTlas(tlas.tlas_index, true); });
            connect(tlas_overview_pane.tlas_title_address_, &QPushButton::clicked, this, [=]() { this->SelectTlas(tlas.tlas_index, true); });
        }
        else
        {
            tlas_overview_pane.tlas_title_->setStyleSheet("color: rgb(192,192,192)");
            tlas_overview_pane.tlas_title_address_->setStyleSheet("color: rgb(192,192,192)");
            tlas_overview_pane.tlas_title_->setEnabled(false);
            tlas_overview_pane.tlas_title_address_->setEnabled(false);
        }

        if (model_->RebraidingEnabled())
        {
            tlas_overview_pane.number_of_inactive_instances_label_->hide();
            tlas_overview_pane.number_of_inactive_instances_content_->hide();
        }

        ui_->tlas_pane_list_->addWidget(tlas_pane);
    }
}

void SummaryPane::OnTraceOpen()
{
    model_->Update();

    AddTlasPanes();

    tlas_index_ = 0;
    ui_->search_box_->setText("");

    BasePane::OnTraceOpen();
}

void SummaryPane::OnTraceClose()
{
    for (QWidget* tlas_pane : tlas_pane_widgets_)
    {
        ui_->tlas_pane_list_->removeWidget(tlas_pane);
        delete tlas_pane;
    }
    tlas_pane_widgets_.clear();
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
    if (tlas_index != -1)
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
