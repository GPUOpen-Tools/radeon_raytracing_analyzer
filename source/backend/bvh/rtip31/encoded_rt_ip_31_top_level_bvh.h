//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 3.1 (Navi4x) specific top level acceleration structure
/// definition.
//=============================================================================

#ifndef RRA_BACKEND_BVH_ENCODED_RT_IP_31_TOP_LEVEL_BVH_H_
#define RRA_BACKEND_BVH_ENCODED_RT_IP_31_TOP_LEVEL_BVH_H_

#include <optional>
#include <unordered_map>

#include "public/rra_error.h"

#include "bvh/node_types/instance_node.h"
#include "bvh/rtip31/primitive_node.h"
#include "bvh/rtip_common/encoded_top_level_bvh.h"

namespace rta
{
    class EncodedRtIp31TopLevelBvh final : public EncodedTopLevelBvh
    {
    public:
        // Global identifier of tlas dump in chunk files.
        static constexpr const char* kChunkIdentifier = "GpuEncTlasDump";

        /// @brief Default constructor.
        EncodedRtIp31TopLevelBvh();

        /// @brief Destructor.
        virtual ~EncodedRtIp31TopLevelBvh();

        /// @brief Get the instance node for a given node id.
        ///
        /// @param [in]  node_id            The instance node pointer.
        /// @param [out] out_instance_node  The instance node.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        RraErrorCode GetInstanceNodeFromInstancePointer(uint32_t node_id, InstanceNodeDataRRA* out_instance_node) const;

        /// @brief Get the index of an instance node from an instance node pointer.
        ///
        /// @param [in] node_id  The instance node pointer.
        ///
        /// @return The instance index, or -1 if the index is invalid.
        int32_t GetInstanceIndex(uint32_t node_id) const override;

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
        /// @param [in} chunk_identifier  The BVH chunk name.
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

        /// @brief Convert the instance list from using BLAS addresses to using BLAS indices.
        ///
        /// @param [in] blas_map A map of (blas_address, blas_index) used to convert instance list.
        void ConvertBlasAddressesToIndices(const std::unordered_map<GpuVirtualAddress, uint64_t>& blas_map) override;

        /// @brief Get the number of instances in this TLAS.
        ///
        /// @return The instance count.
        uint64_t GetInstanceCount(uint64_t index) const;

        /// @brief Get the number of unique BLASes referenced in the TLAS.
        ///
        /// @param [in] empty_placeholder Does this trace contain an empty placeholder.
        ///
        /// @return The number of BLASes.
        uint64_t GetBlasCount(bool empty_placeholder) const;

        /// @brief Get the memory size for all the BLASes referenced by this TLAS.
        ///
        /// @return The total memory for all referenced BLASes, in bytes.
        virtual uint64_t GetReferencedBlasMemorySize() const override;

        /// @brief Get the total triangle count for this TLAS.
        ///
        /// This is the number of triangles needed to render the complete TLAS and
        /// all of its instance nodes
        ///
        /// For each instance node, add the total number of triangles in the BLAS
        /// the instance node references.
        ///
        /// @return The total number of triangles.
        virtual uint64_t GetTotalTriangleCount() const override;

        /// @brief Get the unique triangle count for this TLAS.
        ///
        /// This is the sum of triangles in each BLAS referenced by the TLAS.
        ///
        /// @return The number unique of triangles.
        virtual uint64_t GetUniqueTriangleCount() const override;

        /// @brief Get the node name for an instance node.
        ///
        /// @param [in]  node_id             The instance node pointer.
        /// @param [out] out_name            A pointer to receive the name string.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetNodeName(uint32_t node_id, const char** out_name) const override;

        /// @brief Get the node tooltip name for an instance node.
        ///
        /// @param [in]  node_id             The instance node pointer.
        /// @param [out] out_name            A pointer to receive the tooltip string.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetNodeNameToolTip(uint32_t node_id, const char** out_tooltip) const override;

        /// @brief Get the blas index of an instance node.
        ///
        /// @param [in]  node_id         The instance node pointer.
        /// @param [out] out_blas_index  A pointer to receive the blas index.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetBlasIndex(uint32_t node_id, uint64_t* out_blas_index) const override;

        /// @brief Get the instance index of an instance node.
        ///
        /// @param [in]  node_id             The instance node pointer.
        /// @param [out] out_instance_index  A pointer to receive the instance index.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetInstanceIndex(uint32_t node_id, uint32_t* out_instance_index) const override;

        /// @brief Get the instance transformation for an instance node.
        ///
        /// @param [in]  node_id             The instance node pointer.
        /// @param [out] out_transform       A pointer to receive the transform data, 12 floating points of allocation is needed.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetInstanceNodeTransform(uint32_t node_id, float* out_transform) const override;

        /// @brief Get the original (not inverse) instance transformation for an instance node.
        ///
        /// @param [in]  node_id             The instance node pointer.
        /// @param [out] out_transform       A pointer to receive the transform data, 12 floating points of allocation is needed.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetOriginalInstanceNodeTransform(uint32_t node_id, float* out_transform) const override;

        /// @brief Get the instance mask as specified through the API.
        ///
        /// @param [in]  node_id       The node pointer containing the instance.
        /// @param [out] out_mask      The mask of this instance.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetInstanceNodeMask(uint32_t node_id, uint32_t* out_mask) const override;

        /// @brief Get the instance ID as specified through the API.
        ///
        /// @param [in]  node_id       The node pointer containing the instance.
        /// @param [out] out_id        The ID of this instance.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetInstanceNodeID(uint32_t node_id, uint32_t* out_id) const override;

        /// @brief Get the instance hit group as specified through the API.
        ///
        /// @param [in]  node_id        The node pointer containing the instance.
        /// @param [out] out_hit_group  The hit group of this instance.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetInstanceNodeHitGroup(uint32_t node_id, uint32_t* out_hit_group) const override;

        /// @brief Retrieve the instance flags.
        ///
        /// @param [in]  node_id 	The node pointer containing the instance.
        /// @param [out] out_flags	The instance flags.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetInstanceFlags(uint32_t node_id, uint32_t* out_flags) const override;

        /// @brief Get the instance node for a given blas index and instance index.
        ///
        /// @param [in] blas_index     The index of the blas where the node is.
        /// @param [in] instance_index The index of the instance.
        ///
        /// @return The instance node.
        dxr::amd::NodePointer GetInstanceNode(uint64_t blas_index, uint64_t instance_index) const;

        /// @brief Get the surface area heuristic for a given leaf node.
        ///
        /// @param [in] node_id The leaf node whose SAH is to be found.
        /// @param [in] node_id The leaf's global child index.
        ///
        /// @return The surface area heuristic.
        float GetLeafNodeSurfaceAreaHeuristic(uint32_t node_id, uint32_t global_child_index) const override;

        /// @brief Set the surface area heuristic for a given leaf node.
        ///
        /// @param [in] node_id                The interior node whose SAH is to be set.
        /// @param [in] surface_area_heuristic The surface area heuristic value to be set.
        void SetLeafNodeSurfaceAreaHeuristic(uint32_t node_id, float surface_area_heuristic);

        /// @brief Build the list for the number of instances of each BLAS.
        ///
        /// @return true if the build succeeded, false if error.
        virtual bool BuildInstanceList() override;

        /// @brief Does this TLAS use RTIP3.1 node packing (>1 box slot of a parent sharing one child node)?
        ///
        /// Computed once during BuildInstanceList(). When false, node ids are unique per tree position and the frontend
        /// can key its scene/tree items by bare node id; when true it must fold in the global child index to de-collide
        /// the shared node's several slots (mirrors the BLAS behavior).
        ///
        /// @return true if any interior node reuses a child pointer across its slots, false otherwise.
        bool HasNodePacking() const;

        /// @brief Replace all absolute references with relative references.
        ///
        /// This includes replacing absolute VA's with index values for quick lookup.
        ///
        /// @param [in] reference_map A map of virtual addresses to the acceleration structure index.
        /// @param [in] map_self If true, the map is the same type as the acceleration structure ie a BLAS using the BLAS mapping.
        /// Setting to false can be used when a TLAS needs to use a BLAS mapping to fix up the instance nodes.
        virtual void SetRelativeReferences(const std::unordered_map<GpuVirtualAddress, std::uint64_t>& reference_map,
                                           bool                                                        map_self,
                                           std::unordered_set<GpuVirtualAddress>&                      missing_set) override;

        /// @brief Convert a BLAS address to an index.
        ///
        /// @param [in]  address    The BLAS address.
        /// @param [out] out_index  A pointer to receive the BLAS index.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred. If out_index is nullptr,
        ///         kRraErrorInvalidPointer is returned.
        RraErrorCode BlasAddressToIndex(GpuVirtualAddress address, uint64_t* out_index) const;

        /// @brief Get the parent node of the node passed in.
        ///
        /// @param [in] node_id The node whose parent is to be found.
        /// @param [in] global_child_index The node's global child index.
        ///
        /// @return The parent node. If the node passed in is the root node, the
        /// parent node will be an invalid node.
        virtual uint32_t GetParentNode(uint32_t node_id, uint32_t global_child_index = 0) const override;

    private:
        /// @brief Get the size of an instance node.
        ///
        /// This will be dependent on whether it's a fused instance or not.
        ///
        /// @return The instance node size, in bytes.
        std::int32_t GetInstanceNodeSize() const;

        /// @brief Obtain the byte size of the encoded buffer.
        ///
        /// @param [in] import_option Flag indicating which sections of the chunk to load/discard.
        ///
        /// @return The buffer size.
        std::uint64_t GetBufferByteSizeImpl(const ExportOption export_option) const override;

        /// @brief Derived class implementation of GetInactiveInstanceCount().
        ///
        /// @return The number of inactive instances.
        virtual uint64_t GetInactiveInstanceCountImpl() const override;

    private:
        /// @brief Get an instance node from an instance node pointer.
        ///
        /// @param [in] node_ptr  The instance node pointer.
        ///
        /// @return The instance node, or null optional if instance node is invalid.
        std::optional<InstanceNodeDataRRA> GetHwInstanceNode(const dxr::amd::NodePointer* node_ptr) const;

        /// @brief Resolve a decoded instance BLAS childBasePtr to a BLAS index, tolerating the GPURT pointer offset.
        ///
        /// Older captures point the childBasePtr straight at the traversal root that blas_map is keyed on, so an
        /// exact lookup hits. Newer GPURT encodes it one BLAS-metadata page higher, so the exact lookup misses; in
        /// that case retry one page lower. Version- and sideband-agnostic: traces that already resolve exactly never
        /// take the fallback, so it cannot regress them.
        ///
        /// @param [in]  blas_address        The decoded (raw) BLAS address from the instance childBasePtr.
        /// @param [in]  blas_metadata_size  The instance's BLAS-metadata page size (0 if unknown).
        /// @param [out] out_index           A pointer to receive the BLAS index.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        RraErrorCode ResolveInstanceBlasAddress(GpuVirtualAddress blas_address, uint64_t blas_metadata_size, uint64_t* out_index) const;

        std::unordered_map<GpuVirtualAddress, uint64_t> blas_map_;
        // BLAS-metadata page size per decoded instance address, recorded in BuildInstanceList() so
        // ConvertBlasAddressesToIndices() can apply the version-tolerant fallback (see ResolveInstanceBlasAddress()).
        std::unordered_map<GpuVirtualAddress, uint64_t> instance_blas_metadata_size_;
        bool                                            has_node_packing_{false};  // Set by BuildInstanceList(); see HasNodePacking().
    };
}  // namespace rta

#endif  // RRA_BACKEND_BVH_ENCODED_RT_IP_31_TOP_LEVEL_BVH_H_

