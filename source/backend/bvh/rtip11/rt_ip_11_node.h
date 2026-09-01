//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the RTIP11 node class.
//=============================================================================

#ifndef RRA_BACKEND_BVH_RT_IP_11_NODE_H_
#define RRA_BACKEND_BVH_RT_IP_11_NODE_H_

#include "bvh/inode.h"

#include "bvh/dxr_definitions.h"
#include "bvh/ibvh.h"

namespace rta
{
    class Rtip11Node : public INode
    {
    public:
        /// @brief Constructor.
        Rtip11Node() = default;

        /// @brief Destructor.
        virtual ~Rtip11Node() = default;

        /// @brief Compute the bounding box for a root node.
        ///
        /// These don't have bounding boxes so the bounding box is calculated from the bounding boxes of the child nodes.
        ///
        /// @param [in] bvh The BVH containing the node.
        ///
        /// @return The bounding box.
        virtual dxr::amd::AxisAlignedBoundingBox ComputeRootNodeBoundingBox(const IBvh* bvh) const override;

        /// @brief Get the maximum number of child nodes per node.
        ///
        /// @return The maximum number of child nodes.
        virtual uint32_t GetMaxChildCount() const override;

        /// @brief Get the node's oriented bounding box index.
        ///
        /// @param [in] node_id The node to get the orientation of.
        /// @param [in] bvh     The BVH containing the node.
        ///
        /// @return The bounding box.
        virtual uint32_t GetNodeObbIndex(uint32_t node_id, const IBvh* bvh) const override;

        /// @brief Get a reference to the array of child nodes for a particular node.
        ///
        /// Assumes the parent node is an internal/box node.
        ///
        /// @param [in] bvh            The acceleration structure to use.
        /// @param [in] root_id        The parent node.
        /// @param [in] node_offset    The offset into the interior nodes array.
        ///
        /// @return A reference to the array of child nodes.
        virtual std::array<uint32_t, MAX_CHILD_NODES> GetChildNodeArray(const rta::IBvh* bvh, uint32_t root_id, uint32_t node_offset) const override;

        /// @brief Get the child node count for a given node.
        ///
        /// @param [in]  bvh                The acceleration structure containing the node of interest.
        /// @param [in]  parent_node        The parent to get count for.
        /// @param [out] out_child_count    A pointer to the child node count.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetChildNodeCount(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_count) const override;

        /// @brief Get the child nodes for a given node.
        ///
        /// @param [in]  bvh                The acceleration structure containing the node of interest.
        /// @param [in]  parent_node        The parent node to get child nodes for.
        /// @param [out] out_child_nodes    A pointer to a list allocated with the count of child nodes.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetChildNodes(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_nodes) const override;

        /// @brief Get the child indices for a given node.
        ///
        /// @param [in]  bvh                The acceleration structure containing the node of interest.
        /// @param [in]  parent_node        The parent node to get child nodes for.
        /// @param [out] out_child_indices   A pointer to a list allocated with the count of child nodes.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetChildIndices(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_indices) const override;

        /// @brief Get the child node pointer for a given node.
        ///
        /// @param [in]  bvh            The acceleration structure containing the node of interest.
        /// @param [in]  parent_node    The parent of the child node to find.
        /// @param [in]  child_index    The index of the child node held in the parent node.
        /// @param [out] out_node_id    The child node pointer.
        ///
        /// @return kRraOk if successful otherwise there are three possible error codes:
        ///         kRraErrorInvalidPointer if either the TLAS is invalid.
        ///         kRraErrorInvalidChildNode if the child node is invalid. (Meaning there may be more child nodes)
        ///         kRraErrorIndexOutOfRange if the child node index is out of range.
        virtual RraErrorCode GetChildNodePtr(const rta::IBvh* bvh, uint32_t parent_node, uint32_t child_index, uint32_t* out_node_id) const override;

        /// @brief Get the bounding volume for a provided node.
        ///
        /// @param [in]  bvh                The acceleration structure containing the node of interest.
        /// @param [in]  node_id            The node of interest.
        /// @param [in]  child_index        The node's child index.
        /// @param [in]  global_child_index The node's global child index.
        /// @param [out] out_bounding_box   The calculated bounding volume.
        ///
        /// @return RraOk if successful, an error code if not.
        virtual RraErrorCode GetNodeBoundingVolume(const rta::IBvh*                  bvh,
                                                   uint32_t                          node_id,
                                                   uint32_t                          child_index,
                                                   uint32_t                          global_child_index,
                                                   dxr::amd::AxisAlignedBoundingBox& out_bounding_box) const override;
    };
}  // namespace rta

#endif  //RRA_BACKEND_BVH_RT_IP_11_NODE_H_

