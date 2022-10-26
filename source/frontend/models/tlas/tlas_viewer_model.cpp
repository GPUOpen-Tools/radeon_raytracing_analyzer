//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the TLAS viewer model.
//=============================================================================

#include "models/tlas/tlas_viewer_model.h"
#include "models/tlas/tlas_scene_collection_model.h"

#include "qt_common/custom_widgets/arrow_icon_combo_box.h"
#include "qt_common/custom_widgets/scaled_tree_view.h"
#include "qt_common/utils/qt_util.h"

#include "public/rra_assert.h"
#include "public/rra_blas.h"
#include "public/rra_tlas.h"

#include "constants.h"
#include "models/acceleration_structure_tree_view_model.h"
#include "models/acceleration_structure_viewer_model.h"
#include "settings/settings.h"
#include "views/widget_util.h"

namespace rra
{
    // Flag to indicate if this model represents a TLAS.
    static bool kIsTlasModel = true;

    TlasViewerModel::TlasViewerModel(ScaledTreeView* tree_view)
        : AccelerationStructureViewerModel(tree_view, kTlasStatsNumWidgets, kIsTlasModel)
    {
        scene_collection_model_ = new TlasSceneCollectionModel();
    }

    TlasViewerModel::~TlasViewerModel()
    {
        delete position_table_model_;
    }

    void TlasViewerModel::InitializeTransformTableModel(ScaledTableView* table_view)
    {
        transform_table_model_ = new QStandardItemModel(3, 3);

        table_view->setModel(transform_table_model_);
        table_view->GetHeaderView()->setVisible(false);
    }

    void TlasViewerModel::InitializePositionTableModel(ScaledTableView* table_view)
    {
        position_table_model_ = new QStandardItemModel(3, 1);

        table_view->setModel(position_table_model_);
        table_view->GetHeaderView()->setVisible(false);
    }

    RraErrorCode TlasViewerModel::AccelerationStructureGetCount(uint64_t* out_count) const
    {
        return RraBvhGetTlasCount(out_count);
    }

    RraErrorCode TlasViewerModel::AccelerationStructureGetBaseAddress(uint64_t index, uint64_t* out_address) const
    {
        return RraTlasGetBaseAddress(index, out_address);
    }

    RraErrorCode TlasViewerModel::AccelerationStructureGetTotalNodeCount(uint64_t index, uint64_t* out_node_count) const
    {
        return RraTlasGetTotalNodeCount(index, out_node_count);
    }

    bool TlasViewerModel::AccelerationStructureGetIsEmpty(uint64_t index) const
    {
        return RraTlasIsEmpty(index);
    }

    GetChildNodeFunction TlasViewerModel::AccelerationStructureGetChildNodeFunction() const
    {
        return RraTlasGetChildNodePtr;
    }

    uint64_t TlasViewerModel::GetBlasIndex(int tlas_index, const QModelIndex& model_index) const
    {
        uint64_t blas_index = 0;
        uint32_t node_id    = GetNodeIdFromModelIndex(model_index, tlas_index, kIsTlasModel);
        if (RraTlasGetBlasIndexFromInstanceNode(tlas_index, node_id, &blas_index) != kRraOk)
        {
            return UINT64_MAX;
        }
        return blas_index;
    }

    uint32_t TlasViewerModel::GetInstanceIndex(int tlas_index, const QModelIndex& model_index) const
    {
        uint32_t instance_index = 0;
        uint32_t node_id        = GetNodeIdFromModelIndex(model_index, tlas_index, kIsTlasModel);
        if (RraTlasGetInstanceIndexFromInstanceNode(tlas_index, node_id, &instance_index) != kRraOk)
        {
            return UINT32_MAX;
        }
        return instance_index;
    }

    uint32_t TlasViewerModel::GetInstanceIndexFromNode(int tlas_index, const uint32_t node_id) const
    {
        uint32_t instance_index = 0;
        if (RraTlasGetInstanceIndexFromInstanceNode(tlas_index, node_id, &instance_index) != kRraOk)
        {
            return UINT32_MAX;
        }
        return instance_index;
    }

    uint64_t TlasViewerModel::BlasValid(int blas_index) const
    {
        if (blas_index != ULLONG_MAX && !RraBlasIsEmpty(blas_index))
        {
            return true;
        }
        return false;
    }

    void TlasViewerModel::UpdateUI(const QModelIndex& model_index, uint64_t tlas_index)
    {
        AccelerationStructureViewerModel::SetSelectedNodeIndex(model_index);
        uint32_t node_id = GetNodeIdFromModelIndex(model_index, tlas_index, kIsTlasModel);

        // Show Node name and base address.
        SetModelData(kTlasStatsType, RraBvhGetNodeName(node_id));

        uint64_t           node_address = 0;
        RraErrorCode       error_code   = kRraErrorInvalidPointer;
        TreeviewNodeIDType node_type    = rra::Settings::Get().GetTreeviewNodeIdType();

        switch (node_type)
        {
        case kTreeviewNodeIDTypeVirtualAddress:
            error_code = RraTlasGetNodeBaseAddress(tlas_index, node_id, &node_address);
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
            SetModelData(kTlasStatsAddress, address_string);
        }

        // Show instance node info.
        uint64_t blas_address   = 0;
        uint64_t instance_count = 0;
        bool     is_empty       = false;
        if (SelectedNodeIsLeaf())
        {
            RraTlasGetInstanceNodeInfo(tlas_index, node_id, &blas_address, &instance_count, &is_empty);
            if (is_empty)
            {
                return;
            }

            QString address_string = "0x" + QString("%1").arg(blas_address, 0, 16);
            SetModelData(kTlasStatsBlasAddress, address_string);

            uint32_t instance_id{};
            RraTlasGetInstanceNodeID(tlas_index, node_id, &instance_id);
            SetModelData(kTlasStatsInstanceId, QString::number(instance_id));

            uint32_t instance_mask{};
            RraTlasGetInstanceNodeMask(tlas_index, node_id, &instance_mask);
            SetModelData(kTlasStatsInstanceMask, QString("0x%1%2").arg((instance_mask & 0xF0) >> 4, 0, 16).arg(instance_mask & 0x0F, 0, 16));

            uint32_t instance_hit_group{};
            RraTlasGetInstanceNodeHitGroup(tlas_index, node_id, &instance_hit_group);
            SetModelData(kTlasStatsInstanceHitGroupIndex, QString::number(instance_hit_group));

            // Row major 3x4 matrix.
            float instance_transform[12] = {};
            RraTlasGetOriginalInstanceNodeTransform(tlas_index, node_id, instance_transform);

            widget_util::SetTableModelDecimalData(transform_table_model_, instance_transform[0], 0, 0, Qt::AlignRight);
            widget_util::SetTableModelDecimalData(transform_table_model_, instance_transform[1], 0, 1, Qt::AlignRight);
            widget_util::SetTableModelDecimalData(transform_table_model_, instance_transform[2], 0, 2, Qt::AlignRight);
            widget_util::SetTableModelDecimalData(position_table_model_, instance_transform[3], 0, 0, Qt::AlignRight);

            widget_util::SetTableModelDecimalData(transform_table_model_, instance_transform[4], 1, 0, Qt::AlignRight);
            widget_util::SetTableModelDecimalData(transform_table_model_, instance_transform[5], 1, 1, Qt::AlignRight);
            widget_util::SetTableModelDecimalData(transform_table_model_, instance_transform[6], 1, 2, Qt::AlignRight);
            widget_util::SetTableModelDecimalData(position_table_model_, instance_transform[7], 1, 0, Qt::AlignRight);

            widget_util::SetTableModelDecimalData(transform_table_model_, instance_transform[8], 2, 0, Qt::AlignRight);
            widget_util::SetTableModelDecimalData(transform_table_model_, instance_transform[9], 2, 1, Qt::AlignRight);
            widget_util::SetTableModelDecimalData(transform_table_model_, instance_transform[10], 2, 2, Qt::AlignRight);
            widget_util::SetTableModelDecimalData(position_table_model_, instance_transform[11], 2, 0, Qt::AlignRight);
        }
        else
        {
            SetModelData(kTlasStatsBlasAddress, "");
            SetModelData(kTlasStatsParent, "");
            SetModelData(kTlasStatsInstanceId, "");
            SetModelData(kTlasStatsInstanceMask, "");
            SetModelData(kTlasStatsInstanceHitGroupIndex, "");
        }

        uint64_t parent_address{};
        RraTlasGetNodeParentBaseAddress(tlas_index, node_id, &parent_address);
        QString parent_string = "0x" + QString("%1").arg(parent_address, 0, 16);
        SetModelData(kTlasStatsParent, parent_string);

        // Show bounding box extents.
        BoundingVolumeExtents bounding_volume_extents;
        if (RraTlasGetBoundingVolumeExtents(tlas_index, node_id, &bounding_volume_extents) == kRraOk)
        {
            PopulateExtentsTable(bounding_volume_extents);
        }

        uint32_t instance_flags{};
        if (RraTlasGetInstanceFlags(tlas_index, node_id, &instance_flags) == kRraOk)
        {
            PopulateFlagsTable(instance_flags);
        }
    }

    void TlasViewerModel::InitializeFlagsTableModel(ScaledTableView* table_view)
    {
        flags_table_model_ = new FlagsTableItemModel();
        flags_table_model_->SetRowCount(4);
        flags_table_model_->SetColumnCount(2);

        table_view->setModel(flags_table_model_);

        flags_table_model_->SetRowFlagName(0, "Triangle facing cull disable");
        flags_table_model_->SetRowFlagName(1, "Triangle flip facing");
        flags_table_model_->SetRowFlagName(2, "Force opaque");
        flags_table_model_->SetRowFlagName(3, "Force no opaque");

        flags_table_model_->Initialize(table_view);

        table_view->GetHeaderView()->setVisible(false);
    }

    void TlasViewerModel::PopulateFlagsTable(uint32_t flags)
    {
        bool triangle_facing_cull_disable = flags & VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        bool triangle_flip_facing         = flags & VK_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT_KHR;
        bool force_opaque                 = flags & VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
        bool force_no_opaque              = flags & VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR;

        flags_table_model_->SetRowChecked(0, triangle_facing_cull_disable);
        flags_table_model_->SetRowChecked(1, triangle_flip_facing);
        flags_table_model_->SetRowChecked(2, force_opaque);
        flags_table_model_->SetRowChecked(3, force_no_opaque);

        // The table will not be updated without this.
        flags_table_model_->dataChanged(QModelIndex(), QModelIndex());
    }

    void TlasViewerModel::SetSceneSelection(const QModelIndex& model_index, uint64_t index)
    {
        uint32_t node_id = GetNodeIdFromModelIndex(model_index, index, kIsTlasModel);

        Scene* current_scene_info_ = scene_collection_model_->GetSceneByIndex(index);
        current_scene_info_->SetSceneSelection(node_id);
    }

    bool TlasViewerModel::SelectedNodeIsLeaf() const
    {
        return last_selected_node_is_instance_;
    }

    void TlasViewerModel::UpdateLastSelectedNodeIsLeaf(const QModelIndex& model_index, uint64_t index)
    {
        if (model_index.isValid())
        {
            uint32_t node_id                = GetNodeIdFromModelIndex(model_index, index, kIsTlasModel);
            last_selected_node_is_instance_ = RraBvhIsInstanceNode(node_id);
        }
        else
        {
            last_selected_node_is_instance_ = false;
        }
    }

    void TlasViewerModel::ResetModelValues(bool reset_scene)
    {
        SetModelData(kTlasStatsType, "No node selected.");
        SetModelData(kTlasStatsAddress, "");
        SetModelData(kTlasStatsBlasAddress, "-");
        SetModelData(kTlasStatsParent, "-");
        SetModelData(kTlasStatsInstanceId, "-");
        SetModelData(kTlasStatsInstanceMask, "-");
        SetModelData(kTlasStatsInstanceHitGroupIndex, "-");
        AccelerationStructureViewerModel::ResetModelValues(reset_scene);
    }

}  // namespace rra
