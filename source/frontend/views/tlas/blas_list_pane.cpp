//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the BLAS list pane.
//=============================================================================

#include "views/tlas/blas_list_pane.h"

#include "managers/message_manager.h"
#include "models/tlas/blas_list_model.h"
#include "models/tlas/blas_list_table_item_delegate.h"
#include "views/widget_util.h"

BlasListPane::BlasListPane(QWidget* parent)
    : BasePane(parent)
    , ui_(new Ui::BlasListPane)
    , tlas_index_(0)
    , blas_index_(UINT64_MAX)
    , data_valid_(false)
{
    ui_->setupUi(this);
    rra::widget_util::ApplyStandardPaneStyle(ui_->main_scroll_area_);
    model_ = new rra::BlasListModel(rra::kBlasListNumWidgets);

    // Initialize table.
    model_->InitializeTableModel(ui_->blas_table_, 0, rra::kBlasListColumnCount);
    ui_->blas_table_->setCursor(Qt::PointingHandCursor);

    connect(ui_->search_box_, &QLineEdit::textChanged, model_, &rra::BlasListModel::SearchTextChanged);
    connect(ui_->blas_table_, &QAbstractItemView::doubleClicked, this, &BlasListPane::GotoBlasPaneFromTableSelect);
    connect(&rra::MessageManager::Get(), &rra::MessageManager::TlasSelected, this, &BlasListPane::SetTlasIndex);
    connect(&rra::MessageManager::Get(), &rra::MessageManager::BlasSelected, this, &BlasListPane::SetBlasIndex);

    ui_->content_blas_filter_->InitMultiSelect(this, "Filter by flag");
    connect(ui_->content_blas_filter_, &ArrowIconComboBox::CheckboxChanged, this, &BlasListPane::BlasFilterComboBoxItemClicked);

    table_delegate_ = new BlasListTableItemDelegate();
    ui_->blas_table_->setItemDelegate(table_delegate_);

    // Set up a connection between the blas list being sorted and making sure the selected blas is visible.
    connect(model_->GetProxyModel(), &rra::BlasListProxyModel::layoutChanged, this, &BlasListPane::ScrollToSelectedBlas);

    // This event filter allows us to override right click to deselect all rows instead of select one.
    ui_->blas_table_->viewport()->installEventFilter(this);

    for (int i = 0; i < kBlasFiltersCount; i++)
    {
        AddBlasFilterItem(static_cast<BlasFilters>(i));
    }

    ui_->blas_table_->setColumnHidden(rra::kBlasListColumnBlasIndex, true);
}

BlasListPane::~BlasListPane()
{
    delete model_;
    delete table_delegate_;
    delete ui_;
}

void BlasListPane::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        // Deselect rows when escape is pressed.
        ui_->blas_table_->selectionModel()->clearSelection();
        break;
    default:
        break;
    }

    BasePane::keyPressEvent(event);
}

void BlasListPane::showEvent(QShowEvent* event)
{
    if (data_valid_ == false)
    {
        // Deselect any selected rows.
        QItemSelectionModel* selected_item = ui_->blas_table_->selectionModel();
        if (selected_item->hasSelection())
        {
            QModelIndexList item_list = selected_item->selectedIndexes();
            if (item_list.size() > 0)
            {
                for (const auto& item : item_list)
                {
                    ui_->blas_table_->selectionModel()->select(item, QItemSelectionModel::Deselect);
                }
            }
        }

        ui_->blas_table_->setSortingEnabled(false);
        model_->UpdateTable(tlas_index_);
        ui_->blas_table_->setSortingEnabled(true);
        data_valid_ = true;

        // If there's a selected BLAS, highlight the row.
        bool row_selected = false;
        if (blas_index_ != UINT64_MAX)
        {
            const QModelIndex model_index = model_->GetTableModelIndex(blas_index_);
            if (model_index.isValid())
            {
                ui_->blas_table_->selectRow(model_index.row());
                ui_->blas_table_->scrollTo(model_index, QAbstractItemView::ScrollHint::PositionAtTop);
                row_selected = true;
            }
        }
        if (!row_selected)
        {
            ui_->blas_table_->selectRow(-1);
        }
    }

    BasePane::showEvent(event);
}

void BlasListPane::OnTraceClose()
{
    data_valid_ = false;
    blas_index_ = UINT64_MAX;
    ui_->search_box_->setText("");
}

void BlasListPane::BlasFilterComboBoxItemClicked(QCheckBox* check_box)
{
    BlasFilters filter = {};

    // Find the view.
    for (BlasFilters i = {}; i < kBlasFiltersCount; i = BlasFilters(i + 1))
    {
        if (blas_filter_array_[i] == check_box)
        {
            filter = i;
            break;
        }
    }

    switch (filter)
    {
    case kAllowUpdate:
        model_->GetProxyModel()->SetFilterByAllowUpdate(check_box->isChecked());
        break;
    case kAllowCompaction:
        model_->GetProxyModel()->SetFilterByAllowCompaction(check_box->isChecked());
        break;
    case kLowMemory:
        model_->GetProxyModel()->SetFilterByLowMemory(check_box->isChecked());
        break;
    case kBuildTypeFastBuild:
        model_->GetProxyModel()->SetFilterByFastBuild(check_box->isChecked());
        break;
    case kBuildTypeFastTrace:
        model_->GetProxyModel()->SetFilterByFastTrace(check_box->isChecked());
        break;

    default:
        break;
    }

    model_->GetProxyModel()->invalidate();
}

void BlasListPane::AddBlasFilterItem(BlasFilters filter)
{
    switch (filter)
    {
    case kAllowUpdate:
        blas_filter_array_[filter] = ui_->content_blas_filter_->AddCheckboxItem("Allow update", filter, false, false);
        break;
    case kAllowCompaction:
        blas_filter_array_[filter] = ui_->content_blas_filter_->AddCheckboxItem("Allow compaction", filter, false, false);
        break;
    case kLowMemory:
        blas_filter_array_[filter] = ui_->content_blas_filter_->AddCheckboxItem("Low memory", filter, false, false);
        break;
    case kBuildTypeFastBuild:
        blas_filter_array_[filter] = ui_->content_blas_filter_->AddCheckboxItem("Fast build", filter, false, false);
        break;
    case kBuildTypeFastTrace:
        blas_filter_array_[filter] = ui_->content_blas_filter_->AddCheckboxItem("Fast trace", filter, false, false);
        break;
    default:
        Q_ASSERT(!"Invalid BLAS filter value");
    }
}

bool BlasListPane::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
        if (mouse_event->button() == Qt::MouseButton::RightButton)
        {
            // Deselect all rows in BLAS table.
            ui_->blas_table_->selectionModel()->clearSelection();
            return true;
        }
    }

    // Standard event processing.
    return QObject::eventFilter(obj, event);
}

void BlasListPane::GotoBlasPaneFromTableSelect(const QModelIndex& index)
{
    if (index.isValid())
    {
        int32_t blas_index = model_->GetBlasIndex(index);
        if (blas_index != -1)
        {
            // Emit a message that the TLAS instance has been double-clicked on. The BLAS UI
            // will listen for this signal and change the BLAS combo box selected item to
            // blas_index, which in turn will update the UI with the new BLAS selected.
            emit rra::MessageManager::Get().BlasSelected(blas_index);

            // Switch to the BLAS pane.
            emit rra::MessageManager::Get().PaneSwitchRequested(rra::kPaneIdBlasViewer);
        }
    }
}

void BlasListPane::SetTlasIndex(uint64_t tlas_index)
{
    if (tlas_index != tlas_index_)
    {
        tlas_index_ = tlas_index;
        data_valid_ = false;
    }
}

void BlasListPane::SetBlasIndex(uint64_t blas_index)
{
    if (blas_index != blas_index_)
    {
        blas_index_ = blas_index;
        data_valid_ = false;
    }
}

void BlasListPane::ScrollToSelectedBlas()
{
    QItemSelectionModel* selected_item = ui_->blas_table_->selectionModel();
    if (selected_item->hasSelection())
    {
        QModelIndexList item_list = selected_item->selectedRows();
        if (item_list.size() > 0)
        {
            // Get the model index of the name column since column 0 (compare ID) is hidden and scrollTo
            // doesn't appear to scroll on hidden columns.
            QModelIndex model_index = model_->GetProxyModel()->index(item_list[0].row(), rra::kBlasListColumnAddress);
            ui_->blas_table_->scrollTo(model_index, QAbstractItemView::ScrollHint::PositionAtTop);
            return;
        }
    }
    ui_->blas_table_->scrollToTop();
}

