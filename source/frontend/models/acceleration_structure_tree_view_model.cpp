//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of an acceleration structure tree-view model.
//=============================================================================

#include "models/acceleration_structure_tree_view_model.h"

#include <array>
#include <deque>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "public/rra_assert.h"
#include "public/rra_blas.h"
#include "public/rra_bvh.h"
#include "public/rra_rtip_info.h"
#include "public/rra_tlas.h"

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

    template <typename ParentHandle, typename VisitFn>
    void AccelerationStructureTreeViewModel::TraverseTree(uint32_t             index,
                                                          GetChildNodeFunction get_child,
                                                          bool                 use_composite_keys,
                                                          ParentHandle         root_handle,
                                                          VisitFn              visit)
    {
        uint32_t     root_node  = UINT32_MAX;
        RraErrorCode error_code = RraBvhGetRootNodePtr(&root_node);
        RRA_ASSERT(error_code == kRraOk);
        RRA_UNUSED(error_code);

        uint32_t global_child_index = UINT32_MAX;

        // NOTE: Tree traversal uses back() rather than front() so the global_child_index is set up correctly (it must
        // match the numbering the scene uses in SceneNode::GetChildIdHash when composite keys are in play).
        std::deque<std::tuple<uint32_t, uint32_t, ParentHandle, bool>> traversal_stack;  // (node_id, child_index, parent_handle, is_packed).
        traversal_stack.push_back(std::make_tuple(root_node, 0u, root_handle, false));

        while (!traversal_stack.empty())
        {
            auto         tuple            = traversal_stack.back();
            uint32_t     node_id          = std::get<0>(tuple);
            uint32_t     node_child_index = std::get<1>(tuple);
            ParentHandle parent           = std::get<2>(tuple);
            bool         is_packed        = std::get<3>(tuple);
            traversal_stack.pop_back();
            ++global_child_index;

            const bool     use_composite_here = use_composite_keys && (!is_tlas_ || tlas_node_packing_);
            const uint64_t node_id_global =
                use_composite_here ? (((uint64_t)global_child_index << 32) | node_id) : (uint64_t)node_id;

            ParentHandle handle = visit(node_id_global, node_child_index, parent, is_packed);

            if (use_composite_keys)
            {
                // A packed-ref duplicate shares the primary sibling's subtree, so don't re-expand it here (mirrors the
                // scene tree, which expands the shared subtree exactly once). This packed-skip applies to packed BLAS
                // nodes and to RTIP3.1 TLAS packing; a non-packing TLAS never has is_packed set so it is unaffected.
                bool has_children = is_tlas_ ? (!(tlas_node_packing_ && is_packed) && RraTlasHasChildren(index, node_id))
                                             : (!is_packed && RraBlasHasChildren(index, node_id));
                if (has_children)
                {
                    uint32_t                              max_child_count = RraBvhGetMaxChildCount();
                    std::array<uint32_t, MAX_CHILD_NODES> seen_child_ids{};
                    uint32_t                              seen_count{0};
                    for (uint32_t i = 0; i < max_child_count; i++)
                    {
                        uint32_t child_node = UINT32_MAX;
                        if (get_child(index, node_id_global, i, &child_node) == kRraOk)
                        {
                            const bool child_is_box = is_tlas_ ? RraTlasIsBoxNode(index, child_node) : RraBlasIsBoxNode(index, child_node);
                            bool       child_is_packed{false};
                            if (child_is_box)
                            {
                                // Node packing: if this child node id already appeared in an earlier sibling slot, this
                                // slot is a packed-ref duplicate.
                                for (uint32_t s{0}; s < seen_count; ++s)
                                {
                                    if (seen_child_ids[s] == child_node)
                                    {
                                        child_is_packed = true;
                                        break;
                                    }
                                }
                                if (!child_is_packed)
                                {
                                    seen_child_ids[seen_count++] = child_node;
                                }
                            }
                            traversal_stack.push_back(std::make_tuple(child_node, i, handle, child_is_packed));
                        }
                    }
                }
            }
            else
            {
                uint32_t max_child_count = RraBvhGetMaxChildCount();
                for (uint32_t child_index = 0; child_index < max_child_count; child_index++)
                {
                    uint32_t child_node = UINT32_MAX;
                    if (get_child(index, node_id, child_index, &child_node) == kRraOk)
                    {
                        traversal_stack.push_back(std::make_tuple(child_node, child_index, handle, false));
                    }
                }
            }
        }
    }

    bool AccelerationStructureTreeViewModel::InitializeModel(uint64_t node_count, uint32_t index, GetChildNodeFunction get_child)
    {
        RRA_UNUSED(node_count);  // The buffer is sized by the count pass below; the header node count under-reports for PTLAS.
        beginResetModel();

        as_index_ = index;

        if (item_buffer_ != nullptr)
        {
            delete[] item_buffer_;
            item_buffer_ = nullptr;
        }
        buffer_item_index_ = 0;
        node_data_to_item_.clear();

        const rta::RayTracingIpLevel rtip = (rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel();
        tlas_node_packing_      = is_tlas_ && (rtip == rta::RayTracingIpLevel::RtIp3_1) && RraTlasHasNodePacking(index);
        bool use_composite_keys = (rtip == rta::RayTracingIpLevel::RtIp3_1) && ((!is_tlas_ && RraBlasHasNodePacking(index)) || tlas_node_packing_);

        // Count pass: size the item buffer to exactly what the build pass will allocate. A PTLAS carries partition-level
        // internal nodes above the base level (and node packing re-visits shared slots), so the header's flat interior +
        // leaf count is not a reliable size. Counting via the same traversal used to build guarantees an exact fit.
        uint64_t item_count = 1;  // The explicit root item allocated below.
        TraverseTree<int>(index, get_child, use_composite_keys, 0, [&](uint64_t, uint32_t, int, bool is_packed) -> int {
            ++item_count;
            // Each packed-ref row also gets one "Referenced subtree" placeholder child (created in the post-pass below).
            if (is_packed)
            {
                ++item_count;
            }
            return 0;
        });

        // Allocate a single block of memory for the AS Treeview Items. This saves Qt having to allocate small blocks and
        // means that all memory can be allocated/deallocated at once, and eliminates memory leaks due to Qt sometimes not
        // cleaning up properly.
        item_buffer_size_ = item_count;
        item_buffer_      = new AccelerationStructureTreeViewItem[item_buffer_size_];

        // Allocate the Treeview root node.
        root_item_ = AllocateMemory(UINT32_MAX, UINT32_MAX, nullptr);

        // Build pass: identical traversal, now materializing the tree view items. Also record the canonical (primary)
        // composite key per shared node id, and collect the packed-ref items so the post-pass can hang a "referenced
        // subtree" placeholder under each one.
        std::unordered_map<uint32_t, uint64_t>          canonical_key_by_node_id;
        std::vector<AccelerationStructureTreeViewItem*> packed_items;
        TraverseTree<AccelerationStructureTreeViewItem*>(
            index,
            get_child,
            use_composite_keys,
            root_item_,
            [&](uint64_t node_id_global, uint32_t node_child_index, AccelerationStructureTreeViewItem* parent, bool is_packed) {
                AccelerationStructureTreeViewItem* item = AllocateMemory(node_id_global, node_child_index, parent);
                item->SetIsPacked(is_packed);
                parent->AppendChild(item);
                node_data_to_item_[node_id_global] = item;
                const uint32_t node_id = (uint32_t)(node_id_global & 0xFFFFFFFF);
                if (is_packed)
                {
                    packed_items.push_back(item);
                }
                else
                {
                    // Node packing is single-parent-multi-ref, so the first (non-packed) occurrence of a node id is its
                    // unique canonical/primary tree item.
                    canonical_key_by_node_id[node_id] = node_id_global;
                }
                return item;
            });

        // Post-pass: give each packed-ref row a single "Referenced subtree" placeholder child that carries the primary's
        // composite key, so expanding it reveals a link that jumps to the canonical subtree. Done as a post-pass because
        // LIFO traversal visits packed-refs before their (lower-slot) primary sibling, so the primary key isn't known yet
        // at packed-ref visit time.
        for (AccelerationStructureTreeViewItem* packed_item : packed_items)
        {
            QVariant       user_data = packed_item->Data(0, Qt::UserRole, is_tlas_, index);
            const uint32_t node_id   = (uint32_t)(user_data.toULongLong() & 0xFFFFFFFF);
            auto           it        = canonical_key_by_node_id.find(node_id);
            if (it != canonical_key_by_node_id.end())
            {
                AccelerationStructureTreeViewItem* placeholder = AllocateMemory(it->second, 0, packed_item);
                placeholder->SetIsReferencePlaceholder(true);
                packed_item->AppendChild(placeholder);
            }
        }

        endResetModel();

        return true;
    }

    AccelerationStructureTreeViewItem* AccelerationStructureTreeViewModel::AllocateMemory(uint64_t                           node_data,
                                                                                          uint32_t                           child_index,
                                                                                          AccelerationStructureTreeViewItem* parent)
    {
        RRA_ASSERT(buffer_item_index_ < item_buffer_size_);
        AccelerationStructureTreeViewItem* item = GetItemAtIndex(buffer_item_index_);
        RRA_ASSERT(item != nullptr);
        item->Initialize(node_data, child_index, parent);
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

    QModelIndex AccelerationStructureTreeViewModel::GetModelIndexForNode(uint64_t node_child_id)
    {
        QModelIndex result;

        // Search for the node id within the value to item map.
        auto item_iter = node_data_to_item_.find(node_child_id);
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

    QModelIndex AccelerationStructureTreeViewModel::GetModelIndexForNodeAndTriangle(uint64_t node_child_id, uint32_t triangle_index)
    {
        QModelIndex result;

        // Search for the node id within the value to item map.
        auto item_iter = node_data_to_item_.find(node_child_id);
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


