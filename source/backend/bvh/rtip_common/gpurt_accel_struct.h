//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definitions for rtip3 (Navi 4 BVH definitions).
//=============================================================================

#ifndef RRA_BACKEND_RTIP3_TYPES_H_
#define RRA_BACKEND_RTIP3_TYPES_H_

#include <stdint.h>
#include "bvh/rt_binary_file_defs.h"

/// @brief Acceleration structure type, stored in AccelStructHeaderInfo2::type2 (GPURT >= 16.9).
///
/// Matches GpuRt::AccelStructType. Prior to v16.9 only the 1-bit info.type field existed
/// (TopLevel=0 / BottomLevel=1); PartitionTopLevel and the cluster types are only
/// distinguishable on v16.9+ captures via VersionedType().
enum class AccelStructType : uint32_t
{
    kTopLevel           = 0,
    kBottomLevel        = 1,
    kOmmArray           = 2,
    kClusterLevel       = 3,  // CLAS
    kClusterTemplate    = 4,
    kClusterBottomLevel = 5,
    kPartitionTopLevel  = 6,  // PTLAS
};

/// @brief GPURT geometry-type value stored in AccelStructHeader::geometryType.
///
/// Matches GpuRt::GeometryType. A normal DXR bottom-level structure only ever contains
/// Triangles(0) or AABBs(1). A Cluster BLAS (CBLAS) is the one bottom-level structure that
/// stores Instances(5) — its leaves are hardware instance nodes pointing at CLAS headers.
static constexpr uint32_t kGeometryTypeInstances = 5;

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

#if GPURT_BUILD_RTIP3
        uint32_t highPrecisionBoxNodeEnable : 1;  // Internal nodes are encoded as high precision box nodes
        uint32_t hwInstanceNodeEnable : 1;        // Instance nodes are formatted in hardware instance node format
#endif

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
#if GPURT_BUILD_RTIP3_1
        uint32_t bvh8Enable : 1;  // Internal box nodes are BVH8 nodes
#elif GPURT_BUILD_RTIP3
        uint32_t bvh8Enable : 1;  // Internal box nodes are BVH8 nodes
#else
        uint32_t reserved : 1;  // Unused bits
#endif
        uint32_t reserved2 : 30;  // Unused bits
    };

    uint32_t u32All;
};

// AccelStructType, stored in info2 bits [3:8] (GPURT >= 16.9). Read via AccelStructHeader::Type().
#define ACCEL_STRUCT_HEADER_INFO_2_TYPE_2_SHIFT 3
#define ACCEL_STRUCT_HEADER_INFO_2_TYPE_2_MASK 0x3F

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

#if GPURT_BUILD_RTIP3
#define ACCEL_STRUCT_HEADER_INFO_HIGH_PRECISION_BOX_NODE_FLAGS_SHIFT 14
#define ACCEL_STRUCT_HEADER_INFO_HIGH_PRECISION_BOX_NODE_FLAGS_MASK 0x1
// From v16.12 this bit was repurposed (high precision box node is implied for RTIP3.1+):
// 0 = legacy per-leaf sideband layout, 1 = new per-instance-index sideband layout.
#define ACCEL_STRUCT_HEADER_INFO_HW_INSTANCE_NODE_FMT_SHIFT 15
#define ACCEL_STRUCT_HEADER_INFO_HW_INSTANCE_NODE_FMT_MASK 0x1

#define ACCEL_STRUCT_HEADER_INFO_FLAGS_SHIFT 16
#define ACCEL_STRUCT_HEADER_INFO_FLAGS_MASK 0xffff

#define ACCEL_STRUCT_HEADER_INFO_2_BVH_COMPACTION_FLAGS_SHIFT 0
#define ACCEL_STRUCT_HEADER_INFO_2_BVH_COMPACTION_FLAGS_MASK 0x1

#define ACCEL_STRUCT_HEADER_INFO_2_BVH8_FLAGS_SHIFT 1
#define ACCEL_STRUCT_HEADER_INFO_2_BVH8_FLAGS_MASK 0x1
#endif

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

    uint32_t GetInfo() const
    {
        return info.u32All;
    }

    uint32_t GetInfo2() const
    {
        return info2.u32All;
    }

    uint32_t GetMajorVersion() const
    {
        return (accelStructVersion >> 16U);
    }

    uint32_t GetMinorVersion() const
    {
        return (accelStructVersion & 0xFFFFU);
    }

    bool IsOlderThanVersion(uint32_t major, uint32_t minor) const
    {
        return (GetMajorVersion() < major) || ((GetMajorVersion() == major) && (GetMinorVersion() < minor));
    }

    // Acceleration structure type from the 6-bit info2.type2 field (GPURT >= 16.9).
    AccelStructType Type() const
    {
        return static_cast<AccelStructType>((GetInfo2() >> ACCEL_STRUCT_HEADER_INFO_2_TYPE_2_SHIFT) & ACCEL_STRUCT_HEADER_INFO_2_TYPE_2_MASK);
    }

    // Version-aware type: pre-16.9 falls back to the 1-bit info.type (TopLevel/BottomLevel only).
    AccelStructType VersionedType() const
    {
        if (IsOlderThanVersion(16, 9))
        {
            return static_cast<AccelStructType>((GetInfo() >> ACCEL_STRUCT_HEADER_INFO_TYPE_SHIFT) & ACCEL_STRUCT_HEADER_INFO_TYPE_MASK);
        }
        return Type();
    }

    // True if this is a partitioned TLAS (PTLAS). Only detectable on v16.9+ captures.
    bool IsPartitioned() const
    {
        return VersionedType() == AccelStructType::kPartitionTopLevel;
    }

    // True if this is a Cluster BLAS (CBLAS): a bottom-level structure whose leaves are
    // hardware instance nodes referencing CLAS headers (geometryType == Instances). This is
    // the CLAS-hierarchy middle tier (TLAS -> CBLAS -> CLAS -> triangles).
    bool IsClusterBlas() const
    {
        return (VersionedType() == AccelStructType::kBottomLevel) && (geometryType == kGeometryTypeInstances);
    }

    // True if this is a CLAS (Cluster Level Acceleration Structure): the leaf tier holding
    // ordinary triangle geometry, referenced by CBLAS instance leaves.
    bool IsCluster() const
    {
        return VersionedType() == AccelStructType::kClusterLevel;
    }

    // Number of partitions (excluding the trailing global-partition slot).
    // Valid only when IsPartitioned(). See copy/EmitAS.hlsl and copy/DecodeAS.hlsl.
    uint32_t GetNumPartitions() const
    {
        return (numPrimitives >= numDescs) ? (numPrimitives - numDescs) : 0;
    }

    // PTLAS build-time capacities (gpurt padding0/padding1 @ byte 120/124). 0 if unset.
    uint32_t MaxNumPartitionInstances() const
    {
        return numChildPrims[2];
    }

    uint32_t MaxNumGlobalInstances() const
    {
        return numChildPrims[3];
    }

    bool UsesFusedInstanceNode()
    {
        return ((GetInfo() >> ACCEL_STRUCT_HEADER_INFO_FUSED_INSTANCE_NODE_FLAGS_SHIFT) & ACCEL_STRUCT_HEADER_INFO_FUSED_INSTANCE_NODE_FLAGS_MASK);
    }

    bool RebraidEnabled()
    {
        return ((GetInfo() >> ACCEL_STRUCT_HEADER_INFO_REBRAID_FLAGS_SHIFT) & ACCEL_STRUCT_HEADER_INFO_REBRAID_FLAGS_MASK);
    }

#if GPURT_BUILD_RTIP3
    bool UsesHighPrecisionBoxNode()
    {
        return ((GetInfo() >> ACCEL_STRUCT_HEADER_INFO_HIGH_PRECISION_BOX_NODE_FLAGS_SHIFT) & ACCEL_STRUCT_HEADER_INFO_HIGH_PRECISION_BOX_NODE_FLAGS_MASK);
    }

    bool UsesHardwareInstanceNode()
    {
        return ((GetInfo() >> ACCEL_STRUCT_HEADER_INFO_HW_INSTANCE_NODE_FMT_SHIFT) & ACCEL_STRUCT_HEADER_INFO_HW_INSTANCE_NODE_FMT_MASK);
    }

    // 16.12 is the version that moved the RTIP3.1 instance sideband, so the version alone names the layout.
    bool UsesLegacySidebandLayout() const
    {
        return IsOlderThanVersion(16, 12);
    }

    bool UsesBVH8()
    {
        return ((GetInfo2() >> ACCEL_STRUCT_HEADER_INFO_2_BVH8_FLAGS_SHIFT) & ACCEL_STRUCT_HEADER_INFO_2_BVH8_FLAGS_MASK);
    }
#endif
};

#endif  // RRA_BACKEND_RTIP3_TYPES_H_

