//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the TLAS instances model.
//=============================================================================

#include "models/tlas/tlas_instances_model.h"

#include <QTableView>
#include <QScrollBar>
#include <QHeaderView>
#include <QSortFilterProxyModel>

#include "qt_common/utils/qt_util.h"
#include "qt_common/utils/scaling_manager.h"

#include "models/instances_item_model.h"

#include "public/rra_bvh.h"
#include "public/rra_tlas.h"

#include "../scene.h"

namespace rra
{
    TlasInstancesModel::TlasInstancesModel(int32_t num_model_widgets)
        : ModelViewMapper(num_model_widgets)
        , table_model_(nullptr)
        , proxy_model_(nullptr)
    {
    }

    TlasInstancesModel::~TlasInstancesModel()
    {
        delete table_model_;
        delete proxy_model_;
    }

    void TlasInstancesModel::ResetModelValues()
    {
        table_model_->removeRows(0, table_model_->rowCount());
        table_model_->SetRowCount(0);
        SetModelData(kTlasInstancesTlasBaseAddress, "-");
    }

    bool TlasInstancesModel::UpdateTable(uint64_t tlas_index)
    {
        uint64_t tlas_address = 0;
        if (RraTlasGetBaseAddress(tlas_index, &tlas_address) == kRraOk)
        {
            QString address_string = "TLAS base address: 0x" + QString("%1").arg(tlas_address, 0, 16);
            SetModelData(kTlasInstancesTlasBaseAddress, address_string);
        }

        // Get blas count to iterate over each.
        uint64_t blas_count = 0;
        if (RraBvhGetTotalBlasCount(&blas_count) != kRraOk || blas_count == 0)
        {
            return false;
        }

        Scene scene;
        scene.Initialize(SceneNode::ConstructFromTlas(tlas_index));

        // Get the total instance count to allocate.
        uint64_t total_instance_count = 0;

        auto blas_instance_counts = scene.GetBlasInstanceCounts();
        for (auto it = blas_instance_counts->begin(); it != blas_instance_counts->end(); it++)
        {
            total_instance_count += it->second;
        }

        if (total_instance_count == 0)
        {
            // No instances in tlas, report it as such.
            return false;
        }

#ifdef _DEBUG
        uint64_t total_instances = 0;
        for (uint64_t blas_index = 0; blas_index < blas_count; blas_index++)
        {
            uint64_t instance_count = 0;
            if (RraTlasGetInstanceCount(tlas_index, blas_index, &instance_count) != kRraOk)
            {
                return false;
            }
            total_instances += instance_count;
        }
        RRA_ASSERT(total_instances == total_instance_count);
#endif

        table_model_->SetRowCount(total_instance_count);

        // Iterate over each blas and each instance to gather data.
        InstancesTableStatistics stats      = {};
        uint64_t                 rows_added = 0;

        addressable_instance_index_.clear();

        for (uint64_t blas_index = 0; blas_index < blas_count; blas_index++)
        {
            uint64_t instance_count = 0;
            if (RraTlasGetInstanceCount(tlas_index, blas_index, &instance_count) != kRraOk)
            {
                return false;
            }

            for (uint64_t instance_index = 0; instance_index < instance_count; instance_index++)
            {
                uint32_t node_ptr = 0;
                if (RraTlasGetInstanceNode(tlas_index, blas_index, instance_index, &node_ptr) != kRraOk)
                {
                    continue;
                }

                if (RraTlasGetUniqueInstanceIndexFromInstanceNode(tlas_index, node_ptr, &stats.unique_instance_index))
                {
                    continue;
                }

                uint32_t tlas_instance_index = 0;
                if (RraTlasGetInstanceIndexFromInstanceNode(tlas_index, node_ptr, &tlas_instance_index))
                {
                    continue;
                }
                stats.instance_index = tlas_instance_index;

                auto rebraid_siblings       = scene.GetRebraidedInstances(tlas_instance_index);
                stats.rebraid_sibling_count = static_cast<uint32_t>(rebraid_siblings.size() - 1);

                if (RraTlasGetNodeBaseAddress(tlas_index, node_ptr, &stats.instance_address) != kRraOk)
                {
                    continue;
                }

                uint32_t instance_flags{};
                if (RraTlasGetInstanceFlags(tlas_index, node_ptr, &instance_flags) != kRraOk)
                {
                    continue;
                }
                stats.cull_disable_flag = instance_flags & VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
                stats.flip_facing_flag  = instance_flags & VK_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT_KHR;
                stats.force_opaque      = instance_flags & VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
                stats.force_no_opaque   = instance_flags & VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR;

                if (RraBvhGetNodeOffset(node_ptr, &stats.instance_offset) != kRraOk)
                {
                    continue;
                }

                if (RraTlasGetOriginalInstanceNodeTransform(tlas_index, node_ptr, reinterpret_cast<float*>(&stats.transform)) != kRraOk)
                {
                    continue;
                }

                uint32_t instance_mask{};
                if (RraTlasGetInstanceNodeMask(tlas_index, node_ptr, &instance_mask) != kRraOk)
                {
                    continue;
                }
                stats.instance_mask = instance_mask;

                addressable_instance_index_[rows_added] = {blas_index, instance_index};

                table_model_->AddAccelerationStructure(stats);
                rows_added++;
            }
        }

        Q_ASSERT(rows_added == total_instance_count);
        proxy_model_->invalidate();
        return total_instance_count > 0;
    }

    void TlasInstancesModel::InitializeTableModel(ScaledTableView* table_view, uint num_rows, uint num_columns)
    {
        if (proxy_model_ != nullptr)
        {
            delete proxy_model_;
            proxy_model_ = nullptr;
        }

        proxy_model_ = new InstancesProxyModel();
        table_model_ = proxy_model_->InitializeAccelerationStructureTableModels(table_view, num_rows, num_columns);
        table_model_->Initialize(table_view);
    }

    int32_t TlasInstancesModel::GetInstanceIndex(const QModelIndex& model_index)
    {
        const QModelIndex proxy_model_index = proxy_model_->mapToSource(model_index);

        if (proxy_model_index.isValid() == true)
        {
            return proxy_model_index.row();
        }
        return -1;
    }

    QModelIndex TlasInstancesModel::GetTableModelIndex(uint64_t instance_index) const
    {
        return proxy_model_->FindModelIndex(instance_index, kInstancesColumnUniqueInstanceIndex);
    }

    void TlasInstancesModel::SearchTextChanged(const QString& filter)
    {
        proxy_model_->SetSearchFilter(filter);
        proxy_model_->invalidate();
    }

    TlasInstancesModelAddress TlasInstancesModel::GetInstanceAddress(int32_t instance_index)
    {
        return addressable_instance_index_[instance_index];
    }

    InstancesProxyModel* TlasInstancesModel::GetProxyModel() const
    {
        return proxy_model_;
    }
}  // namespace rra
