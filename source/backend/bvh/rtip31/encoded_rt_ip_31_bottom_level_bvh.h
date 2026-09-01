//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 3.1 (Navi4x) specific bottom level acceleration structure
/// definition.
//=============================================================================

#ifndef RRA_BACKEND_BVH_ENCODED_RT_IP_31_BOTTOM_LEVEL_BVH_H_
#define RRA_BACKEND_BVH_ENCODED_RT_IP_31_BOTTOM_LEVEL_BVH_H_

#include <array>
#include <unordered_map>
#include <utility>

#include "public/rra_bvh.h"
#include "public/rra_error.h"

#include "bvh/geometry_info.h"
#include "bvh/node_types/procedural_node.h"
#include "bvh/node_types/triangle_node.h"
#include "bvh/rtip31/primitive_node.h"
#include "bvh/rtip_common/encoded_bottom_level_bvh.h"

namespace rta
{
    class EncodedRtIp31BottomLevelBvh final : public EncodedBottomLevelBvh
    {
    public:
        // Global identifier of tlas dump in chunk files.
        static constexpr const char* kChunkIdentifier = "GpuEncBlasDump";

        /// @brief Default constructor.
        EncodedRtIp31BottomLevelBvh();

        /// @brief Destructor.
        virtual ~EncodedRtIp31BottomLevelBvh();

        /// @brief Get the total number of leaf nodes.
        ///
        /// @return The number of leaf nodes.
        virtual uint32_t GetLeafNodeCount() const override;

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

        /// @brief Get triangle pair indices of a triangle node.
        ///
        /// @param [in]  node_ptr  The node pointer.
        /// @param [out] out_count The number of triangle pair indices.
        ///
        /// @return List of pairs of (primitive_structure, triangle_pair_index).
        std::array<std::pair<const PrimitiveStructure*, uint32_t>, 8> GetTrianglePairIndices(dxr::amd::NodePointer node_ptr, uint32_t* out_count) const;

        /// @brief Get triangle primitive structure offsets of a triangle node.
        ///
        /// @param [in]  node_ptr  The node pointer.
        /// @param [out] out_count The number of primitive structure offsets.
        ///
        /// @return List of PrimitiveStructure offsets corresponding to each triangle pair of node_ptr.
        std::array<uint32_t, 8> GetPrimitiveStructureOffsets(dxr::amd::NodePointer node_ptr, uint32_t* out_count);

        /// @brief Get the number of interior/leaf nodes.
        ///
        /// @param [in] flag A BvhNodeFlags indicating which node count to return.
        ///
        /// @return The number of nodes.
        std::uint32_t GetNodeCount(const BvhNodeFlags flag) override;

        /// @brief Do the post-load step.
        ///
        /// This will be called once all the acceleration structures are loaded and fixed up. Tasks here include
        /// the surface area heuristic calculations.
        ///
        /// @return true if successful, false if error.
        bool PostLoad() override;

        /// @brief Count the number of interior and leaf nodes. Needs to only be called once before calling GetNodeCount.
        void CountNodes();

        /// @brief Does this BLAS use RTIP3.1 node packing (>1 box slot of a parent sharing one child node)?
        ///
        /// Computed once during CountNodes(). When false, node ids are unique per tree position and the frontend can
        /// key its scene/tree items by bare node id (as on non-packing hardware); when true it must fold in the global
        /// child index to de-collide the shared node's several slots.
        ///
        /// @return true if any interior node reuses a child pointer across its slots, false otherwise.
        bool HasNodePacking() const;

        /// @brief Traverse the tree for compute leaf node surface area heuristics.
        virtual void ComputeSurfaceAreaHeuristic() override;

        /// @brief Get the parent node of the node passed in.
        ///
        /// @param [in] node_addr          The node whose parent is to be found.
        /// @param [in] global_child_index The node's global child index.
        ///
        /// @return The parent node. If the node passed in is the root node, the
        /// parent node will be an invalid node.
        virtual uint32_t GetParentNode(uint32_t node_addr, uint32_t global_child_index = 0) const;

        /// @brief Traverse nodes to associate children with parents where necessary.
        virtual void PreprocessParents() override;

        /// @brief Get the surface area heuristic for a given leaf node.
        ///
        /// @param [in] node_id            The leaf node whose SAH is to be found.
        /// @param [in] global_child_index The leaf's global child index.
        ///
        /// @return The surface area heuristic.
        float GetLeafNodeSurfaceAreaHeuristic(uint32_t node_id, uint32_t global_child_index) const override;

        /// @brief Set the surface area heuristic for a given leaf node.
        ///
        /// @param [in] node_ptr               The node pointer whose SAH is to be set.
        /// @param [in] surface_area_heuristic The surface area heuristic value to be set.
        void SetLeafNodeSurfaceAreaHeuristic(uint32_t node_ptr, float surface_area_heuristic);

        /// @brief Is this a Cluster BLAS (CBLAS)?
        ///
        /// A CBLAS is a bottom-level structure whose leaves are hardware instance nodes referencing
        /// CLAS headers (geometryType == Instances). It is the middle tier of the CLAS hierarchy
        /// (TLAS -> CBLAS -> CLAS -> triangles).
        ///
        /// @return true if this is a Cluster BLAS, false otherwise.
        bool IsClusterBlas() const;

        /// @brief Is this a CLAS (Cluster Level Acceleration Structure)?
        ///
        /// A CLAS is the leaf tier of the CLAS hierarchy (TLAS -> CBLAS -> CLAS -> triangles): a
        /// ClusterLevel structure that stores triangles directly in its cluster leaf nodes.
        ///
        /// @return true if this is a CLAS, false otherwise.
        bool IsCluster() const;

        /// @brief Get the number of triangles in this BVH.
        ///
        /// A CLAS (ClusterLevel) stores its triangles directly in cluster leaf nodes and leaves the
        /// geometry-info primitive count at 0, so the base geometry-info sum reports 0. Override to
        /// return the header's active primitive count for a CLAS so vertex-buffer sizing is correct.
        ///
        /// @return The triangle count.
        uint32_t GetTriangleCount() const override;

        /// @brief Is the given node a cluster reference (a CBLAS instance-leaf pointing at a CLAS)?
        ///
        /// @param [in] node_id The node pointer.
        ///
        /// @return true if the node is a cluster reference, false otherwise.
        bool IsClusterRefNode(uint32_t node_id) const;

        /// @brief Resolve the CLAS referenced by a cluster-reference leaf to its BLAS index.
        ///
        /// @param [in]  node_id              The cluster-reference node pointer.
        /// @param [out] out_clas_blas_index  The BLAS index of the referenced CLAS.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        RraErrorCode GetClasIndexFromClusterRefNode(uint32_t node_id, uint64_t* out_clas_blas_index) const;

        /// @brief Get the (world-to-object) transform of a cluster-reference leaf.
        ///
        /// @param [in]  node_id       The cluster-reference node pointer.
        /// @param [out] out_transform A pointer to receive 12 floats of transform data.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        RraErrorCode GetClusterRefNodeTransform(uint32_t node_id, float* out_transform) const;

        /// @brief Get the instance ID stored in a cluster-reference leaf (used to label "CLAS [id]").
        ///
        /// @param [in]  node_id The cluster-reference node pointer.
        /// @param [out] out_id  A pointer to receive the instance ID.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        RraErrorCode GetClusterRefNodeId(uint32_t node_id, uint32_t* out_id) const;

        /// @brief Get the instance mask stored in a cluster-reference leaf.
        ///
        /// @param [in]  node_id  The cluster-reference node pointer.
        /// @param [out] out_mask A pointer to receive the 8-bit instance mask.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        RraErrorCode GetClusterRefNodeMask(uint32_t node_id, uint32_t* out_mask) const;

        /// @brief Store the BLAS address->index map so a CBLAS can resolve its leaves to CLAS indices.
        ///
        /// @param [in] blas_map A map of (blas_address, blas_index).
        void ConvertBlasAddressesToIndices(const std::unordered_map<GpuVirtualAddress, uint64_t>& blas_map) override;

    private:
        /// @brief Read the hardware instance node backing a CBLAS cluster-reference leaf.
        ///
        /// @param [in] node_id The cluster-reference node pointer.
        ///
        /// @return A pointer to the instance node within interior_nodes_, or nullptr if out of range.
        const HwInstanceNodeRRA* GetClusterRefInstanceNode(uint32_t node_id) const;

        /// @brief Obtain the byte size of the encoded buffer.
        ///
        /// @param [in] import_option Flag indicating which sections of the chunk to load/discard.
        ///
        /// @return The buffer size.
        std::uint64_t GetBufferByteSizeImpl(const ExportOption export_option) const override;

        /// @brief Validate the loaded BVH data for accuracy where possible.
        ///
        /// @return true if data is valid, false otherwise.
        bool Validate();

        uint32_t                               interior_node_count_{};
        uint32_t                               leaf_node_count_{};
        bool                                   has_node_packing_{false};  // Set by CountNodes(); see HasNodePacking().
        std::unordered_map<uint32_t, uint32_t> triangle_node_parents_{};  // Pairs of (triangle_node_pointer, parent_pointer).

        // Populated only for a Cluster BLAS: maps a CLAS header address to its BLAS index so cluster-reference
        // leaves resolve their referenced CLAS via an exact lookup (mirrors the TLAS blas_map_).
        std::unordered_map<GpuVirtualAddress, uint64_t> blas_map_{};
    };

}  // namespace rta

#endif  // RRA_BACKEND_BVH_ENCODED_RT_IP_31_BOTTOM_LEVEL_BVH_H_

