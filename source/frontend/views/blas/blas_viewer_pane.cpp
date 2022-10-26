//=============================================================================
// Copyright (c) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the BLAS viewer pane.
//=============================================================================

#include "views/blas/blas_viewer_pane.h"

#include "qt_common/utils/scaling_manager.h"

#include "constants.h"
#include "managers/message_manager.h"
#include "models/acceleration_structure_tree_view_item.h"
#include "models/blas/blas_viewer_model.h"
#include "views/widget_util.h"

static const int kSplitterWidth = 300;

BlasViewerPane::BlasViewerPane(QWidget* parent)
    : AccelerationStructureViewerPane(parent)
    , ui_(new Ui::BlasViewerPane)
{
    ui_->setupUi(this);
    ui_->viewer_container_widget_->SetupUI(this);

    rra::widget_util::ApplyStandardPaneStyle(this, ui_->main_content_, ui_->main_scroll_area_);
    ui_->blas_tree_->setIndentation(rra::kTreeViewIndent);
    ui_->blas_tree_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui_->blas_tree_->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui_->blas_tree_->header()->setStretchLastSection(false);
    ui_->blas_tree_->installEventFilter(this);

    ui_->expand_collapse_tree_->Init(QStringList({rra::text::kTextExpandTree, rra::text::kTextCollapseTree}));
    ui_->expand_collapse_tree_->setCursor(Qt::PointingHandCursor);

    int        size           = ScalingManager::Get().Scaled(kSplitterWidth);
    QList<int> splitter_sizes = {size, size};
    ui_->splitter_->setSizes(splitter_sizes);

    SetTableParams(ui_->extents_table_);
    SetTableParams(ui_->geometry_flags_table_);
    SetTableParams(ui_->vertex_table_);
    ui_->geometry_flags_table_->horizontalHeader()->setStretchLastSection(true);

    derived_model_                    = new rra::BlasViewerModel(ui_->blas_tree_);
    model_                            = derived_model_;
    acceleration_structure_combo_box_ = ui_->content_bvh_;

    ui_->content_parent_blas_->setCursor(Qt::PointingHandCursor);

    // Initialize tables.
    model_->InitializeExtentsTableModel(ui_->extents_table_);
    derived_model_->InitializeFlagsTableModel(ui_->geometry_flags_table_);
    derived_model_->InitializeVertexTableModel(ui_->vertex_table_);

    model_->InitializeModel(ui_->content_node_address_, rra::kBlasStatsAddress, "text");
    model_->InitializeModel(ui_->content_node_type_, rra::kBlasStatsType, "text");
    model_->InitializeModel(ui_->content_current_sah_, rra::kBlasStatsCurrentSAH, "text");
    model_->InitializeModel(ui_->content_subtree_min_, rra::kBlasStatsSAHSubTreeMax, "text");
    model_->InitializeModel(ui_->content_subtree_mean_, rra::kBlasStatsSAHSubTreeMean, "text");
    model_->InitializeModel(ui_->content_primitive_index_, rra::kBlasStatsPrimitiveIndex, "text");
    model_->InitializeModel(ui_->content_geometry_index_, rra::kBlasStatsGeometryIndex, "text");
    model_->InitializeModel(ui_->content_parent_blas_, rra::kBlasStatsParent, "text");

    connect(acceleration_structure_combo_box_, &ArrowIconComboBox::SelectionChanged, this, &BlasViewerPane::UpdateSelectedBlas);
    connect(ui_->blas_tree_->selectionModel(), &QItemSelectionModel::selectionChanged, this, &BlasViewerPane::TreeNodeChanged);
    connect(ui_->blas_tree_, &QAbstractItemView::doubleClicked, [=]() { this->SelectLeafNode(true); });
    connect(&rra::MessageManager::Get(), &rra::MessageManager::BlasSelected, this, &BlasViewerPane::SetBlasSelection);
    connect(model_, &rra::AccelerationStructureViewerModel::SceneSelectionChanged, [=]() { this->UpdateSceneSelection(ui_->blas_tree_); });
    connect(ui_->expand_collapse_tree_, &ScaledCycleButton::Clicked, model_, &rra::AccelerationStructureViewerModel::ExpandCollapseTreeView);
    connect(ui_->search_box_, &TextSearchWidget::textChanged, model_, &rra::AccelerationStructureViewerModel::SearchTextChanged);

    ui_->tree_depth_slider_->setCursor(Qt::PointingHandCursor);
    connect(ui_->tree_depth_slider_, &DepthSliderWidget::SpanChanged, this, &BlasViewerPane::UpdateTreeDepths);

    connect(ui_->side_panel_container_->GetViewPane(), &ViewPane::ControlStyleChanged, this, &BlasViewerPane::UpdateCameraController);
    connect(ui_->side_panel_container_->GetViewPane(), &ViewPane::RenderModeChanged, [=](bool geometry_mode) {
        ui_->viewer_container_widget_->ShowColoringMode(geometry_mode);
    });
    connect(&rra::MessageManager::Get(), &rra::MessageManager::TriangleTableSelected, this, &BlasViewerPane::SelectTriangle);
    connect(&ScalingManager::Get(), &ScalingManager::ScaleFactorChanged, this, &BlasViewerPane::OnScaleFactorChanged);
    connect(ui_->content_parent_blas_, &ScaledPushButton::clicked, this, &BlasViewerPane::SelectParentNode);

    ui_->side_panel_container_->MarkAsBLAS();

    flag_table_delegate_ = new FlagTableItemDelegate();
    ui_->geometry_flags_table_->setItemDelegate(flag_table_delegate_);
}

BlasViewerPane::~BlasViewerPane()
{
}

void BlasViewerPane::OnTraceClose()
{
    // Only attempt to disconnect the mouse click signal if the renderer widget has already been initialized.
    if (renderer_widget_ != nullptr)
    {
        renderer_widget_->disconnect();
    }
    ui_->content_node_address_->setDisabled(true);
    ui_->content_node_address_->setToolTip("");
    ui_->search_box_->setText("");

    AccelerationStructureViewerPane::OnTraceClose();
}

void BlasViewerPane::OnTraceOpen()
{
    InitializeRendererWidget(ui_->blas_scene_, ui_->side_panel_container_, ui_->viewer_container_widget_, rra::renderer::BvhTypeFlags::BottomLevel);
    last_selected_as_id_ = 0;
    ui_->side_panel_container_->OnTraceOpen();
    ui_->tree_depth_slider_->SetLowerValue(0);
    ui_->tree_depth_slider_->SetUpperValue(0);
    ui_->viewer_container_widget_->ShowColoringMode(true);
}

void BlasViewerPane::showEvent(QShowEvent* event)
{
    ui_->label_bvh_->setText("BLAS:");
    AccelerationStructureViewerPane::showEvent(event);
}

void BlasViewerPane::SetBlasSelection(uint64_t blas_index)
{
    last_selected_as_id_ = blas_index;
}

void BlasViewerPane::UpdateTreeDepths(int min_value, int max_value)
{
    rra::Scene* scene = model_->GetSceneCollectionModel()->GetSceneByIndex(last_selected_as_id_);
    if (scene)
    {
        scene->SetDepthRange(min_value, max_value);
    }

    ui_->tree_depth_start_value_->setText(QString::number(min_value));
    ui_->tree_depth_end_value_->setText(QString::number(max_value));
}

void BlasViewerPane::OnScaleFactorChanged()
{
    int height = ui_->geometry_flags_table_->verticalHeader()->minimumSectionSize();
    ui_->geometry_flags_table_->setMaximumHeight(2 * height);
}

void BlasViewerPane::UpdateWidgets(const QModelIndex& index)
{
    // Figure out which groups to show.
    // Show the common group if a valid node is selected.
    // Show the instance group if a valid instance is selected.
    bool common_valid = index.isValid();

    ui_->common_group_->setVisible(common_valid);
    ui_->triangle_group_->setVisible(model_->SelectedNodeIsLeaf());

    // Subtrees are redundant for leaf nodes.
    ui_->label_subtree_max_->setVisible(!model_->SelectedNodeIsLeaf());
    ui_->content_subtree_min_->setVisible(!model_->SelectedNodeIsLeaf());
    ui_->label_subtree_mean_->setVisible(!model_->SelectedNodeIsLeaf());
    ui_->content_subtree_mean_->setVisible(!model_->SelectedNodeIsLeaf());
}

void BlasViewerPane::UpdateSelectedBlas()
{
    last_selected_as_id_ = AccelerationStructureViewerPane::UpdateSelectedBvh();
    if (last_selected_as_id_ != UINT64_MAX)
    {
        const rra::Scene* scene = model_->GetSceneCollectionModel()->GetSceneByIndex(last_selected_as_id_);
        ui_->tree_depth_slider_->SetLowerValue(scene->GetDepthRangeLowerBound());
        ui_->tree_depth_slider_->SetUpperValue(scene->GetDepthRangeUpperBound());
        ui_->tree_depth_slider_->SetUpperBound(scene->GetSceneStatistics().max_node_depth);

        ui_->blas_tree_->SetViewerModel(model_, last_selected_as_id_);
        ui_->expand_collapse_tree_->SetCurrentItemIndex(rra::AccelerationStructureViewerModel::TreeViewExpandMode::kCollapsed);

        int current_row = acceleration_structure_combo_box_->CurrentRow();
        int row         = model_->FindRowFromAccelerationStructureIndex(last_selected_as_id_);
        if (current_row != row)
        {
            acceleration_structure_combo_box_->SetSelectedRow(row);
            renderer_interface_->MarkAsDirty();
        }

        emit rra::MessageManager::Get().BlasSelected(last_selected_as_id_);
    }
}

void BlasViewerPane::SelectLeafNode(const uint64_t blas_index, const bool navigate_to_triangles_pane)
{
    if (blas_index != ULLONG_MAX)
    {
        SelectLeafNode(navigate_to_triangles_pane);
    }
}

void BlasViewerPane::SelectLeafNode(const bool navigate_to_triangles_pane)
{
    rra::Scene* scene = model_->GetSceneCollectionModel()->GetSceneByIndex(last_selected_as_id_);
    if (scene)
    {
        uint32_t node_id = scene->GetMostRecentSelectedNodeId();

        // Select the selected triangle in the Triangles pane.
        emit rra::MessageManager::Get().TriangleViewerSelected(node_id);

        if (navigate_to_triangles_pane)
        {
            // Switch to the triangles pane.
            emit rra::MessageManager::Get().PaneSwitchRequested(rra::kPaneIdBlasTriangles);
        }
    }
}

void BlasViewerPane::SelectTriangle(uint32_t triangle_node_id)
{
    rra::Scene* scene = model_->GetSceneCollectionModel()->GetSceneByIndex(last_selected_as_id_);
    if (scene)
    {
        // Select the triangle node in the scene.
        scene->SetSceneSelection(triangle_node_id);

        QModelIndex selected_index = QModelIndex();
        if (scene->HasSelection())
        {
            selected_index = model_->GetModelIndexForNode(scene->GetMostRecentSelectedNodeId());
            SelectTreeItem(ui_->blas_tree_, selected_index);
        }
    }
}

void BlasViewerPane::SelectParentNode(bool checked)
{
    Q_UNUSED(checked);

    QModelIndexList indexes = ui_->blas_tree_->selectionModel()->selectedIndexes();
    if (indexes.size() > 0)
    {
        const QModelIndex& selected_index = indexes.at(0);
        ui_->blas_tree_->selectionModel()->reset();
        ui_->blas_tree_->selectionModel()->setCurrentIndex(selected_index.parent(), QItemSelectionModel::Select);
    }
}

void BlasViewerPane::TreeNodeChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    const QModelIndexList& selected_indices = selected.indexes();
    if (selected_indices.size() > 0)
    {
        const QModelIndex& model_index = selected_indices[0];
        bool               is_root     = !model_index.parent().isValid();

        // We only show the parent address if the selected node is not the root node.
        ui_->label_parent_blas_->setVisible(!is_root);
        ui_->content_parent_blas_->setVisible(!is_root);
    }

    AccelerationStructureViewerPane::SelectedTreeNodeChanged(selected, deselected);
    SelectLeafNode(false);
}
