//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 1.1 (Navi2x) specific bottom level acceleration structure
/// definition.
//=============================================================================

#ifndef RRA_BACKEND_BVH_ENCODED_RT_IP_11_BOTTOM_LEVEL_BVH_H_
#define RRA_BACKEND_BVH_ENCODED_RT_IP_11_BOTTOM_LEVEL_BVH_H_

#include "public/rra_error.h"

#include "bvh/geometry_info.h"
#include "bvh/node_types/procedural_node.h"
#include "bvh/node_types/triangle_node.h"
#include "bvh/rtip_common/encoded_bottom_level_bvh.h"

namespace rta
{
    class EncodedRtIp11BottomLevelBvh final : public EncodedBottomLevelBvh
    {
    public:
        // Global identifier of tlas dump in chunk files.
        static constexpr const char* kChunkIdentifier = "GpuEncBlasDump";

        /// @brief Default constructor.
        EncodedRtIp11BottomLevelBvh();

        /// @brief Destructor.
        virtual ~EncodedRtIp11BottomLevelBvh();

        /// @brief Get the total number of leaf nodes.
        ///
        /// @return The number of leaf nodes.
        virtual uint32_t GetLeafNodeCount() const override;

        /// @brief Get the data for the leaf nodes.
        ///
        /// @return The leaf nodes data.
        const std::vector<uint8_t>& GetLeafNodesData() const;

        /// @brief Get the geometry info data.
        ///
        /// @return The geometry info.
        const std::vector<dxr::amd::GeometryInfo>& GetGeometryInfos() const;

        /// @brief Get the list of primitive node pointers.
        ///
        /// @return The list of node pointers.
        const std::vector<dxr::amd::NodePointer>& GetPrimitiveNodePtrs() const;

        /// @brief Does this BVH have references.
        ///
        /// Call this function to recreate the original GPU virtual addresses stored in the
        /// instance nodes of this dump. Use this function before exporting the dumps
        /// to the original Dxc format.
        ///
        /// @return true if the TLAS has references, false if not.
        bool HasBvhReferences() const override;

        /// @brief Update the primitive node pointers.
        void UpdatePrimitiveNodePtrs();

        /// @brief Load the BVH data from a file.
        ///
        /// @param [in] chunk_file        A Reference to a ChunkFile object which describes the file chunk being loaded.
        /// @param [in] chunk_index       The index of the chunk in the file.
        /// @param [in] header            The raw acceleration structure header.
        /// @param [in] chunk_identifier  The BVH chunk name.
        /// @param [in] import_option     Flag indicating which sections of the chunk to load/discard.
        ///
        /// @return true if the BVH data loaded successfully, false if not.
        bool LoadRawAccelStrucFromFile(rdf::ChunkFile&                     chunk_file,
                                       const std::uint64_t                 chunk_index,
                                       const RawAccelStructRdfChunkHeader& header,
                                       const char* const                   chunk_identifier,
                                       const BvhBundleReadOption           import_option) override;

        /// @brief Do the post-load step.
        ///
        /// This will be called once all the acceleration structures are loaded and fixed up. Tasks here include
        /// the surface area heuristic calculations.
        ///
        /// @return true if successful, false if error.
        bool PostLoad() override;

        /// @brief Get the top-level surface area heuristic for this BLAS.
        ///
        /// @return The surface area heuristic.
        float GetSurfaceAreaHeuristic() const;

        /// @brief Set the top-level surface area heuristic for this BLAS.
        ///
        /// @param [in] surface_area_heuristic The surface area heuristic value to be set.
        void SetSurfaceAreaHeuristic(float surface_area_heuristic);

        /// @brief Get the node name for a node.
        ///
        /// @param [in]  node_id             The node id.
        /// @param [out] out_name            A pointer to receive the name string.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetNodeName(uint32_t node_id, const char** out_name) const override;

        /// @brief Get the node tooltip name for a node.
        ///
        /// @param [in]  node_id             The node id.
        /// @param [out] out_name            A pointer to receive the tooltip string.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetNodeNameToolTip(uint32_t node_id, const char** out_tooltip) const override;

        /// @brief Retrieve the geometry index for the triangle node.
        ///
        /// @param [in]  node_id             The node whose geometry index is to be found.
        /// @param [in]  child_index		 The node's child index from its parent.
        /// @param [in]  global_child_index  The node's global child index.
        /// @param [out] out_geometry_index  The geometry index for the triangle node.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetGeometryIndex(uint32_t node_id, uint32_t child_index, uint32_t global_child_index, uint32_t* out_geometry_index) const override;

        /// @brief Retrieve the primitive index for the triangle node.
        ///
        /// @param [in]  node_id				The node whose primitive index is to be found.
        /// @param [in]  child_index			The node's child index from its parent.
        /// @param [in]  global_child_index     The node's global child index.
        /// @param [in]  local_primitive_index	The local primitive index within the given node.
        /// @param [out] out_primitive_index	The primitive index for the triangle node.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetPrimitiveIndex(uint32_t  node_id,
                                               uint32_t  child_index,
                                               uint32_t  global_child_index,
                                               uint32_t  local_primitive_index,
                                               uint32_t* out_primitive_index) const override;

        /// @brief Retrieve whether the node is inactive.
        ///
        /// @param [in]  node_id            The node whose geometry flags are to be retrieved.
        /// @param [in]  child_index		The node's child index from its parent.
        /// @param [in]  global_child_index The node's global child index.
        /// @param [out] out_is_inactive	Whether the node is inactive.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetIsInactive(uint32_t node_id, uint32_t child_index, uint32_t global_child_index, bool* out_is_inactive) const override;

        /// @brief Retrieve the number of triangles on a given node.
        ///
        /// @param [in]  node_id            The node ID to retrieve the triangles from.
        /// @param [in]  child_index		The node's child index from its parent.
        /// @param [in]  global_child_index The node's global child index.
        /// @param [out] out_triangle_count The number of triangles in the BLAS.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetNodeTriangleCount(uint32_t  node_id,
                                                  uint32_t  child_index,
                                                  uint32_t  global_child_index,
                                                  uint32_t* out_triangle_count) const override;

        /// @brief Retrieve the triangles stored in the given node id.
        ///
        /// @param [in]  node_id            The node ID to retrieve the triangles for.
        /// @param [in]  child_index	    The node's child index from its parent.
        /// @param [in]  global_child_index The node's global child index.
        /// @param [out] out_triangles      A preallocated pointer to dump the triangles into.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetNodeTriangles(uint32_t          node_id,
                                              uint32_t          child_index,
                                              uint32_t          global_child_index,
                                              TriangleVertices* out_triangles) const override;

        /// @brief Retrieve the vertices stored in the given node id.
        ///
        /// @param [in]  node_id            The node ID to retrieve the vertices for.
        /// @param [in]  child_index        The node's child index from its parent.
        /// @param [in]  global_child_index The node's global child index.
        /// @param [out] out_count          The number of vertices in the triangle node.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetNodeVertexCount(uint32_t node_id, uint32_t child_index, uint32_t global_child_index, uint32_t* out_count) const override;

        /// @brief Retrieve the vertices stored in the given node id.
        ///
        /// @param [in]  node_id            The node ID to retrieve the vertices for.
        /// @param [in]  child_index        The node's child index from its parent.
        /// @param [in]  global_child_index The node's global child index.
        /// @param [out] out_triangles      A preallocated pointer to dump the vertices into.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetNodeVertices(uint32_t               node_id,
                                             uint32_t               child_index,
                                             uint32_t               global_child_index,
                                             struct VertexPosition* out_vertices) const override;

        /// @brief Get the triangle node associated with the node pointer.
        ///
        /// Assumes that the node pointer passed in is a triangle node.
        ///
        /// @param node_pointer The node pointer.
        /// @param offset       Defines which node should be returned. Offset 0 => the one referenced by node_pointer, 1 => the node behind the node with offset 0.
        const dxr::amd::TriangleNode* GetTriangleNode(const dxr::amd::NodePointer node_pointer, const int offset = 0) const;

        /// @brief Get the procedural node associated with the node pointer.
        ///
        /// Assumes that the node pointer passed in is a procedural node.
        ///
        /// @param node_pointer The node pointer.
        /// @param offset       Defines which node should be returned. Offset 0 => the one referenced by node_pointer, 1 => the node behind the node with offset 0.
        const dxr::amd::ProceduralNode* GetProceduralNode(const dxr::amd::NodePointer node_pointer, const int offset = 0) const;

        /// @brief Traverse the tree for compute leaf node surface area heuristics.
        virtual void ComputeSurfaceAreaHeuristic() override;

        /// @brief Get the parent node of the node passed in.
        ///
        /// @param [in] node_id            The node whose parent is to be found.
        /// @param [in] global_child_index The node's global child index.
        ///
        /// @return The parent node. If the node passed in is the root node, the
        /// parent node will be an invalid node.
        virtual uint32_t GetParentNode(uint32_t node_id, uint32_t global_child_index = 0) const override;

        /// @brief Get the surface area heuristic for a given leaf node.
        ///
        /// @param [in] node_id The leaf node whose SAH is to be found.
        /// @param [in] node_id The leaf's global child index.
        ///
        /// @return The surface area heuristic.
        float GetLeafNodeSurfaceAreaHeuristic(uint32_t node_id, uint32_t global_child_index = 0) const override;

        /// @brief Set the surface area heuristic for a given leaf node.
        ///
        /// @param [in] node_ptr               The node pointer whose SAH is to be set.
        /// @param [in] surface_area_heuristic The surface area heuristic value to be set.
        void SetLeafNodeSurfaceAreaHeuristic(uint32_t node_ptr, float surface_area_heuristic);

        /// @brief Get all the triangle NodePointers from the BLAS
        ///
        /// @param [in] blas_index Index of BLAS to get the triangle NodePointers from.
        /// @param [out] triangle_nodes The triangle nodes are written into this vector.
        ///
        /// @returns The error code.
        RraErrorCode GetTriangleNodes(std::vector<dxr::amd::NodePointer>& triangle_nodes);

    private:
        /// @brief Obtain the byte size of the encoded buffer.
        ///
        /// @param [in] import_option Flag indicating which sections of the chunk to load/discard.
        ///
        /// @return The buffer size.
        std::uint64_t GetBufferByteSizeImpl(const ExportOption export_option) const override;

        /// @brief Recursive function to calculate the surface area heuristic for a given BLAS.
        ///
        /// The BLAS will be traversed starting at the provided node given and the surface area heuristic will be
        /// calculated for each child node.
        ///
        /// @param [in] root_node The root node of the BLAS to start from.
        ///
        /// @return The surface area heuristic for the node passed in.
        float CalculateSAHForBlasNode(const dxr::amd::NodePointer root_node);

        /// @brief Calculate the surface area of a bounding volume around a triangle.
        ///
        /// Uses the triangle coordinates to construct a bounding volume around the
        /// triangle.
        ///
        /// @param [in] triangle The triangle node structure describing the triangle.
        /// @param [in] tri_count The number of triangles in the node.
        ///
        /// @return The triangle node's bounding volume surface area.
        float CalculateTriangleAABBSurfaceArea(const dxr::amd::TriangleNode& triangle, uint32_t tri_count) const;

        /// @brief Validate the loaded BVH data for accuracy where possible.
        ///
        /// @return true if data is valid, false otherwise.
        bool Validate() const;

        std::vector<std::uint8_t> leaf_nodes_ = {};  ///< Leaf nodes (triangle, procedural).
    };

}  // namespace rta

#endif  // RRA_BACKEND_BVH_ENCODED_RT_IP_11_BOTTOM_LEVEL_BVH_H_

