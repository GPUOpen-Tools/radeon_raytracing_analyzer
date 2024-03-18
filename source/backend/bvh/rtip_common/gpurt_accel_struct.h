//=============================================================================
// Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definitions for rtip3 (Navi 4 BVH definitions).
//=============================================================================

#ifndef RRA_BACKEND_RTIP3_TYPES_H_
#define RRA_BACKEND_RTIP3_TYPES_H_

#include <stdint.h>
#include "../rt_binary_file_defs.h"

/// @brief Offsets into the node data of the acceleration structure to separate the
/// different interior / leaf node buffers and geometry descriptions.
struct AccelerationStructureBufferOffsets
{
    uint32_t interior_nodes;
    uint32_t leaf_nodes;
    uint32_t geometry_info;
    uint32_t prim_node_ptrs;
};

union AccelStructHeaderInfo
{
    struct
    {
        uint32_t type : 1;                    // AccelStructType: TLAS=0, BLAS=1
        uint32_t buildType : 1;               // AccelStructBuilderType: GPU=0, CPU=1
        uint32_t mode : 4;                    // BvhBuildMode/BvhCpuBuildMode based on buildType
                                              // BvhBuildMode: Linear=0, AC=1, PLOC=2
                                              // BvhCpuBuildMode: RecursiveSAH=0, RecursiveLargestExtent=1
        uint32_t triCompression : 3;          // BLAS TriangleCompressionMode: None=0, Two=1, Pair=2
        uint32_t fp16BoxNodesInBlasMode : 2;  // BLAS FP16 box mode: None=0, Leaf=1, Mixed=2, All=3
        uint32_t triangleSplitting : 1;       // Enable TriangleSplitting
        uint32_t rebraid : 1;                 // Enable Rebraid
        uint32_t fusedInstanceNode : 1;       // Enable fused instance nodes

        uint32_t flags : 16;  // AccelStructBuildFlags
    };

    uint32_t u32All;
};

// =====================================================================================================================
// Miscellaneous packed fields describing the acceleration structure and the build method.
union AccelStructHeaderInfo2
{
    struct
    {
        uint32_t compacted : 1;  // This BVH has been compacted
        uint32_t reserved : 1;  // Unused bits
        uint32_t reserved2 : 30;  // Unused bits
    };

    uint32_t u32All;
};

#define ACCEL_STRUCT_HEADER_INFO_TYPE_SHIFT 0
#define ACCEL_STRUCT_HEADER_INFO_TYPE_MASK 0x1
#define ACCEL_STRUCT_HEADER_INFO_BUILD_TYPE_SHIFT 1
#define ACCEL_STRUCT_HEADER_INFO_BUILD_TYPE_MASK 0x1
#define ACCEL_STRUCT_HEADER_INFO_MODE_SHIFT 2
#define ACCEL_STRUCT_HEADER_INFO_MODE_MASK 0xf
#define ACCEL_STRUCT_HEADER_INFO_TRI_COMPRESS_SHIFT 6
#define ACCEL_STRUCT_HEADER_INFO_TRI_COMPRESS_MASK 0x7
#define ACCEL_STRUCT_HEADER_INFO_FP16_BOXNODE_IN_BLAS_MODE_SHIFT 9
#define ACCEL_STRUCT_HEADER_INFO_FP16_BOXNODE_IN_BLAS_MODE_MASK 0x3
#define ACCEL_STRUCT_HEADER_INFO_TRIANGLE_SPLITTING_FLAGS_SHIFT 11
#define ACCEL_STRUCT_HEADER_INFO_TRIANGLE_SPLITTING_FLAGS_MASK 0x1
#define ACCEL_STRUCT_HEADER_INFO_REBRAID_FLAGS_SHIFT 12
#define ACCEL_STRUCT_HEADER_INFO_REBRAID_FLAGS_MASK 0x1
#define ACCEL_STRUCT_HEADER_INFO_FUSED_INSTANCE_NODE_FLAGS_SHIFT 13
#define ACCEL_STRUCT_HEADER_INFO_FUSED_INSTANCE_NODE_FLAGS_MASK 0x1

struct AccelStructHeader
{
    AccelStructHeaderInfo              info;                    // Miscellaneous information about the accel struct
    uint32_t                           metadataSizeInBytes;     // Total size of the metadata in bytes (including metadata header)
    uint32_t                           sizeInBytes;             // Total size of the accel struct beginning with this header
    uint32_t                           numPrimitives;           // Number of primitives encoded in the structure
    uint32_t                           numActivePrims;          // Number of active primitives
    uint32_t                           taskIdCounter;           // Counter for allocting IDs to tasks in a persistent thread group
    uint32_t                           numDescs;                // Number of instance/geometry descs in the structure
    uint32_t                           geometryType;            // Type of geometry contained in a bottom level structure
    AccelerationStructureBufferOffsets offsets;                 // Offsets within accel struct (not including the header)
    uint32_t                           numInternalNodesFp32;    // Number of FP32 internal nodes in the acceleration structure
    uint32_t                           numInternalNodesFp16;    // Number of FP16 internal nodes in the acceleration structure
    uint32_t                           numLeafNodes;            // Number of leaf nodes used by the acceleration structure
    uint32_t                           accelStructVersion;      // GPURT_ACCEL_STRUCT_VERSION
    uint32_t                           uuidLo;                  // Client-specific UUID (low part)
    uint32_t                           uuidHi;                  // Client-specific UUID (high part)
    uint32_t                           rtIpLevel;               // Raytracing hardware IP level
    uint32_t                           fp32RootBoundingBox[6];  // Root bounding box for bottom level acceleration structures

    AccelStructHeaderInfo2 info2;
    uint32_t               packedFlags;  // Bottom level acceleration structure node flags and instance mask
                                         // Flags [0:7], Instance Exclusion Mask [8:15]
    uint32_t compactedSizeInBytes;       // Total compacted size of the accel struct

    // If enableSAHCost is enabled,
    // this can be also used to store the actual SAH cost rather than the number of primitives.
    uint32_t numChildPrims[4];

    uint32_t GetInfo()
    {
        return info.u32All;
    }

    uint32_t GetInfo2()
    {
        return info2.u32All;
    }

    bool UsesFusedInstanceNode()
    {
        return ((GetInfo() >> ACCEL_STRUCT_HEADER_INFO_FUSED_INSTANCE_NODE_FLAGS_SHIFT) & ACCEL_STRUCT_HEADER_INFO_FUSED_INSTANCE_NODE_FLAGS_MASK);
    }

    bool RebraidEnabled()
    {
        return ((GetInfo() >> ACCEL_STRUCT_HEADER_INFO_REBRAID_FLAGS_SHIFT) & ACCEL_STRUCT_HEADER_INFO_REBRAID_FLAGS_MASK);
    }

};

#endif  // RRA_BACKEND_RTIP3_TYPES_H_
