//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the ray inspector ray list item model.
///
//=============================================================================

#include <math.h>

#include "vulkan/include/vulkan/vulkan_core.h"

#include "qt_common/utils/qt_util.h"

#include "public/rra_api_info.h"

#include "constants.h"
#include "ray_inspector_ray_tree_item.h"
#include "settings/settings.h"
#include "util/string_util.h"

namespace rra
{
    RayInspectorRayTreeItem::RayInspectorRayTreeItem()
    {
    }

    RayInspectorRayTreeItem::~RayInspectorRayTreeItem()
    {
        qDeleteAll(child_trees_);
    }

    void RayInspectorRayTreeItem::AddNewRays(RayTreeItemList item_list)
    {
        for (auto& item : item_list)
        {
            auto child       = new RayInspectorRayTreeItem();
            child->self_ray_ = item;
            child->AddNewRays(item->child_rays);
            child_trees_.append(child);
            child->parent_ = this;
        }
    }

    void RayInspectorRayTreeItem::ClearRays()
    {
        child_trees_.clear();
    }

    int RayInspectorRayTreeItem::Row() const
    {
        if (parent_)
        {
            return parent_->child_trees_.indexOf(const_cast<RayInspectorRayTreeItem*>(this));
        }

        return 0;
    }

    int RayInspectorRayTreeItem::ChildCount() const
    {
        return child_trees_.size();
    }

    int RayInspectorRayTreeItem::ColumnCount() const
    {
        return static_cast<int>(RayInspectorRayTreeColumn::kRayInspectorRayListColumnCount);
    }

    RayInspectorRayTreeItem* RayInspectorRayTreeItem::Child(int row)
    {
        if (row < 0 || row >= child_trees_.size())
        {
            return nullptr;
        }
        return child_trees_.at(row);
    }

    QVariant RayInspectorRayTreeItem::Data(int role, int column)
    {
        if (!self_ray_)
        {
            return QVariant();
        }

        if (role == Qt::DisplayRole)
        {
            switch (column)
            {
            case kRayInspectorRayIndexColumn:
                return QString::number(self_ray_->row_index);
            case kRayInspectorRayListColumnTraversalLoopCount:
                return QString::number(self_ray_->ray_event_count);
            case kRayInspectorRayListColumnInstanceIntersections:
                return QString::number(self_ray_->ray_instance_intersections);
            case kRayInspectorRayListColumnAnyHitInvocations:
                return QString::number(self_ray_->ray_any_hit_invocations);
            default:
                break;
            }
        }
        else if (role == Qt::UserRole)
        {
            return QString::number(self_ray_->row_index);
        }
        else if (role == Qt::CheckStateRole)
        {
            switch (column)
            {
            case kRayInspectorRayListColumnHit:
                return self_ray_->hit ? Qt::Checked : Qt::Unchecked;
            }
        }
        return QVariant();
    }

    RayInspectorRayTreeItem* RayInspectorRayTreeItem::Parent()
    {
        return parent_;
    }

}  // namespace rra

