//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the TLAS viewer pane.
//=============================================================================

#include "views/tlas/tlas_viewer_pane.h"

#include "constants.h"
#include "managers/message_manager.h"
#include "models/acceleration_structure_tree_view_item.h"
#include "models/acceleration_structure_viewer_model.h"
#include "models/tlas/tlas_viewer_model.h"
#include "settings/settings.h"
#include "views/widget_util.h"
#include "public/rra_api_info.h"

static const int kSplitterWidth = 300;

TlasViewerPane::TlasViewerPane(QWidget* parent)
    : AccelerationStructureViewerPane(parent)
    , ui_(new Ui::TlasViewerPane)
{
    ui_->setupUi(this);
    ui_->side_panel_container_->GetViewPane()->SetParentPaneId(rra::kPaneIdTlasViewer);
    ui_->viewer_container_widget_->SetupUI(this, rra::kPaneIdTlasViewer);

    rra::widget_util::ApplyStandardPaneStyle(this, ui_->main_content_, ui_->main_scroll_area_);
    ui_->tlas_tree_->setIndentation(rra::kTreeViewIndent);
    ui_->tlas_tree_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui_->tlas_tree_->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui_->tlas_tree_->header()->setStretchLastSection(false);
    ui_->tlas_tree_->installEventFilter(this);

    ui_->expand_collapse_tree_->Init(QStringList({rra::text::kTextExpandTree, rra::text::kTextCollapseTree}));
    ui_->expand_collapse_tree_->setCursor(Qt::PointingHandCursor);

    int        size           = kSplitterWidth;
    QList<int> splitter_sizes = {size, size};
    ui_->splitter_->setSizes(splitter_sizes);

    SetTableParams(ui_->extents_table_);
    SetTableParams(ui_->flags_table_);
    SetTableParams(ui_->transform_table_);
    SetTableParams(ui_->position_table_);
    ui_->flags_table_->horizontalHeader()->setStretchLastSection(true);

    derived_model_                    = new rra::TlasViewerModel(ui_->tlas_tree_);
    model_                            = derived_model_;
    acceleration_structure_combo_box_ = ui_->content_bvh_;

    ui_->content_blas_address_->setCursor(Qt::PointingHandCursor);
    ui_->content_parent_->setCursor(Qt::PointingHandCursor);
    ui_->rebraid_info_->setCursor(Qt::PointingHandCursor);
    ui_->rebraid_info_->hide();

    ui_->side_panel_container_->GetViewPane()->HideRAYWidgets();

    // Initialize tables.
    model_->InitializeExtentsTableModel(ui_->extents_table_);
    derived_model_->InitializeFlagsTableModel(ui_->flags_table_);
    derived_model_->InitializeTransformTableModel(ui_->transform_table_);
    derived_model_->InitializePositionTableModel(ui_->position_table_);

    model_->InitializeModel(ui_->content_node_address_, rra::kTlasStatsAddress, "text");
    model_->InitializeModel(ui_->content_node_type_, rra::kTlasStatsType, "text");
    model_->InitializeModel(ui_->content_focus_selected_volume_, rra::kTlasStatsFocus, "visible");
    model_->InitializeModel(ui_->content_blas_address_, rra::kTlasStatsBlasAddress, "text");
    model_->InitializeModel(ui_->content_parent_, rra::kTlasStatsParent, "text");
    model_->InitializeModel(ui_->content_instance_index_, rra::kTlasStatsInstanceIndex, "text");
    model_->InitializeModel(ui_->content_instance_id_, rra::kTlasStatsInstanceId, "text");
    model_->InitializeModel(ui_->content_instance_mask_, rra::kTlasStatsInstanceMask, "text");
    model_->InitializeModel(ui_->content_instance_hit_group_index_, rra::kTlasStatsInstanceHitGroupIndex, "text");

    connect(ui_->tlas_tree_, &QAbstractItemView::clicked, [=](const QModelIndex& index) { this->SelectBlasFromTree(index, false); });
    connect(ui_->tlas_tree_, &QAbstractItemView::doubleClicked, [=](const QModelIndex& index) { this->SelectBlasFromTree(index, true); });
    connect(ui_->tlas_tree_->selectionModel(), &QItemSelectionModel::selectionChanged, this, &TlasViewerPane::TreeNodeChanged);
    connect(acceleration_structure_combo_box_, &ArrowIconComboBox::SelectionChanged, this, &TlasViewerPane::UpdateSelectedTlas);
    connect(&rra::MessageManager::Get(), &rra::MessageManager::InstancesTableDoubleClicked, this, &TlasViewerPane::SetBlasInstanceSelection);
    connect(model_, &rra::AccelerationStructureViewerModel::SceneSelectionChanged, [=]() { HandleSceneSelectionChanged(); });
    connect(ui_->expand_collapse_tree_, &ScaledCycleButton::Clicked, model_, &rra::AccelerationStructureViewerModel::ExpandCollapseTreeView);
    connect(ui_->search_box_, &TextSearchWidget::textChanged, model_, &rra::AccelerationStructureViewerModel::SearchTextChanged);
    connect(ui_->content_blas_address_, &ScaledPushButton::clicked, this, &TlasViewerPane::GotoBlasPaneFromBlasAddress);
    connect(ui_->content_parent_, &ScaledPushButton::clicked, this, &TlasViewerPane::SelectParentNode);
    connect(&rra::MessageManager::Get(), &rra::MessageManager::TlasSelected, this, &TlasViewerPane::SetTlasIndex);
    connect(&rra::MessageManager::Get(), &rra::MessageManager::TlasAssumeCamera, this, &TlasViewerPane::SetTlasCamera);
    connect(&rra::MessageManager::Get(), &rra::MessageManager::InspectorInstanceSelected, this, &TlasViewerPane::SelectInstance);

    ui_->tree_depth_slider_->setCursor(Qt::PointingHandCursor);
    connect(ui_->tree_depth_slider_, &DepthSliderWidget::SpanChanged, this, &TlasViewerPane::UpdateTreeDepths);

    connect(ui_->side_panel_container_->GetViewPane(), &ViewPane::ControlStyleChanged, this, &TlasViewerPane::UpdateCameraController);
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
            rra::Settings::Get().SetControlStyle(rra::kPaneIdTlasViewer, (ControlStyleType)camera_controller->GetComboBoxIndex());
        }
    });
    connect(ui_->side_panel_container_->GetViewPane(), &ViewPane::RenderModeChanged, [=](bool geometry_mode) {
        ui_->viewer_container_widget_->ShowColoringMode(geometry_mode);
    });

    // Reset the UI state. When the 'reset' button is clicked, it broadcasts a message from the message manager. Any objects interested in this
    // message can then act upon it.
    connect(&rra::MessageManager::Get(), &rra::MessageManager::ResetUIState, [=](rra::RRAPaneId pane) {
        if (pane == rra::kPaneIdTlasViewer)
        {
            ui_->side_panel_container_->GetViewPane()->ApplyUIStateFromSettings(rra::kPaneIdTlasViewer);
            ui_->viewer_container_widget_->ApplyUIStateFromSettings(rra::kPaneIdTlasViewer);
        }
    });

    flag_table_delegate_ = new FlagTableItemDelegate();
    ui_->flags_table_->setItemDelegate(flag_table_delegate_);

    ui_->content_focus_selected_volume_->SetNormalIcon(QIcon(":/Resources/assets/third_party/ionicons/scan-outline-clickable.svg"));
    ui_->content_focus_selected_volume_->SetHoverIcon(QIcon(":/Resources/assets/third_party/ionicons/scan-outline-hover.svg"));
    ui_->content_focus_selected_volume_->setBaseSize(QSize(25, 25));
    connect(ui_->content_focus_selected_volume_, &ScaledPushButton::clicked, [&]() { model_->GetCameraController()->FocusOnSelection(); });
    ui_->content_focus_selected_volume_->setCursor(QCursor(Qt::PointingHandCursor));
}

TlasViewerPane::~TlasViewerPane()
{
    delete flag_table_delegate_;
}

void TlasViewerPane::showEvent(QShowEvent* event)
{
    ui_->label_bvh_->setText("TLAS:");
    ui_->side_panel_container_->GetViewPane()->SetControlStyle(rra::Settings::Get().GetControlStyle(rra::kPaneIdTlasViewer));
    UpdateRebraidUI();
    AccelerationStructureViewerPane::showEvent(event);
}

void TlasViewerPane::OnTraceClose()
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

void TlasViewerPane::OnTraceOpen()
{
    InitializeRendererWidget(ui_->tlas_scene_, ui_->side_panel_container_, ui_->viewer_container_widget_, rra::renderer::BvhTypeFlags::TopLevel);

    last_selected_as_id_ = 0;
    ui_->side_panel_container_->OnTraceOpen();
    AccelerationStructureViewerPane::OnTraceOpen();
    ui_->tree_depth_slider_->SetLowerValue(0);
    ui_->tree_depth_slider_->SetUpperValue(0);
    ui_->viewer_container_widget_->ShowColoringMode(true);

    if (RraApiInfoIsVulkan())
    {
        ui_->label_instance_id_->setText("Instance custom index");
        ui_->label_instance_mask_->setText("Instance mask");
        ui_->label_instance_hit_group_index_->setText("Instance shader binding table record offset");
    }
    else
    {
        ui_->label_instance_id_->setText("Instance ID");
        ui_->label_instance_mask_->setText("Instance mask");
        ui_->label_instance_hit_group_index_->setText("Instance contribution to hit group index");
    }
}

void TlasViewerPane::UpdateTreeDepths(int min_value, int max_value)
{
    rra::Scene* scene = model_->GetSceneCollectionModel()->GetSceneByIndex(last_selected_as_id_);
    if (scene)
    {
        scene->SetDepthRange(min_value, max_value);
    }

    ui_->tree_depth_start_value_->setText(QString::number(min_value));
    ui_->tree_depth_end_value_->setText(QString::number(max_value));
}

void TlasViewerPane::SelectBlasFromTree(const QModelIndex& index, const bool navigate_to_blas_pane)
{
    rra::TlasViewerModel* model = dynamic_cast<rra::TlasViewerModel*>(model_);
    RRA_ASSERT(model != nullptr);
    if (model != nullptr)
    {
        uint64_t acceleration_structure_index = model_->FindAccelerationStructureIndex(acceleration_structure_combo_box_);
        if (acceleration_structure_index != UINT64_MAX)
        {
            uint64_t blas_index = model->GetBlasIndex(acceleration_structure_index, index);
            SelectLeafNode(blas_index, navigate_to_blas_pane);

            // Emit a signal that an instance has been selected.
            uint32_t instance_index = model->GetInstanceIndex(acceleration_structure_index, index);
            emit     rra::MessageManager::Get().InstanceSelected(instance_index);
        }
    }
}

void TlasViewerPane::UpdateRebraidUI()
{
    rra::TlasViewerModel* model = dynamic_cast<rra::TlasViewerModel*>(model_);
    RRA_ASSERT(model != nullptr);
    if (model != nullptr)
    {
        uint64_t tlas_index = model_->FindAccelerationStructureIndex(acceleration_structure_combo_box_);
        if (tlas_index != UINT64_MAX)
        {
            auto scene = model->GetSceneCollectionModel()->GetSceneByIndex(tlas_index);
            if (!scene)
            {
                return;
            }

            auto     node_id        = scene->GetMostRecentSelectedNodeId();
            uint32_t instance_index = model->GetInstanceIndexFromNode(tlas_index, node_id);

            if (derived_model_->SelectedNodeIsLeaf())
            {
                if (scene->IsInstanceRebraided(instance_index))
                {
                    const auto& rebraid_siblings = scene->GetRebraidedInstances(instance_index);

                    // Delete previous buttons.
                    for (ScaledPushButton* button : rebraid_sibling_buttons_)
                    {
                        ui_->rebraid_siblings_list_->removeWidget(button);
                        delete button;
                    }
                    rebraid_sibling_buttons_.clear();

                    uint32_t           row{0};
                    uint32_t           col{0};
                    constexpr uint32_t col_count{2};
                    for (rra::SceneNode* sibling : rebraid_siblings)
                    {
                        ScaledPushButton* rebraid_sibling_button = new ScaledPushButton();
                        rebraid_sibling_buttons_.push_back(rebraid_sibling_button);
                        rebraid_sibling_button->setText(model_->AddressString(tlas_index, sibling->GetId()));
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
                                ui_->tlas_tree_->selectionModel()->reset();
                                ui_->tlas_tree_->selectionModel()->setCurrentIndex(sibling_model_index, QItemSelectionModel::Select);
                                HandleSceneSelectionChanged();
                            });
                        }
                        else
                        {
                            rebraid_sibling_button->setDisabled(true);
                        }

                        ui_->rebraid_siblings_list_->addWidget(rebraid_sibling_button, row, col);

                        if (++col == col_count)
                        {
                            col = 0;
                            ++row;
                        }
                    }
                }
            }
        }
    }
}

void TlasViewerPane::HandleSceneSelectionChanged()
{
    UpdateSceneSelection(ui_->tlas_tree_);

    rra::TlasViewerModel* model = dynamic_cast<rra::TlasViewerModel*>(model_);
    RRA_ASSERT(model != nullptr);
    if (model != nullptr)
    {
        uint64_t acceleration_structure_index = model_->FindAccelerationStructureIndex(acceleration_structure_combo_box_);
        if (acceleration_structure_index != UINT64_MAX)
        {
            auto     scene                 = model->GetSceneCollectionModel()->GetSceneByIndex(acceleration_structure_index);
            auto     selection             = scene->GetMostRecentSelectedNodeId();
            uint32_t unique_instance_index = model->GetInstanceUniqueIndexFromNode(acceleration_structure_index, selection);

            UpdateRebraidUI();

            // Emit a signal to update the selection list if TLAS.
            emit rra::MessageManager::Get().InstanceSelected(unique_instance_index);
        }
    }
}

void TlasViewerPane::SelectLeafNode(const uint64_t blas_index, const bool navigate_to_blas_pane)
{
    rra::TlasViewerModel* model = dynamic_cast<rra::TlasViewerModel*>(model_);
    RRA_ASSERT(model != nullptr);
    if (model != nullptr && model->BlasValid(blas_index))
    {
        // Emit a signal that a BLAS has been selected.
        emit rra::MessageManager::Get().BlasSelected(blas_index);

        if (navigate_to_blas_pane)
        {
            // Switch to the BLAS pane.
            emit rra::MessageManager::Get().PaneSwitchRequested(rra::kPaneIdBlasViewer);
        }
    }
}

void TlasViewerPane::TreeNodeChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    const QModelIndexList& selected_indices = selected.indexes();
    if (selected_indices.size() > 0)
    {
        const QModelIndex& model_index = selected_indices[0];
        bool               is_root     = !model_index.parent().isValid();

        // We only show the parent address if the selected node is not the root node.
        ui_->label_parent_->setVisible(!is_root);
        ui_->content_parent_->setVisible(!is_root);
    }

    AccelerationStructureViewerPane::SelectedTreeNodeChanged(selected, deselected);
}

void TlasViewerPane::SetTlasCamera(glm::vec3 origin, glm::vec3 forward, glm::vec3 up, float fov, float speed)
{
    UpdateCameraController();
    if (last_camera_controller_)
    {
        last_camera_controller_->FitCameraParams(origin, forward, up, fov, speed);
        renderer_interface_->MarkAsDirty();
    }
}

void TlasViewerPane::SelectInstance(uint32_t instance_index)
{
    rra::Scene* scene = model_->GetSceneCollectionModel()->GetSceneByIndex(last_selected_as_id_);

    auto node = scene->GetNodeByInstanceIndex(instance_index);
    if (node == nullptr)
    {
        return;
    }

    scene->SetSceneSelection(node->GetId());

    HandleSceneSelectionChanged();
}

void TlasViewerPane::UpdateWidgets(const QModelIndex& index)
{
    // Figure out which groups to show.
    // Show the common group if a valid node is selected.
    // Show the instance group if a valid instance is selected.
    bool common_valid   = false;
    bool instance_valid = false;
    if (index.isValid())
    {
        common_valid = true;
        if (model_->SelectedNodeIsLeaf())
        {
            instance_valid              = true;
            rra::TlasViewerModel* model = dynamic_cast<rra::TlasViewerModel*>(model_);
            RRA_ASSERT(model != nullptr);
            if (model != nullptr)
            {
                if (last_selected_as_id_ != UINT64_MAX)
                {
                    uint64_t blas_index = model->GetBlasIndex(last_selected_as_id_, index);
                    if (!model->BlasValid(blas_index))
                    {
                        instance_valid = false;
                        common_valid   = false;
                    }
                }
            }
        }
    }

    ui_->common_group_->setVisible(common_valid);
    ui_->instance_group_->setVisible(instance_valid);
    if (model_)
    {
        ui_->rebraid_group_->setVisible(model_->IsRebraidedNode(last_selected_as_id_));
        UpdateRebraidUI();
    }
}

void TlasViewerPane::UpdateSelectedTlas()
{
    last_selected_as_id_ = AccelerationStructureViewerPane::UpdateSelectedBvh();

    rra::Scene* scene = model_->GetSceneCollectionModel()->GetSceneByIndex(last_selected_as_id_);
    ui_->tree_depth_slider_->SetLowerValue(scene->GetDepthRangeLowerBound());
    ui_->tree_depth_slider_->SetUpperValue(scene->GetDepthRangeUpperBound());
    ui_->tree_depth_slider_->SetUpperBound(scene->GetSceneStatistics().max_node_depth);

    ui_->tlas_tree_->SetViewerModel(model_, last_selected_as_id_);
    ui_->viewer_container_widget_->SetScene(scene);
    ui_->expand_collapse_tree_->SetCurrentItemIndex(rra::AccelerationStructureViewerModel::TreeViewExpandMode::kCollapsed);

    emit rra::MessageManager::Get().TlasSelected(last_selected_as_id_);
}

void TlasViewerPane::SetBlasInstanceSelection(uint64_t tlas_index, uint64_t blas_index, uint64_t instance_index)
{
    rra::Scene*                       scene     = model_->GetSceneCollectionModel()->GetSceneByIndex(tlas_index);
    const rra::renderer::InstanceMap& instances = scene->GetInstances();

    auto blas_instance_iter = instances.find(blas_index);
    if (blas_instance_iter != instances.end())
    {
        auto blas_instances          = blas_instance_iter->second;
        bool is_instance_index_valid = blas_instances.size() > 0 && instance_index < blas_instances.size();
        RRA_ASSERT(is_instance_index_valid);
        if (is_instance_index_valid)
        {
            // Retrieve the info for the instance that's being selected.
            const rra::renderer::Instance& instance_info = blas_instances[instance_index];

            // Move the camera to focus on the selected BLAS instance's bounding volume.
            rra::renderer::Camera& scene_camera      = renderer_interface_->GetCamera();
            rra::ViewerIO*         camera_controller = static_cast<rra::ViewerIO*>(scene_camera.GetCameraController());
            camera_controller->FocusCameraOnVolume(&renderer_interface_->GetCamera(), instance_info.bounding_volume);

            scene->SetSceneSelection(instance_info.instance_node);

            // Update the scene selection, which will select the instance in the viewport and the treeview.
            UpdateSceneSelection(ui_->tlas_tree_);

            HandleSceneSelectionChanged();

            emit rra::MessageManager::Get().BlasSelected(instance_info.blas_index);
        }
    }
}

void TlasViewerPane::GotoBlasPaneFromBlasAddress(bool checked)
{
    Q_UNUSED(checked);

    QModelIndexList indexes = ui_->tlas_tree_->selectionModel()->selectedIndexes();
    if (indexes.size() > 0)
    {
        const QModelIndex& selected_index = indexes.at(0);
        SelectBlasFromTree(selected_index, true);
    }
}

void TlasViewerPane::SelectParentNode(bool checked)
{
    Q_UNUSED(checked);

    QModelIndexList indexes = ui_->tlas_tree_->selectionModel()->selectedIndexes();
    if (indexes.size() > 0)
    {
        const QModelIndex& selected_index = indexes.at(0);
        ui_->tlas_tree_->selectionModel()->reset();
        ui_->tlas_tree_->selectionModel()->setCurrentIndex(selected_index.parent(), QItemSelectionModel::Select);
    }
}

void TlasViewerPane::SetTlasIndex(uint64_t tlas_index)
{
    if (tlas_index != last_selected_as_id_)
    {
        last_selected_as_id_ = tlas_index;
    }
}
