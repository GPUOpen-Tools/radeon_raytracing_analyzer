//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of an acceleration structure viewer model base class.
//=============================================================================

#include "models/acceleration_structure_viewer_model.h"

#include <QTreeView>

#include "qt_common/custom_widgets/scaled_tree_view.h"
#include "qt_common/utils/qt_util.h"

#include "managers/message_manager.h"
#include "models/acceleration_structure_tree_view_model.h"
#include "models/tree_view_proxy_model.h"

#include "public/rra_assert.h"
#include "public/camera.h"
#include "public/rra_blas.h"
#include "public/rra_print.h"
#include "public/intersect.h"

#include "constants.h"
#include "qt_common/custom_widgets/scaled_check_box.h"
#include "views/widget_util.h"

#include "settings/settings.h"

namespace rra
{
    AccelerationStructureViewerModel::AccelerationStructureViewerModel(ScaledTreeView* tree_view, uint32_t num_widgets, bool is_tlas)
        : ModelViewMapper(num_widgets)
    {
        tree_view_model_ = new AccelerationStructureTreeViewModel(is_tlas);
        is_tlas_         = is_tlas;

        // Create a proxy model to handle tree text searching.
        tree_view_proxy_model_ = new TreeViewProxyModel(this);
        tree_view_proxy_model_->setSourceModel(tree_view_model_);
        tree_view->setModel(tree_view_proxy_model_);

        // Set up some proxy model properties.
        tree_view_proxy_model_->setFilterKeyColumn(0);
        tree_view_proxy_model_->setRecursiveFilteringEnabled(true);

        tree_view_           = tree_view;
        selected_node_index_ = QModelIndex();
    }

    AccelerationStructureViewerModel::~AccelerationStructureViewerModel()
    {
        delete tree_view_model_;
        delete tree_view_proxy_model_;
        for (const auto& bvh_delegate : item_delegate_map_)
        {
            delete bvh_delegate.second;
        }
        delete extents_table_model_;
    }

    void AccelerationStructureViewerModel::InitializeExtentsTableModel(ScaledTableView* table_view)
    {
        extents_table_model_ = new QStandardItemModel(3, 3);

        QStandardItem* extents = new QStandardItem("Extents");
        extents->setTextAlignment(Qt::AlignLeft);
        extents_table_model_->setHorizontalHeaderItem(0, extents);
        QStandardItem* min = new QStandardItem("Min");
        min->setTextAlignment(Qt::AlignRight);
        extents_table_model_->setHorizontalHeaderItem(1, min);
        QStandardItem* max = new QStandardItem("Max");
        max->setTextAlignment(Qt::AlignRight);
        extents_table_model_->setHorizontalHeaderItem(2, max);

        table_view->setModel(extents_table_model_);

        widget_util::SetTableModelData(extents_table_model_, "X", 0, 0);
        widget_util::SetTableModelData(extents_table_model_, "Y", 1, 0);
        widget_util::SetTableModelData(extents_table_model_, "Z", 2, 0);
    }

    void AccelerationStructureViewerModel::PopulateExtentsTable(const BoundingVolumeExtents& bounding_volume_extents)
    {
        widget_util::SetTableModelDecimalData(extents_table_model_, bounding_volume_extents.min_x, 0, 1, Qt::AlignRight);
        widget_util::SetTableModelDecimalData(extents_table_model_, bounding_volume_extents.min_y, 1, 1, Qt::AlignRight);
        widget_util::SetTableModelDecimalData(extents_table_model_, bounding_volume_extents.min_z, 2, 1, Qt::AlignRight);
        widget_util::SetTableModelDecimalData(extents_table_model_, bounding_volume_extents.max_x, 0, 2, Qt::AlignRight);
        widget_util::SetTableModelDecimalData(extents_table_model_, bounding_volume_extents.max_y, 1, 2, Qt::AlignRight);
        widget_util::SetTableModelDecimalData(extents_table_model_, bounding_volume_extents.max_z, 2, 2, Qt::AlignRight);
    }

    void AccelerationStructureViewerModel::SetRendererAdapters(const rra::renderer::RendererAdapterMap& adapters)
    {
        using namespace rra::renderer;

        render_state_adapter_ = GetAdapter<RenderStateAdapter*>(adapters, RendererAdapterType::kRendererAdapterTypeRenderState);
    }

    void AccelerationStructureViewerModel::ClearSelection(uint64_t index)
    {
        Scene* current_scene_info_ = scene_collection_model_->GetSceneByIndex(index);
        current_scene_info_->ResetSceneSelection();
    }

    void AccelerationStructureViewerModel::SetupAccelerationStructureList(ArrowIconComboBox* combo_box)
    {
        combo_box->ClearItems();
        as_index_to_row_index_map_.clear();

        uint64_t as_count = 0;
        if (AccelerationStructureGetCount(&as_count) != kRraOk)
        {
            return;
        }

        // There are some cases where the acceleration structure will contain no nodes. These acceleration structures
        // are ignored and not added to the combo box. However, the index of the combo box will no longer match the
        // index of the acceleration structure in the backend so a map is needed to get the combo box row from the
        // acceleration structure index. Getting the acceleration structure from the row is easy, since it is encoded
        // as UserData in the combobox item; see FindAccelerationStructureIndex() below for an example.
        uint64_t row = 0;
        for (uint64_t blas_index = 0; blas_index < as_count; blas_index++)
        {
            uint64_t address = 0;
            if (AccelerationStructureGetBaseAddress(blas_index, &address) == kRraOk)
            {
                QListWidgetItem* item = new QListWidgetItem();
                if (!AccelerationStructureGetIsEmpty(blas_index))
                {
                    QString address_string = "0x" + QString("%1").arg(address, 0, 16);
                    item->setText(address_string);
                    item->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(blas_index));
                    combo_box->AddItem(item);
                    as_index_to_row_index_map_.insert(std::make_pair(blas_index, row));
                    row++;
                }
            }
        }
    }

    uint64_t AccelerationStructureViewerModel::FindAccelerationStructureIndex(ArrowIconComboBox* combo_box)
    {
        if (combo_box->RowCount() > 0)
        {
            int selected_row = combo_box->CurrentRow();
            if (selected_row >= 0)
            {
                QListWidgetItem* item = combo_box->FindItem(selected_row);
                return item->data(Qt::UserRole).toULongLong();
            }
        }
        return UINT64_MAX;
    }

    int AccelerationStructureViewerModel::FindRowFromAccelerationStructureIndex(uint64_t blas_index)
    {
        const auto& it = as_index_to_row_index_map_.find(blas_index);
        if (it != as_index_to_row_index_map_.end())
        {
            return it->second;
        }
        return 0;
    }

    void AccelerationStructureViewerModel::PopulateTreeView(uint64_t index)
    {
        uint64_t node_count = 0;
        if (AccelerationStructureGetTotalNodeCount(index, &node_count) == kRraOk)
        {
            last_clicked_node_scene_ = scene_collection_model_->GetSceneByIndex(index);

            auto delegate_iter = item_delegate_map_.find(index);
            if (delegate_iter != item_delegate_map_.end() && delegate_iter->second)
            {
                delete delegate_iter->second;
            }

            auto func                 = [&, index]() { RefreshUI(index); };
            auto delegate             = new AccelerationStructureTreeViewItemDelegate(last_clicked_node_scene_, func);
            item_delegate_map_[index] = delegate;

            tree_view_->setItemDelegate(delegate);
            tree_view_model_->InitializeModel(node_count, index, AccelerationStructureGetChildNodeFunction());
        }
    }

    float GetNearDistance(BoundingVolumeExtents scene_volume)
    {
        float x               = scene_volume.max_x - scene_volume.min_x;
        float y               = scene_volume.max_y - scene_volume.min_y;
        float z               = scene_volume.max_z - scene_volume.min_z;
        float diagonal_length = std::sqrt(x * x + y * y + z * z);

        return 0.01f * diagonal_length;
    }

    void AccelerationStructureViewerModel::PopulateScene(renderer::RendererInterface* renderer, uint64_t index)
    {
        if (scene_collection_model_ != nullptr)
        {
            scene_collection_model_->PopulateScene(renderer, index);

            RRA_ASSERT(renderer != nullptr);
            if (renderer != nullptr)
            {
                // Set the scene info callback in the renderer so the renderer can acces up-to-date info about the scene.
                Scene* bvh_scene               = scene_collection_model_->GetSceneByIndex(index);
                auto&  node_colors             = GetSceneNodeColors();
                bool   fused_instances_enabled = scene_collection_model_->GetFusedInstancesEnabled(index);
                renderer->SetSceneInfoCallback(
                    [bvh_scene, &node_colors, fused_instances_enabled](
                        renderer::RendererSceneInfo& info, renderer::Camera* camera, bool frustum_culling, bool force_camera_update) {
                        info.scene_iteration                       = bvh_scene->GetSceneIteration();
                        info.depth_range_lower_bound               = bvh_scene->GetDepthRangeLowerBound();
                        info.depth_range_upper_bound               = bvh_scene->GetDepthRangeUpperBound();
                        auto& stats                                = bvh_scene->GetSceneStatistics();
                        info.max_instance_count                    = stats.max_instance_count;
                        info.max_triangle_count                    = stats.max_triangle_count;
                        info.max_tree_depth                        = stats.max_tree_depth;
                        info.max_node_depth                        = stats.max_node_depth;
                        info.box16_node_color                      = node_colors.box16_node_color;
                        info.box32_node_color                      = node_colors.box32_node_color;
                        info.instance_node_color                   = node_colors.instance_node_color;
                        info.procedural_node_color                 = node_colors.procedural_node_color;
                        info.triangle_node_color                   = node_colors.triangle_node_color;
                        info.selected_node_color                   = node_colors.selected_node_color;
                        info.wireframe_normal_color                = node_colors.wireframe_normal_color;
                        info.wireframe_selected_color              = node_colors.wireframe_selected_color;
                        info.selected_geometry_color               = node_colors.selected_geometry_color;
                        info.background1_color                     = node_colors.background1_color;
                        info.background2_color                     = node_colors.background2_color;
                        info.transparent_color                     = node_colors.transparent_color;
                        info.opaque_color                          = node_colors.opaque_color;
                        info.positive_color                        = node_colors.positive_color;
                        info.negative_color                        = node_colors.negative_color;
                        info.build_algorithm_none_color            = node_colors.build_algorithm_none_color;
                        info.build_algorithm_fast_build_color      = node_colors.build_algorithm_fast_build_color;
                        info.build_algorithm_fast_trace_color      = node_colors.build_algorithm_fast_trace_color;
                        info.build_algorithm_both_color            = node_colors.build_algorithm_both_color;
                        info.instance_opaque_none_color            = node_colors.instance_opaque_none_color;
                        info.instance_opaque_force_opaque_color    = node_colors.instance_opaque_force_opaque_color;
                        info.instance_opaque_force_no_opaque_color = node_colors.instance_opaque_force_no_opaque_color;
                        info.instance_opaque_force_both_color      = node_colors.instance_opaque_force_both_color;

                        bool scene_updated  = info.scene_iteration != info.last_iteration;
                        bool camera_changed = camera->GetViewProjection() != info.last_view_proj;

                        if (scene_updated)
                        {
                            info.custom_triangles          = bvh_scene->GetCustomTriangles();
                            info.bounding_volume_list      = bvh_scene->GetBoundingVolumeList();
                            info.selected_volume_instances = bvh_scene->GetSelectedVolumeInstances();
                            info.traversal_tree            = bvh_scene->GenerateTraversalTree();
                            info.instance_counts           = bvh_scene->GetBlasInstanceCounts();
                        }

                        if (scene_updated || camera_changed || force_camera_update)
                        {
                            if (frustum_culling)
                            {
                                // Set near clip to 0 since we don't know ahead of time how close the closest object is.
                                // It will be updated appropriately after frustum culling.
                                camera->SetNearClipScale(0.0f);
                                auto frustum_info                = camera->GetFrustumInfo();
                                frustum_info.fov_threshold_ratio = rra::Settings::Get().GetFrustumCullRatio();
                                info.instance_map            = bvh_scene->GetFrustumCulledInstanceMap(frustum_info);  ///< Writes to closest_point_to_camera.
                                info.closest_point_to_camera = frustum_info.closest_point_to_camera;
                            }
                            else
                            {
                                BoundingVolumeExtents volume;
                                bvh_scene->GetSceneBoundingVolume(volume);
                                float closest_point_distance = GetNearDistance(volume);
                                info.instance_map            = bvh_scene->GetInstanceMap();
                                info.closest_point_to_camera = camera->GetPosition() + glm::vec3(closest_point_distance, 0.0f, 0.0f);
                            }
                            camera->SetNearClipScale(glm::distance(camera->GetPosition(), info.closest_point_to_camera));
                            info.camera = camera;
                        }

                        info.last_view_proj = camera->GetViewProjection();
                        info.last_iteration = info.scene_iteration;

                        info.fused_instances_enabled = fused_instances_enabled;
                    });
            }
        }
    }

    std::shared_ptr<renderer::GraphicsContextSceneInfo> GetGraphicsContextSceneInfo()
    {
        uint64_t blas_count = 0;
        RRA_ASSERT(RraBvhGetTotalBlasCount(&blas_count) == kRraOk);

        auto info = std::make_shared<renderer::GraphicsContextSceneInfo>();
        info->acceleration_structures.resize(blas_count);

        for (uint64_t blas_index = 0; blas_index < blas_count; blas_index++)
        {
            auto scene_root = SceneNode::ConstructFromBlas(static_cast<uint32_t>(blas_index));

            // Add to the tree using the scene root.
            scene_root->AddToTraversalTree(info->acceleration_structures[blas_index]);

            delete scene_root;
        }

        return info;
    }

    bool AccelerationStructureViewerModel::IsInstanceNode(uint64_t tlas_index) const
    {
        if (selected_node_index_.isValid())
        {
            uint32_t node_id = GetNodeIdFromModelIndex(selected_node_index_, tlas_index, is_tlas_);
            return RraBvhIsInstanceNode(node_id);
        }
        return false;
    }

    bool AccelerationStructureViewerModel::IsTriangleNode(uint64_t tlas_index) const
    {
        if (selected_node_index_.isValid())
        {
            uint32_t node_id = GetNodeIdFromModelIndex(selected_node_index_, tlas_index, is_tlas_);
            return RraBvhIsTriangleNode(node_id);
        }
        return false;
    }

    bool AccelerationStructureViewerModel::IsRebraidedNode(uint64_t tlas_index) const
    {
        if (IsInstanceNode(tlas_index))
        {
            uint32_t   node_id = GetNodeIdFromModelIndex(selected_node_index_, tlas_index, is_tlas_);
            SceneNode* node    = last_clicked_node_scene_->GetNodeById(node_id);
            return last_clicked_node_scene_->IsInstanceRebraided(node->GetInstance()->instance_index);
        }
        return false;
    }

    bool AccelerationStructureViewerModel::IsTriangleSplit(uint64_t blas_index) const
    {
        if (IsTriangleNode(blas_index))
        {
            uint32_t   node_id = GetNodeIdFromModelIndex(selected_node_index_, blas_index, is_tlas_);
            SceneNode* node    = last_clicked_node_scene_->GetNodeById(node_id);
            return last_clicked_node_scene_->IsTriangleSplit(node->GetGeometryIndex(), node->GetPrimitiveIndex());
        }
        return false;
    }

    void AccelerationStructureViewerModel::SetSelectedNodeIndex(const QModelIndex& model_index)
    {
        selected_node_index_ = model_index;
    }

    SceneContextMenuOptions AccelerationStructureViewerModel::GetSceneContextOptions(uint64_t bvh_index, SceneContextMenuRequest request)
    {
        auto scene = GetSceneCollectionModel()->GetSceneByIndex(bvh_index);
        if (scene)
        {
            auto options = scene->GetSceneContextOptions(request);
            for (const auto& option : options)
            {
                auto option_func      = option.second;
                options[option.first] = [&, option_func, bvh_index]() {
                    option_func();
                    emit SceneSelectionChanged();
                    RefreshUI(bvh_index);
                };
            }
            return options;
        }
        return {};
    }

    void AccelerationStructureViewerModel::HideSelectedNodes(uint32_t bvh_index_)
    {
        Scene* scene = GetSceneCollectionModel()->GetSceneByIndex(bvh_index_);
        scene->HideSelectedNodes();
        RefreshUI(bvh_index_);
    }

    void AccelerationStructureViewerModel::ShowAllNodes(uint32_t bvh_index_)
    {
        Scene* scene = GetSceneCollectionModel()->GetSceneByIndex(bvh_index_);
        scene->ShowAllNodes();
        RefreshUI(bvh_index_);
    }

    void AccelerationStructureViewerModel::SetMultiSelect(bool multi_select)
    {
        Scene::SetMultiSelect(multi_select);
    }

    void AccelerationStructureViewerModel::SetCameraController(rra::ViewerIO* controller)
    {
        camera_controller_ = controller;
    }

    rra::ViewerIO* AccelerationStructureViewerModel::GetCameraController() const
    {
        return camera_controller_;
    }

    void AccelerationStructureViewerModel::RefreshUI(uint64_t index)
    {
        if (selected_node_index_.isValid())
        {
            UpdateUI(selected_node_index_, index);
        }

        auto delegate_iter = item_delegate_map_.find(index);
        if (delegate_iter != item_delegate_map_.end() && delegate_iter->second)
        {
            // Forces an UI update for the tree_view.
            tree_view_->setItemDelegate(nullptr);
            tree_view_->setItemDelegate(delegate_iter->second);
        }
    }

    void AccelerationStructureViewerModel::ResetModelValues(bool reset_scene)
    {
        selected_node_index_ = QModelIndex();

        if (reset_scene)
        {
            tree_view_model_->ResetModelValues();
            scene_collection_model_->ResetModelValues();
            tree_view_->setItemDelegate(nullptr);
            for (const auto& bvh_delegate : item_delegate_map_)
            {
                delete bvh_delegate.second;
            }
            item_delegate_map_.clear();
        }
    }

    SceneCollectionModelClosestHit AccelerationStructureViewerModel::SelectFromScene(uint64_t                scene_index,
                                                                                     const renderer::Camera* camera,
                                                                                     glm::vec2               normalized_window_coords)
    {
        // Invert to match coord space.
        normalized_window_coords.y *= -1.0f;

        renderer::CameraRay ray = camera->CastRay(normalized_window_coords);

        // Trace ray.
        SceneCollectionModelClosestHit scene_model_closest_hit;

        scene_collection_model_->CastClosestHitRayOnBvh(scene_index, ray.origin, ray.direction, scene_model_closest_hit);

        auto     scene           = scene_collection_model_->GetSceneByIndex(scene_index);
        uint32_t scene_selection = UINT32_MAX;

        if (scene && scene_model_closest_hit.distance > 0.0f)
        {
            if (scene_model_closest_hit.instance_node != UINT32_MAX)
            {
                scene_selection = scene_model_closest_hit.instance_node;
            }

            if (scene_model_closest_hit.triangle_node != UINT32_MAX)
            {
                scene_selection = scene_model_closest_hit.triangle_node;
            }
        }

        scene->SetSceneSelection(scene_selection);

        // Emit a signal with the new selection info.
        emit SceneSelectionChanged();

        return scene_model_closest_hit;
    }

    QModelIndex AccelerationStructureViewerModel::GetModelIndexForNode(uint32_t node_id) const
    {
        // Use the tree view model to get the model index associated with the node.
        QModelIndex source_index = tree_view_model_->GetModelIndexForNode(node_id);
        return tree_view_proxy_model_->mapFromSource(source_index);
    }

    QModelIndex AccelerationStructureViewerModel::GetModelIndexForNodeAndTriangle(uint32_t node_id, uint32_t triangle_index) const
    {
        QModelIndex source_index = tree_view_model_->GetModelIndexForNodeAndTriangle(node_id, triangle_index);
        return tree_view_proxy_model_->mapFromSource(source_index);
    }

    SceneCollectionModel* AccelerationStructureViewerModel::GetSceneCollectionModel() const
    {
        return scene_collection_model_;
    }

    uint32_t AccelerationStructureViewerModel::GetNodeIdFromModelIndex(const QModelIndex& model_index, uint64_t index, bool is_tlas) const
    {
        const QModelIndex                       proxy_model_index = tree_view_proxy_model_->mapToSource(model_index);
        rra::AccelerationStructureTreeViewItem* item              = static_cast<rra::AccelerationStructureTreeViewItem*>(proxy_model_index.internalPointer());
        RRA_ASSERT(item != nullptr);
        if (!item)
        {
            return 0;
        }

        QVariant node_data = item->Data(proxy_model_index.column(), Qt::UserRole, is_tlas, index);
        return node_data.toUInt();
    }

    bool AccelerationStructureViewerModel::IsModelIndexNode(const QModelIndex& model_index) const
    {
        const QModelIndex                       proxy_model_index = tree_view_proxy_model_->mapToSource(model_index);
        rra::AccelerationStructureTreeViewItem* item              = static_cast<rra::AccelerationStructureTreeViewItem*>(proxy_model_index.internalPointer());
        RRA_ASSERT(item != nullptr);
        if (!item)
        {
            return false;
        }

        return item->IsNode();
    }

    void AccelerationStructureViewerModel::ToggleInstanceTransformWireframe()
    {
        if (render_state_adapter_ != nullptr)
        {
            if (render_state_adapter_->GetRenderInstancePretransform())
            {
                render_state_adapter_->SetRenderInstancePretransform(false);
            }
            else
            {
                render_state_adapter_->SetRenderInstancePretransform(true);
            }
            emit MessageManager::Get().RenderStateChanged();
        }
    }

    void AccelerationStructureViewerModel::ToggleBVHWireframe()
    {
        if (render_state_adapter_ != nullptr)
        {
            if (render_state_adapter_->GetRenderBoundingVolumes())
            {
                render_state_adapter_->SetRenderBoundingVolumes(false);
            }
            else
            {
                render_state_adapter_->SetRenderBoundingVolumes(true);
            }
            emit MessageManager::Get().RenderStateChanged();
        }
    }

    void AccelerationStructureViewerModel::ToggleMeshWireframe()
    {
        if (render_state_adapter_ != nullptr)
        {
            if (render_state_adapter_->GetRenderWireframe())
            {
                render_state_adapter_->SetRenderWireframe(false);
            }
            else
            {
                render_state_adapter_->SetRenderWireframe(true);
            }
            emit MessageManager::Get().RenderStateChanged();
        }
    }

    void AccelerationStructureViewerModel::ToggleRenderGeometry()
    {
        if (render_state_adapter_ != nullptr && !render_state_adapter_->GetRenderTraversal())
        {
            if (render_state_adapter_->GetRenderGeometry())
            {
                render_state_adapter_->SetRenderGeometry(false);
            }
            else
            {
                render_state_adapter_->SetRenderGeometry(true);
            }
            emit MessageManager::Get().RenderStateChanged();
        }
    }

    void AccelerationStructureViewerModel::AdaptTraversalCounterRangeToView(void)
    {
        if (render_state_adapter_ != nullptr)
        {
            render_state_adapter_->AdaptTraversalCounterRangeToView(
                [=](uint32_t min, uint32_t max) { emit rra::MessageManager::Get().TraversalSliderChanged(min, max); });
        }
    }

    void AccelerationStructureViewerModel::SearchTextChanged(const QString& search_text)
    {
        tree_view_proxy_model_->SetSearchText(search_text);

        // Expand the treeview when searching.
        treeview_expand_state_ = TreeViewExpandMode::kExpanded;
        tree_view_->expandAll();
    }

    void AccelerationStructureViewerModel::ExpandCollapseTreeView(int index)
    {
        if (index == TreeViewExpandMode::kCollapsed)
        {
            tree_view_->collapseAll();
        }
        else
        {
            tree_view_->expandAll();
        }
        treeview_expand_state_ = static_cast<TreeViewExpandMode>(index);
    }

}  // namespace rra
