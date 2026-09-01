//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
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

        /// @brief Get the child node count for a given node.
        ///
        /// @param [in]  bvh                The acceleration structure containing the node of interest.
        /// @param [in]  parent_node        The parent to get count for.
        /// @param [out] out_child_count    A pointer to the child node count.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetChildNodeCount(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_count) const = 0;

        /// @brief Get the child nodes for a given node.
        ///
        /// @param [in]  bvh                The acceleration structure containing the node of interest.
        /// @param [in]  parent_node        The parent node to get child nodes for.
        /// @param [out] out_child_nodes    A pointer to a list allocated with the count of child nodes.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetChildNodes(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_nodes) const = 0;

        /// @brief Get the child indices for a given node.
        ///
        /// @param [in]  bvh                The acceleration structure containing the node of interest.
        /// @param [in]  parent_node        The parent node to get child nodes for.
        /// @param [out] out_child_indices   A pointer to a list allocated with the count of child nodes.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetChildIndices(const rta::IBvh* bvh, uint32_t parent_node, uint32_t* out_child_indices) const = 0;

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
        virtual RraErrorCode GetChildNodePtr(const rta::IBvh* bvh, uint32_t parent_node, uint32_t child_index, uint32_t* out_node_id) const = 0;

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
                                                   dxr::amd::AxisAlignedBoundingBox& out_bounding_box) const = 0;

        /// @brief Get the surface area heuristic for a provided node.
        ///
        /// The default implementation dispatches on whether the node is a box or leaf node using the
        /// IBvh SAH accessors, which is correct for all current IP versions. Subclasses may override
        /// if their encoding requires different SAH computation.
        ///
        /// @param [in]  bvh                The acceleration structure containing the node of interest.
        /// @param [in]  node_ptr           The node of interest.
        /// @param [in]  global_child_index The node of interest's global child ID.
        /// @param [out] out_surface_area   The calculated surface area heuristic.
        ///
        /// @return RraOk if successful, an error code if not.
        virtual RraErrorCode GetSurfaceAreaHeuristic(const rta::IBvh* bvh,
                                                     uint32_t         node_id,
                                                     uint32_t         global_child_index,
                                                     float*           out_surface_area_heuristic) const;

        /// @brief Get the root node pointer for an acceleration structure.
        ///
        /// The default implementation constructs a canonical root pointer at the standard header
        /// offset using an FP32 box node type, which is uniform across all current IP versions.
        /// Subclasses may override if their root pointer encoding differs.
        ///
        /// @param [out] out_node_ptr        A pointer to receive the node pointer.
        /// This value is valid for the root index of all the TLAS hierarchies.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetRootNodePtr(uint32_t* out_node_ptr) const;

        /// @brief Get the offset of the node provided.
        ///
        /// The default implementation reinterprets the node ID as a NodePointer and reads its GPU
        /// virtual address, which is the correct encoding for all current IP versions. Subclasses
        /// may override if their node address encoding differs.
        ///
        /// @param [in]  node_id      The node of interest.
        /// @param [out] out_offset   A pointer to receive the node offset.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetNodeOffset(uint32_t node_id, uint64_t* out_offset) const;

        /// @brief Get the base address for a given node.
        ///
        /// @param [in]  node_id        The node of interest.
        /// @param [in]  bvh            The acceleration structure containing the node of interest.
        /// @param [out] out_address    The base address of the node.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        ///
        /// @note The default implementation decodes the address via NodePointer GPU virtual address encoding.
        ///       RTIP variants with a different node_id encoding must override this method; failing to do so
        ///       will silently produce wrong addresses.
        virtual RraErrorCode GetBaseAddress(uint32_t node_id, const IBvh* bvh, uint64_t* out_address) const;

    protected:
        glm::mat3 DecodeRotationMatrix(uint32_t id) const;
    };
}  // namespace rta

#endif  //RRA_BACKEND_BVH_INODE_H_

