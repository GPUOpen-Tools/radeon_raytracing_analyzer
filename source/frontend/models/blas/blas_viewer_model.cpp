//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the BLAS viewer model.
//=============================================================================

#include "models/blas/blas_viewer_model.h"
#include "models/blas/blas_scene_collection_model.h"

#include "qt_common/custom_widgets/arrow_icon_combo_box.h"
#include "qt_common/custom_widgets/scaled_tree_view.h"
#include "qt_common/utils/qt_util.h"

#include "public/rra_assert.h"
#include "public/rra_blas.h"

#include "constants.h"
#include "models/acceleration_structure_tree_view_model.h"
#include "models/acceleration_structure_viewer_model.h"
#include "settings/settings.h"
#include "views/widget_util.h"
#include "public/rra_rtip_info.h"

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

        QStandardItem* vertex = new QStandardItem("Triangle");
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

        widget_util::SetTableModelData(item_model, "Vertex 0", 0, 0);
        widget_util::SetTableModelData(item_model, "Vertex 1", 1, 0);
        widget_util::SetTableModelData(item_model, "Vertex 2", 2, 0);

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

        table_view->GetHeaderView()->setVisible(false);
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

    void BlasViewerModel::UpdateStatistics(uint64_t blas_index, uint32_t node_id)
    {
        // Show node name and base address.
        const char* node_str{};
        RraBlasGetNodeName(blas_index, node_id, &node_str);
        std::string node_name{node_str};

        if (IsTriangleSplit(blas_index))
        {
            node_name += " (split)";
        }

        SetModelData(kBlasStatsType, node_name.c_str());

        // Show the focus button.
        SetModelData(kBlasStatsFocus, true);

        int decimal_precision = rra::Settings::Get().GetDecimalPrecision();

        SetModelData(kBlasStatsAddress, AddressString(blas_index, node_id));

        // Show surface area and bounding box extents.
        BoundingVolumeExtents bounding_volume_extents;
        if (RraBlasGetBoundingVolumeExtents(blas_index, node_id, &bounding_volume_extents) == kRraOk)
        {
            PopulateExtentsTable(bounding_volume_extents);
        }

        uint32_t parent_id = GetParentNodeOfSelected(blas_index);
        bool     parent_valid{parent_id != std::numeric_limits<uint32_t>::max()};
        if (parent_valid)
        {
            SetModelData(kBlasStatsParent, AddressString(blas_index, parent_id));
        }

        // Show vertex data.
        if (SelectedNodeIsLeaf())
        {
            uint32_t tri_count{};
            RraBlasGetNodeTriangleCount(blas_index, node_id, &tri_count);
            std::vector<TriangleVertices> tri_verts{};
            tri_verts.resize(tri_count);
            RraBlasGetNodeTriangles(blas_index, node_id, tri_verts.data());

            last_selected_node_tri_count_ = tri_count;

            for (uint32_t tri_idx{0}; tri_idx < tri_count; ++tri_idx)
            {
                TriangleVertices& verts = tri_verts[tri_idx];
                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.a.x, 0, 1, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.a.y, 0, 2, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.a.z, 0, 3, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.b.x, 1, 1, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.b.y, 1, 2, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.b.z, 1, 3, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.c.x, 2, 1, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.c.y, 2, 2, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_models_triangle_[tri_idx], verts.c.z, 2, 3, Qt::AlignRight);

                uint32_t primitive_index{};
                if (RraBlasGetPrimitiveIndex(blas_index, node_id, tri_idx, &primitive_index) == kRraOk)
                {
                    SetModelData(kBlasStatsPrimitiveIndexTriangle1 + tri_idx, QString::number(primitive_index));
                }
            }

            uint32_t geometry_index{};
            if (RraBlasGetGeometryIndex(blas_index, node_id, &geometry_index) == kRraOk)
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

        if (RraRtipInfoGetOBBSupported())
        {
            glm::mat3 rotation(1.0f);
            if (parent_valid)
            {
                RraErrorCode result = RraBlasGetNodeBoundingVolumeOrientation(blas_index, parent_id, &rotation[0][0]);
                RRA_ASSERT(result == kRraOk);
            }
            PopulateRotationTable(rotation);
        }

        // Show surface area heuristic.
        float surface_area_heuristic = 0.0;
        if (RraBlasGetSurfaceAreaHeuristic(blas_index, node_id, &surface_area_heuristic) == kRraOk)
        {
            SetModelData(kBlasStatsCurrentSAH,
                         QString::number(surface_area_heuristic, kQtFloatFormat, decimal_precision),
                         QString::number(surface_area_heuristic, kQtFloatFormat, kQtTooltipFloatPrecision));
        }

        // Show SAH max and average.
        if (RraBlasGetMinimumSurfaceAreaHeuristic(blas_index, node_id, false, &surface_area_heuristic) == kRraOk)
        {
            SetModelData(kBlasStatsSAHSubTreeMax,
                         QString::number(surface_area_heuristic, kQtFloatFormat, decimal_precision),
                         QString::number(surface_area_heuristic, kQtFloatFormat, kQtTooltipFloatPrecision));
        }

        if (RraBlasGetAverageSurfaceAreaHeuristic(blas_index, node_id, false, &surface_area_heuristic) == kRraOk)
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
            uint32_t node_id = GetNodeIdFromModelIndex(model_index, blas_index, kIsTlasModel);

            UpdateStatistics(blas_index, node_id);
        }
        else
        {
            uint32_t node_id = GetNodeIdFromModelIndex(model_index.parent(), blas_index, kIsTlasModel);

            // Fill in the common stats for the triangle parent node.
            UpdateStatistics(blas_index, node_id);
        }
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

        Scene* current_scene_info_ = scene_collection_model_->GetSceneByIndex(index);
        current_scene_info_->SetSceneSelection(node_id);
    }

    bool BlasViewerModel::SelectedNodeIsLeaf() const
    {
        return last_selected_node_is_tri_;
    }

    void BlasViewerModel::UpdateLastSelectedNodeIsLeaf(const QModelIndex& model_index, uint64_t index)
    {
        if (model_index.isValid())
        {
            uint32_t node_id           = GetNodeIdFromModelIndex(model_index, index, kIsTlasModel);
            last_selected_node_is_tri_ = RraBlasIsTriangleNode(index, node_id);
        }
        else
        {
            last_selected_node_is_tri_ = false;
        }
    }

    QString BlasViewerModel::AddressString(uint64_t bvh_index, uint32_t node_id) const
    {
        TreeviewNodeIDType node_type    = rra::Settings::Get().GetTreeviewNodeIdType();
        uint64_t           node_address = 0;

        switch (node_type)
        {
        case kTreeviewNodeIDTypeVirtualAddress:
            RraBlasGetNodeBaseAddress(bvh_index, node_id, &node_address);
            break;

        case kTreeviewNodeIDTypeOffset:
            RraBvhGetNodeOffset(node_id, &node_address);
            break;

        default:
            break;
        }

        return "0x" + QString("%1").arg(node_address, 0, 16);
    }

    uint32_t BlasViewerModel::GetProceduralNodeCount(uint64_t blas_index) const
    {
        uint32_t node_count{};
        RraBlasGetProceduralNodeCount(blas_index, &node_count);
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
