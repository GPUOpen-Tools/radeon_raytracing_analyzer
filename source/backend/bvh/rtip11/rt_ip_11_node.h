//=============================================================================
// Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
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
    };
}  // namespace rta

#endif  //RRA_BACKEND_BVH_RT_IP_11_NODE_H_

