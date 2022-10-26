//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the BLAS instances model.
//=============================================================================

#include "models/blas/blas_instances_model.h"

#include <QTableView>
#include <QScrollBar>
#include <QHeaderView>
#include <QSortFilterProxyModel>

#include "qt_common/utils/qt_util.h"
#include "qt_common/utils/scaling_manager.h"

#include "models/blas/blas_instances_item_model.h"

#include "public/rra_bvh.h"
#include "public/rra_blas.h"
#include "public/rra_tlas.h"

namespace rra
{
    BlasInstancesModel::BlasInstancesModel(int32_t num_model_widgets)
        : ModelViewMapper(num_model_widgets)
        , table_model_(nullptr)
        , proxy_model_(nullptr)
    {
    }

    BlasInstancesModel::~BlasInstancesModel()
    {
        delete table_model_;
        delete proxy_model_;
    }

    void BlasInstancesModel::ResetModelValues()
    {
        table_model_->removeRows(0, table_model_->rowCount());
        table_model_->SetRowCount(0);
        SetModelData(kBlasInstancesBaseAddress, "-");
        SetModelData(kTlasInstancesBaseAddress, "-");
    }

    bool BlasInstancesModel::UpdateTable(uint64_t tlas_index, uint64_t blas_index)
    {
        uint64_t tlas_address = 0;
        if (RraTlasGetBaseAddress(tlas_index, &tlas_address) == kRraOk)
        {
            QString address_string = "TLAS base address: 0x" + QString("%1").arg(tlas_address, 0, 16);
            SetModelData(kTlasInstancesBaseAddress, address_string);
        }

        uint64_t blas_address = 0;
        if (RraBlasGetBaseAddress(blas_index, &blas_address) == kRraOk)
        {
            QString address_string = "BLAS base address: 0x" + QString("%1").arg(blas_address, 0, 16);
            SetModelData(kBlasInstancesBaseAddress, address_string);
        }

        uint64_t instance_count = 0;
        if (RraTlasGetInstanceCount(tlas_index, blas_index, &instance_count) != kRraOk)
        {
            return false;
        }
        table_model_->SetRowCount(instance_count);

        BlasInstancesStatistics stats      = {};
        uint64_t                rows_added = 0;
        for (int instance_index = 0; instance_index < instance_count; instance_index++)
        {
            uint32_t node_ptr = 0;
            if (RraTlasGetInstanceNode(tlas_index, blas_index, instance_index, &node_ptr) != kRraOk)
            {
                continue;
            }

            if (RraTlasGetInstanceIndexFromInstanceNode(tlas_index, node_ptr, &stats.instance_index))
            {
                continue;
            }

            if (RraTlasGetNodeBaseAddress(tlas_index, node_ptr, &stats.instance_address) != kRraOk)
            {
                continue;
            }

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

            table_model_->AddAccelerationStructure(stats);
            rows_added++;
        }

        Q_ASSERT(rows_added == instance_count);
        proxy_model_->invalidate();
        return instance_count > 0;
    }

    void BlasInstancesModel::InitializeTableModel(ScaledTableView* table_view, uint num_rows, uint num_columns)
    {
        if (proxy_model_ != nullptr)
        {
            delete proxy_model_;
            proxy_model_ = nullptr;
        }

        proxy_model_ = new BlasInstancesProxyModel();
        table_model_ = proxy_model_->InitializeAccelerationStructureTableModels(table_view, num_rows, num_columns);
        table_model_->Initialize(table_view);
    }

    int32_t BlasInstancesModel::GetInstanceIndex(const QModelIndex& model_index)
    {
        const QModelIndex proxy_model_index = proxy_model_->mapToSource(model_index);

        if (proxy_model_index.isValid() == true)
        {
            return proxy_model_index.row();
        }
        return -1;
    }

    QModelIndex BlasInstancesModel::GetTableModelIndex(uint64_t instance_index) const
    {
        return proxy_model_->FindModelIndex(instance_index, kBlasInstancesColumnInstanceIndex);
    }

    void BlasInstancesModel::SearchTextChanged(const QString& filter)
    {
        proxy_model_->SetSearchFilter(filter);
        proxy_model_->invalidate();
    }

    BlasInstancesProxyModel* BlasInstancesModel::GetProxyModel() const
    {
        return proxy_model_;
    }
}  // namespace rra
