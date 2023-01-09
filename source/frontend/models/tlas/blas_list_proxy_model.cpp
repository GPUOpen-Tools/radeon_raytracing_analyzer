//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a proxy filter that processes the BLAS list
/// table.
//=============================================================================

#include <math.h>

#include "models/tlas/blas_list_proxy_model.h"

#include <QTableView>

#include "public/rra_assert.h"

#include "models/tlas/blas_list_item_model.h"
#include "util/string_util.h"

namespace rra
{
    static constexpr int kAllowUpdateCol     = 1;
    static constexpr int kAllowCompactionCol = 2;
    static constexpr int kLowMemoryCol       = 3;
    static constexpr int kBuildTypeCol       = 4;

    BlasListProxyModel::BlasListProxyModel(QObject* parent)
        : TableProxyModel(parent)
    {
    }

    BlasListProxyModel::~BlasListProxyModel()
    {
    }

    BlasListItemModel* BlasListProxyModel::InitializeAccelerationStructureTableModels(QTableView* view, int num_rows, int num_columns)
    {
        BlasListItemModel* model = new BlasListItemModel();
        model->SetRowCount(num_rows);
        model->SetColumnCount(num_columns);

        setSourceModel(model);
        SetFilterKeyColumns({
            kBlasListColumnAddress,
            kBlasListColumnAllowUpdate,
            kBlasListColumnAllowCompaction,
            kBlasListColumnLowMemory,
            kBlasListColumnBuildType,
            kBlasListColumnInstanceCount,
            kBlasListColumnNodeCount,
            kBlasListColumnBoxCount,
            kBlasListColumnBox32Count,
            kBlasListColumnBox16Count,
            kBlasListColumnTriangleNodeCount,
            kBlasListColumnProceduralNodeCount,
            kBlasListColumnRootSAH,
            kBlasListColumnMinSAH,
            kBlasListColumnMeanSAH,
            kBlasListColumnMaxDepth,
            kBlasListColumnAvgDepth,
            kBlasListColumnBlasIndex,
        });

        view->setModel(this);

        return model;
    }

    void BlasListProxyModel::SetFilterByAllowUpdate(bool filter)
    {
        filter_by_allow_update_ = filter;
    }

    void BlasListProxyModel::SetFilterByAllowCompaction(bool filter)
    {
        filter_by_allow_compaction_ = filter;
    }

    void BlasListProxyModel::SetFilterByLowMemory(bool filter)
    {
        filter_by_low_memory_ = filter;
    }

    void BlasListProxyModel::SetFilterByFastBuild(bool filter)
    {
        filter_by_fast_build_ = filter;
    }

    void BlasListProxyModel::SetFilterByFastTrace(bool filter)
    {
        filter_by_fast_trace_ = filter;
    }

    bool BlasListProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
    {
        QString fast_build_str = rra::string_util::GetBuildTypeString(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR);
        QString fast_trace_str = rra::string_util::GetBuildTypeString(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

        // If no flags are selected we will show everything instead of showing nothing.
        bool use_flag_filter =
            filter_by_allow_update_ || filter_by_allow_compaction_ || filter_by_low_memory_ || filter_by_fast_build_ || filter_by_fast_trace_;

        if (use_flag_filter)
        {
            if (filter_by_allow_update_ &&
                sourceModel()->index(source_row, kAllowUpdateCol, source_parent).data(Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Unchecked)
            {
                return false;
            }
            if (filter_by_allow_compaction_ &&
                sourceModel()->index(source_row, kAllowCompactionCol, source_parent).data(Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Unchecked)
            {
                return false;
            }
            if (filter_by_low_memory_ &&
                sourceModel()->index(source_row, kLowMemoryCol, source_parent).data(Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Unchecked)
            {
                return false;
            }
            if (filter_by_fast_build_ && sourceModel()->index(source_row, kBuildTypeCol, source_parent).data().value<QString>() != fast_build_str)
            {
                return false;
            }
            if (filter_by_fast_trace_ && sourceModel()->index(source_row, kBuildTypeCol, source_parent).data().value<QString>() != fast_trace_str)
            {
                return false;
            }
        }

        return FilterSearchString(source_row, source_parent);
    }

    bool BlasListProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        int left_column  = left.column();
        int right_column = right.column();
        if (left_column == right_column)
        {
            switch (left_column)
            {
            case kBlasListColumnAddress:
            case kBlasListColumnInstanceCount:
            case kBlasListColumnNodeCount:
            case kBlasListColumnBoxCount:
            case kBlasListColumnBlasIndex:
            {
                const uint64_t left_data  = left.data(Qt::UserRole).toULongLong();
                const uint64_t right_data = right.data(Qt::UserRole).toULongLong();
                return left_data < right_data;
            }

            case kBlasListColumnRootSAH:
            case kBlasListColumnMinSAH:
            case kBlasListColumnMeanSAH:
            {
                const float left_data  = left.data(Qt::UserRole).toFloat();
                const float right_data = right.data(Qt::UserRole).toFloat();

                if (isnan(right_data))
                {
                    return false;
                }

                if (isnan(left_data))
                {
                    return true;
                }

                return left_data < right_data;
            }

            case kBlasListColumnAllowUpdate:
            case kBlasListColumnAllowCompaction:
            case kBlasListColumnLowMemory:
            case kBlasListColumnBuildType:
            case kBlasListColumnBox32Count:
            case kBlasListColumnBox16Count:
            case kBlasListColumnTriangleNodeCount:
            case kBlasListColumnProceduralNodeCount:
            case kBlasListColumnMemoryUsage:
            case kBlasListColumnMaxDepth:
            case kBlasListColumnAvgDepth:
            {
                const uint32_t left_data  = left.data(Qt::UserRole).toUInt();
                const uint32_t right_data = right.data(Qt::UserRole).toUInt();
                return left_data < right_data;
            }
            default:
                break;
            }
        }

        return QSortFilterProxyModel::lessThan(left, right);
    }
}  // namespace rra
