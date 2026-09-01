//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the TLAS partitions model.
//=============================================================================

#include "models/tlas/tlas_partitions_model.h"

#include <QHeaderView>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QTableView>

#include "qt_common/utils/qt_util.h"

#include "public/rra_tlas.h"

#include "models/partitions_item_model.h"

namespace rra
{
    TlasPartitionsModel::TlasPartitionsModel(int32_t num_model_widgets)
        : ModelViewMapper(num_model_widgets)
        , table_model_(nullptr)
        , proxy_model_(nullptr)
    {
    }

    TlasPartitionsModel::~TlasPartitionsModel()
    {
        delete table_model_;
        delete proxy_model_;
    }

    void TlasPartitionsModel::ResetModelValues()
    {
        table_model_->removeRows(0, table_model_->rowCount());
        table_model_->SetRowCount(0);
        SetModelData(kTlasPartitionsTlasBaseAddress, "-");
    }

    bool TlasPartitionsModel::UpdateTable(uint64_t tlas_index)
    {
        uint64_t tlas_address = 0;
        if (RraTlasGetBaseAddress(tlas_index, &tlas_address) == kRraOk)
        {
            QString address_string = "TLAS base address: 0x" + QString("%1").arg(tlas_address, 0, 16);
            SetModelData(kTlasPartitionsTlasBaseAddress, address_string);
        }

        if (!RraTlasIsPartitioned(tlas_index))
        {
            table_model_->SetRowCount(0);
            proxy_model_->invalidate();
            return false;
        }

        uint32_t partition_count = 0;
        if (RraTlasGetPartitionCount(tlas_index, &partition_count) != kRraOk)
        {
            table_model_->SetRowCount(0);
            proxy_model_->invalidate();
            return false;
        }

        // Iterate 0..partition_count inclusive; the last slot is the global partition.
        const uint32_t total_rows = partition_count + 1;
        table_model_->SetRowCount(total_rows);

        uint32_t rows_added = 0;
        for (uint32_t partition_index = 0; partition_index < total_rows; partition_index++)
        {
            RraPartitionInfo info = {};
            if (RraTlasGetPartitionInfo(tlas_index, partition_index, &info) != kRraOk)
            {
                table_model_->SetRowCount(0);
                proxy_model_->invalidate();
                return false;
            }

            PartitionsTableStatistics stats = {};
            stats.partition_index           = info.partition_index;
            stats.instance_count            = info.instance_count;
            stats.internal_node_count       = info.internal_node_count;
            stats.fat_leaf_count            = info.fat_leaf_count;
            stats.translation[0]            = info.translation[0];
            stats.translation[1]            = info.translation[1];
            stats.translation[2]            = info.translation[2];
            stats.is_global                 = info.is_global;

            table_model_->AddPartition(stats);
            rows_added++;
        }

        Q_ASSERT(rows_added == total_rows);
        proxy_model_->invalidate();
        return rows_added > 0;
    }

    void TlasPartitionsModel::InitializeTableModel(QTableView* table_view, uint num_rows, uint num_columns)
    {
        if (proxy_model_ != nullptr)
        {
            delete proxy_model_;
            proxy_model_ = nullptr;
        }

        proxy_model_ = new PartitionsProxyModel();
        table_model_ = proxy_model_->InitializeAccelerationStructureTableModels(table_view, num_rows, num_columns);
        table_model_->Initialize(table_view);
    }

    int32_t TlasPartitionsModel::GetPartitionIndex(const QModelIndex& model_index)
    {
        const QModelIndex proxy_model_index = proxy_model_->mapToSource(model_index);

        if (proxy_model_index.isValid() == true)
        {
            return proxy_model_index.row();
        }
        return -1;
    }

    void TlasPartitionsModel::SearchTextChanged(const QString& filter)
    {
        proxy_model_->SetSearchFilter(filter);
        proxy_model_->invalidate();
    }

    PartitionsProxyModel* TlasPartitionsModel::GetProxyModel() const
    {
        return proxy_model_;
    }
}  // namespace rra

