//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the BLAS viewer pane.
//=============================================================================

#include "views/blas/blas_viewer_pane.h"

#include "constants.h"
#include "managers/message_manager.h"
#include "models/acceleration_structure_tree_view_item.h"
#include "settings/settings.h"
#include "views/widget_util.h"

static const int kSplitterWidth = 300;

BlasViewerPane::BlasViewerPane(QWidget* parent)
    : AccelerationStructureViewerPane(parent)
    , ui_(new Ui::BlasViewerPane)
{
    ui_->setupUi(this);
    ui_->side_panel_container_->GetViewPane()->SetParentPaneId(rra::kPaneIdBlasViewer);
    ui_->viewer_container_widget_->SetupUI(this, rra::kPaneIdBlasViewer);

    rra::widget_util::ApplyStandardPaneStyle(this, ui_->main_content_, ui_->main_scroll_area_);
    ui_->blas_tree_->setIndentation(rra::kTreeViewIndent);
    ui_->blas_tree_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui_->blas_tree_->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui_->blas_tree_->header()->setStretchLastSection(false);
    ui_->blas_tree_->installEventFilter(this);

    ui_->expand_collapse_tree_->Init(QStringList({rra::text::kTextExpandTree, rra::text::kTextCollapseTree}));
    ui_->expand_collapse_tree_->setCursor(Qt::PointingHandCursor);

    int        size           = kSplitterWidth;
    QList<int> splitter_sizes = {size, size};
    ui_->splitter_->setSizes(splitter_sizes);

    SetTableParams(ui_->extents_table_);
    SetTableParams(ui_->geometry_flags_table_1_);
    SetTableParams(ui_->vertex_table_1_);
    SetTableParams(ui_->vertex_table_2_);
    ui_->geometry_flags_table_1_->horizontalHeader()->setStretchLastSection(true);

    derived_model_                    = new rra::BlasViewerModel(ui_->blas_tree_);
    model_                            = derived_model_;
    acceleration_structure_combo_box_ = ui_->content_bvh_;

    ui_->content_parent_blas_->setCursor(Qt::PointingHandCursor);

    // Initialize tables.
    model_->InitializeExtentsTableModel(ui_->extents_table_);
    derived_model_->InitializeFlagsTableModel(ui_->geometry_flags_table_1_);
    derived_model_->InitializeVertexTableModels(ui_->vertex_table_1_, ui_->vertex_table_2_);

    model_->InitializeModel(ui_->content_node_address_, rra::kBlasStatsAddress, "text");
    model_->InitializeModel(ui_->content_node_type_, rra::kBlasStatsType, "text");
    model_->InitializeModel(ui_->content_focus_selected_volume_, rra::kBlasStatsFocus, "visible");
    model_->InitializeModel(ui_->content_current_sah_, rra::kBlasStatsCurrentSAH, "text");
    model_->InitializeModel(ui_->content_subtree_min_, rra::kBlasStatsSAHSubTreeMax, "text");
    model_->InitializeModel(ui_->content_subtree_mean_, rra::kBlasStatsSAHSubTreeMean, "text");
    model_->InitializeModel(ui_->content_primitive_index_1_, rra::kBlasStatsPrimitiveIndexTriangle1, "text");
    model_->InitializeModel(ui_->content_primitive_index_2_, rra::kBlasStatsPrimitiveIndexTriangle2, "text");
    model_->InitializeModel(ui_->content_geometry_index_1_, rra::kBlasStatsGeometryIndex, "text");
    model_->InitializeModel(ui_->content_parent_blas_, rra::kBlasStatsParent, "text");

    ui_->triangle_split_info_->setCursor(Qt::PointingHandCursor);
    ui_->triangle_split_info_->hide();

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
    // Save selected control style to settings.
    connect(ui_->side_panel_container_->GetViewPane(), &ViewPane::ControlStyleChanged, this, [&]() {
        if (renderer_interface_ == nullptr)
        {
            return;
        }
        rra::renderer::Camera& camera = renderer_interface_->GetCamera();

        auto camera_controller = static_cast<rra::ViewerIO*>(camera.GetCameraController());
        if (camera_controller && (acceleration_structure_combo_box_->RowCount() > 0))
        {
            rra::Settings::Get().SetControlStyle(rra::kPaneIdBlasViewer, (ControlStyleType)camera_controller->GetComboBoxIndex());
        }
    });
    connect(ui_->side_panel_container_->GetViewPane(), &ViewPane::RenderModeChanged, [=](bool geometry_mode) {
        ui_->viewer_container_widget_->ShowColoringMode(geometry_mode);
    });
    connect(&rra::MessageManager::Get(), &rra::MessageManager::TriangleTableSelected, this, &BlasViewerPane::SelectTriangle);
    connect(ui_->content_parent_blas_, &ScaledPushButton::clicked, this, &BlasViewerPane::SelectParentNode);

    // Reset the UI state. When the 'reset' button is clicked, it broadcasts a message from the message manager. Any objects interested in this
    // message can then act upon it.
    connect(&rra::MessageManager::Get(), &rra::MessageManager::ResetUIState, [=](rra::RRAPaneId pane) {
        if (pane == rra::kPaneIdBlasViewer)
        {
            ui_->side_panel_container_->GetViewPane()->ApplyUIStateFromSettings(rra::kPaneIdBlasViewer);
            ui_->viewer_container_widget_->ApplyUIStateFromSettings(rra::kPaneIdBlasViewer);
        }
    });

    ui_->side_panel_container_->MarkAsBLAS();

    flag_table_delegate_ = new FlagTableItemDelegate();
    ui_->geometry_flags_table_1_->setItemDelegate(flag_table_delegate_);

    ui_->content_focus_selected_volume_->SetNormalIcon(QIcon(":/Resources/assets/third_party/ionicons/scan-outline-clickable.svg"));
    ui_->content_focus_selected_volume_->SetHoverIcon(QIcon(":/Resources/assets/third_party/ionicons/scan-outline-hover.svg"));
    ui_->content_focus_selected_volume_->setBaseSize(QSize(25, 25));
    connect(ui_->content_focus_selected_volume_, &ScaledPushButton::clicked, [&]() { model_->GetCameraController()->FocusOnSelection(); });
    ui_->content_focus_selected_volume_->setCursor(QCursor(Qt::PointingHandCursor));
}

BlasViewerPane::~BlasViewerPane()
{
    delete flag_table_delegate_;
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
    ui_->side_panel_container_->GetViewPane()->SetControlStyle(rra::Settings::Get().GetControlStyle(rra::kPaneIdBlasViewer));
    AccelerationStructureViewerPane::showEvent(event);
}

void BlasViewerPane::SetBlasSelection(uint64_t blas_index)
{
    last_selected_as_id_ = blas_index;

    uint32_t procedural_node_count = derived_model_->GetProceduralNodeCount(blas_index);
    ui_->side_panel_container_->MarkProceduralGeometry(procedural_node_count > 0);
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

void BlasViewerPane::UpdateWidgets(const QModelIndex& index)
{
    // Figure out which groups to show.
    // Show the common group if a valid node is selected.
    // Show the instance group if a valid instance is selected.
    bool common_valid = index.isValid();

    ui_->common_group_->setVisible(common_valid);
    ui_->triangle_group_->setVisible(model_->SelectedNodeIsLeaf());
    ui_->triangle_information_2_->setVisible(derived_model_->SelectedNodeHasSecondTriangle());
    ui_->triangle_split_group_->setVisible(model_->IsTriangleSplit(last_selected_as_id_));

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

        rra::SceneNode* node = scene->GetNodeById(node_id);
        UpdateTriangleSplitUI(scene, last_selected_as_id_, node->GetGeometryIndex(), node->GetPrimitiveIndex(), node_id);

        if (navigate_to_triangles_pane)
        {
            // Switch to the triangles pane.
            emit rra::MessageManager::Get().PaneSwitchRequested(rra::kPaneIdBlasTriangles);
        }
    }
}

void BlasViewerPane::UpdateTriangleSplitUI(rra::Scene* scene, uint32_t blas_index, uint32_t geometry_index, uint32_t primitive_index, uint32_t node_id)
{
    if (derived_model_->SelectedNodeIsLeaf())
    {
        if (scene->IsTriangleSplit(geometry_index, primitive_index))
        {
            const auto& split_siblings = scene->GetSplitTriangles(geometry_index, primitive_index);

            // Delete previous buttons.
            for (ScaledPushButton* button : split_triangle_sibling_buttons_)
            {
                ui_->split_triangle_siblings_list_->removeWidget(button);
                delete button;
            }
            split_triangle_sibling_buttons_.clear();

            uint32_t           row{0};
            uint32_t           col{0};
            constexpr uint32_t col_count{2};
            for (rra::SceneNode* sibling : split_siblings)
            {
                ScaledPushButton* rebraid_sibling_button = new ScaledPushButton();
                split_triangle_sibling_buttons_.push_back(rebraid_sibling_button);
                rebraid_sibling_button->setText(model_->AddressString(blas_index, sibling->GetId()));
                rebraid_sibling_button->setObjectName(QString::fromUtf8("rebraid_button_"));
                QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                sizePolicy.setHorizontalStretch(0);
                sizePolicy.setVerticalStretch(0);
                sizePolicy.setHeightForWidth(rebraid_sibling_button->sizePolicy().hasHeightForWidth());
                rebraid_sibling_button->setSizePolicy(sizePolicy);

                if (sibling->GetId() != node_id)
                {
                    rebraid_sibling_button->setCursor(Qt::PointingHandCursor);

                    QModelIndex sibling_model_index = derived_model_->GetModelIndexForNode(sibling->GetId());
                    connect(rebraid_sibling_button, &ScaledPushButton::clicked, this, [=]() {
                        ui_->blas_tree_->selectionModel()->reset();
                        ui_->blas_tree_->selectionModel()->setCurrentIndex(sibling_model_index, QItemSelectionModel::Select);
                    });
                }
                else
                {
                    rebraid_sibling_button->setDisabled(true);
                }

                ui_->split_triangle_siblings_list_->addWidget(rebraid_sibling_button, row, col);

                if (++col == col_count)
                {
                    col = 0;
                    ++row;
                }
            }
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

        rra::SceneNode* node = scene->GetNodeById(triangle_node_id);
        UpdateTriangleSplitUI(scene, last_selected_as_id_, node->GetGeometryIndex(), node->GetPrimitiveIndex(), triangle_node_id);
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
