//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the BLAS viewer pane.
//=============================================================================

#include "views/blas/blas_viewer_pane.h"

#include <QTimer>

#include "qt_common/utils/qt_util.h"

#include "public/rra_blas.h"
#include "public/rra_rtip_info.h"

#include "ui_triangle_group.h"

#include "constants.h"
#include "managers/message_manager.h"
#include "models/acceleration_structure_tree_view_item.h"
#include "settings/settings.h"
#include "util/rra_util.h"
#include "views/widget_util.h"

#undef min

static const int kSplitterWidth = 300;

BlasViewerPane::BlasViewerPane(QWidget* parent)
    : AccelerationStructureViewerPane(parent)
    , ui_(new Ui::BlasViewerPane)
{
    ui_->setupUi(this);
    ui_->side_panel_container_->GetViewPane()->SetParentPaneId(rra::kPaneIdBlasViewer);
    ui_->viewer_container_widget_->SetupUI(this, rra::kPaneIdBlasViewer);

    rra::widget_util::ApplyStandardPaneStyle(ui_->main_scroll_area_);
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
    SetTableParams(ui_->geometry_flags_table_);
    ui_->geometry_flags_table_->horizontalHeader()->setStretchLastSection(true);

    derived_model_                    = new rra::BlasViewerModel(ui_->blas_tree_);
    model_                            = derived_model_;
    acceleration_structure_combo_box_ = ui_->content_bvh_;

    ui_->content_parent_blas_->setCursor(Qt::PointingHandCursor);

    // Create list of triangle widgets to use when a triangle node is selected.
    Ui_TriangleGroup triangle_group{};
    ui_->triangle_list_->setSpacing(0);
    ui_->triangle_scroll_area_->setWidgetResizable(true);
    uint32_t max_tri_count{MAX_TRIANGLES};
    for (uint32_t i{0}; i < max_tri_count; ++i)
    {
        QWidget* triangle_widget = new QWidget();
        triangle_widgets_.push_back(triangle_widget);
        triangle_group.setupUi(triangle_widget);

        // Set triangle_group data here.
        SetTableParams(triangle_group.vertex_table_);
        model_->InitializeModel(triangle_group.label_primitive_index_, rra::kBlasStatsPrimitiveIndexLabel1 + i, "text");
        model_->InitializeModel(triangle_group.content_primitive_index_, rra::kBlasStatsPrimitiveIndexTriangle1 + i, "text");
        derived_model_->InitializeVertexTableModels(triangle_group.vertex_table_);

        ui_->triangle_list_->addWidget(triangle_widget);
    }

    // Initialize tables.
    model_->InitializeExtentsTableModel(ui_->extents_table_);
    derived_model_->InitializeFlagsTableModel(ui_->geometry_flags_table_);

    model_->InitializeRotationTableModel(ui_->bottom_table_);
    SetTableParams(ui_->bottom_table_);
    ui_->label_bottom_table_->setText("Bounding box orientation");
    ui_->label_bottom_table_->setToolTip("The rotation matrix of the bounding volume.");
    ui_->label_bottom_table_->setVisible(false);
    ui_->bottom_table_->setVisible(false);

    model_->InitializeModel(ui_->content_node_address_, rra::kBlasStatsAddress, "text");
    model_->InitializeModel(ui_->content_node_type_, rra::kBlasStatsType, "text");
    model_->InitializeModel(ui_->content_focus_selected_volume_, rra::kBlasStatsFocus, "visible");
    ui_->node_pointer_group_->hide();
    model_->InitializeModel(ui_->content_current_sah_, rra::kBlasStatsCurrentSAH, "text");
    model_->InitializeModel(ui_->content_subtree_min_, rra::kBlasStatsSAHSubTreeMax, "text");
    model_->InitializeModel(ui_->content_subtree_mean_, rra::kBlasStatsSAHSubTreeMean, "text");
    model_->InitializeModel(ui_->content_geometry_index_, rra::kBlasStatsGeometryIndex, "text");
    model_->InitializeModel(ui_->content_parent_blas_, rra::kBlasStatsParent, "text");
    model_->InitializeModel(ui_->content_packed_ref_count_, rra::kBlasStatsPackedRefCount, "text");

    ui_->content_parent_blas_->SetLinkStyleSheet();

    // RTIP3.1 node-packing metric; shown only when the selected node is shared by multiple parent slots.
    ui_->packed_ref_group_->hide();

    ui_->triangle_split_info_->setCursor(Qt::PointingHandCursor);
    ui_->triangle_split_info_->hide();

    connect(acceleration_structure_combo_box_, &ArrowIconComboBox::SelectionChanged, this, &BlasViewerPane::UpdateSelectedBlas);
    connect(ui_->blas_tree_->selectionModel(), &QItemSelectionModel::selectionChanged, this, &BlasViewerPane::TreeNodeChanged);
    connect(ui_->blas_tree_, &QAbstractItemView::doubleClicked, [=, this]() { this->SelectLeafNode(true); });
    connect(&rra::MessageManager::Get(), &rra::MessageManager::BlasSelected, this, &BlasViewerPane::SetBlasSelection);
    connect(model_, &rra::AccelerationStructureViewerModel::SceneSelectionChanged, [=, this]() { this->UpdateSceneSelection(ui_->blas_tree_); });
    connect(ui_->expand_collapse_tree_, &ScaledCycleButton::Clicked, model_, &rra::AccelerationStructureViewerModel::ExpandCollapseTreeView);
    connect(ui_->search_box_, &TextSearchWidget::textChanged, model_, &rra::AccelerationStructureViewerModel::SearchTextChanged);

    ui_->tree_depth_slider_->setCursor(Qt::PointingHandCursor);
    connect(ui_->tree_depth_slider_, &DepthSliderWidget::SpanChanged, this, &BlasViewerPane::UpdateTreeDepths);

    connect(ui_->side_panel_container_->GetViewPane(), &ViewPane::ShowBoundsChanged, this, &AccelerationStructureViewerPane::UpdateShowBoundingVolumes);

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
    connect(ui_->side_panel_container_->GetViewPane(), &ViewPane::RenderModeChanged, [=, this](bool geometry_mode) {
        ui_->viewer_container_widget_->ShowColoringMode(geometry_mode);
    });
    connect(&rra::MessageManager::Get(), &rra::MessageManager::TriangleTableSelected, this, &BlasViewerPane::SelectTriangle);
    connect(ui_->content_parent_blas_, &ScaledPushButton::clicked, this, &BlasViewerPane::SelectParentNode);

    // Reset the UI state. When the 'reset' button is clicked, it broadcasts a message from the message manager. Any objects interested in this
    // message can then act upon it.
    connect(&rra::MessageManager::Get(), &rra::MessageManager::ResetUIState, [=, this](rra::RRAPaneId pane) {
        if (pane == rra::kPaneIdBlasViewer)
        {
            ui_->side_panel_container_->GetViewPane()->ApplyUIStateFromSettings(rra::kPaneIdBlasViewer);
            ui_->viewer_container_widget_->ApplyUIStateFromSettings(rra::kPaneIdBlasViewer);
        }
    });

    ui_->side_panel_container_->MarkAsBLAS();

    flag_table_delegate_ = new FlagTableItemDelegate();
    ui_->geometry_flags_table_->setItemDelegate(flag_table_delegate_);

    if (QtCommon::QtUtils::ColorTheme::Get().GetColorTheme() == ColorThemeType::kColorThemeTypeDark)
    {
        ui_->content_focus_selected_volume_->SetNormalIcon(QIcon(":/Resources/assets/third_party/ionicons/scan-outline-clickable-dark-theme.svg"));
    }
    else
    {
        ui_->content_focus_selected_volume_->SetNormalIcon(QIcon(":/Resources/assets/third_party/ionicons/scan-outline-clickable.svg"));
    }

    connect(&QtCommon::QtUtils::ColorTheme::Get(), &QtCommon::QtUtils::ColorTheme::ColorThemeUpdated, this, &BlasViewerPane::OnColorThemeUpdated);

    ui_->content_focus_selected_volume_->SetHoverIcon(QIcon(":/Resources/assets/third_party/ionicons/scan-outline-hover.svg"));
    ui_->content_focus_selected_volume_->setBaseSize(QSize(25, 25));
    connect(ui_->content_focus_selected_volume_, &ScaledPushButton::clicked, [&]() { model_->GetCameraController()->FocusOnSelection(); });
    ui_->content_focus_selected_volume_->setCursor(QCursor(Qt::PointingHandCursor));
}

BlasViewerPane::~BlasViewerPane()
{
    delete flag_table_delegate_;
    delete ui_;
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

    for (QWidget* widget : triangle_widgets_)
    {
        ui_->triangle_list_->removeWidget(widget);
        delete widget;
    }
    triangle_widgets_.clear();

    AccelerationStructureViewerPane::OnTraceClose();
}

void BlasViewerPane::OnTraceOpen()
{
    InitializeRendererWidget(ui_->blas_scene_, ui_->side_panel_container_, ui_->viewer_container_widget_, rra::renderer::BvhTypeFlags::BottomLevel);

    last_selected_as_id_ = 0;
    cluster_drill_stack_.clear();
    ui_->side_panel_container_->OnTraceOpen();
    ui_->tree_depth_slider_->SetLowerValue(0);
    ui_->tree_depth_slider_->SetUpperValue(0);
    ui_->viewer_container_widget_->ShowColoringMode(true);
}

void BlasViewerPane::showEvent(QShowEvent* event)
{
    UpdateBvhTypeLabel();
    ui_->side_panel_container_->GetViewPane()->SetControlStyle(rra::Settings::Get().GetControlStyle(rra::kPaneIdBlasViewer));
    AccelerationStructureViewerPane::showEvent(event);
}

void BlasViewerPane::UpdateBvhTypeLabel()
{
    // The BLAS pane inspects three AS tiers of the CLAS hierarchy; label them so the user knows which one they see.
    if (RraBlasIsClusterBlas(last_selected_as_id_))
    {
        ui_->label_bvh_->setText("CBLAS:");
    }
    else if (RraBlasIsCluster(last_selected_as_id_))
    {
        ui_->label_bvh_->setText("CLAS:");
    }
    else
    {
        ui_->label_bvh_->setText("BLAS:");
    }
}

void BlasViewerPane::SetBlasSelection(uint64_t blas_index)
{
    // A fresh BLAS selection from outside the CLAS drill machinery (a TLAS drill-in or a manual combo-box pick)
    // starts a new inspection chain, so discard any pending CLAS drill-down history. In-drill switches keep it.
    if (!in_cluster_nav_)
    {
        cluster_drill_stack_.clear();
    }

    last_selected_as_id_ = blas_index;

    uint32_t procedural_node_count = derived_model_->GetProceduralNodeCount(blas_index);
    ui_->side_panel_container_->MarkProceduralGeometry(procedural_node_count > 0);
}

void BlasViewerPane::SelectClusterBlas(uint64_t clas_blas_index)
{
    // Remember the CBLAS (or parent CLAS) we're drilling from so Back can walk CLAS -> CBLAS -> TLAS.
    cluster_drill_stack_.push_back(last_selected_as_id_);
    SwitchToClusterBlas(clas_blas_index);
}

bool BlasViewerPane::PopClusterDrillBack()
{
    if (cluster_drill_stack_.empty())
    {
        return false;
    }

    uint64_t parent_blas_index = cluster_drill_stack_.back();
    cluster_drill_stack_.pop_back();
    SwitchToClusterBlas(parent_blas_index);
    return true;
}

void BlasViewerPane::SwitchToClusterBlas(uint64_t blas_index)
{
    // Switch the AS combo box to the target row, which reloads the viewer scene and tree via UpdateSelectedBlas.
    int target_row = -1;
    for (int i = 0; i < acceleration_structure_combo_box_->RowCount(); ++i)
    {
        QListWidgetItem* item = acceleration_structure_combo_box_->FindItem(i);
        if (item && item->data(Qt::UserRole).toULongLong() == blas_index)
        {
            target_row = i;
            break;
        }
    }
    if (target_row < 0)
    {
        return;
    }

    // Guard so the BlasSelected emissions below don't clear the drill stack we're maintaining.
    in_cluster_nav_ = true;

    // Keep the BLAS sub-panes (properties/geometries/instances/triangles) in sync with the drilled-into AS.
    emit rra::MessageManager::Get().BlasSelected(blas_index);

    acceleration_structure_combo_box_->SetSelectedRow(target_row);
    UpdateSelectedBlas();

    in_cluster_nav_ = false;
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

rra::BlasSceneCollectionModel* BlasViewerPane::GetSceneCollection()
{
    return (rra::BlasSceneCollectionModel*)model_->GetSceneCollectionModel();
}

void BlasViewerPane::SetBlasRootNodes(std::vector<rra::SceneNode*>* blas_root_nodes)
{
    model_->SetBlasRootNodes(blas_root_nodes);
}

void BlasViewerPane::UpdateWidgets(const QModelIndex& index)
{
    // Figure out which groups to show.
    // Show the common group if a valid node is selected.
    // Show the instance group if a valid instance is selected.
    bool common_valid = index.isValid();

    ui_->common_group_->setVisible(common_valid);
    ui_->geometry_information_->setVisible(model_->SelectedNodeIsLeaf());
    ui_->triangle_scroll_area_->setVisible(model_->SelectedNodeIsLeaf());
    ui_->triangle_split_group_->setVisible(model_->IsTriangleSplit(last_selected_as_id_));
    ui_->packed_ref_group_->setVisible(common_valid && derived_model_->SelectedNodePackedRefCount() > 1);

    uint32_t tri_count{derived_model_->SelectedNodeTriangleCount()};
    ui_->triangle_scroll_area_->setMinimumHeight(150 * std::min(tri_count, 2u));
    for (uint32_t i{0}; i < (uint32_t)triangle_widgets_.size(); ++i)
    {
        triangle_widgets_[i]->setVisible(i < tri_count);
    }

    // Subtrees are redundant for leaf nodes. A CBLAS cluster-ref leaf is a hardware instance node with no
    // surface-area heuristic; repurpose the top SAH row to show the instance mask, hide the sub-tree rows, and
    // show the leaf's world-to-object transform in the bottom matrix table (mirroring the TLAS instance view).
    const bool is_cluster_ref = derived_model_->SelectedNodeIsClusterRef();
    const bool show_subtree   = !model_->SelectedNodeIsLeaf() && !is_cluster_ref;
    ui_->label_current_sah_->setVisible(true);
    ui_->content_current_sah_->setVisible(true);
    ui_->label_current_sah_->setText(is_cluster_ref ? "Instance mask" : "Surface area heuristic");
    ui_->label_subtree_max_->setVisible(show_subtree);
    ui_->content_subtree_min_->setVisible(show_subtree);
    ui_->label_subtree_mean_->setVisible(show_subtree);
    ui_->content_subtree_mean_->setVisible(show_subtree);

    if (is_cluster_ref)
    {
        ui_->label_bottom_table_->setText("Instance transform");
        ui_->label_bottom_table_->setToolTip("The world-to-object transform of this CLAS reference.");
        ui_->label_bottom_table_->setVisible(true);
        ui_->bottom_table_->setVisible(true);
    }
    else
    {
        ui_->label_bottom_table_->setText("Bounding box orientation");
        ui_->label_bottom_table_->setToolTip("The rotation matrix of the bounding volume.");
        ui_->label_bottom_table_->setVisible(RraRtipInfoGetOBBSupported());
        ui_->bottom_table_->setVisible(RraRtipInfoGetOBBSupported());
    }
}

std::vector<rra::SceneNode*>* BlasViewerPane::GetBlasRootNodes()
{
    return model_->GetBlasRootNodes();
}

void BlasViewerPane::UpdateSelectedBlas()
{
    last_selected_as_id_ = AccelerationStructureViewerPane::UpdateSelectedBvh();
    if (last_selected_as_id_ != UINT64_MAX)
    {
        rra::Scene* scene = model_->GetSceneCollectionModel()->GetSceneByIndex(last_selected_as_id_);
        ui_->tree_depth_slider_->SetLowerValue(scene->GetDepthRangeLowerBound());
        ui_->tree_depth_slider_->SetUpperValue(scene->GetDepthRangeUpperBound());
        ui_->tree_depth_slider_->SetUpperBound(scene->GetSceneStatistics().max_node_depth);

        ui_->blas_tree_->SetViewerModel(model_, last_selected_as_id_);
        ui_->viewer_container_widget_->SetScene(scene);
        ui_->expand_collapse_tree_->SetCurrentItemIndex(rra::AccelerationStructureViewerModel::TreeViewExpandMode::kCollapsed);

        UpdateBvhTypeLabel();

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
        uint64_t        node_id = scene->GetMostRecentSelectedNodeId();
        rra::SceneNode* node    = scene->GetNodeById(node_id);

        // If the node isn't visible or enabled (it's grayed out), then don't select anything.
        if (node && !(node->IsEnabled() && node->IsVisible()))
        {
            return;
        }

        // A Cluster BLAS (CBLAS) cluster-ref leaf references a CLAS, which is itself a BLAS index. Double-clicking
        // drills into that CLAS within this same BLAS pane (nested instancing); single-clicking just leaves it
        // selected (already applied to the scene) with no triangle-pane side effects, since a cluster-ref leaf
        // has no triangle of its own.
        if (RraBlasIsClusterRefNode(last_selected_as_id_, static_cast<uint32_t>(node_id)))
        {
            if (navigate_to_triangles_pane)
            {
                uint64_t clas_blas_index = 0;
                if (RraBlasGetClasIndexFromClusterRefNode(last_selected_as_id_, static_cast<uint32_t>(node_id), &clas_blas_index) == kRraOk)
                {
                    SelectClusterBlas(clas_blas_index);
                }
            }
            return;
        }

        // Select the selected triangle in the Triangles pane.
        emit rra::MessageManager::Get().TriangleViewerSelected(node_id);

        if (node != nullptr)
        {
            UpdateTriangleSplitUI(scene, last_selected_as_id_, node->GetGeometryIndex(), node->GetPrimitiveIndex(), node_id);
        }

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
                    connect(rebraid_sibling_button, &ScaledPushButton::clicked, this, [=, this]() {
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

void BlasViewerPane::SelectTriangle(uint64_t triangle_node_id)
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
        if (node != nullptr)
        {
            UpdateTriangleSplitUI(scene, last_selected_as_id_, node->GetGeometryIndex(), node->GetPrimitiveIndex(), triangle_node_id);
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

        // RTIP3.1 node packing: a "Referenced subtree" placeholder redirects to the canonical (primary) node that owns
        // the shared subtree. Selecting it jumps the tree to that canonical node, which then drives the normal
        // selection/highlight path via the re-fired selection change.
        if (model_ != nullptr && model_->IsModelIndexReferencePlaceholder(model_index))
        {
            // The tree model's data() only exposes DisplayRole/ToolTipRole, so read the canonical key from the display
            // payload (node_child_id) rather than UserRole, which the model returns empty.
            const auto        item_data       = qvariant_cast<rra::AccelerationStructureTreeViewItemData>(model_index.data(Qt::DisplayRole));
            const uint64_t    canonical_id    = item_data.node_child_id;
            const QModelIndex canonical_index = model_->GetModelIndexForNode(canonical_id);
            if (canonical_index.isValid())
            {
                // Defer the jump to the next event-loop tick: changing the selection synchronously inside this
                // selectionChanged handler gets clobbered when the in-flight click finishes, leaving the placeholder as
                // the active (blue) row. setCurrentIndex moves both the current row and the selection so the tree
                // scrolls to and focuses the canonical node, which then drives the normal highlight path.
                QTreeView*                  tree = ui_->blas_tree_;
                const QPersistentModelIndex target(canonical_index);
                QTimer::singleShot(0, this, [this, tree, target]() {
                    if (target.isValid())
                    {
                        QModelIndex idx(target);
                        // SelectTreeItem expands collapsed ancestors and scrolls the canonical node into view;
                        // setCurrentIndex then moves the active (blue) row and re-fires the normal highlight path.
                        SelectTreeItem(tree, idx);
                        tree->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                    }
                });
                return;
            }
        }

        bool is_root = !model_index.parent().isValid();

        // We only show the parent address if the selected node is not the root node.
        ui_->label_parent_blas_->setVisible(!is_root);
        ui_->content_parent_blas_->setVisible(!is_root);
    }

    AccelerationStructureViewerPane::SelectedTreeNodeChanged(selected, deselected);
    SelectLeafNode(false);
}

void BlasViewerPane::OnColorThemeUpdated()
{
    if (QtCommon::QtUtils::ColorTheme::Get().GetColorTheme() == ColorThemeType::kColorThemeTypeDark)
    {
        ui_->content_focus_selected_volume_->SetNormalIcon(QIcon(":/Resources/assets/third_party/ionicons/scan-outline-clickable-dark-theme.svg"));
    }
    else
    {
        ui_->content_focus_selected_volume_->SetNormalIcon(QIcon(":/Resources/assets/third_party/ionicons/scan-outline-clickable.svg"));
    }
}

void BlasViewerPane::UpdateToolTip(QString tool_tip_string)
{
    rra_util::UpdateRendererTooltip(rra::kPaneIdBlasViewer, ui_->blas_scene_, tool_tip_string);
}

