//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of the DXR helper functions and definitions.
//=============================================================================

#ifndef RRA_BACKEND_BVH_DXR_DEFINITIONS_H_
#define RRA_BACKEND_BVH_DXR_DEFINITIONS_H_

#include <cstdint>
#include <array>

#include "gpu_def.h"

// Enable this if built with C++11
#if 0  // #ifdef _LINUX
namespace std
{
    // make_unique isn't present on gcc in C++11, so make our own.
    template <typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}  // namespace std
#endif

// The following structs are similar to the structs defined in d3d12.h
// Added dxr namespace
namespace dxr
{
    constexpr std::uint64_t kGpuNullptr = UINT64_MAX;

    // DXR-like data type defines for transform (Matrix3x4) and float32-precision bounding boxes
    using Matrix3x4   = std::array<float, 12>;
    using BBoxFloat32 = std::array<float, 6>;
    using HalfFloat   = std::uint16_t;

    // Byte sizes for all structs contained in decoded (mesh) dumps on RT IP 1.1.
    constexpr std::uint32_t kBoundingBoxFp32Size = 6 * sizeof(float);
    constexpr std::uint32_t kMatrix3x4Size       = 12 * sizeof(float);

    enum class BvhType : std::uint32_t
    {
        kBvhTypeTopLevel    = 0,
        kBvhTypeBottomLevel = 1
    };

    enum class InstanceGeometryTypeFlag : std::uint32_t
    {
        kNone     = 0,
        kAABB     = 0x1,
        kTriangle = 0x2
    };

    // Description of the instance data (pointer to the internal nodes in memory,
    // blas meta data size (important for parent node pointer block), and
    // GPU address of the blas.
    enum class HardwareInstanceFlags : std::uint32_t
    {
        kAmdFlagNone = 0,
    };

    // Instance flag bits from the DXR spec
    // See https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#d3d12_raytracing_instance_flags
    enum class InstanceFlags : std::uint32_t
    {
        kFlagNone                = 0,
        kFlagTriangleCullDisable = 0x1,
        kFlagTriangleCCW         = 0x2,
        kFlagForceOpaque         = 0x4,
        kFlagForceNonOpaque      = 0x8
    };

    // DXGI formats for the index buffer.
    enum class IndexBufferFormat : std::uint32_t
    {
        kFormatUnknown = 0,
    };

    enum class VertexBufferFormat : std::uint32_t
    {
        kFormatUnknown = 0,
    };

    // Description of geometry: vertex and index buffer for meshes,
    // bounding boxes for procedural geometry.

    // Geometry flag bits
    enum class GeometryFlags : std::uint32_t
    {
        kAmdFlagNone              = 0x0,
        kAmdFlagOpaque            = 0x1,
        kAmdFlagNoDuplicateAnyHit = 0x2
    };

    namespace amd
    {
        // Global definitions / enums

        // Defines the type of the acceleration structure.
        enum class AccelerationStructureBvhType : std::uint32_t
        {
            kAmdTopLevel    = 0,
            kAmdBottomLevel = 1
        };

        // Contains all the DXR-specific build flags including update and compaction options.
        enum class AccelerationStructureBuildFlags : std::uint32_t
        {
            kNone            = 0x0,
            kAllowUpdate     = 0x1,
            kAllowCompaction = 0x2,
            kPreferFastTrace = 0x4,
            kPreferFastBuild = 0x8,
            kMinimizeMemory  = 0x10,
            kPerformUpdate   = 0x20
        };

        // Defines the type of node stored in the node pointer.
        enum class NodeType : std::uint32_t
        {
            kAmdNodeTriangle0  = 0,
            kAmdNodeTriangle1  = 1,
            kAmdNodeTriangle2  = 2,
            kAmdNodeTriangle3  = 3,
            kAmdNodeBoxFp16    = 4,
            kAmdNodeBoxFp32    = 5,
            kAmdNodeInstance   = 6,
            kAmdNodeProcedural = 7,
        };

        // Defines how many triangles are compressed in one single leaf node.
        // Four triangle compression mode has been removed since May 13, 2021.
        enum class TriangleCompressionMode : std::uint32_t
        {
            // Compression is disabled.
            kAmdNoTriangleCompression = 0,

            // Two triangles are stored in one leaf node, each of the two triangles is linked individually
            // by a single node pointer.
            kAmdTwoTriangleCompression = 1,

            // Two triangles are stored in one leaf node, but the triangles are only linked once by just
            // tracking the node type. For example, a node pointer with (Triangle1, ptr) references the
            // first triangle, (Triangle0, ptr) references the second one.
            kAmdPairTriangleCompression = 2,

            // Automatic selection of the compression mode.
            kAmdAutoTriangleCompression = 3,
        };

        // Defines if and how many nodes are converted to half-precision box nodes during the BVH build.
        enum class BottomLevelFp16Mode : std::uint32_t
        {
            // All nodes use full precision for bounding boxes.
            kNone = 0,

            // Nodes referencing a leaf node (triangles) use half-precision for bounding boxes.
            kLeafNodesOnly = 1,

            // Nodes are converted to half-precision if the volume / surface of the enlarged bounding box
            // (after the fp16 conversion) lies below a user-defined threshold. This mode is not used.
            kMixedWithFp32 = 2,

            // All nodes are converted to half-precision.
            kAll = 3
        };

        // Byte sizes for all structs contained in BVH4 dumps on RT IP 1.1.
        constexpr std::uint32_t kInvalidNode                     = UINT32_MAX;
        constexpr std::uint32_t kFusedInstanceNodeSize           = 256;
        constexpr std::uint32_t kInstanceNodeSize                = 128;
        constexpr std::uint32_t kFp32BoxNodeSize                 = 128;
        constexpr std::uint32_t kFp16BoxNodeSize                 = 64;
        constexpr std::uint32_t kLeafNodeSize                    = 64;
        constexpr std::uint32_t kInstanceExtraDataSize           = 64;
        constexpr std::uint32_t kMetaDataV1Size                  = 108;
        constexpr std::uint32_t kAccelerationStructureHeaderSize = 128;
        constexpr std::uint32_t kParentChunkSize                 = 64;
        constexpr std::uint32_t kGeometryInfoSize                = 12;
        constexpr std::uint32_t kMetaDataAlignment               = 128;

        // Custom float3 definition
        class Float3 final
        {
        public:
            float x, y, z;

            bool operator==(const Float3& rhs) const
            {
                return x == rhs.x && y == rhs.y && z == rhs.z;
            }

            bool operator!=(const Float3& rhs) const
            {
                return !(*this == rhs);
            }
        };

        // Default definition of axis aligned bboxes.
        class AxisAlignedBoundingBox final
        {
        public:
            AxisAlignedBoundingBox() = default;
            AxisAlignedBoundingBox(const Float3& min, const Float3& max)
                : min(min)
                , max(max)
            {
            }

            Float3 min = {};
            Float3 max = {};
        };

    }  // namespace amd
}  // namespace dxr

#endif  // RRA_BACKEND_BVH_DXR_DEFINITIONS_H_
