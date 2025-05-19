//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 1.1 (Navi2x) specific acceleration structure header
/// implementation.
//=============================================================================

#include "bvh/rtip11/rt_ip_11_acceleration_structure_header.h"

#include <cstring>  // --> Linux, memcpy

#include "bvh/dxr_type_conversion.h"
#include "bvh/utils.h"

namespace rta
{
    struct DxrAccelerationStructureHeader
    {
        std::uint32_t                      build_info;                       ///< The build info flags.
        std::uint32_t                      meta_data_size_in_bytes;          ///< The size of the metadata, in bytes.
        std::uint32_t                      file_size_in_bytes;               ///< The size of the acceleration structure, in bytes.
        std::uint32_t                      primitive_count;                  ///< Num Primitives, 0 for Blas
        std::uint32_t                      active_primitive_count;           ///< The number of active primitives.
        std::uint32_t                      task_id_counter;                  ///< The task ID count.
        std::uint32_t                      desc_count;                       ///< The number of descriptors.
        dxr::GeometryType                  geometry_type;                    ///< Type of primitive contained in blas, invalid for tlas.
        AccelerationStructureBufferOffsets offsets;                          ///< Offsets to the node and primitive data relative to start of this header.
        std::uint32_t                      interior_fp32_node_count;         ///< Number of interior nodes in float32.
        std::uint32_t                      interior_fp16_node_count;         ///< Number of interior nodes in float16 (for compression).
        std::uint32_t                      leaf_node_count;                  ///< Number of leaf nodes (instances for tlas, geometry for blas).
        RayTracingBinaryVersion            driver_gpu_rt_interface_version;  ///< GpuRT version.
        VulkanUniversalIdentifier          universal_identifier;             ///< Vulkan-specific universal identifier.
        std::uint32_t                      reserved;                         ///< Reserved for future expansion.
        std::uint32_t                      fp32_root_bounding_box[6];        ///< Root bounding box for bottom level acceleration structures.
        AccelStructHeaderInfo2             info2;
        std::uint32_t                      node_flags;               ///< Bottom level acceleration structure node flags.
        std::uint32_t                      compacted_size_in_bytes;  ///< Total compacted size of the accel struct.
        std::uint32_t                      num_child_prims[4];       ///< Number of primitives for 4 children for rebraid.
    };

    void DxrRtIp11AccelerationStructureHeader::LoadFromBufferImpl(const std::uint64_t            size,
                                                                  const void*                    buffer,
                                                                  const RayTracingBinaryVersion& rt_binary_header_version)
    {
        RRA_UNUSED(size);
        RRA_UNUSED(rt_binary_header_version);
        DxrAccelerationStructureHeader old_header{};
        size_t                         struct_size = sizeof(DxrAccelerationStructureHeader);
        assert(size == struct_size);
        memcpy(&old_header, buffer, struct_size);
        header_.info.u32All          = old_header.build_info;
        header_.metadataSizeInBytes  = old_header.meta_data_size_in_bytes;
        header_.sizeInBytes          = old_header.file_size_in_bytes;
        header_.numPrimitives        = old_header.primitive_count;
        header_.numActivePrims       = old_header.active_primitive_count;
        header_.taskIdCounter        = old_header.task_id_counter;
        header_.numDescs             = old_header.desc_count;
        header_.geometryType         = (uint32_t)old_header.geometry_type;
        header_.offsets              = old_header.offsets;
        header_.numInternalNodesFp32 = old_header.interior_fp32_node_count;
        header_.numInternalNodesFp16 = old_header.interior_fp16_node_count;
        header_.numLeafNodes         = old_header.leaf_node_count;
        header_.accelStructVersion   = old_header.driver_gpu_rt_interface_version.version;
        header_.uuidLo               = old_header.universal_identifier.gfx_ip_;
        header_.uuidHi               = old_header.universal_identifier.build_time_hash_;
        memcpy(header_.numChildPrims, old_header.num_child_prims, 4 * sizeof(uint32_t));
        /*
        The following fields have no equivalent in old_header:
        header_.rtIpLevel;
        header_.fp32RootBoundingBox[6];
        header_.info2;
        header_.packedFlags;
        header_.compactedSizeInBytes;
        */
        build_info_->LoadFromBuffer(sizeof(header_.info), &header_.info);
    }

}  // namespace rta

