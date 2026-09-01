//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the TLAS viewer model.
//=============================================================================

#include "models/tlas/tlas_viewer_model.h"

#include <array>

#include "qt_common/custom_widgets/arrow_icon_combo_box.h"
#include "qt_common/custom_widgets/scaled_tree_view.h"
#include "qt_common/utils/qt_util.h"

#include "public/rra_assert.h"
#include "public/rra_blas.h"
#include "public/rra_rtip_info.h"
#include "public/rra_tlas.h"

#include "constants.h"
#include "models/acceleration_structure_tree_view_model.h"
#include "models/acceleration_structure_viewer_model.h"
#include "models/tlas/tlas_scene_collection_model.h"
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
        delete transform_table_model_;
        delete flags_table_model_;
    }

    void TlasViewerModel::InitializeTransformTableModel(ScaledTableView* table_view)
    {
        transform_table_model_ = new QStandardItemModel(3, 3);

        table_view->setModel(transform_table_model_);
        table_view->horizontalHeader()->setVisible(false);
    }

    void TlasViewerModel::InitializePositionTableModel(ScaledTableView* table_view)
    {
        position_table_model_ = new QStandardItemModel(3, 1);

        table_view->setModel(position_table_model_);
        table_view->horizontalHeader()->setVisible(false);
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

    bool TlasViewerModel::IsNodeSelectable(int tlas_index, const QModelIndex& model_index) const
    {
        uint32_t node_id = GetNodeIdFromModelIndex(model_index, tlas_index, kIsTlasModel);

        SceneNode* node = last_clicked_node_scene_->GetNodeById(node_id);
        if (node && (node->IsEnabled() && node->IsVisible()))
        {
            return true;
        }
        return false;
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
        if (RraTlasGetUniqueInstanceIndexFromInstanceNode(tlas_index, node_id, &instance_index) != kRraOk)
        {
            return UINT32_MAX;
        }
        return instance_index;
    }

    uint32_t TlasViewerModel::GetInstanceUniqueIndexFromNode(int tlas_index, const uint32_t node_id) const
    {
        uint32_t instance_index = 0;
        if (RraTlasGetUniqueInstanceIndexFromInstanceNode(tlas_index, node_id, &instance_index) != kRraOk)
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

    bool TlasViewerModel::BlasValid(uint64_t blas_index) const
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
        uint32_t node_id     = GetNodeIdFromModelIndex(model_index, tlas_index, kIsTlasModel);
        uint32_t child_index = GetChildIndexFromModelIndex(model_index);

        const char*  node_str{};
        RraErrorCode error_code = RraTlasGetNodeName(tlas_index, node_id, &node_str);
        RRA_ASSERT(error_code == kRraOk);
        std::string node_type{node_str};

        if (IsRebraidedNode(tlas_index))
        {
            node_type += " (rebraided)";
        }

        // RTIP3.1 node packing: count how many sibling parent slots reference this node. When more than one, the node is
        // shared ("packed") -- surface the count in the stats and annotate the type label inline (mirrors the BLAS tab).
        last_selected_packed_ref_count_ = 1;
        if (RraTlasHasNodePacking(tlas_index))
        {
            uint32_t     packing_parent_id{};
            RraErrorCode packing_parent_error = RraTlasGetNodeParent(tlas_index, node_id, &packing_parent_id);
            if (packing_parent_error == kRraOk && packing_parent_id != std::numeric_limits<uint32_t>::max())
            {
                uint32_t child_count{};
                if (RraTlasGetChildNodeCount(tlas_index, packing_parent_id, &child_count) == kRraOk)
                {
                    std::array<uint32_t, MAX_CHILD_NODES> child_nodes{};
                    if (RraTlasGetChildNodes(tlas_index, packing_parent_id, child_nodes.data()) == kRraOk)
                    {
                        uint32_t refs = 0;
                        for (uint32_t i = 0; i < child_count; ++i)
                        {
                            if (child_nodes[i] == node_id)
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
        }

        if (last_selected_packed_ref_count_ > 1)
        {
            SetModelData(kTlasStatsPackedRefCount, QString::number(last_selected_packed_ref_count_) + " boxes");
            node_type += " (packed)";
        }

        // Show Node name and base address.
        SetModelData(kTlasStatsType, node_type.c_str());

        // Show the focus button.
        SetModelData(kTlasStatsFocus, true);

        SetModelData(kTlasStatsAddress, AddressString(tlas_index, node_id));

        uint32_t parent_id{};
        error_code = RraTlasGetNodeParent(tlas_index, node_id, &parent_id);
        RRA_ASSERT(error_code == kRraOk);
        RRA_UNUSED(error_code);
        bool parent_valid{parent_id != std::numeric_limits<uint32_t>::max()};
        if (parent_valid)
        {
            SetModelData(kTlasStatsParent, AddressString(tlas_index, parent_id));
        }

        // Show instance node info.
        uint64_t blas_address   = 0;
        uint64_t instance_count = 0;
        bool     is_empty       = false;
        if (SelectedNodeIsLeaf())
        {
            error_code = RraTlasGetInstanceNodeInfo(tlas_index, node_id, &blas_address, &instance_count, &is_empty);
            RRA_ASSERT(error_code == kRraOk);
            if (is_empty)
            {
                return;
            }

            QString address_string = "0x" + QString("%1").arg(blas_address, 0, 16);
            SetModelData(kTlasStatsBlasAddress, address_string);

            uint32_t instance_index{};
            error_code = RraTlasGetInstanceIndexFromInstanceNode(tlas_index, node_id, &instance_index);
            RRA_ASSERT(error_code == kRraOk);
            SetModelData(kTlasStatsInstanceIndex, QString::number(instance_index));

            uint32_t instance_id{};
            error_code = RraTlasGetInstanceNodeID(tlas_index, node_id, &instance_id);
            RRA_ASSERT(error_code == kRraOk);
            SetModelData(kTlasStatsInstanceId, QString::number(instance_id));

            uint32_t instance_mask{};
            error_code = RraTlasGetInstanceNodeMask(tlas_index, node_id, &instance_mask);
            RRA_ASSERT(error_code == kRraOk);
            SetModelData(kTlasStatsInstanceMask, QString("0x%1%2").arg((instance_mask & 0xF0) >> 4, 0, 16).arg(instance_mask & 0x0F, 0, 16));

            uint32_t instance_hit_group{};
            error_code = RraTlasGetInstanceNodeHitGroup(tlas_index, node_id, &instance_hit_group);
            RRA_ASSERT(error_code == kRraOk);
            SetModelData(kTlasStatsInstanceHitGroupIndex, QString::number(instance_hit_group));

            // Row major 3x4 matrix.
            float instance_transform[12] = {};
            error_code                   = RraTlasGetOriginalInstanceNodeTransform(tlas_index, node_id, instance_transform);
            RRA_ASSERT(error_code == kRraOk);

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
            SetModelData(kTlasStatsInstanceIndex, "");
            SetModelData(kTlasStatsInstanceId, "");
            SetModelData(kTlasStatsInstanceMask, "");
            SetModelData(kTlasStatsInstanceHitGroupIndex, "");

            if (RraRtipInfoGetOBBSupported())
            {
                glm::mat3 rotation(1.0f);
                if (parent_valid)
                {
                    RraErrorCode result = RraTlasGetNodeBoundingVolumeOrientation(tlas_index, parent_id, &rotation[0][0]);
                    RRA_ASSERT(result == kRraOk);
                }
                PopulateRotationTable(rotation);
            }
        }

        // Show bounding box extents.
        BoundingVolumeExtents bounding_volume_extents;
        if (RraTlasGetBoundingVolumeExtents(tlas_index, node_id, child_index, &bounding_volume_extents) == kRraOk)
        {
            PopulateExtentsTable(bounding_volume_extents);
        }

        uint32_t instance_flags{};
        if (RraTlasGetInstanceFlags(tlas_index, node_id, &instance_flags) == kRraOk)
        {
            PopulateFlagsTable(instance_flags);
        }
    }

    QString TlasViewerModel::UpdateToolTip(uint64_t bvh_index, rra::SceneCollectionModelClosestHit closest_hit)
    {
        uint32_t instance_node = (uint32_t)closest_hit.instance_node;
        if (instance_node == UINT32_MAX || !render_state_adapter_)
        {
            return "";
        }

        uint64_t   blas_index    = closest_hit.blas_index;
        uint64_t   packed_node   = closest_hit.triangle_child_node;
        SceneNode* triangle_node = closest_hit.triangle_node;
        bool       has_triangle  = (packed_node != UINT32_MAX) && (triangle_node != nullptr);

        uint32_t node_id            = (uint32_t)(packed_node & UINT32_MAX);
        uint32_t global_child_index = (uint32_t)(packed_node >> 32);

        int decimal_precision = rra::Settings::Get().GetDecimalPrecision();

        renderer::GeometryColoringMode mode = render_state_adapter_->GetCurrentGeometryColoringModeValue();

        switch (mode)
        {
        case renderer::GeometryColoringMode::kBlasAverageSAH:
        {
            uint32_t root_node = UINT32_MAX;
            if (RraBvhGetRootNodePtr(&root_node) == kRraOk)
            {
                float sah = 0.0f;
                if (RraBlasGetAverageSurfaceAreaHeuristic(blas_index, root_node, 0, true, &sah) == kRraOk)
                    return "Avg. SAH: " + QString::number(sah, kQtFloatFormat, decimal_precision);
            }
            break;
        }
        case renderer::GeometryColoringMode::kBlasMinSAH:
        {
            uint32_t root_node = UINT32_MAX;
            if (RraBvhGetRootNodePtr(&root_node) == kRraOk)
            {
                float sah = 0.0f;
                if (RraBlasGetMinimumSurfaceAreaHeuristic(blas_index, root_node, 0, true, &sah) == kRraOk)
                    return "Min. SAH: " + QString::number(sah, kQtFloatFormat, decimal_precision);
            }
            break;
        }
        case renderer::GeometryColoringMode::kTriangleSAH:
        {
            if (!has_triangle)
                break;
            float sah = 0.0f;
            if (RraBlasGetSurfaceAreaHeuristic(blas_index, node_id, global_child_index, &sah) == kRraOk)
                return "SAH: " + QString::number(sah, kQtFloatFormat, decimal_precision);
            break;
        }
        case renderer::GeometryColoringMode::kTreeLevel:
        {
            if (!has_triangle)
                break;
            return "Depth: " + QString::number(triangle_node->GetDepth());
        }
        case renderer::GeometryColoringMode::kBlasMaxDepth:
        {
            uint32_t depth = 0;
            if (RraBlasGetMaxTreeDepth(blas_index, &depth) == kRraOk)
                return "Max depth: " + QString::number(depth);
            break;
        }
        case renderer::GeometryColoringMode::kBlasAverageDepth:
        {
            uint32_t depth = 0;
            if (RraBlasGetAvgTreeDepth(blas_index, &depth) == kRraOk)
                return "Avg. depth: " + QString::number(depth);
            break;
        }
        case renderer::GeometryColoringMode::kBlasInstanceId:
            return "BLAS index: " + QString::number(blas_index);
        case renderer::GeometryColoringMode::kInstanceIndex:
        {
            uint32_t unique_index = 0;
            if (RraTlasGetUniqueInstanceIndexFromInstanceNode(bvh_index, instance_node, &unique_index) == kRraOk)
                return "Instance index: " + QString::number(unique_index);
            break;
        }
        case renderer::GeometryColoringMode::kPartitionIndex:
        {
            uint32_t instance_index = 0;
            if (RraTlasGetInstanceIndexFromInstanceNode(bvh_index, instance_node, &instance_index) == kRraOk)
            {
                uint32_t partition_index = 0;
                if (RraTlasGetInstancePartitionIndex(bvh_index, instance_index, &partition_index, nullptr) == kRraOk)
                    return "Partition index: " + QString::number(partition_index);
            }
            break;
        }
        case renderer::GeometryColoringMode::kBlasInstanceCount:
        {
            uint64_t count = 0;
            if (RraTlasGetInstanceCount(bvh_index, blas_index, &count) == kRraOk)
                return "Instance count: " + QString::number(count);
            break;
        }
        case renderer::GeometryColoringMode::kBlasTriangleCount:
        {
            uint32_t count = 0;
            if (RraBlasGetUniqueTriangleCount(blas_index, &count) == kRraOk)
                return "Triangle count: " + QString::number(count);
            break;
        }
        case renderer::GeometryColoringMode::kInstanceMask:
        {
            uint32_t mask = 0;
            if (RraTlasGetInstanceNodeMask(bvh_index, instance_node, &mask) == kRraOk)
                return "Mask: 0x" + QString("%1").arg(mask, 2, 16, QChar('0'));
            break;
        }
        case renderer::GeometryColoringMode::kGeometryIndex:
        {
            if (!has_triangle)
                break;
            return "Geometry index: " + QString::number(triangle_node->GetGeometryIndex());
        }
        case renderer::GeometryColoringMode::kOpacity:
        {
            if (!has_triangle)
                break;
            uint32_t geometry_flags = 0;
            if (RraBlasGetGeometryFlags(blas_index, triangle_node->GetGeometryIndex(), &geometry_flags) == kRraOk)
            {
                bool is_opaque = (geometry_flags & GeometryFlags::kOpaque) != 0;
                return QString("Opaque: ") + (is_opaque ? "yes" : "no");
            }
            break;
        }
        case renderer::GeometryColoringMode::kFinalOpacity:
        {
            if (!has_triangle)
                break;
            uint32_t     instance_flags = 0;
            uint32_t     geometry_flags = 0;
            RraErrorCode error_code     = RraTlasGetInstanceFlags(bvh_index, instance_node, &instance_flags);
            RRA_ASSERT(error_code == kRraOk);
            error_code = RraBlasGetGeometryFlags(blas_index, triangle_node->GetGeometryIndex(), &geometry_flags);
            RRA_ASSERT(error_code == kRraOk);
            RRA_UNUSED(error_code);
            bool geo_opaque      = (geometry_flags & GeometryFlags::kOpaque) != 0;
            bool force_opaque    = (instance_flags & VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR) != 0;
            bool force_no_opaque = (instance_flags & VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR) != 0;
            bool final_opaque    = force_opaque || (!force_no_opaque && geo_opaque);
            return QString("Opaque: ") + (final_opaque ? "yes" : "no");
        }
        case renderer::GeometryColoringMode::kInstanceForceOpaqueOrNoOpaqueBits:
        {
            uint32_t instance_flags = 0;
            if (RraTlasGetInstanceFlags(bvh_index, instance_node, &instance_flags) == kRraOk)
            {
                bool force_opaque    = (instance_flags & VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR) != 0;
                bool force_no_opaque = (instance_flags & VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR) != 0;
                if (force_opaque)
                    return "Force opaque";
                if (force_no_opaque)
                    return "Force no opaque";
                return "No force";
            }
            break;
        }
        case renderer::GeometryColoringMode::kFastBuildOrTraceFlag:
        {
            VkBuildAccelerationStructureFlagBitsKHR flags{};
            if (RraBlasGetBuildFlags(blas_index, &flags) == kRraOk)
            {
                bool fast_trace = (flags & VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR) != 0;
                bool fast_build = (flags & VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR) != 0;
                if (fast_trace)
                    return "Fast trace";
                if (fast_build)
                    return "Fast build";
                return "None";
            }
            break;
        }
        case renderer::GeometryColoringMode::kAllowUpdateFlag:
        {
            VkBuildAccelerationStructureFlagBitsKHR flags{};
            if (RraBlasGetBuildFlags(blas_index, &flags) == kRraOk)
            {
                bool allow_update = (flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR) != 0;
                return QString("Allow update: ") + (allow_update ? "yes" : "no");
            }
            break;
        }
        case renderer::GeometryColoringMode::kAllowCompactionFlag:
        {
            VkBuildAccelerationStructureFlagBitsKHR flags{};
            if (RraBlasGetBuildFlags(blas_index, &flags) == kRraOk)
            {
                bool allow_compaction = (flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR) != 0;
                return QString("Allow compaction: ") + (allow_compaction ? "yes" : "no");
            }
            break;
        }
        case renderer::GeometryColoringMode::kLowMemoryFlag:
        {
            VkBuildAccelerationStructureFlagBitsKHR flags{};
            if (RraBlasGetBuildFlags(blas_index, &flags) == kRraOk)
            {
                bool low_memory = (flags & VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR) != 0;
                return QString("Low memory: ") + (low_memory ? "yes" : "no");
            }
            break;
        }
        case renderer::GeometryColoringMode::kInstanceFacingCullDisableBit:
        {
            uint32_t instance_flags = 0;
            if (RraTlasGetInstanceFlags(bvh_index, instance_node, &instance_flags) == kRraOk)
            {
                bool cull_disable = (instance_flags & VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR) != 0;
                return QString("Facing cull disable: ") + (cull_disable ? "yes" : "no");
            }
            break;
        }
        case renderer::GeometryColoringMode::kInstanceFlipFacingBit:
        {
            uint32_t instance_flags = 0;
            if (RraTlasGetInstanceFlags(bvh_index, instance_node, &instance_flags) == kRraOk)
            {
                bool flip_facing = (instance_flags & VK_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT_KHR) != 0;
                return QString("Flip facing: ") + (flip_facing ? "yes" : "no");
            }
            break;
        }
        case renderer::GeometryColoringMode::kInstanceRebraiding:
        {
            uint32_t unique_index = 0;
            if (RraTlasGetUniqueInstanceIndexFromInstanceNode(bvh_index, instance_node, &unique_index) == kRraOk)
            {
                Scene* tlas_scene = scene_collection_model_->GetSceneByIndex(bvh_index);
                if (tlas_scene)
                {
                    bool rebraided = tlas_scene->IsInstanceRebraided(unique_index);
                    return QString("Rebraided: ") + (rebraided ? "yes" : "no");
                }
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

        table_view->horizontalHeader()->setVisible(false);
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
        uint32_t node_id            = GetNodeIdFromModelIndex(model_index, index, kIsTlasModel);
        uint32_t global_child_index = GetGlobalChildIndexFromModelIndex(model_index);
        uint64_t node_child_id      = ((uint64_t)global_child_index << 32) | node_id;

        Scene* scene = scene_collection_model_->GetSceneByIndex(index);
        scene->SetSceneSelection(node_child_id);
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
            last_selected_node_is_instance_ = RraTlasIsInstanceNode(index, node_id);
        }
        else
        {
            last_selected_node_is_instance_ = false;
        }
    }

    QString TlasViewerModel::AddressString(uint64_t bvh_index, uint32_t node_id) const
    {
        TreeviewNodeIDType node_type    = rra::Settings::Get().GetTreeviewNodeIdType();
        uint64_t           node_address = 0;

        RraErrorCode error_code = kRraOk;
        switch (node_type)
        {
        case kTreeviewNodeIDTypeVirtualAddress:
            error_code = RraTlasGetNodeBaseAddress(bvh_index, node_id, &node_address);
            RRA_ASSERT(error_code == kRraOk);
            break;

        case kTreeviewNodeIDTypeOffset:
            error_code = RraBvhGetNodeOffset(node_id, &node_address);
            RRA_ASSERT(error_code == kRraOk);
            break;

        default:
            break;
        }

        RRA_UNUSED(error_code);
        return "0x" + QString("%1").arg(node_address, 0, 16);
    }

    void TlasViewerModel::ResetModelValues(bool reset_scene)
    {
        SetModelData(kTlasStatsType, "No node selected.");
        SetModelData(kTlasStatsFocus, false);
        SetModelData(kTlasStatsAddress, "");
        SetModelData(kTlasStatsBlasAddress, "-");
        SetModelData(kTlasStatsParent, "-");
        SetModelData(kTlasStatsInstanceIndex, "-");
        SetModelData(kTlasStatsInstanceId, "-");
        SetModelData(kTlasStatsInstanceMask, "-");
        SetModelData(kTlasStatsInstanceHitGroupIndex, "-");
        AccelerationStructureViewerModel::ResetModelValues(reset_scene);
    }

}  // namespace rra

