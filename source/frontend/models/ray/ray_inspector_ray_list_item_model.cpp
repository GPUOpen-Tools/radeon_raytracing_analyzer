//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the ray inspector ray list item model.
///
//=============================================================================

#include <math.h>

#include "models/ray/ray_inspector_ray_list_item_model.h"

#include "vulkan/include/vulkan/vulkan_core.h"

#include "qt_common/utils/qt_util.h"

#include "constants.h"
#include "util/string_util.h"
#include "settings/settings.h"
#include "public/rra_api_info.h"

namespace rra
{
    RayInspectorRayListItemModel::RayInspectorRayListItemModel(QObject* parent)
        : QAbstractItemModel(parent)
        , num_rows_(0)
        , num_columns_(0)
    {
    }

    RayInspectorRayListItemModel::~RayInspectorRayListItemModel()
    {
    }

    void RayInspectorRayListItemModel::SetRowCount(int rows)
    {
        num_rows_ = rows;
        cache_.clear();
    }

    void RayInspectorRayListItemModel::SetColumnCount(int columns)
    {
        num_columns_ = columns;
    }

    void RayInspectorRayListItemModel::Initialize(ScaledTableView* acceleration_structure_table)
    {
        acceleration_structure_table->horizontalHeader()->setSectionsClickable(true);

        // Set default column widths wide enough to show table contents.
        acceleration_structure_table->SetColumnPadding(0);

        acceleration_structure_table->SetColumnWidthEms(kRayInspectorRayIndexColumn, 6);
        acceleration_structure_table->SetColumnWidthEms(kRayInspectorRayListColumnTraversalLoopCount, 8);
        acceleration_structure_table->SetColumnWidthEms(kRayInspectorRayListColumnInstanceIntersections, 9);
        acceleration_structure_table->SetColumnWidthEms(kRayInspectorRayListColumnAnyHitInvocations, 8);
        acceleration_structure_table->SetColumnWidthEms(kRayInspectorRayListColumnHit, 3);

        // Allow users to resize columns if desired.
        acceleration_structure_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);
    }

    void RayInspectorRayListItemModel::AddRow(const RayInspectorRayListStatistics& stats)
    {
        cache_.push_back(stats);
        num_rows_ = (int)cache_.size();
    }

    QVariant RayInspectorRayListItemModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        int row = index.row();

        const RayInspectorRayListStatistics& cache = cache_[row];

        if (role == Qt::DisplayRole)
        {
            switch (index.column())
            {
            case kRayInspectorRayIndexColumn:
                return QString::number(row);
            case kRayInspectorRayListColumnTraversalLoopCount:
                return QString::number(cache.ray_event_count);
            case kRayInspectorRayListColumnInstanceIntersections:
                return QString::number(cache.ray_instance_intersections);
            case kRayInspectorRayListColumnAnyHitInvocations:
                return QString::number(cache.ray_any_hit_invocations);
            default:
                break;
            }
        }
        else if (role == Qt::UserRole)
        {
            switch (index.column())
            {
            case kRayInspectorRayIndexColumn:
                return QString::number(row);
            case kRayInspectorRayListColumnTraversalLoopCount:
                return QVariant::fromValue<uint32_t>(cache.ray_event_count);
            case kRayInspectorRayListColumnInstanceIntersections:
                return QVariant::fromValue<uint32_t>(cache.ray_instance_intersections);
            case kRayInspectorRayListColumnAnyHitInvocations:
                return QVariant::fromValue<uint32_t>(cache.ray_any_hit_invocations);
            default:
                break;
            }
        }
        else if (role == Qt::CheckStateRole)
        {
            switch (index.column())
            {
            case kRayInspectorRayListColumnHit:
                return cache.hit ? Qt::Checked : Qt::Unchecked;
            }
        }
        return QVariant();
    }

    Qt::ItemFlags RayInspectorRayListItemModel::flags(const QModelIndex& index) const
    {
        return QAbstractItemModel::flags(index);
    }

    QVariant RayInspectorRayListItemModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal)
        {
            if (role == Qt::DisplayRole)
            {
                switch (section)
                {
                case kRayInspectorRayIndexColumn:
                    return "Index";
                case kRayInspectorRayListColumnTraversalLoopCount:
                    return "Traversal\nloop count";
                case kRayInspectorRayListColumnInstanceIntersections:
                    return "Instance\nintersections";
                case kRayInspectorRayListColumnAnyHitInvocations:
                    return "Any hit\ninvocations";
                case kRayInspectorRayListColumnHit:
                    return "Hit";

                default:
                    break;
                }
            }
            else if (role == Qt::ToolTipRole)
            {
                switch (section)
                {
                case kRayInspectorRayIndexColumn:
                    return QString("The index of the ray, ascending means the ray was shot later.");
                case kRayInspectorRayListColumnTraversalLoopCount:
                    return QString("The number of traversal steps needed by this ray.");
                case kRayInspectorRayListColumnInstanceIntersections:
                    return QString("The number of instances intersected by this ray.");
                case kRayInspectorRayListColumnAnyHitInvocations:
                    return QString("The number of any hit shaders invoked during traversal for this ray.");
                case kRayInspectorRayListColumnHit:
                    return QString("Whether this ray had any hits during traversal.");
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

    QModelIndex RayInspectorRayListItemModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        return createIndex(row, column);
    }

    QModelIndex RayInspectorRayListItemModel::parent(const QModelIndex& index) const
    {
        Q_UNUSED(index);
        return QModelIndex();
    }

    int RayInspectorRayListItemModel::rowCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_rows_;
    }

    int RayInspectorRayListItemModel::columnCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_columns_;
    }
}  // namespace rra
