//=============================================================================
// Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the node class interface.
//=============================================================================

#ifndef RRA_BACKEND_BVH_INODE_H_
#define RRA_BACKEND_BVH_INODE_H_

#include "glm/glm/glm.hpp"

#include "public/rra_bvh.h"

#include "bvh/dxr_definitions.h"
#include "bvh/ibvh.h"

namespace rta
{
    class INode
    {
    public:
        /// @brief Constructor.
        INode() = default;

        /// @brief Destructor.
        virtual ~INode() = default;

        /// @brief Compute the bounding box for a root node.
        ///
        /// These don't have bounding boxes so the bounding box is calculated from the bounding boxes of the child nodes.
        ///
        /// @param [in] bvh The BVH containing the node.
        ///
        /// @return The bounding box.
        virtual dxr::amd::AxisAlignedBoundingBox ComputeRootNodeBoundingBox(const IBvh* bvh) const = 0;

        /// @brief Get the maximum number of child nodes per node.
        ///
        /// @return The maximum number of child nodes.
        virtual uint32_t GetMaxChildCount() const = 0;

        /// @brief Get the node's oriented bounding box index.
        ///
        /// @param [in] node_id The node to get the orientation of.
        /// @param [in] bvh     The BVH containing the node.
        ///
        /// @return The bounding box.
        virtual uint32_t GetNodeObbIndex(uint32_t node_id, const IBvh* bvh) const = 0;

        /// @brief Get the orientation of node's OBB.
        ///
        /// @param [in] node_id The node to get the orientation of.
        /// @param [in] bvh     The BVH containing the node.
        ///
        /// @return The bounding box.
        virtual glm::mat3 GetNodeBoundingVolumeOrientation(uint32_t node_id, const IBvh* bvh) const;

        /// @brief Check if the given node is an instance node.
        ///
        /// Instance nodes are leaf nodes in TLAS that reference BLAS objects. This should only
        /// be called for TLAS acceleration structures.
        ///
        /// @param [in]  node_id       The encoded node pointer.
        /// @param [in]  bvh           The acceleration structure containing the node of interest.
        ///
        /// @return true if it's an instance node, false if not.
        virtual bool GetIsInstanceNode(uint32_t node_id, const IBvh* bvh) const;

        /// @brief Check if the given node is a box node (internal node with bounding volumes).
        ///
        /// Box nodes are internal nodes in the BVH tree that contain bounding volumes for
        /// child nodes. They can be either 32-bit (FP32) or 16-bit (FP16) precision.
        ///
        /// @param [in]  node_id       The encoded node pointer.
        /// @param [in]  bvh           The acceleration structure containing the node of interest.
        ///
        /// @return true if it's a box node, false if not.
        virtual bool GetIsBoxNode(uint32_t node_id, const IBvh* bvh) const;

        /// @brief Check if the given node is a 32-bit precision box node.
        ///
        /// FP32 box nodes store child bounding volumes using 32-bit floating point precision.
        ///
        /// @param [in]  node_id       The encoded node pointer.
        /// @param [in]  bvh           The acceleration structure containing the node of interest.
        ///
        /// @return true if it's a 32-bit box node, false if not.
        virtual bool GetIsBox32Node(uint32_t node_id, const IBvh* bvh) const;

        /// @brief Check if the given node is a 16-bit precision box node.
        ///
        /// FP16 box nodes store child bounding volumes using 16-bit floating point precision
        /// to reduce memory usage. Availability depends on hardware version.
        ///
        /// @param [in]  node_id       The encoded node pointer.
        /// @param [in]  bvh           The acceleration structure containing the node of interest.
        ///
        /// @return true if it's a 16-bit box node, false if not.
        virtual bool GetIsBox16Node(uint32_t node_id, const IBvh* bvh) const;

        /// @brief Check if the given node has child nodes.
        ///
        /// Internal nodes have children, while leaf nodes (triangles, procedurals, instances) do not.
        ///
        /// @param [in]  node_id       The encoded node pointer.
        /// @param [in]  bvh           The acceleration structure containing the node of interest.
        ///
        /// @return true if the node has children, false if not.
        virtual bool GetHasChildren(uint32_t node_id, const IBvh* bvh) const;

        /// @brief Check if the given node is a procedural node.
        ///
        /// Procedural nodes are leaf nodes in BLAS that represent procedural geometry
        /// (AABBs that trigger intersection shaders). Only relevant for BLAS with procedural geometries.
        ///
        /// @param [in]  node_id       The encoded node pointer.
        /// @param [in]  bvh           The acceleration structure containing the node of interest.
        ///
        /// @return true if it's a procedural node, false if not.
        virtual bool GetIsProceduralNode(uint32_t node_id, const IBvh* bvh) const;

        /// @brief Check if the given node is a triangle node.
        ///
        /// @param [in]  node_id       The encoded node pointer.
        /// @param [in]  bvh           The acceleration structure containing the node of interest.
        ///
        /// @return true if it's a triangle node, false if not.
        virtual bool GetIsTriangleNode(uint32_t node_id, const IBvh* bvh) const;

        /// @brief Get a reference to the array of child nodes for a particular node.
        ///
        /// Assumes the parent node is an internal/box node.
        ///
        /// @param [in] bvh            The acceleration structure to use.
        /// @param [in] root_id        The parent node.
        /// @param [in] node_offset    The offset into the interior nodes array.
        ///
        /// @return A reference to the array of child nodes.
        virtual std::array<uint32_t, MAX_CHILD_NODES> GetChildNodeArray(const rta::IBvh* bvh, uint32_t root_id, uint32_t node_offset) const = 0;

    protected:
        glm::mat3 DecodeRotationMatrix(uint32_t id) const;
    };
}  // namespace rta

#endif  //RRA_BACKEND_BVH_INODE_H_

