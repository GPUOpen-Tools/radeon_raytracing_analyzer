//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of an acceleration structure tree-view model.
//=============================================================================

#include "models/acceleration_structure_tree_view_model.h"

#include <deque>

#include "public/rra_assert.h"
#include "public/rra_bvh.h"
#include "public/rra_rtip_info.h"

#include "models/acceleration_structure_tree_view_item.h"

namespace rra
{
    AccelerationStructureTreeViewModel::AccelerationStructureTreeViewModel(bool is_tlas, QObject* parent)
        : QAbstractItemModel(parent)
        , root_item_(nullptr)
        , item_buffer_(nullptr)
        , item_buffer_size_(0)
        , buffer_item_index_(0)
        , is_tlas_(is_tlas)
        , as_index_(0)
    {
    }

    AccelerationStructureTreeViewModel::~AccelerationStructureTreeViewModel()
    {
        delete[] item_buffer_;
    }

    bool AccelerationStructureTreeViewModel::InitializeModel(uint64_t node_count, uint32_t index, GetChildNodeFunction function)
    {
        beginResetModel();

        as_index_ = index;

        if (item_buffer_ != nullptr)
        {
            delete[] item_buffer_;
        }
        buffer_item_index_ = 0;

        // Allocate a single block of memory for the AS Treeview Items. This
        // saves Qt having to allocate small blocks and means that all memory can
        // be allocated/deallocated at once, and eliminates memory leaks due to Qt
        // sometimes not cleaning up properly.
        item_buffer_size_ = node_count;

        // Add an extra entry for the root node.
        item_buffer_size_++;

        item_buffer_ = new AccelerationStructureTreeViewItem[item_buffer_size_];

        // Allocate the Treeview root node.
        root_item_ = AllocateMemory(UINT32_MAX, nullptr);

        // Get the root node of the acceleration structure from the backend and add it.
        uint32_t root_node = UINT32_MAX;
        RraBvhGetRootNodePtr(&root_node);

        std::deque<std::pair<uint32_t, AccelerationStructureTreeViewItem*>> traversal_stack;
        traversal_stack.push_back(std::make_pair(root_node, root_item_));

        // Traverse the tree and enter stuff into the Treeview.
        while (!traversal_stack.empty())
        {
            auto front       = traversal_stack.front();
            auto node_data   = front.first;
            auto parent_item = front.second;
            traversal_stack.pop_front();

            AccelerationStructureTreeViewItem* item = AllocateMemory(node_data, parent_item);
            parent_item->AppendChild(item);

            node_data_to_item_[node_data] = item;

            uint32_t max_child_count{4};
            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
            {
                max_child_count = 8;
            }
            // For each item on the stack, add the children if valid.
            // Sort node types. Loop once for box (interior) nodes, then once for leaf nodes.
            for (uint32_t child_index = 0; child_index < max_child_count; child_index++)
            {
                uint32_t child_node = UINT32_MAX;
                if (function(index, node_data, child_index, &child_node) == kRraOk)
                {
                    if (RraBvhIsBoxNode(child_node))
                    {
                        traversal_stack.push_back(std::make_pair(child_node, item));
                    }
                }
            }

            for (uint32_t child_index = 0; child_index < max_child_count; child_index++)
            {
                uint32_t child_node = UINT32_MAX;
                if (function(index, node_data, child_index, &child_node) == kRraOk)
                {
                    if (!RraBvhIsBoxNode(child_node))
                    {
                        traversal_stack.push_back(std::make_pair(child_node, item));
                    }
                }
            }
        }

        endResetModel();

        return true;
    }

    AccelerationStructureTreeViewItem* AccelerationStructureTreeViewModel::AllocateMemory(uint32_t node_data, AccelerationStructureTreeViewItem* parent)
    {
        RRA_ASSERT(buffer_item_index_ < item_buffer_size_);
        AccelerationStructureTreeViewItem* item = GetItemAtIndex(buffer_item_index_);
        RRA_ASSERT(item != nullptr);
        item->Initialize(node_data, parent);
        buffer_item_index_++;
        return item;
    }

    AccelerationStructureTreeViewItem* AccelerationStructureTreeViewModel::GetItemAtIndex(uint32_t index) const
    {
        AccelerationStructureTreeViewItem* item = nullptr;
        if (index < item_buffer_size_)
        {
            item = &item_buffer_[index];
        }
        return item;
    }

    int AccelerationStructureTreeViewModel::rowCount(const QModelIndex& parent) const
    {
        AccelerationStructureTreeViewItem* parent_item = nullptr;

        if (parent.column() > 0)
        {
            return 0;
        }

        if (!parent.isValid())
        {
            parent_item = root_item_;
        }
        else
        {
            parent_item = static_cast<AccelerationStructureTreeViewItem*>(parent.internalPointer());
        }

        if (parent_item == nullptr)
        {
            return 0;
        }

        return parent_item->ChildCount();
    }

    int AccelerationStructureTreeViewModel::columnCount(const QModelIndex& parent) const
    {
        AccelerationStructureTreeViewItem* parent_item = nullptr;

        if (parent.isValid())
        {
            parent_item = static_cast<AccelerationStructureTreeViewItem*>(parent.internalPointer());
        }
        else
        {
            parent_item = root_item_;
        }

        if (parent_item == nullptr)
        {
            return 0;
        }

        return kNumColumns;
    }

    QVariant AccelerationStructureTreeViewModel::data(const QModelIndex& index, int role) const
    {
        AccelerationStructureTreeViewItem* item = static_cast<AccelerationStructureTreeViewItem*>(index.internalPointer());
        RRA_ASSERT(item != nullptr);
        if ((!index.isValid()) || (item == nullptr))
        {
            return QVariant();
        }

        if (role == Qt::DisplayRole)
        {
            return item->Data(index.column(), role, is_tlas_, as_index_);
        }

        else if (role == Qt::ToolTipRole)
        {
            return item->Data(index.column(), role, is_tlas_, as_index_);
        }

        return QVariant();
    }

    QModelIndex AccelerationStructureTreeViewModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        AccelerationStructureTreeViewItem* parent_item = nullptr;

        if (!parent.isValid())
        {
            parent_item = root_item_;
        }
        else
        {
            parent_item = static_cast<AccelerationStructureTreeViewItem*>(parent.internalPointer());
            RRA_ASSERT(parent_item != nullptr);
        }

        AccelerationStructureTreeViewItem* child_item = nullptr;
        if (parent_item != nullptr)
        {
            child_item = parent_item->Child(row);
        }

        if (child_item != nullptr)
        {
            return createIndex(row, column, child_item);
        }

        return QModelIndex();
    }

    QModelIndex AccelerationStructureTreeViewModel::parent(const QModelIndex& child) const
    {
        if (!child.isValid())
        {
            return QModelIndex();
        }

        AccelerationStructureTreeViewItem* child_item = static_cast<AccelerationStructureTreeViewItem*>(child.internalPointer());
        RRA_ASSERT(child_item != nullptr);

        AccelerationStructureTreeViewItem* parent_item = nullptr;
        if (child_item != nullptr)
        {
            parent_item = child_item->ParentItem();
        }

        if (parent_item == nullptr || parent_item == root_item_)
        {
            return QModelIndex();
        }

        return createIndex(parent_item->Row(), 0, parent_item);
    }

    /// @brief Return the index of the given tree item in the parent's list of children.
    ///
    /// @param [in] child_item The child to get the index for.
    ///
    /// @returns The child index for the given tree item.
    int GetIndexOfChild(AccelerationStructureTreeViewItem* child_item)
    {
        int found_child_index = 0;

        AccelerationStructureTreeViewItem* this_parent = child_item->ParentItem();
        for (int child_index = 0; child_index < this_parent->ChildCount(); ++child_index)
        {
            AccelerationStructureTreeViewItem* current_child = this_parent->Child(child_index);
            if (current_child == child_item)
            {
                found_child_index = child_index;
                break;
            }
        }

        return found_child_index;
    }

    /// @brief A recursive helper used to get the tree model index for the given tree item.
    ///
    /// @param [in] model The acceleration structure tree model.
    /// @param [in] item The tree item.
    ///
    /// @returns The tree model index for the given tree item.
    QModelIndex ComputeItemIndex(AccelerationStructureTreeViewModel* model, AccelerationStructureTreeViewItem* item)
    {
        QModelIndex result;

        AccelerationStructureTreeViewItem* parent = item->ParentItem();
        if (parent != nullptr)
        {
            // Compute the model index for the item's parent.
            QModelIndex parent_index = ComputeItemIndex(model, parent);

            // Determine the child index for the given item.
            int child_index = GetIndexOfChild(item);

            // Provide the parent item's index to compute the model index for the item.
            result = model->index(child_index, 0, parent_index);
        }
        else
        {
            // Return an invalid model index for the root node.
            result = QModelIndex();
        }

        return result;
    }

    QModelIndex AccelerationStructureTreeViewModel::GetModelIndexForNode(uint32_t node_id)
    {
        QModelIndex result;

        // Search for the node id within the value to item map.
        auto item_iter = node_data_to_item_.find(node_id);
        if (item_iter != node_data_to_item_.end())
        {
            AccelerationStructureTreeViewItem* item = item_iter->second;
            result                                  = ComputeItemIndex(this, item);
        }
        else
        {
            // The provided node id wasn't found- return an invalid model index.
            result = QModelIndex();
        }

        return result;
    }

    QModelIndex AccelerationStructureTreeViewModel::GetModelIndexForNodeAndTriangle(uint32_t node_id, uint32_t triangle_index)
    {
        QModelIndex result;

        // Search for the node id within the value to item map.
        auto item_iter = node_data_to_item_.find(node_id);
        if (item_iter != node_data_to_item_.end())
        {
            AccelerationStructureTreeViewItem* item = item_iter->second;
            if (static_cast<uint32_t>(item->ChildCount()) > triangle_index && !item->Child(triangle_index)->IsNode())
            {
                item = item->Child(triangle_index);
            }
            result = ComputeItemIndex(this, item);
        }
        else
        {
            // The provided node id wasn't found- return an invalid model index.
            result = QModelIndex();
        }

        return result;
    }

    void AccelerationStructureTreeViewModel::ResetModelValues()
    {
        if (item_buffer_ != nullptr)
        {
            delete[] item_buffer_;
            item_buffer_      = nullptr;
            item_buffer_size_ = 0;
        }
        root_item_         = nullptr;
        buffer_item_index_ = 0;
    }

    std::vector<uint32_t> AccelerationStructureTreeViewModel::GetAllNodeIds() const
    {
        std::vector<uint32_t> node_ids;
        node_ids.reserve(node_data_to_item_.size());
        for (auto& item : node_data_to_item_)
        {
            node_ids.push_back(item.first);
        }
        return node_ids;
    }

}  // namespace rra

