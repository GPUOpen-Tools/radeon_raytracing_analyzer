//=============================================================================
// Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the ray inspector ray list item model.
///
//=============================================================================

#include <math.h>

#include "models/ray/ray_inspector_ray_tree_model.h"

#include "vulkan/include/vulkan/vulkan_core.h"

#include "qt_common/utils/qt_util.h"

#include "constants.h"
#include "util/string_util.h"
#include "settings/settings.h"
#include "public/rra_api_info.h"

namespace rra
{
    RayInspectorRayTreeModel::RayInspectorRayTreeModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
    }

    RayInspectorRayTreeModel::~RayInspectorRayTreeModel()
    {
        ClearRays();
    }

    void RayInspectorRayTreeModel::Initialize(ScaledTreeView* acceleration_structure_table)
    {
        // Set default column widths wide enough to show table contents.
        acceleration_structure_table->SetColumnPadding(0);

        acceleration_structure_table->SetColumnWidthEms(kRayInspectorRayIndexColumn, 8);
        acceleration_structure_table->SetColumnWidthEms(kRayInspectorRayListColumnTraversalLoopCount, 8);
        acceleration_structure_table->SetColumnWidthEms(kRayInspectorRayListColumnInstanceIntersections, 9);
        acceleration_structure_table->SetColumnWidthEms(kRayInspectorRayListColumnAnyHitInvocations, 8);
        acceleration_structure_table->SetColumnWidthEms(kRayInspectorRayListColumnHit, 3);
    }

    void RayInspectorRayTreeModel::AddNewRays(RayTreeItemList item_list)
    {
        beginResetModel();
        root_ = new RayInspectorRayTreeItem();
        root_->AddNewRays(item_list);
        endResetModel();

        emit dataChanged(index(0, 0, {}), index(rowCount(), 0, {}));
    }

    void RayInspectorRayTreeModel::ClearRays()
    {
        beginResetModel();
        delete root_;
        root_ = nullptr;
        endResetModel();
    }

    QVariant RayInspectorRayTreeModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        RayInspectorRayTreeItem* item = static_cast<RayInspectorRayTreeItem*>(index.internalPointer());

        return item->Data(role, index.column());
    }

    Qt::ItemFlags RayInspectorRayTreeModel::flags(const QModelIndex& index) const
    {
        return QAbstractItemModel::flags(index);
    }

    QVariant RayInspectorRayTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
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

    QModelIndex RayInspectorRayTreeModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        RayInspectorRayTreeItem* parent_item;

        if (!parent.isValid())
        {
            parent_item = root_;
        }
        else
        {
            parent_item = static_cast<RayInspectorRayTreeItem*>(parent.internalPointer());
        }

        RayInspectorRayTreeItem* child_item = parent_item->Child(row);
        if (child_item)
        {
            return createIndex(row, column, child_item);
        }
        return QModelIndex();
    }

    QModelIndex RayInspectorRayTreeModel::parent(const QModelIndex& index) const
    {
        if (!index.isValid() || root_ == nullptr)
        {
            return QModelIndex();
        }

        RayInspectorRayTreeItem* child_item = static_cast<RayInspectorRayTreeItem*>(index.internalPointer());

        RayInspectorRayTreeItem* parent_item = child_item->Parent();

        if (parent_item == nullptr)
        {
            return QModelIndex();
        }

        return createIndex(parent_item->Row(), 0, parent_item);
    }

    int RayInspectorRayTreeModel::rowCount(const QModelIndex& parent) const
    {
        RayInspectorRayTreeItem* parent_item;
        if (parent.column() > 0 || root_ == nullptr)
        {
            return 0;
        }

        if (!parent.isValid())
        {
            parent_item = root_;
        }
        else
        {
            parent_item = static_cast<RayInspectorRayTreeItem*>(parent.internalPointer());
        }

        if (parent_item == nullptr)
        {
            return 0;
        }

        return parent_item->ChildCount();
    }

    int RayInspectorRayTreeModel::columnCount(const QModelIndex& parent) const
    {
        if (parent.isValid())
        {
            return static_cast<RayInspectorRayTreeItem*>(parent.internalPointer())->ColumnCount();
        }
        return root_->ColumnCount();
    }
}  // namespace rra
