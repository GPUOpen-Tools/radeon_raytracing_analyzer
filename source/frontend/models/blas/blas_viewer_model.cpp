//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
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

namespace rra
{
    // Flag to indicate if this model represents a TLAS.
    static bool kIsTlasModel = false;

    BlasViewerModel::BlasViewerModel(ScaledTreeView* tree_view)
        : AccelerationStructureViewerModel(tree_view, kBlasStatsNumWidgets, kIsTlasModel)
    {
        // Create a BLAS scene model.
        scene_collection_model_ = new BlasSceneCollectionModel();
    }

    BlasViewerModel::~BlasViewerModel()
    {
        delete vertex_table_model_;
        delete flags_table_model_;
    }

    void BlasViewerModel::InitializeVertexTableModel(ScaledTableView* table_view)
    {
        vertex_table_model_ = new QStandardItemModel(4, 4);

        QStandardItem* vertex = new QStandardItem("Vertex");
        vertex->setTextAlignment(Qt::AlignLeft);
        vertex_table_model_->setHorizontalHeaderItem(0, vertex);
        QStandardItem* x = new QStandardItem("X");
        x->setTextAlignment(Qt::AlignRight);
        vertex_table_model_->setHorizontalHeaderItem(1, x);
        QStandardItem* y = new QStandardItem("Y");
        y->setTextAlignment(Qt::AlignRight);
        vertex_table_model_->setHorizontalHeaderItem(2, y);
        QStandardItem* z = new QStandardItem("Z");
        z->setTextAlignment(Qt::AlignRight);
        vertex_table_model_->setHorizontalHeaderItem(3, z);

        table_view->setModel(vertex_table_model_);

        widget_util::SetTableModelData(vertex_table_model_, "Vertex 0", 0, 0);
        widget_util::SetTableModelData(vertex_table_model_, "Vertex 1", 1, 0);
        widget_util::SetTableModelData(vertex_table_model_, "Vertex 2", 2, 0);
        widget_util::SetTableModelData(vertex_table_model_, "Vertex 3", 3, 0);
    }

    void BlasViewerModel::InitializeFlagsTableModel(ScaledTableView* table_view)
    {
        flags_table_model_ = new FlagsTableItemModel();
        flags_table_model_->SetRowCount(2);
        flags_table_model_->SetColumnCount(2);

        table_view->setModel(flags_table_model_);

        flags_table_model_->SetRowFlagName(0, "Opaque");
        flags_table_model_->SetRowFlagName(1, "No duplicate any hit invocation");

        flags_table_model_->Initialize(table_view);

        table_view->GetHeaderView()->setVisible(false);
    }

    void BlasViewerModel::PopulateFlagsTable(uint32_t flags)
    {
        bool opaque                          = flags & VK_GEOMETRY_OPAQUE_BIT_KHR;
        bool no_duplicate_any_hit_invocation = flags & VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;

        flags_table_model_->SetRowChecked(0, opaque);
        flags_table_model_->SetRowChecked(1, no_duplicate_any_hit_invocation);

        // The table will not be updated without this.
        flags_table_model_->dataChanged(QModelIndex(), QModelIndex());
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

    void BlasViewerModel::UpdateStatistics(uint64_t blas_index, uint32_t node_id)
    {
        // Show node name and base address.
        SetModelData(kBlasStatsType, RraBvhGetNodeName(node_id));

        uint64_t           node_address      = 0;
        RraErrorCode       error_code        = kRraErrorInvalidPointer;
        TreeviewNodeIDType node_type         = rra::Settings::Get().GetTreeviewNodeIdType();
        int                decimal_precision = rra::Settings::Get().GetDecimalPrecision();

        switch (node_type)
        {
        case kTreeviewNodeIDTypeVirtualAddress:
            error_code = RraBlasGetNodeBaseAddress(blas_index, node_id, &node_address);
            break;

        case kTreeviewNodeIDTypeOffset:
            error_code = RraBvhGetNodeOffset(node_id, &node_address);
            break;

        default:
            break;
        }

        if (error_code == kRraOk)
        {
            QString address_string = "0x" + QString("%1").arg(node_address, 0, 16);
            SetModelData(kBlasStatsAddress, address_string);
        }

        // Show surface area and bounding box extents.
        BoundingVolumeExtents bounding_volume_extents;
        if (RraBlasGetBoundingVolumeExtents(blas_index, node_id, &bounding_volume_extents) == kRraOk)
        {
            PopulateExtentsTable(bounding_volume_extents);
        }

        // Show vertex data.
        if (SelectedNodeIsLeaf())
        {
            uint32_t tri_count{};
            RraBlasGetNodeTriangleCount(blas_index, node_id, &tri_count);
            uint32_t                    vertex_count = (tri_count == 1 ? 3 : 4);
            std::vector<VertexPosition> verts(vertex_count);
            RraBlasGetNodeVertices(blas_index, node_id, verts.data());

            widget_util::SetTableModelDecimalData(vertex_table_model_, verts[0].x, 0, 1, Qt::AlignRight);
            widget_util::SetTableModelDecimalData(vertex_table_model_, verts[0].y, 0, 2, Qt::AlignRight);
            widget_util::SetTableModelDecimalData(vertex_table_model_, verts[0].z, 0, 3, Qt::AlignRight);
            widget_util::SetTableModelDecimalData(vertex_table_model_, verts[1].x, 1, 1, Qt::AlignRight);
            widget_util::SetTableModelDecimalData(vertex_table_model_, verts[1].y, 1, 2, Qt::AlignRight);
            widget_util::SetTableModelDecimalData(vertex_table_model_, verts[1].z, 1, 3, Qt::AlignRight);
            widget_util::SetTableModelDecimalData(vertex_table_model_, verts[2].x, 2, 1, Qt::AlignRight);
            widget_util::SetTableModelDecimalData(vertex_table_model_, verts[2].y, 2, 2, Qt::AlignRight);
            widget_util::SetTableModelDecimalData(vertex_table_model_, verts[2].z, 2, 3, Qt::AlignRight);

            if (tri_count == 2)
            {
                widget_util::SetTableModelData(vertex_table_model_, "Vertex 3", 3, 0);
                widget_util::SetTableModelDecimalData(vertex_table_model_, verts[3].x, 3, 1, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_model_, verts[3].y, 3, 2, Qt::AlignRight);
                widget_util::SetTableModelDecimalData(vertex_table_model_, verts[3].z, 3, 3, Qt::AlignRight);
            }
            else
            {
                widget_util::SetTableModelData(vertex_table_model_, "", 3, 0);
                widget_util::SetTableModelData(vertex_table_model_, "", 3, 1);
                widget_util::SetTableModelData(vertex_table_model_, "", 3, 2);
                widget_util::SetTableModelData(vertex_table_model_, "", 3, 3);
            }
        }
        uint64_t parent_address{};
        RraBlasGetNodeParentBaseAddress(blas_index, node_id, &parent_address);
        QString parent_string = "0x" + QString("%1").arg(parent_address, 0, 16);
        SetModelData(kBlasStatsParent, parent_string);

        // Show surface area heuristic.
        float surface_area_heuristic = 0.0;
        if (RraBlasGetSurfaceAreaHeuristic(blas_index, node_id, &surface_area_heuristic) == kRraOk)
        {
            SetModelData(kBlasStatsCurrentSAH,
                         QString::number(surface_area_heuristic, kQtFloatFormat, decimal_precision),
                         QString::number(surface_area_heuristic, kQtFloatFormat, kQtTooltipFloatPrecision));
        }

        uint32_t primitive_index{};
        if (RraBlasGetPrimitiveIndex(blas_index, node_id, &primitive_index) == kRraOk)
        {
            SetModelData(kBlasStatsPrimitiveIndex, QString::number(primitive_index));
        }

        uint32_t geometry_index{};
        if (RraBlasGetGeometryIndex(blas_index, node_id, &geometry_index) == kRraOk)
        {
            // Show geometry flag here.
            SetModelData(kBlasStatsGeometryIndex, QString::number(geometry_index));

            uint32_t geometry_flags{};
            if (RraBlasGetGeometryFlags(blas_index, geometry_index, &geometry_flags) == kRraOk)
            {
                PopulateFlagsTable(geometry_flags);
            }
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
            last_selected_node_is_tri_ = RraBvhIsTriangleNode(node_id);
        }
        else
        {
            last_selected_node_is_tri_ = false;
        }
    }

    void BlasViewerModel::ResetModelValues(bool reset_scene)
    {
        SetModelData(kBlasStatsType, "No node selected.");
        SetModelData(kBlasStatsAddress, "");
        SetModelData(kBlasStatsCurrentSAH, "-");
        SetModelData(kBlasStatsSAHSubTreeMax, "-");
        SetModelData(kBlasStatsSAHSubTreeMean, "-");
        last_selected_node_is_tri_ = false;
        AccelerationStructureViewerModel::ResetModelValues(reset_scene);
    }

}  // namespace rra
