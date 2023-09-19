//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of an acceleration structure (AS) tree-view item.
//=============================================================================

#include "models/acceleration_structure_tree_view_item.h"

#include "qt_common/utils/qt_util.h"

#include "public/rra_assert.h"
#include "public/rra_blas.h"
#include "public/rra_tlas.h"

#include "settings/settings.h"

namespace rra
{
    AccelerationStructureTreeViewItem::AccelerationStructureTreeViewItem()
    {
    }

    AccelerationStructureTreeViewItem::~AccelerationStructureTreeViewItem()
    {
        for (int i = 0; i < ChildCount(); i++)
        {
            auto child = Child(i);
            if (!child->IsNode())
            {
                delete child;
            }
        }
    }

    void AccelerationStructureTreeViewItem::Initialize(uint32_t node_data, AccelerationStructureTreeViewItem* parent)
    {
        is_node_     = true;
        parent_item_ = parent;
        node_data_   = node_data;

        child_items_.clear();
    }

    void AccelerationStructureTreeViewItem::InitializeAsNonNode(uint32_t non_node_data, AccelerationStructureTreeViewItem* parent)
    {
        is_node_     = false;
        parent_item_ = parent;
        node_data_   = non_node_data;

        child_items_.clear();
    }

    void AccelerationStructureTreeViewItem::AppendChild(AccelerationStructureTreeViewItem* item)
    {
        child_items_.append(item);
    }

    AccelerationStructureTreeViewItem* AccelerationStructureTreeViewItem::Child(int row) const
    {
        return child_items_.value(row);
    }

    int AccelerationStructureTreeViewItem::ChildCount() const
    {
        return child_items_.count();
    }

    int AccelerationStructureTreeViewItem::ColumnCount() const
    {
        return kNumColumns;
    }

    QVariant AccelerationStructureTreeViewItem::Data(int column, int role, bool is_tlas, uint64_t as_index) const
    {
        RRA_ASSERT(column == 0);
        AccelerationStructureTreeViewItemData item_data;
        item_data.node_id = node_data_;

        if (column == 0)
        {
            switch (role)
            {
            case Qt::DisplayRole:
            {
                uint64_t           node_address = 0;
                RraErrorCode       error_code   = kRraErrorInvalidPointer;
                TreeviewNodeIDType node_type    = rra::Settings::Get().GetTreeviewNodeIdType();

                switch (node_type)
                {
                case kTreeviewNodeIDTypeVirtualAddress:
                    if (is_tlas)
                    {
                        error_code = RraTlasGetNodeBaseAddress(as_index, node_data_, &node_address);
                    }
                    else
                    {
                        error_code = RraBlasGetNodeBaseAddress(as_index, node_data_, &node_address);
                    }
                    break;

                case kTreeviewNodeIDTypeOffset:
                default:
                    error_code = RraBvhGetNodeOffset(node_data_, &node_address);
                    break;
                }

                if (error_code == kRraOk)
                {
                    QVariant variant;

                    item_data.display_name = RraBvhGetNodeName(node_data_) + QString(" - 0x") + QString("%1").arg(node_address, 0, 16);
                    item_data.node_id      = node_data_;

                    variant.setValue(item_data);
                    return variant;
                }
                break;
            }

            case Qt::UserRole:
                return node_data_;

            default:
                RRA_ASSERT_FAIL("Invalid role passed to AccelerationStructureTreeViewItem::Data");
                break;
            }
        }

        return QVariant();
    }

    AccelerationStructureTreeViewItem* AccelerationStructureTreeViewItem::ParentItem() const
    {
        return parent_item_;
    }

    bool AccelerationStructureTreeViewItem::IsNode() const
    {
        return is_node_;
    }

    int AccelerationStructureTreeViewItem::Row() const
    {
        if (parent_item_ != nullptr)
        {
            return parent_item_->child_items_.indexOf(const_cast<AccelerationStructureTreeViewItem*>(this));
        }

        return 0;
    }

}  // namespace rra
