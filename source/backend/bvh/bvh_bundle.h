//=============================================================================
// Copyright (c) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  BVH bundle definition.
///
/// A BVH bundle consists of a TLAS array, a BLAS array and index maps for
/// the TLAS & BLAS arrays.
//=============================================================================

#ifndef RRA_BACKEND_BVH_BVH_BUNDLE_H_
#define RRA_BACKEND_BVH_BVH_BUNDLE_H_

#include <vector>
#include <memory>

#include "public/rra_error.h"

#include "bvh/bvh_index_reference_map.h"
#include "ibvh.h"

namespace rta
{
    // Types of bvh dump formats
    enum class DxcBvhFormat : std::uint32_t
    {
        kUnknown = 0,
        kBvh4    = 1,
    };

    class BvhBundle final
    {
    public:
        static constexpr const char* kBvhReferenceMapIdentifier         = "BvhIndex";  ///< Unique BVH chunk identifier for tio store and load operations.
        static constexpr const char* kTopLevelReferenceMapIdentifier    = "GpuEncTlasIndex";  ///< Deprecated TLAS chunk identifier.
        static constexpr const char* kBottomLevelReferenceMapIdentifier = "GpuEncBlasIndex";  ///< Deprecated BLAS chunk identifier.

        /// @brief Constructor.
        ///
        /// @param [in] top_level_bvhs                   The top level bvh structures.
        /// @param [in] bottom_level_bvhs                The bottom level bvh structures.
        explicit BvhBundle(std::vector<std::unique_ptr<IBvh>>&& top_level_bvhs,
                           std::vector<std::unique_ptr<IBvh>>&& bottom_level_bvhs,
                           bool                                 empty_placeholder,
                           uint64_t                             missing_blas_count,
                           uint64_t                             inactive_instance_count);

        /// @brief Destructor.
        ~BvhBundle();

        /// @brief Do any of the BVH structures contain the same encoding as the value passed in.
        ///
        /// @param [in] encoding The encoding value to search for.
        ///
        /// @return true if any of the BVH structures contain the same encoding as that passed in,
        /// false if not.
        bool HasEncoding(const BvhEncoding encoding) const;

        /// @brief Get the format of the bundle.
        ///
        /// @return The bundle format.
        BvhFormat GetFormat() const;

        /// @brief Get the top level BVH structures.
        ///
        /// @return The top level BVH structures.
        const std::vector<std::unique_ptr<IBvh>>& GetTopLevelBvhs() const;

        /// @brief Get the bottom level BVH structures.
        ///
        /// @return The bottom level BVH structures.
        const std::vector<std::unique_ptr<IBvh>>& GetBottomLevelBvhs() const;

        /// @brief Get the number of BLASes in the trace.
        ///
        /// This includes empty BLASes but not missing ones.
        ///
        /// @return The total number of BLASes in the trace.
        size_t GetBlasCount() const;

        /// @brief Get the total number of BLASes in the backend.
        ///
        /// This is the size of the array used to store the BLASes.
        ///
        /// @return The total number of BLASes in the trace.
        size_t GetTotalBlasCount() const;

        /// @brief Get the number of missing BLASes in the trace.
        ///
        /// @return The number of missing BLASes.
        uint64_t GetMissingBlasCount() const;

        /// @brief Get the number of inactive instances in the trace.
        ///
        /// @return The number of inactive instances.
        uint64_t GetInactiveInstanceCount() const;

        /// @brief Get the number of empty BLASes in the trace.
        ///
        /// @return The number of empty BLASes.
        uint64_t GetEmptyBlasCount() const;

        /// @brief Has an empty placeholder BLAS been added to the list of BLASes.
        ///
        /// This is used in case there are any instance nodes that point to unreferenced
        /// BLASes.
        ///
        /// @return true if a placeholder has been added, false otherwise.
        bool ContainsEmptyPlaceholder() const;

    protected:
        std::vector<std::unique_ptr<IBvh>> top_level_bvhs_;     ///< The list of top level BVH's.
        std::vector<std::unique_ptr<IBvh>> bottom_level_bvhs_;  ///< The list of bottom level BVH's.

        bool     empty_placeholder_       = false;  ///< Has an empty BLAS been placed at index 0 in the bottom_level_bvhs_ array.
        uint64_t missing_blas_count_      = 0;      ///< The number of missing BLASes in the trace.
        uint64_t empty_blas_count_        = 0;      ///< The number of empty BLASes in the trace.
        uint64_t inactive_instance_count_ = 0;      ///< The number of inactive instances in the trace.
    };

    /// @brief Load function.
    ///
    /// @param [in]     chunk_file     A Reference to a ChunkFile object which describes the file chunk being loaded.
    /// @param [in]     encoding       The encoding scheme of the file.
    /// @param [in]     import_option  A flag indicating which sections of the chunk to load/discard.
    /// @param [in,out] io_error_code  An error code indicating whether the file loaded successfully.
    ///
    /// @return A pointer to the bundle information of the loaded file, or nullptr if the load failed.
    std::unique_ptr<BvhBundle> LoadBvhBundleFromFile(rdf::ChunkFile&           chunk_file,
                                                     const BvhEncoding         encoding,
                                                     const BvhBundleReadOption import_option,
                                                     RraErrorCode*             io_error_code);

}  // namespace rta

#endif  // RRA_BACKEND_BVH_BVH_BUNDLE_H_
