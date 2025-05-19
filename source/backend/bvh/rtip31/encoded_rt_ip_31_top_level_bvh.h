//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 3.1 (Navi4x) specific top level acceleration structure
/// definition.
//=============================================================================

#ifndef RRA_BACKEND_BVH_ENCODED_RT_IP_31_TOP_LEVEL_BVH_H_
#define RRA_BACKEND_BVH_ENCODED_RT_IP_31_TOP_LEVEL_BVH_H_

#include <optional>
#include <unordered_map>

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

        /// @brief Get an instance node from an instance node pointer.
        ///
        /// @param [in] node_ptr  The instance node pointer.
        ///
        /// @return The instance node, or null optional if instance node is invalid.
        std::optional<InstanceNodeDataRRA> GetHwInstanceNode(const dxr::amd::NodePointer* node_ptr) const;

        /// @brief Get the index of an instance node from an instance node pointer.
        ///
        /// @param [in] node_ptr  The instance node pointer.
        ///
        /// @return The instance index, or -1 if the index is invalid.
        int32_t GetInstanceIndex(const dxr::amd::NodePointer* node_ptr) const;

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
        void ConvertBlasAddressesToIndices(const std::unordered_map<GpuVirtualAddress, uint64_t>& blas_map);

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

        /// @brief Get the instance node for a given blas index and instance index.
        ///
        /// @param [in] blas_index     The index of the blas where the node is.
        /// @param [in] instance_index The index of the instance.
        ///
        /// @return The instance node.
        dxr::amd::NodePointer GetInstanceNode(uint64_t blas_index, uint64_t instance_index) const;

        /// @brief Get the surface area heuristic for a given leaf node.
        ///
        /// @param [in] node_ptr The leaf node whose SAH is to be found.
        ///
        /// @return The surface area heuristic.
        float GetLeafNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr) const override;

        /// @brief Set the surface area heuristic for a given leaf node.
        ///
        /// @param [in] node_ptr               The interior node whose SAH is to be set.
        /// @param [in] surface_area_heuristic The surface area heuristic value to be set.
        void SetLeafNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr, float surface_area_heuristic);

        /// @brief Build the list for the number of instances of each BLAS.
        ///
        /// @return true if the build succeeded, false if error.
        virtual bool BuildInstanceList() override;

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
        /// @param [in] address The BLAS address.
        ///
        /// @return The BLAS index.
        uint64_t BlasAddressToIndex(GpuVirtualAddress address) const;

        /// @brief Get the parent node of the node passed in.
        ///
        /// @param [in] node_ptr The node whose parent is to be found.
        ///
        /// @return The parent node. If the node passed in is the root node, the
        /// parent node will be an invalid node.
        virtual dxr::amd::NodePointer GetParentNode(const dxr::amd::NodePointer* node_ptr) const;

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

        std::unordered_map<GpuVirtualAddress, uint64_t> blas_map_;
    };
}  // namespace rta

#endif  // RRA_BACKEND_BVH_ENCODED_RT_IP_31_TOP_LEVEL_BVH_H_

