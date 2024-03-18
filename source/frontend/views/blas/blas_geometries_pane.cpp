//=============================================================================
// Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the geometries pane on the BLAS tab.
//=============================================================================

#include "views/blas/blas_geometries_pane.h"

#include "managers/message_manager.h"
#include "models/blas/blas_geometries_model.h"
#include "models/blas/blas_geometries_table_item_delegate.h"
#include "views/widget_util.h"

BlasGeometriesPane::BlasGeometriesPane(QWidget* parent)
    : BasePane(parent)
    , ui_(new Ui::BlasGeometriesPane)
    , blas_index_(0)
    , tlas_index_(0)
    , selected_geometry_index_(UINT32_MAX)
    , data_valid_(false)
{
    ui_->setupUi(this);
    rra::widget_util::ApplyStandardPaneStyle(this, ui_->main_content_, ui_->main_scroll_area_);
    model_ = new rra::BlasGeometriesModel(rra::kBlasGeometriesNumWidgets);

    model_->InitializeModel(ui_->title_tlas_address_, rra::kTlasGeometriesBaseAddress, "text");
    model_->InitializeModel(ui_->title_blas_address_, rra::kBlasGeometriesBaseAddress, "text");

    // Initialize table.
    model_->InitializeTableModel(ui_->geometries_table_, 0, rra::kBlasGeometriesColumnCount);
    ui_->geometries_table_->setCursor(Qt::PointingHandCursor);

    connect(ui_->search_box_, &QLineEdit::textChanged, model_, &rra::BlasGeometriesModel::SearchTextChanged);
    connect(&rra::MessageManager::Get(), &rra::MessageManager::BlasSelected, this, &BlasGeometriesPane::SetBlasIndex);
    connect(&rra::MessageManager::Get(), &rra::MessageManager::TlasSelected, this, &BlasGeometriesPane::SetTlasIndex);

    table_delegate_ = new GeometriesTableItemDelegate(ui_->geometries_table_);
    ui_->geometries_table_->setItemDelegate(table_delegate_);

    // Set up a connection between the blas list being sorted and making sure the selected blas is visible.
    connect(model_->GetProxyModel(), &rra::BlasGeometriesProxyModel::layoutChanged, this, &BlasGeometriesPane::ScrollToSelectedGeometry);

    // This event filter allows us to override right click to deselect all rows instead of select one.
    ui_->geometries_table_->viewport()->installEventFilter(this);
}

BlasGeometriesPane::~BlasGeometriesPane()
{
    delete model_;
    delete table_delegate_;
}

void BlasGeometriesPane::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        // Deselect rows when escape is pressed.
        ui_->geometries_table_->selectionModel()->clearSelection();
        break;
    default:
        break;
    }

    BasePane::keyPressEvent(event);
}

void BlasGeometriesPane::showEvent(QShowEvent* event)
{
    // Deselect any selected rows.
    QItemSelectionModel* selected_item = ui_->geometries_table_->selectionModel();
    if (selected_item->hasSelection())
    {
        QModelIndexList item_list = selected_item->selectedIndexes();
        if (item_list.size() > 0)
        {
            for (const auto& item : item_list)
            {
                ui_->geometries_table_->selectionModel()->select(item, QItemSelectionModel::Deselect);
            }
        }
    }

    if (data_valid_ == false)
    {
        ui_->geometries_table_->setSortingEnabled(false);
        bool populated = model_->UpdateTable(tlas_index_, blas_index_);
        ui_->geometries_table_->setSortingEnabled(true);
        data_valid_ = true;
        if (populated)
        {
            // There are geometries, so show the table.
            ui_->table_valid_switch_->setCurrentIndex(1);

            // Highlight the selected geometry row.
            if (selected_geometry_index_ != UINT32_MAX)
            {
                const QModelIndex& geometry_index = model_->FindGeometryIndex(selected_geometry_index_);
                if (geometry_index.isValid() == true)
                {
                    ui_->geometries_table_->selectRow(geometry_index.row());
                    ui_->geometries_table_->scrollTo(geometry_index, QAbstractItemView::ScrollHint::PositionAtTop);
                }
            }
        }
        else
        {
            // No geometries in this BLAS, show an empty page.
            ui_->table_valid_switch_->setCurrentIndex(0);
        }
    }

    BasePane::showEvent(event);
}

void BlasGeometriesPane::OnTraceClose()
{
    data_valid_              = false;
    blas_index_              = 0;
    tlas_index_              = 0;
    selected_geometry_index_ = UINT32_MAX;
    ui_->search_box_->setText("");
}

bool BlasGeometriesPane::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
        if (mouse_event->button() == Qt::MouseButton::RightButton)
        {
            // Deselect all rows in BLAS table.
            ui_->geometries_table_->selectionModel()->clearSelection();
            return true;
        }
    }

    // Standard event processing.
    return QObject::eventFilter(obj, event);
}

void BlasGeometriesPane::SetBlasIndex(uint64_t blas_index)
{
    if (blas_index != blas_index_)
    {
        blas_index_ = blas_index;
        data_valid_ = false;
    }
}

void BlasGeometriesPane::SetTlasIndex(uint64_t tlas_index)
{
    if (tlas_index != tlas_index_)
    {
        tlas_index_ = tlas_index;
        data_valid_ = false;
    }
}

void BlasGeometriesPane::SelectGeometry(uint32_t geometry_index)
{
    selected_geometry_index_ = geometry_index;
    data_valid_              = false;
}

void BlasGeometriesPane::ScrollToSelectedGeometry()
{
    QItemSelectionModel* selected_item = ui_->geometries_table_->selectionModel();
    if (selected_item->hasSelection())
    {
        QModelIndexList item_list = selected_item->selectedRows();
        if (item_list.size() > 0)
        {
            // Get the model index of the name column since column 0 (compare ID) is hidden and scrollTo
            // doesn't appear to scroll on hidden columns.
            QModelIndex model_index = model_->GetProxyModel()->index(item_list[0].row(), rra::kBlasGeometriesColumnGeometryIndex);
            ui_->geometries_table_->scrollTo(model_index, QAbstractItemView::ScrollHint::PositionAtTop);
            return;
        }
    }
    ui_->geometries_table_->scrollToTop();
}
