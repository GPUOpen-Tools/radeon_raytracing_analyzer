//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of an acceleration structure (AS) tree-view item.
//=============================================================================

#include "models/acceleration_structure_tree_view_item.h"

#include "qt_common/utils/qt_util.h"

#include "public/rra_assert.h"
#include "public/rra_blas.h"
#include "public/rra_tlas.h"

#include "bvh/node_pointer.h"
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

    void AccelerationStructureTreeViewItem::Initialize(uint64_t node_data, uint32_t child_index, AccelerationStructureTreeViewItem* parent)
    {
        is_node_     = true;
        parent_item_ = parent;
        node_data_   = node_data;
        child_index_ = child_index;

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

        if (column == 0)
        {
            // RTIP3.1 node packing: a "Referenced subtree" placeholder redirects to the primary sibling that owns the
            // shared subtree. It carries the primary's composite key (so selecting it jumps to the canonical node via
            // the scene selection round-trip) but shows a fixed label instead of the primary's node name/address.
            if (is_reference_placeholder_)
            {
                switch (role)
                {
                case Qt::DisplayRole:
                {
                    AccelerationStructureTreeViewItemData placeholder_data;
                    placeholder_data.display_name     = "Referenced subtree";
                    placeholder_data.node_child_id    = node_data_;
                    placeholder_data.node_child_index = child_index_;
                    QVariant variant;
                    variant.setValue(placeholder_data);
                    return variant;
                }
                case Qt::ToolTipRole:
                    return QString("This subtree is shared (node packing). Select to jump to the canonical node.");
                case Qt::UserRole:
                    return static_cast<qulonglong>(node_data_);
                default:
                    return QVariant();
                }
            }

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

                    const char* node_name{};

                    if (is_tlas)
                    {
                        error_code = RraTlasGetNodeName(as_index, node_data_, &node_name);
                        RRA_ASSERT(error_code == kRraOk);
                    }
                    else
                    {
                        error_code = RraBlasGetNodeName(as_index, node_data_, &node_name);
                        RRA_ASSERT(error_code == kRraOk);
                    }

                    item_data.display_name     = node_name + QString(" - 0x") + QString("%1").arg(node_address, 0, 16);
                    item_data.node_child_id    = node_data_;
                    item_data.node_child_index = child_index_;

                    // RTIP3.1 node packing: several sibling slots point to the same child node, so tag the duplicate
                    // rows to make the shared subtree obvious in the tree.
                    if (is_packed_)
                    {
                        item_data.display_name += " (packed)";
                    }

                    const dxr::amd::NodePointer* node = reinterpret_cast<const dxr::amd::NodePointer*>(&node_data_);
                    if (is_tlas && node->IsInstanceNode())
                    {
                        uint64_t blas_index{};
                        error_code = RraTlasGetBlasIndexFromInstanceNode(as_index, node_data_, &blas_index);
                        // A partitioned/incomplete trace may reference a BLAS that isn't present in the
                        // capture, so a failed lookup is expected here rather than a hard error.
                        if (error_code != kRraOk || RraBlasIsEmpty(blas_index))
                        {
                            item_data.display_name += " (missing BLAS)";
                        }
                    }
                    else if (!is_tlas && RraBlasIsClusterRefNode(as_index, node_data_))
                    {
                        // A Cluster BLAS (CBLAS) leaf references a CLAS; display it as "CLAS [id]" rather than a
                        // triangle/instance node name.
                        uint32_t clas_id = 0;
                        RraBlasGetClusterRefNodeId(as_index, node_data_, &clas_id);
                        item_data.display_name = QString("CLAS [%1] - 0x").arg(clas_id) + QString("%1").arg(node_address, 0, 16);
                    }

                    variant.setValue(item_data);
                    return variant;
                }
                break;
            }

            case Qt::ToolTipRole:
            {
                static const char* node_tooltip{};

                if (is_tlas)
                {
                    RraErrorCode error_code = RraTlasGetNodeNameToolTip(as_index, node_data_, &node_tooltip);
                    RRA_ASSERT(error_code == kRraOk);
                }
                else
                {
                    RraErrorCode error_code = RraBlasGetNodeNameToolTip(as_index, node_data_, &node_tooltip);
                    RRA_ASSERT(error_code == kRraOk);
                }
                return node_tooltip;
            }

            case Qt::UserRole:
                return static_cast<qulonglong>(node_data_);

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

