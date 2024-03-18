//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the RT IP 1.1 acceleration structure header class.
//=============================================================================

#ifndef RRA_BACKEND_BVH_IRT_IP_11_ACCELERATION_STRUCTURE_HEADER_H_
#define RRA_BACKEND_BVH_IRT_IP_11_ACCELERATION_STRUCTURE_HEADER_H_

#include <assert.h>

#include "bvh/dxr_definitions.h"
#include "bvh/rtip11/irt_ip_11_acceleration_structure_post_build_info.h"
#include "bvh/rt_binary_file_defs.h"
#include "../rtip_common/gpurt_accel_struct.h"

#include "public/rra_macro.h"

namespace rta
{
    /// @brief Represents the universal identifier of the BVH acceleration structure defined
    /// during the BVH build assigned to each BVH when built using the Vulkan API.
    struct VulkanUniversalIdentifier final
    {
        /// @brief Constructor.
        VulkanUniversalIdentifier() = default;

        /// @brief Constructor.
        explicit VulkanUniversalIdentifier(const std::uint32_t gfx_ip, const std::uint32_t build_time_hash);

        /// @brief Destructor.
        ~VulkanUniversalIdentifier() = default;

        std::uint32_t gfx_ip_          = 0;  ///< The graphics IP version.
        std::uint32_t build_time_hash_ = 0;  ///< The hash generated at build time.
    };

    // This interface provides all the information stored in a RT IP 1.1 acceleration
    // header. Allows the user to query and set different BVH attributes.
    // Contains helper functions to compute buffer sizes.
    // The BVH attributes are comprised of meta data size, build infos, total file size
    // (active) primitive counts, description counts, geometry type, offsets, and
    // interior / leaf node counts.
    class IRtIp11AccelerationStructureHeader
    {
    public:
        /// @brief Constructor.
        IRtIp11AccelerationStructureHeader() = default;

        /// @brief Destructor.
        virtual ~IRtIp11AccelerationStructureHeader();

        /// @brief Obtain the post-build information, e.g., the settings used to build the BVH.
        ///
        /// @return The build info.
        const IRtIp11AccelerationStructurePostBuildInfo& GetPostBuildInfo() const;

        /// @brief Get the metadata size.
        ///
        /// @return The size of the meta data in bytes.
        std::uint32_t GetMetaDataSize() const;

        /// @brief Get the file size.
        ///
        /// This includes the meta data size, header size, and buffer size.
        ///
        /// @return The total file size in bytes.
        std::uint32_t GetFileSize() const;

        /// @brief Get the primitive count.
        ///
        /// @return The total number of primitives (instances, triangles, bounding boxes).
        std::uint32_t GetPrimitiveCount() const;

        /// @brief Get the number of primitives that are marked active.
        ///
        /// Invalid primitives may have zero or invalid transforms / references, or degenerated primitives (NaNs).
        ///
        /// @return The number of active primitives.
        std::uint32_t GetActivePrimitiveCount() const;

        /// @brief Get the number of geometry descriptions stored in the BVH.
        ///
        /// For top-level, this represents the number of instances. For bottom-level, it represents
        /// the number of meshes (triangle / procedural).
        ///
        /// @return The geometry description count.
        std::uint32_t GetGeometryDescriptionCount() const;

        /// @brief Get fixed type of geometry for the bottom-level BVH.
        ///
        /// For top-level, this value should be ignored.
        ///
        /// @return The geometry type.
        BottomLevelBvhGeometryType GetGeometryType() const;

        /// @brief Get the offsets of single buffers.
        ///
        /// These can be interior nodes, leaf nodes, geometry infos, etc. and are
        /// relative to the start of the acceleration structure header.
        ///
        /// @return The buffer offsets.
        const AccelerationStructureBufferOffsets& GetBufferOffsets() const;

        /// @brief Set the offsets of single buffers.
        ///
        /// @param [in] offsets The buffer offsets to set.
        void SetBufferOffsets(const AccelerationStructureBufferOffsets& offsets);

        /// @brief Get the number of interior nodes with high-precision bounding boxes.
        ///
        /// @return The number of high-precision bounding boxes.
        std::uint32_t GetInteriorFp32NodeCount() const;

        /// @brief Set the number of interior nodes with high-precision bounding boxes.
        ///
        /// @param [in] interior_node_count The number of high-precision bounding boxes.
        void SetInteriorFp32NodeCount(const std::uint32_t interior_node_count);

        /// @brief Get the number of half interior nodes with high-precision bounding boxes.
        ///
        /// @brief Get the number of interior nodes with low-precision bounding boxes.
        ///
        /// @return The number of low-precision bounding boxes.
        std::uint32_t GetInteriorFp16NodeCount() const;

        /// @brief Set the number of interior nodes with low-precision bounding boxes.
        ///
        /// @param [in] interior_node_count The number of low-precision bounding boxes.
        void SetInteriorFp16NodeCount(const std::uint32_t interior_node_count);

        /// @brief Get the GPURT driver interface version this BVH was build for.
        ///
        /// This version is incremented with changes in the BVH builder or BVH format.
        ///
        /// @return The GPURT driver interface version.
        RayTracingBinaryVersion GetGpuRtDriverInterfaceVersion() const;

        /// @brief Get the total number of interior nodes.
        ///
        /// @return The total number of interior nodes.
        std::uint32_t GetInteriorNodeCount() const;

        /// @brief Calculate the total buffer size according to the number of low- and high-precision interior nodes.
        ///
        /// @return The buffer size.
        std::uint64_t CalculateInteriorNodeBufferSize() const;

        /// @brief Calculate the interior buffer size required based on the fp16 interior node mode (for bottom level only).
        ///
        /// @param [in] branching_factor The branching factor.
        ///
        /// @return The buffer size.
        std::uint64_t CalculateWorstCaseInteriorNodeBufferSize(const std::uint32_t branching_factor = 4) const;

        /// @brief Get the total number of leaf nodes.
        ///
        /// @return The number of leaf nodes.
        std::uint32_t GetLeafNodeCount() const;

        /// @brief Set the total number of leaf nodes.
        ///
        /// @param [in] leaf_node_count The number of leaf nodes.
        void SetLeafNodeCount(const std::uint32_t leaf_node_count);

        /// @brief Computes the worst-case leaf node buffer size based on the comprtession mode.
        ///
        /// @param [in] tri_compression_mode The compression mode.
        ///
        /// @return The buffer size.
        std::uint64_t CalculateWorstCaseLeafNodeBufferSize(const BvhTriangleCompressionMode tri_compression_mode) const;

        /// @brief Computes the actual leaf node buffer size based on the offsets stored in the header.
        ///
        /// @return The buffer size.
        std::uint64_t CalculateActualLeafNodeBufferSize() const;

        /// @brief Computes the expected leaf node buffer size based on the compression mode.
        ///
        /// @param [in] tri_compression_mode The compression mode.
        ///
        /// @return The buffer size.
        std::uint64_t CalculateCompressionModeLeafNodeBufferSize(const BvhTriangleCompressionMode tri_compression_mode) const;

        /// @brief Calculate the leaf node buffer size.
        ///
        /// @return The leaf node buffer size.
        std::uint64_t CalculateLeafNodeBufferSize() const;

        /// @brief Loads the header from a GPU-encoded format stored in buffer.
        ///
        /// The RT binary file version determines which GPURT version the buffer was build for.
        ///
        /// @param [in] size                     The size of the buffer.
        /// @param [in] buffer                   The buffer.
        /// @param [in] rt_binary_header_version The header version.
        void LoadFromBuffer(const std::uint64_t            size,
                            const void*                    buffer,
                            const RayTracingBinaryVersion& rt_binary_header_version = kSupportedRayTracingBinaryHeaderVersion);

        /// @brief Is the header valid.
        ///
        /// @return true if the header is valid, false if not.
        bool IsValid() const;

    private:
        virtual const IRtIp11AccelerationStructurePostBuildInfo& GetPostBuildInfoImpl() const = 0;

        virtual std::uint32_t GetMetaDataSizeImpl() const = 0;

        virtual std::uint32_t GetFileSizeImpl() const = 0;

        virtual std::uint32_t GetPrimitiveCountImpl() const = 0;

        virtual std::uint32_t GetActivePrimitiveCountImpl() const = 0;

        virtual std::uint32_t GetGeometryDescriptionCountImpl() const = 0;

        virtual BottomLevelBvhGeometryType GetGeometryTypeImpl() const = 0;

        virtual const AccelerationStructureBufferOffsets& GetBufferOffsetsImpl() const = 0;

        virtual void SetBufferOffsetsImpl(const AccelerationStructureBufferOffsets& offsets) = 0;

        virtual std::uint32_t GetInteriorFp32NodeCountImpl() const = 0;

        virtual void SetInteriorFp32NodeCountImpl(const std::uint32_t interior_node_count) = 0;

        virtual std::uint32_t GetInteriorFp16NodeCountImpl() const = 0;

        virtual void SetInteriorFp16NodeCountImpl(const std::uint32_t interior_node_count) = 0;

        virtual RayTracingBinaryVersion GetGpuRtDriverInterfaceVersionImpl() const = 0;

        virtual std::uint64_t CalculateInteriorNodeBufferSizeImpl() const = 0;

        virtual std::uint64_t CalculateWorstCaseInteriorNodeBufferSizeImpl(const std::uint32_t branching_factor = 4) const = 0;

        virtual std::uint32_t GetLeafNodeCountImpl() const = 0;

        virtual void SetLeafNodeCountImpl(const std::uint32_t leaf_node_count) = 0;

        virtual std::uint64_t CalculateWorstCaseLeafNodeBufferSizeImpl(const BvhTriangleCompressionMode tri_compression_mode) const = 0;

        virtual std::uint64_t CalculateActualLeafNodeBufferSizeImpl() const = 0;

        virtual std::uint64_t CalculateCompressionModeLeafNodeBufferSizeImpl(const BvhTriangleCompressionMode tri_compression_mode) const = 0;

        virtual std::uint64_t CalculateLeafNodeBufferSizeImpl() const = 0;

        virtual void LoadFromBufferImpl(const std::uint64_t            size,
                                        const void*                    buffer,
                                        const RayTracingBinaryVersion& rt_binary_header_version = kSupportedRayTracingBinaryHeaderVersion) = 0;

        virtual bool IsValidImpl() const = 0;
    };

    /// @brief Create a new RT IP 1.1 acceleration structure header.
    ///
    /// @return The new acceleration structure header.
    std::unique_ptr<IRtIp11AccelerationStructureHeader> CreateRtIp11AccelerationStructureHeader();
}  // namespace rta

#endif  // RRA_BACKEND_BVH_IRT_IP_11_ACCELERATION_STRUCTURE_HEADER_H_
