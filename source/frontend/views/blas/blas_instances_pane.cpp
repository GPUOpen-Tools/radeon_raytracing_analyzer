//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the Instances pane on the BLAS tab.
//=============================================================================

#include "views/blas/blas_instances_pane.h"

#include "managers/message_manager.h"
#include "models/blas/blas_instances_model.h"
#include "models/instance_list_table_item_delegate.h"
#include "views/widget_util.h"

BlasInstancesPane::BlasInstancesPane(QWidget* parent)
    : BasePane(parent)
    , ui_(new Ui::BlasInstancesPane)
    , blas_index_(0)
    , tlas_index_(0)
    , instance_index_(UINT32_MAX)
    , data_valid_(false)
{
    ui_->setupUi(this);
    rra::widget_util::ApplyStandardPaneStyle(ui_->main_scroll_area_);
    model_ = new rra::BlasInstancesModel(rra::kBlasInstancesNumWidgets);

    model_->InitializeModel(ui_->title_tlas_address_, rra::kTlasInstancesBaseAddress, "text");
    model_->InitializeModel(ui_->title_blas_address_, rra::kBlasInstancesBaseAddress, "text");

    // Initialize table.
    model_->InitializeTableModel(ui_->instances_table_, 0, rra::kInstancesColumnCount);
    ui_->instances_table_->setCursor(Qt::PointingHandCursor);

    connect(ui_->search_box_, &QLineEdit::textChanged, model_, &rra::BlasInstancesModel::SearchTextChanged);
    connect(ui_->instances_table_, &QAbstractItemView::doubleClicked, this, &BlasInstancesPane::GotoBlasInstanceFromTableSelect);
    connect(&rra::MessageManager::Get(), &rra::MessageManager::BlasSelected, this, &BlasInstancesPane::SetBlasIndex);
    connect(&rra::MessageManager::Get(), &rra::MessageManager::TlasSelected, this, &BlasInstancesPane::SetTlasIndex);
    connect(&rra::MessageManager::Get(), &rra::MessageManager::InstanceSelected, this, &BlasInstancesPane::SetInstanceIndex);

    table_delegate_ = new InstanceListTableItemDelegate();
    ui_->instances_table_->setItemDelegate(table_delegate_);

    // Set up a connection between the blas list being sorted and making sure the selected blas is visible.
    connect(model_->GetProxyModel(), &rra::InstancesProxyModel::layoutChanged, this, &BlasInstancesPane::ScrollToSelectedInstance);

    // This event filter allows us to override right click to deselect all rows instead of select one.
    ui_->instances_table_->viewport()->installEventFilter(this);

    // Hide the column that does the instance index mapping.
    ui_->instances_table_->setColumnHidden(rra::kInstancesColumnUniqueInstanceIndex, true);
}

BlasInstancesPane::~BlasInstancesPane()
{
    delete model_;
    delete table_delegate_;
    delete ui_;
}

void BlasInstancesPane::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        // Deselect rows when escape is pressed.
        ui_->instances_table_->selectionModel()->clearSelection();
        break;
    default:
        break;
    }

    BasePane::keyPressEvent(event);
}

void BlasInstancesPane::showEvent(QShowEvent* event)
{
    if (data_valid_ == false)
    {
        // Deselect any selected rows.
        QItemSelectionModel* selected_item = ui_->instances_table_->selectionModel();
        if (selected_item->hasSelection())
        {
            QModelIndexList item_list = selected_item->selectedIndexes();
            if (item_list.size() > 0)
            {
                for (const auto& item : item_list)
                {
                    ui_->instances_table_->selectionModel()->select(item, QItemSelectionModel::Deselect);
                }
            }
        }

        ui_->instances_table_->setSortingEnabled(false);
        bool instances = model_->UpdateTable(tlas_index_, blas_index_);
        ui_->instances_table_->setSortingEnabled(true);
        data_valid_ = true;
        if (instances)
        {
            // There are instances, so show the table.
            ui_->table_valid_switch_->setCurrentIndex(1);
        }
        else
        {
            // No instances in this TLAS, show an empty page.
            ui_->table_valid_switch_->setCurrentIndex(0);
        }

        // If there's a selected instance, highlight the row.
        bool row_selected = false;
        if (instance_index_ != UINT32_MAX)
        {
            const QModelIndex model_index = model_->GetTableModelIndex(instance_index_);
            if (model_index.isValid())
            {
                ui_->instances_table_->selectRow(model_index.row());
                ui_->instances_table_->scrollTo(model_index, QAbstractItemView::ScrollHint::PositionAtTop);
                row_selected = true;
            }
        }
        if (!row_selected)
        {
            ui_->instances_table_->selectRow(-1);
        }
    }

    BasePane::showEvent(event);
}

void BlasInstancesPane::OnTraceClose()
{
    data_valid_ = false;
    blas_index_ = 0;
    tlas_index_ = 0;
    ui_->search_box_->setText("");
}

bool BlasInstancesPane::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
        if (mouse_event->button() == Qt::MouseButton::RightButton)
        {
            // Deselect all rows in BLAS table.
            ui_->instances_table_->selectionModel()->clearSelection();
            return true;
        }
    }

    // Standard event processing.
    return QObject::eventFilter(obj, event);
}

void BlasInstancesPane::GotoBlasInstanceFromTableSelect(const QModelIndex& index)
{
    if (index.isValid())
    {
        int32_t instance_row = model_->GetInstanceIndex(index);
        if (instance_row != -1)
        {
            // First, switch the pane so the UI is initialized.
            emit rra::MessageManager::Get().PaneSwitchRequested(rra::kPaneIdTlasViewer);

            // Emit a message indicating that a BLAS instance has been double-clicked.
            emit rra::MessageManager::Get().InstancesTableDoubleClicked(tlas_index_, blas_index_, instance_row);

            // Get the instance index from the table corresponding to the row.
            uint32_t instance_index = model_->GetProxyModel()->GetData(index.row(), rra::kInstancesColumnUniqueInstanceIndex);
            emit     rra::MessageManager::Get().InstanceSelected(instance_index);
        }
    }
}

void BlasInstancesPane::SetBlasIndex(uint64_t blas_index)
{
    if (blas_index != blas_index_)
    {
        blas_index_ = blas_index;
        data_valid_ = false;
    }
}

void BlasInstancesPane::SetTlasIndex(uint64_t tlas_index)
{
    if (tlas_index != tlas_index_)
    {
        tlas_index_ = tlas_index;
        data_valid_ = false;
    }
}

void BlasInstancesPane::SetInstanceIndex(uint32_t instance_index)
{
    if (instance_index != instance_index_)
    {
        instance_index_ = instance_index;
        data_valid_     = false;
    }
}

void BlasInstancesPane::ScrollToSelectedInstance()
{
    QItemSelectionModel* selected_item = ui_->instances_table_->selectionModel();
    if (selected_item->hasSelection())
    {
        QModelIndexList item_list = selected_item->selectedRows();
        if (item_list.size() > 0)
        {
            // Get the model index of the name column since column 0 (compare ID) is hidden and scrollTo
            // doesn't appear to scroll on hidden columns.
            QModelIndex model_index = model_->GetProxyModel()->index(item_list[0].row(), rra::kInstancesColumnInstanceAddress);
            ui_->instances_table_->scrollTo(model_index, QAbstractItemView::ScrollHint::PositionAtTop);
            return;
        }
    }
    ui_->instances_table_->scrollToTop();
}

