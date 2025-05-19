//=============================================================================
// Copyright (c) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for a tlas pane.
//=============================================================================

#include "views/overview/tlas_pane.h"

#include "qt_common/utils/qt_util.h"

#include "util/string_util.h"
#include "views/overview/summary_pane.h"

TlasPane::TlasPane(QWidget* parent)
    : QWidget(parent)
    , ui_(new Ui_TlasPane)
{
    ui_->setupUi(this);
}

TlasPane::~TlasPane()
{
    delete ui_;
}

void TlasPane::SetTlasStats(SummaryPane* summary_pane, const rra::TlasListStatistics& tlas, bool empty, bool rebraiding_enabled)
{
    QString tlas_memory{rra::string_util::LocalizedValueMemory(static_cast<double>(tlas.memory), false, true)};
    QString total_memory{rra::string_util::LocalizedValueMemory(static_cast<double>(tlas.effective_memory), false, true)};
    bool    allow_update     = tlas.build_flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    bool    allow_compaction = tlas.build_flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    bool    low_memory       = tlas.build_flags & VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR;

    ui_->tlas_title_->setText(QString("TLAS 0x%1").arg(tlas.address, 0, 16));
    ui_->total_triangles_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(tlas.total_triangle_count)));
    ui_->unique_triangles_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(tlas.unique_triangle_count)));
    ui_->instance_count_->setText(QString("%1 instances").arg(rra::string_util::LocalizedValue(tlas.instance_count)));
    ui_->blas_count_->setText(QString("%1 BLASes").arg(rra::string_util::LocalizedValue(tlas.blas_count)));
    ui_->tlas_memory_size_->setText(QString("TLAS (%1)").arg(tlas_memory));
    ui_->total_memory_size_->setText(QString("Total (%1)").arg(total_memory));
    ui_->allow_update_content_->setChecked(allow_update);
    ui_->allow_compaction_content_->setChecked(allow_compaction);
    ui_->low_memory_content_->setChecked(low_memory);
    ui_->build_type_content_->setText(rra::string_util::GetBuildTypeString(tlas.build_flags));
    ui_->number_of_nodes_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(tlas.node_count)));
    ui_->number_of_box_nodes_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(tlas.box_node_count)));
    ui_->number_of_box16_nodes_label_->hide();
    ui_->number_of_box16_nodes_content_->hide();
    ui_->number_of_box32_nodes_label_->hide();
    ui_->number_of_box32_nodes_content_->hide();
    ui_->number_of_instance_nodes_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(tlas.instance_count)));
    ui_->total_procedural_nodes_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(tlas.procedural_node_count)));
    ui_->number_of_inactive_instances_content_->setText(QString("%1").arg(rra::string_util::LocalizedValue(tlas.inactive_instance_count)));

    if (!empty)
    {
        ui_->tlas_title_->setCursor(Qt::PointingHandCursor);
        ui_->tlas_title_->SetLinkStyleSheet();
        connect(ui_->tlas_title_, &QPushButton::clicked, summary_pane, [=]() { summary_pane->SelectTlas(tlas.tlas_index, true); });
    }
    else
    {
        ui_->tlas_title_->setStyleSheet("color: rgb(192,192,192)");
        ui_->tlas_title_->setEnabled(false);
    }

    if (rebraiding_enabled)
    {
        ui_->number_of_inactive_instances_label_->hide();
        ui_->number_of_inactive_instances_content_->hide();
    }
}

void TlasPane::paintEvent(QPaintEvent* event)
{
    const QPalette& palette          = QtCommon::QtUtils::ColorTheme::Get().GetCurrentPalette();
    const QColor    background_color = palette.color(QPalette::AlternateBase);
    QPainter        painter(this);
    painter.fillRect(rect(), background_color);
    QWidget::paintEvent(event);
}

