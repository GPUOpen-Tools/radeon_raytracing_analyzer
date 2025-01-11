//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the ray list item model.
///
/// Used for the ray list shown in the RAY tab.
///
//=============================================================================

#include <math.h>

#include "models/ray/ray_list_item_model.h"

#include "vulkan/include/vulkan/vulkan_core.h"

#include "qt_common/utils/qt_util.h"

#include "constants.h"
#include "util/string_util.h"
#include "settings/settings.h"
#include "public/rra_api_info.h"

namespace rra
{
    RayListItemModel::RayListItemModel(QObject* parent)
        : QAbstractItemModel(parent)
        , num_rows_(0)
        , num_columns_(0)
    {
    }

    RayListItemModel::~RayListItemModel()
    {
    }

    void RayListItemModel::SetColumnCount(int columns)
    {
        num_columns_ = columns;
    }

    void RayListItemModel::Initialize(ScaledTableView* acceleration_structure_table)
    {
        acceleration_structure_table->horizontalHeader()->setSectionsClickable(true);

        // Set default column widths wide enough to show table contents.
        acceleration_structure_table->SetColumnPadding(0);

        acceleration_structure_table->SetColumnWidthEms(kRayListColumnInvocationX, 6);
        acceleration_structure_table->SetColumnWidthEms(kRayListColumnInvocationY, 6);
        acceleration_structure_table->SetColumnWidthEms(kRayListColumnInvocationZ, 6);
        acceleration_structure_table->SetColumnWidthEms(kRayListColumnTraversalCount, 9);
        acceleration_structure_table->SetColumnWidthEms(kRayListColumnInstanceIntersections, 9);
        acceleration_structure_table->SetColumnWidthEms(kRayListColumnAnyHitInvocations, 9);
        acceleration_structure_table->SetColumnWidthEms(kRayListColumnRayCount, 9);

        // Allow users to resize columns if desired.
        acceleration_structure_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Interactive);
    }

    void RayListItemModel::SetDispatchDimensions(uint32_t width, uint32_t height, uint32_t depth)
    {
        cache_.reserve((uint64_t)width * height * depth);
    }

    void RayListItemModel::ClearCache()
    {
        cache_.clear();
        num_rows_ = 0;
    }

    void RayListItemModel::AddRow(RayListStatistics&& stats)
    {
        cache_.push_back(std::move(stats));
        num_rows_ = (int)cache_.size();
    }

    void RayListItemModel::GetFilteredAggregateStatistics(const GlobalInvocationID& filter_min,
                                                          const GlobalInvocationID& filter_max,
                                                          uint32_t*                 out_ray_count,
                                                          uint32_t*                 out_traversal_count,
                                                          uint32_t*                 out_instance_intersection_count)
    {
        for (const RayListStatistics& stats : cache_)
        {
            // Skip statistics of rays outside the user-selected box.
            bool x_out_of_range{stats.invocation_id.x < filter_min.x || stats.invocation_id.x > filter_max.x};
            bool y_out_of_range{stats.invocation_id.y < filter_min.y || stats.invocation_id.y > filter_max.y};
            bool z_out_of_range{stats.invocation_id.z < filter_min.z || stats.invocation_id.z > filter_max.z};
            if (x_out_of_range || y_out_of_range || z_out_of_range)
            {
                continue;
            }

            *out_ray_count += stats.ray_count;
            *out_traversal_count += stats.ray_event_count;
            *out_instance_intersection_count += stats.ray_instance_intersections;
        }
    }

    QVariant RayListItemModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        int row = index.row();

        const RayListStatistics& cache = cache_[row];

        if (role == Qt::DisplayRole)
        {
            switch (index.column())
            {
            case kRayListColumnInvocationX:
                return QString::number(cache.invocation_id.x);
            case kRayListColumnInvocationY:
                return QString::number(cache.invocation_id.y);
            case kRayListColumnInvocationZ:
                return QString::number(cache.invocation_id.z);
            case kRayListColumnRayCount:
                return QString::number(cache.ray_count);
            case kRayListColumnTraversalCount:
                return QString::number(cache.ray_event_count);
            case kRayListColumnInstanceIntersections:
                return QString::number(cache.ray_instance_intersections);
            case kRayListColumnAnyHitInvocations:
                return QString::number(cache.ray_any_hit_invocations);
            case kRayListColumnPadding:
                return QString("");
            default:
                break;
            }
        }
        else if (role == Qt::UserRole)
        {
            switch (index.column())
            {
            case kRayListColumnInvocationX:
                return QVariant::fromValue<uint32_t>(cache.invocation_id.x);
            case kRayListColumnInvocationY:
                return QVariant::fromValue<uint32_t>(cache.invocation_id.y);
            case kRayListColumnInvocationZ:
                return QVariant::fromValue<uint32_t>(cache.invocation_id.z);
            case kRayListColumnTraversalCount:
                return QVariant::fromValue<uint32_t>(cache.ray_event_count);
            case kRayListColumnInstanceIntersections:
                return QVariant::fromValue<uint32_t>(cache.ray_instance_intersections);
            case kRayListColumnAnyHitInvocations:
                return QVariant::fromValue<uint32_t>(cache.ray_any_hit_invocations);
            case kRayListColumnRayCount:
                return QVariant::fromValue<uint32_t>(cache.ray_count);
            case kRayListColumnPadding:
                return QVariant();
            default:
                break;
            }
        }
        return QVariant();
    }

    Qt::ItemFlags RayListItemModel::flags(const QModelIndex& index) const
    {
        return QAbstractItemModel::flags(index);
    }

    QVariant RayListItemModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal)
        {
            if (role == Qt::DisplayRole)
            {
                switch (section)
                {
                case kRayListColumnInvocationX:
                    return "X";
                case kRayListColumnInvocationY:
                    return "Y";
                case kRayListColumnInvocationZ:
                    return "Z";
                case kRayListColumnTraversalCount:
                    return "Traversal\nloop count";
                case kRayListColumnInstanceIntersections:
                    return "Instance\nintersections";
                case kRayListColumnAnyHitInvocations:
                    return "Any hit\ninvocations";
                case kRayListColumnRayCount:
                    return "Ray count";
                case kRayListColumnPadding:
                    return "";
                default:
                    break;
                }
            }
            else if (role == Qt::ToolTipRole)
            {
                switch (section)
                {
                case kRayListColumnInvocationX:
                    return QString("The x component of the global invocation ID associated with these rays.");
                case kRayListColumnInvocationY:
                    return QString("The y component of the global invocation ID associated with these rays.");
                case kRayListColumnInvocationZ:
                    return QString("The z component of the global invocation ID associated with these rays.");
                case kRayListColumnTraversalCount:
                    return QString("The number of traversal steps needed by all rays cast in this invocation.");
                case kRayListColumnInstanceIntersections:
                    return QString("The number of instances intersected by all rays cast in this invocation.");
                case kRayListColumnAnyHitInvocations:
                    return QString("The number of any hit shader invocations by all rays cast in this invocation.");
                case kRayListColumnRayCount:
                    return QString("The number of rays cast for this invocation.");
                default:
                    break;
                }
            }
            else if (role == Qt::TextAlignmentRole)
            {
                return Qt::AlignRight;
            }
        }

        return QAbstractItemModel::headerData(section, orientation, role);
    }

    QModelIndex RayListItemModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        return createIndex(row, column);
    }

    QModelIndex RayListItemModel::parent(const QModelIndex& index) const
    {
        Q_UNUSED(index);
        return QModelIndex();
    }

    int RayListItemModel::rowCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_rows_;
    }

    int RayListItemModel::columnCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_columns_;
    }
}  // namespace rra
