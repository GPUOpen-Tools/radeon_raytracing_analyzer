//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Top level acceleration structure definition common to all rt ip levels.
//=============================================================================

#ifndef RRA_BACKEND_BVH_ENCODED_TOP_LEVEL_BVH_H_
#define RRA_BACKEND_BVH_ENCODED_TOP_LEVEL_BVH_H_

#include <unordered_map>

#include "public/rra_error.h"

#include "bvh/ibvh.h"
#include "bvh/node_types/instance_node.h"

namespace rta
{
    class EncodedTopLevelBvh : public IBvh
    {
    public:
        // Global identifier of tlas dump in chunk files.
        static constexpr const char* kChunkIdentifier = "GpuEncTlasDump";

        /// @brief Sentinel partition index for an instance that is not assigned to any partition.
        static constexpr uint32_t kInvalidPartition = 0x7FFFFFFF;

        /// @brief Per-partition information parsed from the PTLAS metadata (PartitionInfo array).
        ///
        /// Fields 0-20 mirror the GPURT PartitionInfo struct; the bounds are derived by
        /// aggregating the per-instance AABBs of the partition's member instances.
        struct PartitionInfo
        {
            uint32_t internal_node_count = 0;              ///< Number of internal nodes in the partition.
            uint32_t instance_count      = 0;              ///< Number of instances assigned to the partition.
            uint32_t root_node_index     = 0;              ///< Root node index of the partition.
            uint32_t leaf_index_offset   = 0;              ///< Leaf index offset of the partition.
            uint32_t fat_leaf_count      = 0;              ///< Number of fat leaf nodes in the partition.
            float    translation[3]      = {0, 0, 0};      ///< Translation applied to the partition.
            float    bounds_min[3]       = {0, 0, 0};      ///< Aggregated min bound of member instances.
            float    bounds_max[3]       = {0, 0, 0};      ///< Aggregated max bound of member instances.
            bool     bounds_valid        = false;          ///< True if at least one member instance contributed bounds.
        };

        /// @brief Default constructor.
        EncodedTopLevelBvh() = default;

        /// @brief Destructor.
        virtual ~EncodedTopLevelBvh();

        /// @brief Is this a partitioned TLAS (PTLAS)?
        ///
        /// @return true if the TLAS is partitioned and partition data was parsed successfully.
        bool IsPartitioned() const;

        /// @brief Get the number of real partitions (excluding the trailing global-partition slot).
        ///
        /// @return The partition count.
        uint32_t GetPartitionCount() const;

        /// @brief Get the build-time maximum number of partition instances (0 if unset).
        ///
        /// @return The max partition instance capacity.
        uint32_t GetMaxPartitionInstances() const;

        /// @brief Get the build-time maximum number of global-partition instances (0 if unset).
        ///
        /// @return The max global instance capacity.
        uint32_t GetMaxGlobalInstances() const;

        /// @brief Get the partition info for a given partition index.
        ///
        /// Valid indices are [0, GetPartitionCount()]; index == GetPartitionCount() is the global partition.
        ///
        /// @param [in] partition_index The partition index.
        ///
        /// @return Pointer to the partition info, or nullptr if the index is out of range.
        const PartitionInfo* GetPartitionInfo(uint32_t partition_index) const;

        /// @brief Get the partition index assigned to a given instance.
        ///
        /// @param [in]  instance_index      The instance index.
        /// @param [out] out_partition_index Receives the partition index (kInvalidPartition if unassigned).
        /// @param [out] out_active          Receives whether the instance is active (may be nullptr).
        ///
        /// @return true if the instance index is valid, false otherwise.
        bool GetInstancePartitionIndex(uint32_t instance_index, uint32_t* out_partition_index, bool* out_active) const;

        /// @brief Get the index of an instance node from an instance node pointer.
        ///
        /// @param [in] node_addr  The instance node pointer.
        ///
        /// @return The instance index, or -1 if the index is invalid.
        virtual int32_t GetInstanceIndex(uint32_t node_addr) const = 0;

        /// @brief Does this BVH have references.
        ///
        /// Call this function to recreate the original GPU virtual addresses stored in the
        /// instance nodes of this dump. Use this function before exporting the dumps
        /// to the original Dxc format.
        ///
        /// @return true if the TLAS has references, false if not.
        bool HasBvhReferences() const override;

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
        virtual uint64_t GetReferencedBlasMemorySize() const = 0;

        /// @brief Get the total triangle count for this TLAS.
        ///
        /// This is the number of triangles needed to render the complete TLAS and
        /// all of its instance nodes
        ///
        /// For each instance node, add the total number of triangles in the BLAS
        /// the instance node references.
        ///
        /// @return The total number of triangles.
        virtual uint64_t GetTotalTriangleCount() const = 0;

        /// @brief Get the unique triangle count for this TLAS.
        ///
        /// This is the sum of triangles in each BLAS referenced by the TLAS.
        ///
        /// @return The number unique of triangles.
        virtual uint64_t GetUniqueTriangleCount() const = 0;

        /// @brief Get the node name for an instance node.
        ///
        /// @param [in]  node_id             The instance node pointer.
        /// @param [out] out_name            A pointer to receive the name string.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetNodeName(uint32_t node_id, const char** out_name) const = 0;

        /// @brief Get the node tooltip name for an instance node.
        ///
        /// @param [in]  node_id             The instance node pointer.
        /// @param [out] out_name            A pointer to receive the tooltip string.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetNodeNameToolTip(uint32_t node_id, const char** out_tooltip) const = 0;

        /// @brief Get the blas index of an instance node.
        ///
        /// @param [in]  node_id         The instance node pointer.
        /// @param [out] out_blas_index  A pointer to receive the blas index.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetBlasIndex(uint32_t node_id, uint64_t* out_blas_index) const = 0;

        /// @brief Get the instance index of an instance node.
        ///
        /// @param [in]  node_id             The instance node pointer.
        /// @param [out] out_instance_index  A pointer to receive the instance index.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetInstanceIndex(uint32_t node_id, uint32_t* out_instance_index) const = 0;

        /// @brief Get the instance transformation for an instance node.
        ///
        /// @param [in]  node_id             The instance node pointer.
        /// @param [out] out_transform       A pointer to receive the transform data, 12 floating points of allocation is needed.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetInstanceNodeTransform(uint32_t node_id, float* out_transform) const = 0;

        /// @brief Get the original (not inverse) instance transformation for an instance node.
        ///
        /// @param [in]  node_id             The instance node pointer.
        /// @param [out] out_transform       A pointer to receive the transform data, 12 floating points of allocation is needed.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetOriginalInstanceNodeTransform(uint32_t node_id, float* out_transform) const = 0;

        /// @brief Get the instance mask as specified through the API.
        ///
        /// @param [in]  node_id       The node pointer containing the instance.
        /// @param [out] out_mask      The mask of this instance.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetInstanceNodeMask(uint32_t node_id, uint32_t* out_mask) const = 0;

        /// @brief Get the instance ID as specified through the API.
        ///
        /// @param [in]  node_id       The node pointer containing the instance.
        /// @param [out] out_id        The ID of this instance.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetInstanceNodeID(uint32_t node_id, uint32_t* out_id) const = 0;

        /// @brief Get the instance hit group as specified through the API.
        ///
        /// @param [in]  node_id        The node pointer containing the instance.
        /// @param [out] out_hit_group  The hit group of this instance.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetInstanceNodeHitGroup(uint32_t node_id, uint32_t* out_hit_group) const = 0;

        /// @brief Retrieve the instance flags.
        ///
        /// @param [in]  node_id 	The node pointer containing the instance.
        /// @param [out] out_flags	The instance flags.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetInstanceFlags(uint32_t node_id, uint32_t* out_flags) const = 0;

        /// @brief Get the instance node for a given blas index and instance index.
        ///
        /// @param [in] blas_index     The index of the blas where the node is.
        /// @param [in] instance_index The index of the instance.
        ///
        /// @return The instance node.
        uint32_t GetInstanceNode(uint64_t blas_index, uint64_t instance_index) const;

        /// @brief Get the total procedural node count.
        ///
        /// @return The total procedural node count.
        uint64_t GetTotalProceduralNodeCount() const;

        /// @brief Get the surface area heuristic for a given leaf node.
        ///
        /// @param [in] node_id            The leaf node whose SAH is to be found.
        /// @param [in] global_child_index The leaf's global child index.
        ///
        /// @return The surface area heuristic.
        float GetLeafNodeSurfaceAreaHeuristic(uint32_t node_id, uint32_t global_child_index) const override;

        /// @brief Set the surface area heuristic for a given leaf node.
        ///
        /// @param [in] node_id                The interior node whose SAH is to be set.
        /// @param [in] surface_area_heuristic The surface area heuristic value to be set.
        void SetLeafNodeSurfaceAreaHeuristic(uint32_t node_id, float surface_area_heuristic);

    protected:
        std::unordered_map<uint64_t, std::vector<uint32_t>> instance_list_ = {};  ///< A map of BLAS index to list of node_ids of instances of that BLAS.
        std::vector<float>                                  instance_surface_area_heuristic_ = {};  ///< Surface area heuristic values for the instances.

        /// @brief Build the list for the number of instances of each BLAS.
        ///
        /// @return true if the build succeeded, false if error.
        virtual bool BuildInstanceList() = 0;

        /// @brief Parse the PTLAS partition arrays from the retained sideband data.
        ///
        /// A no-op for non-partitioned TLASes. Should be called from the derived PostLoad().
        void ParsePartitionData();

        bool                       is_partitioned_             = false;  ///< True if this TLAS is partitioned and partition data was parsed.
        uint32_t                   partition_count_            = 0;      ///< Number of real partitions (excludes the global slot).
        uint32_t                   max_partition_instances_    = 0;      ///< Build-time max partition instance capacity (0 if unset).
        uint32_t                   max_global_instances_       = 0;      ///< Build-time max global instance capacity (0 if unset).
        std::vector<PartitionInfo> partition_infos_            = {};     ///< Partition info array (partition_count_ + 1 entries; last = global).
        std::vector<uint32_t>      instance_partition_indices_ = {};     ///< Per-instance partition index (kInvalidPartition if unassigned).
        std::vector<uint8_t>       instance_partition_active_  = {};     ///< Per-instance active flag (1 = active, 0 = inactive).

    private:
        /// @brief Get the size of an instance node.
        ///
        /// This will be dependent on whether it's a fused instance or not.
        ///
        /// @return The instance node size, in bytes.
        std::int32_t GetInstanceNodeSize() const;
    };
}  // namespace rta

#endif  // RRA_BACKEND_BVH_ENCODED_TOP_LEVEL_BVH_H_

