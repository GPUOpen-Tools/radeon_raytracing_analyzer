//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the BLAS list model.
//=============================================================================

#include "models/tlas/blas_list_model.h"

#include <QTableView>
#include <QScrollBar>
#include <QHeaderView>
#include <QSortFilterProxyModel>

#include "qt_common/utils/scaling_manager.h"

#include "models/tlas/blas_list_item_model.h"

#include "public/rra_bvh.h"
#include "public/rra_blas.h"
#include "public/rra_tlas.h"

namespace rra
{
    BlasListModel::BlasListModel(int32_t num_model_widgets)
        : ModelViewMapper(num_model_widgets)
        , table_model_(nullptr)
        , proxy_model_(nullptr)
    {
    }

    BlasListModel::~BlasListModel()
    {
        delete table_model_;
        delete proxy_model_;
    }

    void BlasListModel::ResetModelValues()
    {
        table_model_->removeRows(0, table_model_->rowCount());
        table_model_->SetRowCount(0);
    }

    void BlasListModel::UpdateTable(uint64_t tlas_index)
    {
        uint64_t blas_count = 0;
        if (RraBvhGetTotalBlasCount(&blas_count) != kRraOk)
        {
            return;
        }

        uint64_t row_count = 0;

        // Don't include any empty BLASes or BLASes that aren't referenced (0 instances).
        for (uint64_t blas_index = 0; blas_index < blas_count; blas_index++)
        {
            uint64_t instance_count = 0;
            RraTlasGetInstanceCount(tlas_index, blas_index, &instance_count);
            if (!RraBlasIsEmpty(blas_index) && instance_count > 0)
            {
                row_count++;
            }
        }

        table_model_->SetRowCount(row_count);

        BlasListStatistics stats      = {};
        uint64_t           rows_added = 0;
        for (uint64_t blas_index = 0; blas_index < blas_count; blas_index++)
        {
            if (RraTlasGetInstanceCount(tlas_index, blas_index, &stats.instance_count) != kRraOk)
            {
                continue;
            }
            if (stats.instance_count == 0)
            {
                continue;
            }
            if (RraBlasIsEmpty(blas_index))
            {
                continue;
            }
            if (RraBlasGetBaseAddress(blas_index, &stats.address) != kRraOk)
            {
                continue;
            }
            if (RraBlasGetBuildFlags(blas_index, &stats.build_flags) != kRraOk)
            {
                continue;
            }
            if (RraBlasGetTotalNodeCount(blas_index, &stats.node_count) != kRraOk)
            {
                continue;
            }
            if (RraBlasGetBoxNodeCount(blas_index, &stats.box_count) != kRraOk)
            {
                continue;
            }
            if (RraBlasGetBox16NodeCount(blas_index, &stats.box16_count) != kRraOk)
            {
                continue;
            }
            if (RraBlasGetBox32NodeCount(blas_index, &stats.box32_count) != kRraOk)
            {
                continue;
            }
            if (RraBlasGetMaxTreeDepth(blas_index, &stats.max_depth) != kRraOk)
            {
                continue;
            }
            if (RraBlasGetAvgTreeDepth(blas_index, &stats.avg_depth) != kRraOk)
            {
                continue;
            }
            if (RraBlasGetTriangleNodeCount(blas_index, &stats.triangle_node_count) != kRraOk)
            {
                continue;
            }
            if (RraBlasGetProceduralNodeCount(blas_index, &stats.procedural_node_count) != kRraOk)
            {
                continue;
            }
            if (RraBlasGetSizeInBytes(blas_index, &stats.memory_usage) != kRraOk)
            {
                continue;
            }

            uint32_t root_node = UINT32_MAX;
            if (RraBvhGetRootNodePtr(&root_node) == kRraOk)
            {
                if (RraBlasGetSurfaceAreaHeuristic(blas_index, root_node, &stats.root_sah) != kRraOk)
                {
                    continue;
                }
                if (RraBlasGetMinimumSurfaceAreaHeuristic(blas_index, root_node, false, &stats.max_sah) != kRraOk)
                {
                    continue;
                }
                if (RraBlasGetAverageSurfaceAreaHeuristic(blas_index, root_node, false, &stats.mean_sah) != kRraOk)
                {
                    continue;
                }
            }

            stats.blas_index = blas_index;
            table_model_->AddAccelerationStructure(stats);
            rows_added++;
        }

        Q_ASSERT(rows_added == row_count);
        proxy_model_->invalidate();
    }

    void BlasListModel::InitializeTableModel(ScaledTableView* table_view, uint num_rows, uint num_columns)
    {
        if (proxy_model_ != nullptr)
        {
            delete proxy_model_;
            proxy_model_ = nullptr;
        }

        proxy_model_ = new BlasListProxyModel();
        table_model_ = proxy_model_->InitializeAccelerationStructureTableModels(table_view, num_rows, num_columns);
        table_model_->Initialize(table_view);
    }

    int32_t BlasListModel::GetBlasIndex(const QModelIndex& model_index)
    {
        const QModelIndex column_index = proxy_model_->index(model_index.row(), kBlasListColumnBlasIndex, QModelIndex());
        if (column_index.isValid() == true)
        {
            const QModelIndex proxy_model_index = proxy_model_->mapToSource(column_index);

            if (proxy_model_index.isValid() == true)
            {
                int32_t blas_index = column_index.data(Qt::UserRole).toULongLong();
                if (!RraBlasIsEmpty(blas_index))
                {
                    return blas_index;
                }
            }
        }
        return -1;
    }

    QModelIndex BlasListModel::GetTableModelIndex(uint64_t blas_index) const
    {
        uint64_t address = 0;
        if (RraBlasGetBaseAddress(blas_index, &address) == kRraOk)
        {
            return proxy_model_->FindModelIndex(address, kBlasListColumnAddress);
        }
        return QModelIndex();
    }

    void BlasListModel::SearchTextChanged(const QString& filter)
    {
        proxy_model_->SetSearchFilter(filter);
        proxy_model_->invalidate();
    }

    BlasListProxyModel* BlasListModel::GetProxyModel() const
    {
        return proxy_model_;
    }

}  // namespace rra
