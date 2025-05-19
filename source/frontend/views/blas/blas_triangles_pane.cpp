//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the Triangles pane on the BLAS tab.
//=============================================================================

#include "views/blas/blas_triangles_pane.h"

#include "public/rra_rtip_info.h"

#include "managers/message_manager.h"
#include "models/blas/blas_triangles_model.h"
#include "models/blas/blas_triangles_table_item_delegate.h"
#include "views/widget_util.h"

BlasTrianglesPane::BlasTrianglesPane(QWidget* parent)
    : BasePane(parent)
    , ui_(new Ui::BlasTrianglesPane)
    , blas_index_(0)
    , tlas_index_(0)
    , triangle_node_id_(UINT32_MAX)
    , data_valid_(false)
{
    ui_->setupUi(this);
    rra::widget_util::ApplyStandardPaneStyle(ui_->main_scroll_area_);
    model_ = new rra::BlasTrianglesModel(rra::kBlasTrianglesNumWidgets);

    model_->InitializeModel(ui_->title_tlas_address_, rra::kTlasTrianglesBaseAddress, "text");
    model_->InitializeModel(ui_->title_blas_address_, rra::kBlasTrianglesBaseAddress, "text");

    // Initialize table.
    model_->InitializeTableModel(ui_->triangles_table_, 0, rra::kBlasTrianglesColumnCount);
    ui_->triangles_table_->setCursor(Qt::PointingHandCursor);

    connect(ui_->search_box_, &QLineEdit::textChanged, model_, &rra::BlasTrianglesModel::SearchTextChanged);
    connect(ui_->triangles_table_, &QAbstractItemView::doubleClicked, [=](const QModelIndex& index) { this->SelectTriangleInBlasViewer(index, true); });
    connect(ui_->triangles_table_, &QAbstractItemView::clicked, [=](const QModelIndex& index) { this->SelectTriangleInBlasViewer(index, false); });
    connect(&rra::MessageManager::Get(), &rra::MessageManager::BlasSelected, this, &BlasTrianglesPane::SetBlasIndex);
    connect(&rra::MessageManager::Get(), &rra::MessageManager::TlasSelected, this, &BlasTrianglesPane::SetTlasIndex);
    connect(&rra::MessageManager::Get(), &rra::MessageManager::TriangleViewerSelected, this, &BlasTrianglesPane::SelectTriangle);

    table_delegate_ = new TrianglesTableItemDelegate(ui_->triangles_table_);
    ui_->triangles_table_->setItemDelegate(table_delegate_);

    // Set up a connection between the blas list being sorted and making sure the selected blas is visible.
    connect(model_->GetProxyModel(), &rra::BlasTrianglesProxyModel::layoutChanged, this, &BlasTrianglesPane::ScrollToSelectedTriangle);

    // This event filter allows us to override right click to deselect all rows instead of select one.
    ui_->triangles_table_->viewport()->installEventFilter(this);
}

BlasTrianglesPane::~BlasTrianglesPane()
{
    delete model_;
    delete table_delegate_;
    delete ui_;
}

void BlasTrianglesPane::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        // Deselect rows when escape is pressed.
        ui_->triangles_table_->selectionModel()->clearSelection();
        break;
    default:
        break;
    }

    BasePane::keyPressEvent(event);
}

void BlasTrianglesPane::showEvent(QShowEvent* event)
{
    // Deselect any selected rows.
    QItemSelectionModel* selected_item = ui_->triangles_table_->selectionModel();
    if (selected_item->hasSelection())
    {
        QModelIndexList item_list = selected_item->selectedIndexes();
        if (item_list.size() > 0)
        {
            for (const auto& item : item_list)
            {
                ui_->triangles_table_->selectionModel()->select(item, QItemSelectionModel::Deselect);
            }
        }
    }

    if (data_valid_ == false)
    {
        ui_->triangles_table_->setSortingEnabled(false);
        bool triangles = model_->UpdateTable(tlas_index_, blas_index_);
        ui_->triangles_table_->setSortingEnabled(true);
        data_valid_ = true;
        if (triangles)
        {
            // There are triangles, so show the table.
            ui_->table_valid_switch_->setCurrentIndex(1);

            // Highlight the selected triangle row.
            if (triangle_node_id_ != UINT32_MAX)
            {
                const QModelIndex& triangle_index = model_->FindTriangleIndex(triangle_node_id_, blas_index_);
                if (triangle_index.isValid() == true)
                {
                    ui_->triangles_table_->selectRow(triangle_index.row());
                    ui_->triangles_table_->scrollTo(triangle_index, QAbstractItemView::ScrollHint::PositionAtTop);
                }
            }
        }
        else
        {
            // No triangles in this BLAS, show an empty page.
            ui_->table_valid_switch_->setCurrentIndex(0);
        }
    }

    BasePane::showEvent(event);
}

void BlasTrianglesPane::OnTraceOpen()
{
    bool                   hide_triangle_verts = true;
    rta::RayTracingIpLevel rtip_level{(rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel()};
    if (rtip_level <= rta::RayTracingIpLevel::RtIp2_0)
    {
        hide_triangle_verts = false;
    }

    ui_->triangles_table_->setColumnHidden(rra::kBlasTrianglesColumnVertex0, hide_triangle_verts);
    ui_->triangles_table_->setColumnHidden(rra::kBlasTrianglesColumnVertex1, hide_triangle_verts);
    ui_->triangles_table_->setColumnHidden(rra::kBlasTrianglesColumnVertex2, hide_triangle_verts);

    ui_->triangles_table_->setColumnHidden(rra::kBlasTrianglesColumnTriangleCount, !hide_triangle_verts);
}

void BlasTrianglesPane::OnTraceClose()
{
    data_valid_       = false;
    blas_index_       = 0;
    tlas_index_       = 0;
    triangle_node_id_ = UINT32_MAX;
    ui_->search_box_->setText("");
}

bool BlasTrianglesPane::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
        if (mouse_event->button() == Qt::MouseButton::RightButton)
        {
            // Deselect all rows in BLAS table.
            ui_->triangles_table_->selectionModel()->clearSelection();
            return true;
        }
    }

    // Standard event processing.
    return QObject::eventFilter(obj, event);
}

void BlasTrianglesPane::SelectTriangleInBlasViewer(const QModelIndex& index, bool navigate_to_pane) const
{
    if (index.isValid())
    {
        uint32_t node_id = model_->GetNodeId(index.row());

        emit rra::MessageManager::Get().TriangleTableSelected(node_id);

        if (navigate_to_pane)
        {
            // Switch the pane so the UI is initialized.
            emit rra::MessageManager::Get().PaneSwitchRequested(rra::kPaneIdBlasViewer);
        }
    }
}

void BlasTrianglesPane::SetBlasIndex(uint64_t blas_index)
{
    if (blas_index != blas_index_)
    {
        blas_index_ = blas_index;
        data_valid_ = false;
    }
}

void BlasTrianglesPane::SetTlasIndex(uint64_t tlas_index)
{
    if (tlas_index != tlas_index_)
    {
        tlas_index_ = tlas_index;
        data_valid_ = false;
    }
}

void BlasTrianglesPane::SelectTriangle(uint32_t triangle_node_id)
{
    triangle_node_id_ = triangle_node_id;
    data_valid_       = false;
}

void BlasTrianglesPane::ScrollToSelectedTriangle()
{
    QItemSelectionModel* selected_item = ui_->triangles_table_->selectionModel();
    if (selected_item->hasSelection())
    {
        QModelIndexList item_list = selected_item->selectedRows();
        if (item_list.size() > 0)
        {
            // Get the model index of the name column since column 0 (compare ID) is hidden and scrollTo
            // doesn't appear to scroll on hidden columns.
            QModelIndex model_index = model_->GetProxyModel()->index(item_list[0].row(), rra::kBlasTrianglesColumnNodeAddress);
            ui_->triangles_table_->scrollTo(model_index, QAbstractItemView::ScrollHint::PositionAtTop);
            return;
        }
    }
    ui_->triangles_table_->scrollToTop();
}

