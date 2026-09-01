//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the Partitions pane on the TLAS tab.
//=============================================================================

#include "views/tlas/tlas_partitions_pane.h"

#include "managers/message_manager.h"
#include "managers/pane_manager.h"
#include "models/table_item_delegate.h"
#include "models/tlas/tlas_partitions_model.h"
#include "views/widget_util.h"

TlasPartitionsPane::TlasPartitionsPane(QWidget* parent)
    : BasePane(parent)
    , ui_(new Ui::TlasPartitionsPane)
    , tlas_index_(0)
    , data_valid_(false)
{
    ui_->setupUi(this);
    rra::widget_util::ApplyStandardPaneStyle(ui_->main_scroll_area_);
    model_ = new rra::TlasPartitionsModel(rra::kTlasPartitionsNumWidgets);

    model_->InitializeModel(ui_->title_tlas_address_, rra::kTlasPartitionsTlasBaseAddress, "text");

    // Initialize table.
    model_->InitializeTableModel(ui_->partitions_table_, 0, rra::kPartitionsColumnCount);
    ui_->partitions_table_->setCursor(Qt::PointingHandCursor);

    connect(ui_->search_box_, &QLineEdit::textChanged, model_, &rra::TlasPartitionsModel::SearchTextChanged);
    connect(&rra::MessageManager::Get(), &rra::MessageManager::TlasSelected, this, &TlasPartitionsPane::SetTlasIndex);
    connect(ui_->partitions_table_, &QAbstractItemView::doubleClicked, this, &TlasPartitionsPane::GotoPartitionFromTableSelect);

    table_delegate_ = new TableItemDelegate();
    ui_->partitions_table_->setItemDelegate(table_delegate_);

    // This event filter allows us to override right click to deselect all rows instead of select one.
    ui_->partitions_table_->viewport()->installEventFilter(this);
}

TlasPartitionsPane::~TlasPartitionsPane()
{
    delete model_;
    delete table_delegate_;
    delete ui_;
}

void TlasPartitionsPane::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        // Deselect rows when escape is pressed.
        ui_->partitions_table_->selectionModel()->clearSelection();
        break;
    default:
        break;
    }

    BasePane::keyPressEvent(event);
}

void TlasPartitionsPane::showEvent(QShowEvent* event)
{
    if (data_valid_ == false)
    {
        ui_->partitions_table_->setSortingEnabled(false);
        bool partitioned = model_->UpdateTable(tlas_index_);
        ui_->partitions_table_->setSortingEnabled(true);
        data_valid_ = true;
        if (partitioned)
        {
            // The TLAS is partitioned, so show the table.
            ui_->table_valid_switch_->setCurrentIndex(1);
        }
        else
        {
            // The TLAS is not partitioned, show an empty page.
            ui_->table_valid_switch_->setCurrentIndex(0);
        }
    }

    BasePane::showEvent(event);
}

void TlasPartitionsPane::OnTraceClose()
{
    data_valid_ = false;
    tlas_index_ = 0;
    ui_->search_box_->setText("");
}

bool TlasPartitionsPane::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
        if (mouse_event->button() == Qt::MouseButton::RightButton)
        {
            // Deselect all rows in the partition table.
            ui_->partitions_table_->selectionModel()->clearSelection();
            return true;
        }
    }

    // Standard event processing.
    return QObject::eventFilter(obj, event);
}

void TlasPartitionsPane::SetTlasIndex(uint64_t tlas_index)
{
    if (tlas_index != tlas_index_)
    {
        tlas_index_ = tlas_index;
        data_valid_ = false;
    }
}

void TlasPartitionsPane::GotoPartitionFromTableSelect(const QModelIndex& index)
{
    if (index.isValid())
    {
        int32_t partition_index = model_->GetPartitionIndex(index);
        if (partition_index != -1)
        {
            // First, switch the pane so the TLAS viewer is initialized.
            emit rra::MessageManager::Get().PaneSwitchRequested(rra::kPaneIdTlasViewer);

            emit rra::MessageManager::Get().PartitionsTableDoubleClicked(tlas_index_, static_cast<uint32_t>(partition_index));
        }
    }
}

