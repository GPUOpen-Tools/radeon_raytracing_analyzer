//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a proxy filter that processes the ray list
/// table.
//=============================================================================

#include <math.h>

#include "models/ray/ray_list_proxy_model.h"

#include <QTableView>

#include "public/rra_assert.h"

#include "models/ray/ray_list_item_model.h"
#include "util/string_util.h"

namespace rra
{
    RayListProxyModel::RayListProxyModel(QObject* parent)
        : TableProxyModel(parent)
    {
    }

    RayListProxyModel::~RayListProxyModel()
    {
    }

    RayListItemModel* RayListProxyModel::InitializeRayTableModels(QTableView* view, int num_rows, int num_columns)
    {
        RRA_UNUSED(num_rows);

        RayListItemModel* model = new RayListItemModel();
        model->SetColumnCount(num_columns);

        setSourceModel(model);
        SetFilterKeyColumns({
            kRayListColumnInvocationX,
            kRayListColumnInvocationY,
            kRayListColumnInvocationZ,
            kRayListColumnTraversalCount,
            kRayListColumnRayCount,
        });

        view->setModel(this);

        return model;
    }

    void RayListProxyModel::SetFilterByInvocationId(GlobalInvocationID min, GlobalInvocationID max)
    {
        filter_min_ = min;
        filter_max_ = max;
    }

    void RayListProxyModel::SetReshapeDimension(uint32_t x, uint32_t y, uint32_t z)
    {
        reshape_width_  = x;
        reshape_height_ = y;
        reshape_depth_  = z;
    }

    void RayListProxyModel::GetFilterMinAndMax(GlobalInvocationID* filter_min_out, GlobalInvocationID* filter_max_out)
    {
        *filter_min_out = filter_min_;
        *filter_max_out = filter_max_;
    }

    void RayListProxyModel::SetFilterAcceptsAll(bool accepts_all)
    {
        filter_accept_all_ = accepts_all;
    }

    bool RayListProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
    {
        if (filter_accept_all_)
        {
            return true;
        }

        uint32_t x = sourceModel()->index(source_row, kRayListColumnInvocationX, source_parent).data(Qt::UserRole).toUInt();
        uint32_t y = sourceModel()->index(source_row, kRayListColumnInvocationY, source_parent).data(Qt::UserRole).toUInt();
        uint32_t z = sourceModel()->index(source_row, kRayListColumnInvocationZ, source_parent).data(Qt::UserRole).toUInt();

        uint32_t dimension = std::max((x > 1 ? 1 : 0) + (y > 1 ? 1 : 0) + (z > 1 ? 1 : 0), 1);

        if (dimension == 1)
        {
            uint32_t coord_1d = std::max(x, std::max(y, z));
            x                 = coord_1d % reshape_width_;
            z                 = coord_1d / (reshape_width_ * reshape_height_);
            y                 = (coord_1d - z * reshape_width_ * reshape_height_) / reshape_width_;
        }

        bool x_out_of_range{x < filter_min_.x || x > filter_max_.x};
        bool y_out_of_range{y < filter_min_.y || y > filter_max_.y};
        bool z_out_of_range{z < filter_min_.z || z > filter_max_.z};

        if (x_out_of_range || y_out_of_range || z_out_of_range)
        {
            return false;
        }

        return FilterSearchString(source_row, source_parent);
    }

    bool RayListProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        int left_column  = left.column();
        int right_column = right.column();
        if (left_column == right_column)
        {
            switch (left_column)
            {
            case kRayListColumnInvocationX:
            case kRayListColumnInvocationY:
            case kRayListColumnInvocationZ:
            case kRayListColumnTraversalCount:
            case kRayListColumnInstanceIntersections:
            case kRayListColumnAnyHitInvocations:
            case kRayListColumnRayCount:
            {
                const uint64_t left_data  = left.data(Qt::UserRole).toULongLong();
                const uint64_t right_data = right.data(Qt::UserRole).toULongLong();
                return left_data < right_data;
            }
            default:
                break;
            }
        }

        return QSortFilterProxyModel::lessThan(left, right);
    }
}  // namespace rra
