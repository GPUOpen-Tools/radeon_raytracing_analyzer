//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the BLAS viewer model.
//=============================================================================

#include "models/blas/blas_viewer_model.h"

#include "qt_common/custom_widgets/arrow_icon_combo_box.h"
#include "qt_common/custom_widgets/scaled_tree_view.h"
#include "qt_common/utils/qt_util.h"

#include "public/rra_assert.h"
#include "public/rra_blas.h"
#include "public/rra_rtip_info.h"

#include "constants.h"
#include "models/acceleration_structure_tree_view_model.h"
#include "models/acceleration_structure_viewer_model.h"
#include "models/blas/blas_scene_collection_model.h"
#include "settings/settings.h"
#include "views/widget_util.h"

namespace rra
{
    // Flag to indicate if this model represents a TLAS.
    static constexpr bool kIsTlasModel = false;

    BlasViewerModel::BlasViewerModel(ScaledTreeView* tree_view)
        : AccelerationStructureViewerModel(tree_view, kBlasStatsNumWidgets, kIsTlasModel)
    {
        // Create a BLAS scene model.
        scene_collection_model_ = new BlasSceneCollectionModel();
    }

    BlasViewerModel::~BlasViewerModel()
    {
        for (QStandardItemModel* vertex_table_model : vertex_table_models_triangle_)
        {
            delete vertex_table_model;
        }
        delete geometry_flags_table_model_;
    }

    void BlasViewerModel::InitializeVertexTableModels(ScaledTableView* table_view_triangle_)
    {
        QStandardItemModel* item_model{new QStandardItemModel(3, 4)};

        QStandardItem* vertex = new QStandardItem("Triangle placeholder");
        vertex->setTextAlignment(Qt::AlignLeft);
        item_model->setHorizontalHeaderItem(0, vertex);
        QStandardItem* x = new QStandardItem("X");
        x->setTextAlignment(Qt::AlignRight);
        item_model->setHorizontalHeaderItem(1, x);
        QStandardItem* y = new QStandardItem("Y");
        y->setTextAlignment(Qt::AlignRight);
        item_model->setHorizontalHeaderItem(2, y);
        QStandardItem* z = new QStandardItem("Z");
        z->setTextAlignment(Qt::AlignRight);
        item_model->setHorizontalHeaderItem(3, z);

        table_view_triangle_->setModel(item_model);

        widget_util::SetTableModelData(item_model, "Vertex 0 placeholder", 0, 0);
        widget_util::SetTableModelData(item_model, "Vertex 1 placeholder", 1, 0);
        widget_util::SetTableModelData(item_model, "Vertex 2 placeholder", 2, 0);

        vertex_table_models_triangle_.push_back(item_model);
    }

    void BlasViewerModel::InitializeFlagsTableModel(ScaledTableView* table_view)
    {
        geometry_flags_table_model_ = new FlagsTableItemModel();
        geometry_flags_table_model_->SetRowCount(2);
        geometry_flags_table_model_->SetColumnCount(2);

        table_view->setModel(geometry_flags_table_model_);

        geometry_flags_table_model_->SetRowFlagName(0, "Opaque");
        geometry_flags_table_model_->SetRowFlagName(1, "No duplicate any hit invocation");

        geometry_flags_table_model_->Initialize(table_view);

        table_view->horizontalHeader()->setVisible(false);
    }

    void BlasViewerModel::PopulateFlagsTable(FlagsTableItemModel* flags_table, uint32_t flags)
    {
        bool opaque                          = flags & VK_GEOMETRY_OPAQUE_BIT_KHR;
        bool no_duplicate_any_hit_invocation = flags & VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;

        flags_table->SetRowChecked(0, opaque);
        flags_table->SetRowChecked(1, no_duplicate_any_hit_invocation);

        // The table will not be updated without this.
        flags_table->dataChanged(QModelIndex(), QModelIndex());
    }

    RraErrorCode BlasViewerModel::AccelerationStructureGetCount(uint64_t* out_count) const
    {
        return RraBvhGetTotalBlasCount(out_count);
    }

    RraErrorCode BlasViewerModel::AccelerationStructureGetBaseAddress(uint64_t index, uint64_t* out_address) const
    {
        return RraBlasGetBaseAddress(index, out_address);
    }

    RraErrorCode BlasViewerModel::AccelerationStructureGetTotalNodeCount(uint64_t index, uint64_t* out_node_count) const
    {
        return RraBlasGetTotalNodeCount(index, out_node_count);
    }

    bool BlasViewerModel::AccelerationStructureGetIsEmpty(uint64_t index) const
    {
        return RraBlasIsEmpty(index);
    }

    GetChildNodeFunction BlasViewerModel::AccelerationStructureGetChildNodeFunction() const
    {
        return RraBlasGetChildNodePtr;
    }

    uint32_t BlasViewerModel::GetParentNodeOfSelected(uint32_t blas_index)
    {
        // Start with current selected index.
        QModelIndexList indexes = tree_view_->selectionModel()->selectedIndexes();
        if (indexes.size() > 0)
        {
            const QModelIndex& selected_index = indexes.at(0);
            QModelIndex        parent_index   = selected_index.parent();

            if (parent_index.isValid())
            {
                return GetNodeIdFromModelIndex(parent_index, blas_index, kIsTlasModel);
            }
            return std::numeric_limits<uint32_t>::max();
        }

        return {};
    }

    void BlasViewerModel::SetTriTableLabels(QStandardItemModel* model)
    {
        QString vertex_name = "Triangle";
        QString vert0       = "Vertex 0";
        QString vert1       = "Vertex 1";
        QString vert2       = "Vertex 2";

        SetTriTableModelLabels(model, vertex_name, vert0, vert1, vert2);
    }

    void BlasViewerModel::SetTriTableModelLabels(QStandardItemModel* model,
                                                 const QString&      vertex_name,
                                                 const QString&      vert0,
                                                 const QString&      vert1,
                                                 const QString&      vert2)
    {
        QStandardItem* vertex = new QStandardItem(vertex_name);
        vertex->setTextAlignment(Qt::AlignLeft);
        model->setHorizontalHeaderItem(0, vertex);

        widget_util::SetTableModelData(model, vert0, 0, 0, Qt::AlignLeft);
        widget_util::SetTableModelData(model, vert1, 1, 0, Qt::AlignLeft);
        widget_util::SetTableModelData(model, vert2, 2, 0, Qt::AlignLeft);
    }

    void BlasViewerModel::UpdateStatistics(uint64_t blas_index, uint32_t node_id, uint32_t child_index, uint32_t global_child_index)
    {
        // Show node name and base address.
        const char*  node_str{};
        RraErrorCode error_code = RraBlasGetNodeName(blas_index, node_id, &node_str);
        RRA_ASSERT(error_code == kRraOk);
        std::string node_name{node_str};

        if (IsTriangleSplit(blas_index))
        {
            node_name += " (split)";
        }

        // Annotate the node type with its place in the CLAS hierarchy (TLAS -> CBLAS -> CLAS -> triangles).
        const bool node_is_cluster_ref = RraBlasIsClusterRefNode(blas_index, node_id);
        if (node_is_cluster_ref)
        {
            // A cluster-ref leaf is a hardware instance node referencing a CLAS, so show instance-style info:
            // the CLAS id, the referenced CLAS base address, and its triangle count (SAH does not apply).
            uint32_t clas_id = 0;
            RraBlasGetClusterRefNodeId(blas_index, node_id, &clas_id);
            node_name = "CLAS [" + std::to_string(clas_id) + "]";

            uint64_t clas_blas_index = 0;
            if (RraBlasGetClasIndexFromClusterRefNode(blas_index, node_id, &clas_blas_index) == kRraOk)
            {
                uint64_t clas_address = 0;
                if (RraBlasGetBaseAddress(clas_blas_index, &clas_address) == kRraOk)
                {
                    node_name += " -> 0x" + QString("%1").arg(clas_address, 0, 16).toStdString();
                }

                uint32_t clas_triangle_count = 0;
                if (RraBlasGetActivePrimitiveCount(clas_blas_index, &clas_triangle_count) == kRraOk)
                {
                    node_name += " (" + std::to_string(clas_triangle_count) + " triangles)";
                }
            }
        }
        else if (RraBlasIsCluster(blas_index))
        {
            node_name += " (Cluster)";
        }
        else if (RraBlasIsClusterBlas(blas_index))
        {
            node_name += " (Cluster BLAS)";
        }

        // The type label (e.g. "Bvh8") is set below, after the packed-ref count is known, so a packed node can be
        // annotated "(packed)" inline with its type.

        // Show the focus button.
        SetModelData(kBlasStatsFocus, true);

        int decimal_precision = rra::Settings::Get().GetDecimalPrecision();

        SetModelData(kBlasStatsAddress, AddressString(blas_index, node_id));

        // Show surface area and bounding box extents.
        BoundingVolumeExtents bounding_volume_extents;
        if (RraBlasGetBoundingVolumeExtents(blas_index, node_id, child_index, global_child_index, &bounding_volume_extents) == kRraOk)
        {
            PopulateExtentsTable(bounding_volume_extents);
        }

        uint32_t parent_id = GetParentNodeOfSelected(blas_index);
        bool     parent_valid{parent_id != std::numeric_limits<uint32_t>::max()};
        if (parent_valid)
        {
            SetModelData(kBlasStatsParent, AddressString(blas_index, parent_id));
        }

        // RTIP3.1 node packing: count how many of the parent's box slots reference this same node. Packing makes
        // RraBlasGetChildNodes report the shared node id in more than one slot, so a duplicate count > 1 means the node
        // is packed (each slot has its own bounding box but they descend into one shared subtree). Non-packed data never
        // duplicates a child id, so the row stays hidden.
        last_selected_packed_ref_count_ = 1;
        if (parent_valid)
        {
            uint32_t parent_child_count = 0;
            if (RraBlasGetChildNodeCount(blas_index, parent_id, &parent_child_count) == kRraOk)
            {
                std::array<uint32_t, MAX_CHILD_NODES> parent_children{};
                RRA_ASSERT(parent_child_count <= MAX_CHILD_NODES);
                if (RraBlasGetChildNodes(blas_index, parent_id, parent_children.data()) == kRraOk)
                {
                    uint32_t refs = 0;
                    for (uint32_t i = 0; i < parent_child_count; ++i)
                    {
                        if (parent_children[i] == node_id)
                        {
                            ++refs;
                        }
                    }
                    if (refs > 1)
                    {
                        last_selected_packed_ref_count_ = refs;
                    }
                }
            }
        }
        SetModelData(kBlasStatsPackedRefCount, QString::number(last_selected_packed_ref_count_) + " boxes");

        // Annotate the type label with "(packed)" when this node is shared by multiple parent slots (RTIP3.1 node
        // packing), so the bold node title reads e.g. "Bvh8 (packed)".
        if (last_selected_packed_ref_count_ > 1)
        {
            node_name += " (packed)";
        }
        SetModelData(kBlasStatsType, node_name.c_str());

        // Show vertex data.
        if (SelectedNodeIsLeaf())
        {
            uint32_t tri_count{};
            error_code = RraBlasGetNodeTriangleCount(blas_index, node_id, child_index, global_child_index, &tri_count);
            RRA_ASSERT(error_code == kRraOk);
            std::array<TriangleVertices, MAX_TRIANGLES> tri_verts{};
            RRA_ASSERT(tri_count <= MAX_TRIANGLES);
            error_code = RraBlasGetNodeTriangles(blas_index, node_id, child_index, global_child_index, tri_verts.data());
            RRA_ASSERT(error_code == kRraOk);

            last_selected_node_tri_count_ = tri_count;

            for (uint32_t tri_idx{0}; tri_idx < tri_count; ++tri_idx)
            {
                TriangleVertices& verts = tri_verts[tri_idx];
                {
                    SetTriTableLabels(vertex_table_models_triangle_[tri_idx]);
                    SetModelData(kBlasStatsPrimitiveIndexLabel1 + tri_idx, QString("Primitive index"));

                    uint32_t primitive_index{};
                    if (RraBlasGetPrimitiveIndex(blas_index, node_id, child_index, global_child_index, tri_idx, &primitive_index) == kRraOk)
                    {
                        SetModelData(kBlasStatsPrimitiveIndexTriangle1 + tri_idx, QString::number(primitive_index));
                    }
                }

                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.a.x, 0, 1, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.a.y, 0, 2, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.a.z, 0, 3, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.b.x, 1, 1, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.b.y, 1, 2, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.b.z, 1, 3, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.c.x, 2, 1, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.c.y, 2, 2, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.c.z, 2, 3, Qt::AlignRight);
            }

            uint32_t geometry_index{};
            if (RraBlasGetGeometryIndex(blas_index, node_id, child_index, global_child_index, &geometry_index) == kRraOk)
            {
                // Show geometry flag here.
                SetModelData(kBlasStatsGeometryIndex, QString::number(geometry_index));

                uint32_t geometry_flags{};
                if (RraBlasGetGeometryFlags(blas_index, geometry_index, &geometry_flags) == kRraOk)
                {
                    PopulateFlagsTable(geometry_flags_table_model_, geometry_flags);
                }
            }
        }

        // Only box nodes carry an OBB orientation (it describes the rotation of their child bounds). Leaf/other nodes
        // have none, so guard the query the same way scene construction does; the backend asserts on a non-box node.
        if (RraRtipInfoGetOBBSupported() && RraBlasIsBoxNode(blas_index, node_id))
        {
            glm::mat3 rotation(1.0f);
            if (parent_valid)
            {
                RraErrorCode result = RraBlasGetNodeBoundingVolumeOrientation(blas_index, parent_id, &rotation[0][0]);
                RRA_ASSERT(result == kRraOk);
            }
            PopulateRotationTable(rotation);
        }

        // A cluster-ref leaf is a hardware instance node: it has no surface-area heuristic, but it does carry
        // instance data (mask + world-to-object transform). Show that instead of NaN SAH. The SAH rows are
        // repurposed/hidden and the transform is shown in the bottom matrix table by BlasViewerPane::UpdateWidgets.
        if (node_is_cluster_ref)
        {
            uint32_t instance_mask = 0;
            if (RraBlasGetClusterRefNodeMask(blas_index, node_id, &instance_mask) == kRraOk)
            {
                SetModelData(kBlasStatsCurrentSAH, QString("0x%1%2").arg((instance_mask & 0xF0) >> 4, 0, 16).arg(instance_mask & 0x0F, 0, 16));
            }
            else
            {
                SetModelData(kBlasStatsCurrentSAH, "-");
            }
            SetModelData(kBlasStatsSAHSubTreeMax, "-");
            SetModelData(kBlasStatsSAHSubTreeMean, "-");

            float transform[12] = {};
            if (RraBlasGetClusterRefNodeTransform(blas_index, node_id, transform) == kRraOk)
            {
                PopulateInstanceTransformTable(transform);
            }
            return;
        }

        // Show surface area heuristic.
        float surface_area_heuristic = 0.0;
        if (RraBlasGetSurfaceAreaHeuristic(blas_index, node_id, global_child_index, &surface_area_heuristic) == kRraOk)
        {
            SetModelData(kBlasStatsCurrentSAH,
                         QString::number(surface_area_heuristic, kQtFloatFormat, decimal_precision),
                         QString::number(surface_area_heuristic, kQtFloatFormat, kQtTooltipFloatPrecision));
        }

        // Show SAH max and average.
        if (RraBlasGetMinimumSurfaceAreaHeuristic(blas_index, node_id, global_child_index, false, &surface_area_heuristic) == kRraOk)
        {
            SetModelData(kBlasStatsSAHSubTreeMax,
                         QString::number(surface_area_heuristic, kQtFloatFormat, decimal_precision),
                         QString::number(surface_area_heuristic, kQtFloatFormat, kQtTooltipFloatPrecision));
        }

        if (RraBlasGetAverageSurfaceAreaHeuristic(blas_index, node_id, global_child_index, false, &surface_area_heuristic) == kRraOk)
        {
            SetModelData(kBlasStatsSAHSubTreeMean,
                         QString::number(surface_area_heuristic, kQtFloatFormat, decimal_precision),
                         QString::number(surface_area_heuristic, kQtFloatFormat, kQtTooltipFloatPrecision));
        }
    }

    void BlasViewerModel::UpdateUI(const QModelIndex& model_index, uint64_t blas_index)
    {
        AccelerationStructureViewerModel::SetSelectedNodeIndex(model_index);
        if (IsModelIndexNode(model_index))
        {
            uint32_t node_id            = GetNodeIdFromModelIndex(model_index, blas_index, kIsTlasModel);
            uint32_t child_index        = GetChildIndexFromModelIndex(model_index);
            uint32_t global_child_index = GetGlobalChildIndexFromModelIndex(model_index);

            UpdateStatistics(blas_index, node_id, child_index, global_child_index);
        }
        else
        {
            uint32_t node_id            = GetNodeIdFromModelIndex(model_index.parent(), blas_index, kIsTlasModel);
            uint32_t child_index        = GetChildIndexFromModelIndex(model_index.parent());
            uint32_t global_child_index = GetGlobalChildIndexFromModelIndex(model_index.parent());

            // Fill in the common stats for the triangle parent node.
            UpdateStatistics(blas_index, node_id, child_index, global_child_index);
        }
    }

    QString BlasViewerModel::UpdateToolTip(uint64_t bvh_index, rra::SceneCollectionModelClosestHit closest_hit)
    {
        RRA_UNUSED(bvh_index);

        uint64_t   packed_node        = closest_hit.triangle_child_node;
        uint32_t   node_id            = (uint32_t)(packed_node & UINT32_MAX);
        uint32_t   global_child_index = (uint32_t)(packed_node >> 32);
        SceneNode* triangle_node      = closest_hit.triangle_node;
        bool       has_triangle       = (node_id != UINT32_MAX) && (triangle_node != nullptr);

        if (!has_triangle || !render_state_adapter_)
            return "";

        uint64_t blas_index        = closest_hit.blas_index;
        int      decimal_precision = rra::Settings::Get().GetDecimalPrecision();

        renderer::GeometryColoringMode mode = render_state_adapter_->GetCurrentGeometryColoringModeValue();

        switch (mode)
        {
        case renderer::GeometryColoringMode::kTriangleSAH:
        {
            float sah = 0.0f;
            if (RraBlasGetSurfaceAreaHeuristic(blas_index, node_id, global_child_index, &sah) == kRraOk)
                return "SAH: " + QString::number(sah, kQtFloatFormat, decimal_precision);
            break;
        }
        case renderer::GeometryColoringMode::kTreeLevel:
            return "Depth: " + QString::number(triangle_node->GetDepth());
        case renderer::GeometryColoringMode::kGeometryIndex:
            return "Geometry index: " + QString::number(triangle_node->GetGeometryIndex());
        case renderer::GeometryColoringMode::kOpacity:
        {
            uint32_t geometry_flags = 0;
            if (RraBlasGetGeometryFlags(blas_index, triangle_node->GetGeometryIndex(), &geometry_flags) == kRraOk)
            {
                bool is_opaque = (geometry_flags & GeometryFlags::kOpaque) != 0;
                return QString("Opaque: ") + (is_opaque ? "yes" : "no");
            }
            break;
        }
        case renderer::GeometryColoringMode::kTriangleSplitting:
        case renderer::GeometryColoringMode::kLit:
        case renderer::GeometryColoringMode::kTechnical:
        default:
            break;
        }

        return "";
    }

    void BlasViewerModel::SetSceneSelection(const QModelIndex& model_index, uint64_t index)
    {
        uint32_t node_id;

        if (IsModelIndexNode(model_index))
        {
            node_id = GetNodeIdFromModelIndex(model_index, index, kIsTlasModel);
        }
        else
        {
            node_id = GetNodeIdFromModelIndex(model_index.parent(), index, kIsTlasModel);
        }

        uint32_t global_child_index  = GetGlobalChildIndexFromModelIndex(model_index);
        uint64_t node_child_id       = ((uint64_t)global_child_index << 32) | node_id;
        Scene*   current_scene_info_ = scene_collection_model_->GetSceneByIndex(index);
        current_scene_info_->SetSceneSelection(node_child_id);
    }

    bool BlasViewerModel::SelectedNodeIsLeaf() const
    {
        return last_selected_node_is_tri_;
    }

    bool BlasViewerModel::SelectedNodeIsClusterRef() const
    {
        return last_selected_node_is_cluster_ref_;
    }

    void BlasViewerModel::UpdateLastSelectedNodeIsLeaf(const QModelIndex& model_index, uint64_t index)
    {
        if (model_index.isValid())
        {
            uint32_t node_id                   = GetNodeIdFromModelIndex(model_index, index, kIsTlasModel);
            last_selected_node_is_tri_         = RraBlasIsTriangleNode(index, node_id);
            last_selected_node_is_cluster_ref_ = RraBlasIsClusterRefNode(index, node_id);
        }
        else
        {
            last_selected_node_is_tri_         = false;
            last_selected_node_is_cluster_ref_ = false;
        }
    }

    QString BlasViewerModel::AddressString(uint64_t bvh_index, uint32_t node_id) const
    {
        TreeviewNodeIDType node_type    = rra::Settings::Get().GetTreeviewNodeIdType();
        uint64_t           node_address = 0;
        RraErrorCode       error_code   = kRraOk;

        switch (node_type)
        {
        case kTreeviewNodeIDTypeVirtualAddress:
            error_code = RraBlasGetNodeBaseAddress(bvh_index, node_id, &node_address);
            RRA_ASSERT(error_code == kRraOk);
            break;

        case kTreeviewNodeIDTypeOffset:
            error_code = RraBvhGetNodeOffset(node_id, &node_address);
            RRA_ASSERT(error_code == kRraOk);
            break;

        default:
            break;
        }

        return "0x" + QString("%1").arg(node_address, 0, 16);
    }

    uint32_t BlasViewerModel::GetProceduralNodeCount(uint64_t blas_index) const
    {
        uint32_t     node_count{};
        RraErrorCode error_code = RraBlasGetProceduralNodeCount(blas_index, &node_count);
        RRA_ASSERT(error_code == kRraOk);
        return node_count;
    }

    uint32_t BlasViewerModel::SelectedNodeTriangleCount() const
    {
        return last_selected_node_is_tri_ ? last_selected_node_tri_count_ : 0;
    }

    void BlasViewerModel::ResetModelValues(bool reset_scene)
    {
        SetModelData(kBlasStatsType, "No node selected.");
        SetModelData(kBlasStatsFocus, false);
        SetModelData(kBlasStatsAddress, "");
        SetModelData(kBlasStatsCurrentSAH, "-");
        SetModelData(kBlasStatsSAHSubTreeMax, "-");
        SetModelData(kBlasStatsSAHSubTreeMean, "-");
        last_selected_node_is_tri_ = false;
        AccelerationStructureViewerModel::ResetModelValues(reset_scene);
    }

}  // namespace rra

