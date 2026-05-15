//=============================================================================
// Copyright (c) 2021-2026 Advanced Micro Devices, Inc. All rights reserved.
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

        /// @brief Default constructor.
        EncodedTopLevelBvh() = default;

        /// @brief Destructor.
        virtual ~EncodedTopLevelBvh();

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

