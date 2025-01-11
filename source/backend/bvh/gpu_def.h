//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for miscellaneous GPU definitions.
//=============================================================================

#ifndef RRA_BACKEND_BVH_GPU_DEF_H_
#define RRA_BACKEND_BVH_GPU_DEF_H_

#include <cstdint>
#include <string>

// RawAccelStruct currently supported version numbers.
#define GPURT_ACCEL_STRUCT_MAJOR_VERSION 16
#define GPURT_ACCEL_STRUCT_MINOR_VERSION 3
#define GPURT_ACCEL_STRUCT_VERSION ((GPURT_ACCEL_STRUCT_MAJOR_VERSION << 16) | GPURT_ACCEL_STRUCT_MINOR_VERSION)

namespace rta
{
    // Virtual address on the GPU
    using GpuVirtualAddress = std::uint64_t;

    enum class RayTracingIpLevel : uint32_t
    {
        _None = 0,

        RtIp1_0      = 0x1,  ///< First Implementation of HW RT
        RtIp1_1      = 0x2,  ///< Added computation of triangle barycentrics into HW
        RtIp2_0      = 0x3,  ///< Added more Hardware RayTracing features, such as BoxSort, PointerFlag, etc
        RtIp3_0      = 0x4,  ///< Added high precision box node, HW instance node, dual intersect ray, BVH8 intersect ray,
        RtIpReserved = 0x5,  ///< Special value, should not be used
        RtIp3_1      = 0x6,  ///< Added improved bvh footprints (change to node pointer, 128 Byte primitive structure format,
                             ///  128 Byte Quantized box node, obb support, wide sort)
        RtIpCount,           ///< The number of supported RtIp levels.
    };

    // Defines the generic type of a BVH.
    enum class BvhType : std::int32_t
    {
        kTopLevel = 0,  // Top level. Has instance nodes, but is never referenced by an instance node
        kBottomLevel    // Bottom level. Has no instance nodes.
    };

    enum class BvhBuildFlags : std::int32_t
    {
        kNone            = 0,
        kAllowUpdate     = 1 << 0,
        kAllowCompaction = 1 << 1,
        kFastTrace       = 1 << 2,
        kFastBuild       = 1 << 3,
        kMinimizeMemory  = 1 << 4,
        kPerformUpdate   = 1 << 5,
    };

    // Defines the type of compression mode for triangles in BVHs
    enum class BvhTriangleCompressionMode : std::int32_t
    {
        // Compression is disabled.
        kNone                       = 0,
        kTwoTriangles               = 1,
        kPairTriangles              = 2,
        kAutomaticNumberOfTriangles = 3,
        kFourTriangles              = 4
    };

    // Defines how low-precision interior nodes are created and stored during the BVH build
    enum class BvhLowPrecisionInteriorNodeMode : std::int32_t
    {
        kNone          = 0,
        kLeafNodesOnly = 1,
        kMixedWithFp32 = 2,
        kAll           = 3
    };

    // Post-build information about the settings and the builder type used
    // to build the BVH.
    class BvhPostBuildInfo final
    {
    public:
        BvhPostBuildInfo() = default;

        BvhBuildFlags                   flags                   = BvhBuildFlags::kNone;
        BvhTriangleCompressionMode      triangleCompressionMode = BvhTriangleCompressionMode::kNone;
        BvhLowPrecisionInteriorNodeMode lowPrecisionMode        = BvhLowPrecisionInteriorNodeMode::kNone;
    };

    // Defines the type of geometry stored in the bottom-level BVHs (similar to D3D12)
    enum class BottomLevelBvhGeometryType : std::int32_t
    {
        kTriangle = 0,
        kAABB     = 1
    };

    // Defines the type of node in GPU-encoded BVHs (such as RT IP 1.1)
    enum class BvhNodeType : std::int32_t
    {
        kLowPrecisionInteriorNode  = 0,
        kHighPrecisionInteriorNode = 1,
        kLeafNode                  = 2
    };

}  // namespace rta

#endif  // RRA_BACKEND_BVH_GPU_DEF_H_
