//=============================================================================
// Copyright (c) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  BVH base class definitions.
//=============================================================================

#ifndef RRA_BACKEND_BVH_IBVH_H_
#define RRA_BACKEND_BVH_IBVH_H_

#include <unordered_map>
#include <unordered_set>

#include "bvh/gpu_def.h"

namespace rta
{
    enum class BvhNodeFlags
    {
        kNone,
        kIsInteriorNode = 1 << 0,
        kIsLeafNode     = 1 << 1
    };

    // Combines the encoding type and branching factor ranges (max and min child node reference count)
    // for interior and leaf nodes.
    struct BvhFormat
    {
        BvhEncoding  encoding;
        std::int32_t interior_node_branching_factor;
        std::int32_t leaf_node_branching_factor;
    };

    // Version of chunk files
    constexpr std::uint32_t kBvhChunkVersion = 4;

    enum class BvhBundleReadOption : std::uint8_t
    {
        // Load meta data, header, and the data containing the information of all
        // actually stored nodes. Skip all padding data.
        kIgnoreUnknown = 0,

        // Assume that there is no meta data stored.
        kNoMetaData = 0x1,

        // Skip the padding data by default.
        kDefault = kIgnoreUnknown
    };

    /// @brief We introduce an interface here as we might want to link across BVHs with
    /// different lead data types, and there's no easy way to express that if the
    /// types are completely unrelated.
    class IBvh
    {
    public:
        /// @brief Constructor.
        IBvh() = default;

        /// @brief Destructor.
        virtual ~IBvh();

        /// @brief Get the format for this BVH.
        ///
        /// @return The BVH format.
        virtual BvhFormat GetFormat() const = 0;

        /// @brief Get the number of interior/leaf nodes.
        ///
        /// @param [in] flag A BvhNodeFlags indicating which node count to return.
        ///
        /// @return The number of nodes.
        virtual std::uint32_t GetNodeCount(const BvhNodeFlags flag) const = 0;

        /// @brief Does this BVH have BVH references?
        ///
        /// Generally, TLAS's have references, BLAS's do not, so this is a quick way of testing if
        /// the derived class is a TLAS or BLAS.
        ///
        /// @return true if the BVH has BVH references, false if not.
        virtual bool HasBvhReferences() const = 0;

        /// @brief Get the size of a buffer, in bytes.
        virtual std::uint64_t GetBufferByteSize() const = 0;

        /// @brief Set the GPU virtual address for this BVH.
        ///
        /// @param [in] address The virtual address to set.
        virtual void SetVirtualAddress(const std::uint64_t address) = 0;

        /// @brief Get the GPU virtual address for this BVH.
        ///
        /// @return The virtual address.
        virtual std::uint64_t GetVirtualAddress() const = 0;

        /// @brief Replace all absolute references with relative references.
        ///
        /// This includes replacing absolute VA's with index values for quick lookup.
        ///
        /// @param [in] reference_map A map of virtual addresses to the acceleration structure index.
        /// @param [in] map_self If true, the map is the same type as the acceleration structure ie a BLAS using the BLAS mapping.
        /// Setting to false can be used when a TLAS needs to use a BLAS mapping to fix up the instance nodes.
        virtual void SetRelativeReferences(const std::unordered_map<GpuVirtualAddress, std::uint64_t>& reference_map,
                                           bool                                                        map_self,
                                           std::unordered_set<GpuVirtualAddress>&                      missing_set) = 0;

        /// @brief Do the post-load step.
        ///
        /// This will be called once all the acceleration structures are loaded and fixed up. Tasks here include
        /// the surface area heuristic calculations.
        ///
        /// @return true if successful, false if error.
        virtual bool PostLoad() = 0;

        /// @brief Checks whether the BVH has been compacted.
        ///
        /// Here, compaction means that the memory consumed by the BVH fits the actually required memory,
        /// so that the BVH does not waste unnecessary memory space.
        ///
        /// @return true if compacted, false if not.
        bool IsCompacted() const;

        /// @brief Test if the BVH is empty according to the format specifications.
        ///
        /// @return true if the BVH is empty, false if not.
        bool IsEmpty() const;

        /// @brief Get the number of inactive instances in this BVH.
        ///
        /// @return The number of inactive instances.
        uint64_t GetInactiveInstanceCount() const;

    private:
        /// @brief Derived class implementation of IsCompacted().
        ///
        /// @return true if compacted, false if not.
        virtual bool IsCompactedImpl() const = 0;

        /// @brief Derived class implementation of IsEmpty().
        ///
        /// @return true if the BVH is empty, false if not.
        virtual bool IsEmptyImpl() const = 0;

        /// @brief Derived class implementation of GetInactiveInstanceCount().
        ///
        /// @return The number of inactive instances.
        virtual uint64_t GetInactiveInstanceCountImpl() const = 0;
    };
}  // namespace rta

#endif  // RRA_BACKEND_BVH_IBVH_H_
