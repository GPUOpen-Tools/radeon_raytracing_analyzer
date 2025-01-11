//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the ray counters item model.
///
/// Used for the ray counters table shown in the RAY tab.
///
//=============================================================================

#include <math.h>

#include "models/ray/ray_counters_item_model.h"

#include "vulkan/include/vulkan/vulkan_core.h"

#include "qt_common/utils/qt_util.h"

#include "constants.h"
#include "util/string_util.h"
#include "settings/settings.h"
#include "public/rra_api_info.h"

namespace rra
{
    RayCountersItemModel::RayCountersItemModel(QObject* parent)
        : QAbstractItemModel(parent)
        , num_rows_(0)
        , num_columns_(0)
    {
    }

    RayCountersItemModel::~RayCountersItemModel()
    {
    }

    void RayCountersItemModel::SetRowCount(int rows)
    {
        num_rows_ = rows;
    }

    void RayCountersItemModel::SetColumnCount(int columns)
    {
        num_columns_ = columns;
    }

    void RayCountersItemModel::Initialize(ScaledTableView* acceleration_structure_table)
    {
        // Set default column widths wide enough to show table contents.
        acceleration_structure_table->SetColumnPadding(0);

        acceleration_structure_table->SetColumnWidthEms(0, 8);
        acceleration_structure_table->SetColumnWidthEms(1, 8);
    }

    void RayCountersItemModel::SetData(float rays_per_pixel, uint32_t total_ray_count, float traversal_count_per_ray, float instance_intersections_per_ray)
    {
        rays_per_pixel_                 = rays_per_pixel;
        total_ray_count_                = total_ray_count;
        traversal_count_per_ray_        = traversal_count_per_ray;
        instance_intersections_per_ray_ = instance_intersections_per_ray;

        QModelIndex top_left     = createIndex(0, 0);
        QModelIndex bottom_right = createIndex(num_rows_ - 1, num_columns_ - 1);
        emit        dataChanged(top_left, bottom_right);
    }

    QVariant RayCountersItemModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        if (role == Qt::DisplayRole)
        {
            if (index.column() == kRayCountersColumnLabel)
            {
                switch (index.row())
                {
                case kRayCountersRowRayPerPixel:
                    return QString("Rays per pixel");
                case kRayCountersRowRayCount:
                    return QString("Total ray count");
                case kRayCountersRowTraversalLoopCount:
                    return QString("Traversal loop count per ray");
                case kRayCountersRowInstanceIntersectionCount:
                    return QString("Instance intersections per ray");
                default:
                    break;
                }
            }
            else if (index.column() == kRayCountersColumnData)
            {
                int decimal_precision = rra::Settings::Get().GetDecimalPrecision();
                switch (index.row())
                {
                case kRayCountersRowRayPerPixel:
                    return QString::number(rays_per_pixel_, rra::kQtFloatFormat, decimal_precision);
                case kRayCountersRowRayCount:
                    return rra::string_util::LocalizedValue(total_ray_count_);
                case kRayCountersRowTraversalLoopCount:
                    return QString::number(traversal_count_per_ray_, rra::kQtFloatFormat, decimal_precision);
                case kRayCountersRowInstanceIntersectionCount:
                    return QString::number(instance_intersections_per_ray_, rra::kQtFloatFormat, decimal_precision);
                default:
                    break;
                }
            }
        }
        else if (role == Qt::ToolTipRole)
        {
            if (index.column() == kRayCountersColumnData)
            {
                switch (index.row())
                {
                case kRayCountersRowRayPerPixel:
                    return QString::number(rays_per_pixel_, rra::kQtFloatFormat, kQtTooltipFloatPrecision);
                case kRayCountersRowTraversalLoopCount:
                    return QString::number(traversal_count_per_ray_, rra::kQtFloatFormat, kQtTooltipFloatPrecision);
                case kRayCountersRowInstanceIntersectionCount:
                    return QString::number(instance_intersections_per_ray_, rra::kQtFloatFormat, kQtTooltipFloatPrecision);
                }
            }
        }
        else if (role == Qt::TextAlignmentRole)
        {
            if (index.column() == kRayCountersColumnLabel)
            {
                return Qt::AlignLeft;
            }
            else
            {
                return Qt::AlignRight;
            }
        }
        return QVariant();
    }

    Qt::ItemFlags RayCountersItemModel::flags(const QModelIndex& index) const
    {
        return QAbstractItemModel::flags(index);
    }

    QModelIndex RayCountersItemModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        return createIndex(row, column);
    }

    QModelIndex RayCountersItemModel::parent(const QModelIndex& index) const
    {
        Q_UNUSED(index);
        return QModelIndex();
    }

    int RayCountersItemModel::rowCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_rows_;
    }

    int RayCountersItemModel::columnCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_columns_;
    }
}  // namespace rra
