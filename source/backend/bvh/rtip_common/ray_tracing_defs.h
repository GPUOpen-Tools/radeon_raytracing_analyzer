//=============================================================================
//  Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 3.1 (Navi4x) specific ray tracing definitions.
//=============================================================================

#ifndef RRA_BACKEND_RAYTRACING_DEF_H_
#define RRA_BACKEND_RAYTRACING_DEF_H_

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4505)  // Disable unreferenced local function warning.
#pragma warning(disable : 4189)  // Disable unreferenced local variable warning.
#pragma warning(disable : 4100)  // Disable unreferenced formal parameter warning.
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#include <cstdint>
#include <cstring>

#include "glm/glm/glm.hpp"


//=====================================================================================================================
///@note Enum is a reserved keyword in glslang. To workaround this limitation, define static constants to replace the
///      HLSL enums that follow for compatibility.
//=====================================================================================================================
namespace PrimitiveType
{
    static const uint32_t Triangle = 0;
    static const uint32_t AABB     = 1;
    static const uint32_t Instance = 2;
}  // namespace PrimitiveType

// These DUMMY_*_FUNC postfix stubs must be included at the end of every driver stub (AmdTraceRay*) declaration to
// work around a Vulkan glslang issue where the compiler can't deal with calls to functions that don't have bodies.
#if defined(AMD_VULKAN)
#define DUMMY_BOOL_FUNC \
    {                   \
        return false;   \
    }
#define DUMMY_VOID_FUNC \
    {                   \
    }
#define DUMMY_UINT_FUNC \
    {                   \
        return 0;       \
    }
#define DUMMY_UINT2_FUNC    \
    {                       \
        return uint2(0, 0); \
    }
#define DUMMY_UINT3_FUNC       \
    {                          \
        return uint3(0, 0, 0); \
    }
#define DUMMY_UINT4_FUNC          \
    {                             \
        return uint4(0, 0, 0, 0); \
    }
#define DUMMY_FLOAT_FUNC \
    {                    \
        return 0;        \
    }
#define DUMMY_FLOAT2_FUNC    \
    {                        \
        return float2(0, 0); \
    }
#define DUMMY_FLOAT3_FUNC       \
    {                           \
        return float3(0, 0, 0); \
    }
#define DUMMY_GENERIC_FUNC(value) \
    {                             \
        return value;             \
    }
#define DUMMY_WIDE_INTERSECT_FUNC      \
    {                                  \
        return (DualIntersectResult)0; \
    }
#else
#define DUMMY_BOOL_FUNC ;
#define DUMMY_VOID_FUNC ;
#define DUMMY_UINT_FUNC ;
#define DUMMY_UINT2_FUNC ;
#define DUMMY_UINT3_FUNC ;
#define DUMMY_UINT4_FUNC ;
#define DUMMY_FLOAT_FUNC ;
#define DUMMY_FLOAT2_FUNC ;
#define DUMMY_FLOAT3_FUNC ;
#define DUMMY_GENERIC_FUNC(value) ;
#define DUMMY_WIDE_INTERSECT_FUNC ;
#endif

#if defined(__cplusplus)
#define __decl extern
#endif

//=====================================================================================================================
// Acceleration structure type
#define TOP_LEVEL 0
#define BOTTOM_LEVEL 1

//=====================================================================================================================
// BVH node types shared between HW and SW nodes
#define NODE_TYPE_TRIANGLE_0 0
#define NODE_TYPE_TRIANGLE_1 1
#define NODE_TYPE_TRIANGLE_2 2
#define NODE_TYPE_TRIANGLE_3 3
#define NODE_TYPE_BOX_FLOAT16 4
#define NODE_TYPE_BOX_FLOAT32x2 2
#define NODE_TYPE_BOX_HP64x2 3
#define NODE_TYPE_BOX_HP64 4
#define NODE_TYPE_BOX_QUANTIZED_BVH8 5
#define NODE_TYPE_BOX_FLOAT32 5
#define NODE_TYPE_USER_NODE_INSTANCE 6
// From the HW IP 2.0 spec: '7: User Node 1 (processed as a Procedural Node for culling)'
#define NODE_TYPE_USER_NODE_PROCEDURAL 7
#define NODE_TYPE_TRIANGLE_4 8
#define NODE_TYPE_TRIANGLE_5 9
#define NODE_TYPE_TRIANGLE_6 10
#define NODE_TYPE_TRIANGLE_7 11
//=====================================================================================================================
// Triangle Compression Modes
#define NO_TRIANGLE_COMPRESSION 0
#define RESERVED 1
#define PAIR_TRIANGLE_COMPRESSION 2
#define AUTO_TRIANGLE_COMPRESSION 3

//=====================================================================================================================
// Amount of ULPs(Unit in Last Place) added to Box node when using hardware intersection instruction
#define BOX_EXPANSION_DEFAULT_AMOUNT 6

//=====================================================================================================================
// Box sorting heuristic value
// 0: closethit
// 1: LargestFirst
// 2: ClosestMidpoint
// 3: undefined / disabled
// 4: LargestFirstOrClosest (auto select with rayFlag)
// 5: BoxSortLargestFirstOrClosestMidPoint  (auto select with rayFlag)
// 6: DisabledOnAcceptFirstHit (disable if bvhNode sort is on, and rayFlag is AcceptFirstHit)
//
// This need to match ILC_BOX_SORT_HEURISTIC_MODE

enum BoxSortHeuristic : uint32_t
{
    Closest                       = 0x0,
    Largest                       = 0x1,
    MidPoint                      = 0x2,
    Disabled                      = 0x3,
    LargestFirstOrClosest         = 0x4,
    LargestFirstOrClosestMidPoint = 0x5,
    DisabledOnAcceptFirstHit      = 0x6,
};

#ifdef AMD_VULKAN_GLSLANG
//=====================================================================================================================
///@note Enum is a reserved keyword in glslang. To workaround this limitation, define static constants to replace the
///      HLSL enums that follow for compatibility.
//=====================================================================================================================
namespace SceneBoundsCalculation
{
    static const uint32_t SceneBoundsBasedOnGeometry         = 0x0;
    static const uint32_t SceneBoundsBasedOnGeometryWithSize = 0x1;
}  // namespace SceneBoundsCalculation
#else
enum SceneBoundsCalculation : uint32_t
{
    SceneBoundsBasedOnGeometry         = 0x0,
    SceneBoundsBasedOnGeometryWithSize = 0x1
};
#endif

//=====================================================================================================================
// Options for where FP16 box nodes are created within BLAS for QBVH
#define NO_NODES_IN_BLAS_AS_FP16 0
#define LEAF_NODES_IN_BLAS_AS_FP16 1
#define MIXED_NODES_IN_BLAS_AS_FP16 2
#define ALL_INTERIOR_NODES_IN_BLAS_AS_FP16 3

// Mask for MSB within node pointer - used to mark nodes in RefitBounds
#define NODE_POINTER_MASK_MSB 0x80000000u

//=====================================================================================================================
#define BVH4_NODE_32_STRIDE_SHIFT 7  // Box 32 node
#define BVH4_NODE_16_STRIDE_SHIFT 6  // Box 16 node
#if GPURT_BUILD_RTIP3
#define BVH4_NODE_HIGH_PRECISION_STRIDE_SHIFT 6  // High precision box node
#endif

#define INVALID_IDX 0xffffffff
#define INACTIVE_PRIM 0xfffffffe
#define PAIRED_TRI_LINKONLY 0xfffffffd

static const uint32_t ByteStrideScratchNode = 64;
static const uint32_t ByteStrideU32         = 12;
static const uint32_t IndexFormatInvalid    = 0;
static const uint32_t IndexFormatU32        = 1;
static const uint32_t IndexFormatU16        = 2;

const static uint32_t TILE_WIDTH = 256;
const static uint32_t TILE_SIZE  = TILE_WIDTH * TILE_WIDTH;

#ifndef BUILD_THREADGROUP_SIZE
#define BUILD_THREADGROUP_SIZE 64
#endif

//=====================================================================================================================
// Common structure definitions
typedef uint64_t GpuVirtualAddress;

#define GPU_VIRTUAL_ADDRESS_SIZE 8

#ifdef __cplusplus
static_assert(GPU_VIRTUAL_ADDRESS_SIZE == sizeof(GpuVirtualAddress), "GPU Virtual Address mismatch");
#endif  // __cplusplus

//=====================================================================================================================
struct BoundingBox  // matches D3D12_RAYTRACING_AABB
{
    glm::vec3 min;
    glm::vec3 max;
};

//=====================================================================================================================
// Internal bounding box type for scene bounds.
struct UintBoundingBox
{
    glm::uvec3 min;
    glm::uvec3 max;
};

struct UintBoundingBox4
{
    glm::uvec4 min;
    glm::uvec4 max;
};

struct PackedUintBoundingBox4
{
    uint64_t min;
    uint64_t max;
};

//=====================================================================================================================
// Acceleration structure result data offsets
//typedef AccelStructDataOffsets AccelStructOffsets;

//=====================================================================================================================
// Hardware 32-bit box node format and offsets
#define FLOAT32_BBOX_STRIDE 24
#define FLOAT32_BOX_NODE_CHILD0_OFFSET 0
#define FLOAT32_BOX_NODE_CHILD1_OFFSET 4
#define FLOAT32_BOX_NODE_CHILD2_OFFSET 8
#define FLOAT32_BOX_NODE_CHILD3_OFFSET 12
#define FLOAT32_BOX_NODE_BB0_MIN_OFFSET 16
#define FLOAT32_BOX_NODE_BB0_MAX_OFFSET 28
#define FLOAT32_BOX_NODE_BB1_MIN_OFFSET 40
#define FLOAT32_BOX_NODE_BB1_MAX_OFFSET 52
#define FLOAT32_BOX_NODE_BB2_MIN_OFFSET 64
#define FLOAT32_BOX_NODE_BB2_MAX_OFFSET 76
#define FLOAT32_BOX_NODE_BB3_MIN_OFFSET 88
#define FLOAT32_BOX_NODE_BB3_MAX_OFFSET 100
#define FLOAT32_BOX_NODE_FLAGS_OFFSET 112
#define FLOAT32_BOX_NODE_NUM_PRIM_OFFSET 116
#define FLOAT32_BOX_NODE_INSTANCE_MASK_OFFSET 120
#define FLOAT32_BOX_NODE_OBB_OFFSET 124
#define FLOAT32_BOX_NODE_SIZE 128

//=====================================================================================================================
// Float32 box node flags contains 4 1-byte fields, 1 per child node:
// Child 0 [ 7: 0]
// Child 1 [15: 8]
// Child 2 [23:16]
// Child 3 [31:24]
//
// Each child node's 1-byte field contains these flags:
// Only Opaque     [  0]
// Only Non-Opaque [  1]
// Only Triangles  [  2]
// Only Procedural [  3]
// Unused          [7:4]
#define BOX_NODE_FLAGS_BIT_STRIDE 8

#define HPB64_BOX_NODE_FLAGS_BIT_STRIDE 4

#define BOX_NODE_FLAGS_ONLY_OPAQUE_SHIFT 0
#define BOX_NODE_FLAGS_ONLY_NON_OPAQUE_SHIFT 1
#define BOX_NODE_FLAGS_ONLY_TRIANGLES_SHIFT 2
#define BOX_NODE_FLAGS_ONLY_PROCEDURAL_SHIFT 3

//=====================================================================================================================
struct Float32BoxNode
{
    uint32_t child0;  /// Child node pointer 0
    uint32_t child1;  /// Child node pointer 1
    uint32_t child2;  /// Child node pointer 2
    uint32_t child3;  /// Child node pointer 3

    glm::vec3 bbox0_min;  /// Node bounding box 0 minimum bounds
    glm::vec3 bbox0_max;  /// Node bounding box 0 maximum bounds

    glm::vec3 bbox1_min;  /// Node bounding box 1 minimum bounds
    glm::vec3 bbox1_max;  /// Node bounding box 1 maximum bounds

    glm::vec3 bbox2_min;  /// Node bounding box 2 minimum bounds
    glm::vec3 bbox2_max;  /// Node bounding box 2 maximum bounds

    glm::vec3 bbox3_min;  /// Node bounding box 3 minimum bounds
    glm::vec3 bbox3_max;  /// Node bounding box 3 maximum bounds

    uint32_t flags;           /// Reserved for RTIP 2.0
    uint32_t numPrimitives;   /// Padding for 64-byte alignment
    uint32_t instanceMask;    /// Packed instance masks of all children
    uint32_t obbMatrixIndex;  /// Discretized OBB matrix index.

#ifdef __cplusplus
    // parameterised constructor for HLSL compatibility
    Float32BoxNode(uint32_t val)
    {
        memset(this, val, sizeof(Float32BoxNode));
    }

    // default constructor
    Float32BoxNode()
        : Float32BoxNode(0)
    {
    }
#endif
};

#ifdef __cplusplus
static_assert(FLOAT32_BOX_NODE_SIZE == sizeof(Float32BoxNode), "Float32BoxNode structure mismatch");
static_assert(FLOAT32_BOX_NODE_CHILD0_OFFSET == offsetof(Float32BoxNode, child0), "");
static_assert(FLOAT32_BOX_NODE_CHILD1_OFFSET == offsetof(Float32BoxNode, child1), "");
static_assert(FLOAT32_BOX_NODE_CHILD2_OFFSET == offsetof(Float32BoxNode, child2), "");
static_assert(FLOAT32_BOX_NODE_CHILD3_OFFSET == offsetof(Float32BoxNode, child3), "");
static_assert(FLOAT32_BOX_NODE_BB0_MIN_OFFSET == offsetof(Float32BoxNode, bbox0_min), "");
static_assert(FLOAT32_BOX_NODE_BB0_MAX_OFFSET == offsetof(Float32BoxNode, bbox0_max), "");
static_assert(FLOAT32_BOX_NODE_BB1_MIN_OFFSET == offsetof(Float32BoxNode, bbox1_min), "");
static_assert(FLOAT32_BOX_NODE_BB1_MAX_OFFSET == offsetof(Float32BoxNode, bbox1_max), "");
static_assert(FLOAT32_BOX_NODE_BB2_MIN_OFFSET == offsetof(Float32BoxNode, bbox2_min), "");
static_assert(FLOAT32_BOX_NODE_BB2_MAX_OFFSET == offsetof(Float32BoxNode, bbox2_max), "");
static_assert(FLOAT32_BOX_NODE_BB3_MIN_OFFSET == offsetof(Float32BoxNode, bbox3_min), "");
static_assert(FLOAT32_BOX_NODE_BB3_MAX_OFFSET == offsetof(Float32BoxNode, bbox3_max), "");
static_assert(FLOAT32_BOX_NODE_FLAGS_OFFSET == offsetof(Float32BoxNode, flags), "");
static_assert(FLOAT32_BOX_NODE_NUM_PRIM_OFFSET == offsetof(Float32BoxNode, numPrimitives), "");
static_assert(FLOAT32_BOX_NODE_INSTANCE_MASK_OFFSET == offsetof(Float32BoxNode, instanceMask), "");
static_assert(FLOAT32_BOX_NODE_OBB_OFFSET == offsetof(Float32BoxNode, obbMatrixIndex), "");
#endif  // __cplusplus

//=====================================================================================================================
// Hardware 16-bit box node format and offsets
#define FLOAT16_BBOX_STRIDE 12
#define FLOAT16_BOX_NODE_CHILD0_OFFSET 0
#define FLOAT16_BOX_NODE_CHILD1_OFFSET 4
#define FLOAT16_BOX_NODE_CHILD2_OFFSET 8
#define FLOAT16_BOX_NODE_CHILD3_OFFSET 12
#define FLOAT16_BOX_NODE_BB0_OFFSET 16
#define FLOAT16_BOX_NODE_BB1_OFFSET 28
#define FLOAT16_BOX_NODE_BB2_OFFSET 40
#define FLOAT16_BOX_NODE_BB3_OFFSET 52
#define FLOAT16_BOX_NODE_SIZE 64

//=====================================================================================================================
struct Float16BoxNode
{
    uint32_t child0;  /// Child node pointer 0
    uint32_t child1;  /// Child node pointer 1
    uint32_t child2;  /// Child node pointer 2
    uint32_t child3;  /// Child node pointer 3

    glm::uvec3 bbox0;  /// Node bounding box 0, packed, uses float16: minx, miny | minz, maxx | maxy, maxz
    glm::uvec3 bbox1;  /// Node bounding box 1, packed, uses float16: minx, miny | minz, maxx | maxy, maxz
    glm::uvec3 bbox2;  /// Node bounding box 2, packed, uses float16: minx, miny | minz, maxx | maxy, maxz
    glm::uvec3 bbox3;  /// Node bounding box 3, packed, uses float16: minx, miny | minz, maxx | maxy, maxz

    // NOTE: each bounding box is defined as glm::uvec3 for simplicity
    // Each 32 bits pack 2x float16s. Order above is written as: a, b
    // with a located in the lower 16 bits, b in the upper 16 bits
    // bbox0.x stores minx, miny
    //
    // Alternatively, one can define each bbox as a pair of float16_t3
    // similar to FLOAT32_BOX_NODE. Indexing in hlsl would require extra work
};

#ifdef __cplusplus
static_assert(FLOAT16_BOX_NODE_SIZE == sizeof(Float16BoxNode), "Float16BoxNode structure mismatch");
static_assert(FLOAT16_BOX_NODE_CHILD0_OFFSET == offsetof(Float16BoxNode, child0), "");
static_assert(FLOAT16_BOX_NODE_CHILD1_OFFSET == offsetof(Float16BoxNode, child1), "");
static_assert(FLOAT16_BOX_NODE_CHILD2_OFFSET == offsetof(Float16BoxNode, child2), "");
static_assert(FLOAT16_BOX_NODE_CHILD3_OFFSET == offsetof(Float16BoxNode, child3), "");
static_assert(FLOAT16_BOX_NODE_BB0_OFFSET == offsetof(Float16BoxNode, bbox0), "");
static_assert(FLOAT16_BOX_NODE_BB1_OFFSET == offsetof(Float16BoxNode, bbox1), "");
static_assert(FLOAT16_BOX_NODE_BB2_OFFSET == offsetof(Float16BoxNode, bbox2), "");
static_assert(FLOAT16_BOX_NODE_BB3_OFFSET == offsetof(Float16BoxNode, bbox3), "");
#endif  // __cplusplus

//=====================================================================================================================
// Hardware triangle node format and offsets
// Note: GPURT limits triangle compression to 2 triangles per node. As a result the remaining bytes in the triangle node
// are used for sideband data. The geometry index is packed in bottom 24 bits and geometry flags in bits 25-26.
#define TRIANGLE_NODE_V0_OFFSET 0
#define TRIANGLE_NODE_V1_OFFSET 12
#define TRIANGLE_NODE_V2_OFFSET 24
#define TRIANGLE_NODE_V3_OFFSET 36
#define TRIANGLE_NODE_GEOMETRY_INDEX_AND_FLAGS_OFFSET 48
#define TRIANGLE_NODE_PRIMITIVE_INDEX0_OFFSET 52
#define TRIANGLE_NODE_PRIMITIVE_INDEX1_OFFSET 56
#define TRIANGLE_NODE_ID_OFFSET 60
#define TRIANGLE_NODE_SIZE 64

#define RTIP3_TRIANGLE_NODE_PRIMITIVE_INDEX0_OFFSET 48
#define RTIP3_TRIANGLE_NODE_PRIMITIVE_INDEX1_OFFSET 52
#define RTIP3_TRIANGLE_NODE_GEOMETRY_INDEX_OFFSET 56

//=====================================================================================================================
// Triangle ID contains 4 1-byte fields, 1 per triangle:
// Triangle 0 [ 7: 0]
// Triangle 1 [15: 8]
// Triangle 2 [23:16]
// Triangle 3 [31:24]
//
// Each triangle's 8-bit segment contains these fields:
// I SRC        [1:0] Specifies which vertex in triangle 0 corresponds to the I barycentric value
// J SRC        [3:2] Specifies which vertex in triangle 0 corresponds to the J barycentric value
// Double Sided [  4] Specifies whether triangle 0 should be treated as double sided for culling
// Flip Winding [  5] Specifies whether triangle 0 should have its facedness flipped
// Procedural   [  6] Specifies whether it is a procedural node
// Opaque       [  7] Specifies whether triangle 0 should be considered as opaque
#define TRIANGLE_ID_BIT_STRIDE 8

#define TRIANGLE_ID_I_SRC_SHIFT 0
#define TRIANGLE_ID_J_SRC_SHIFT 2
#define TRIANGLE_ID_DOUBLE_SIDED_SHIFT 4
#define TRIANGLE_ID_FLIP_WINDING_SHIFT 5
#define TRIANGLE_ID_PROCEDURAL_SHIFT 6
#define TRIANGLE_ID_OPAQUE_SHIFT 7

//=====================================================================================================================
struct TriangleNode
{
    glm::vec3 v0;                     // Vertex 0
    glm::vec3 v1;                     // Vertex 1
    glm::vec3 v2;                     // Vertex 2
    glm::vec3 v3;                     // Vertex 3
    uint32_t  geometryIndexAndFlags;  // Geometry index and flags for pair of triangles
    uint32_t  primitiveIndex0;        // Primitive index for triangle 0
    uint32_t  primitiveIndex1;        // Primitive index for triangle 1
    uint32_t  triangleId;             // Triangle ID
};

//=====================================================================================================================
struct TriangleNode3_0
{
    glm::vec3 v0;               // Vertex 0
    glm::vec3 v1;               // Vertex 1
    glm::vec3 v2;               // Vertex 2
    glm::vec3 v3;               // Vertex 3
    uint32_t  primitiveIndex0;  // Primitive index for triangle 0
    uint32_t  primitiveIndex1;  // Primitive index for triangle 1
    uint32_t  geometryIndex;    // Geometry index for pair of triangles
    uint32_t  triangleId;       // Triangle ID
};

#ifdef __cplusplus
static_assert(TRIANGLE_NODE_SIZE == sizeof(TriangleNode), "TriangleNode structure mismatch");
static_assert(TRIANGLE_NODE_V0_OFFSET == offsetof(TriangleNode, v0), "");
static_assert(TRIANGLE_NODE_V1_OFFSET == offsetof(TriangleNode, v1), "");
static_assert(TRIANGLE_NODE_V2_OFFSET == offsetof(TriangleNode, v2), "");
static_assert(TRIANGLE_NODE_V3_OFFSET == offsetof(TriangleNode, v3), "");
static_assert(TRIANGLE_NODE_ID_OFFSET == offsetof(TriangleNode, triangleId), "");
#endif  // __cplusplus

//=====================================================================================================================
#define USER_NODE_PROCEDURAL_MIN_OFFSET 0
#define USER_NODE_PROCEDURAL_MAX_OFFSET 12
#define USER_NODE_PROCEDURAL_SIZE 64

//=====================================================================================================================
// Procedural node primitive data offsets
#define RTIP3_USER_NODE_PROCEDURAL_PRIMITIVE_INDEX_OFFSET RTIP3_TRIANGLE_NODE_PRIMITIVE_INDEX0_OFFSET
#define RTIP3_USER_NODE_PROCEDURAL_GEOMETRY_INDEX_OFFSET RTIP3_TRIANGLE_NODE_GEOMETRY_INDEX_OFFSET
#define USER_NODE_PROCEDURAL_PRIMITIVE_INDEX_OFFSET TRIANGLE_NODE_PRIMITIVE_INDEX1_OFFSET
#define USER_NODE_PROCEDURAL_GEOMETRY_INDEX_AND_FLAGS_OFFSET TRIANGLE_NODE_GEOMETRY_INDEX_AND_FLAGS_OFFSET
#define USER_NODE_PROCEDURAL_TRIANGLE_ID_OFFSET TRIANGLE_NODE_ID_OFFSET

//=====================================================================================================================
// User defined procedural node format
struct ProceduralNode
{
    glm::vec3 bbox_min;
    glm::vec3 bbox_max;
    uint32_t  padding1[6];
    uint32_t  geometryIndexAndFlags;
    uint32_t  reserved;
    uint32_t  primitiveIndex;
    uint32_t  triangleId;
};

#ifdef __cplusplus
static_assert(USER_NODE_PROCEDURAL_SIZE == sizeof(ProceduralNode), "ProceduralNode structure mismatch");
static_assert(USER_NODE_PROCEDURAL_MIN_OFFSET == offsetof(ProceduralNode, bbox_min), "");
static_assert(USER_NODE_PROCEDURAL_MAX_OFFSET == offsetof(ProceduralNode, bbox_max), "");
static_assert(USER_NODE_PROCEDURAL_GEOMETRY_INDEX_AND_FLAGS_OFFSET == offsetof(ProceduralNode, geometryIndexAndFlags), "");
static_assert(USER_NODE_PROCEDURAL_PRIMITIVE_INDEX_OFFSET == offsetof(ProceduralNode, primitiveIndex), "");
static_assert(USER_NODE_PROCEDURAL_TRIANGLE_ID_OFFSET == offsetof(ProceduralNode, triangleId), "");
#endif  // __cplusplus

//=====================================================================================================================
// User defined procedural node format
struct ProceduralNode3_0
{
    glm::vec3 bbox_min;
    glm::vec3 bbox_max;
    uint32_t  padding1[6];
    uint32_t  primitiveIndex;
    uint32_t  reserved;
    uint32_t  geometryIndex;
    uint32_t  triangleId;
};

#ifdef __cplusplus
//=====================================================================================================================
union NodePointer32
{
    struct
    {
        uint32_t type : 3;                 // Hardware NODE_TYPE_*
        uint32_t aligned_offset_64b : 29;  // 64-byte aligned offset
    };

    uint32_t u32;
};

//=====================================================================================================================
// Instance base pointer layout from the HW raytracing IP 2.0 spec:
// Zero                         [ 2: 0]
// Tree Base Address (64B index)[53: 3]
// Force Opaque                 [   54]
// Force Non-Opaque             [   55]
// Disable Triangle Cull        [   56]
// Flip Facedness               [   57]
// Cull Back Facing Triangles   [   58]
// Cull Front Facing Triangles  [   59]
// Cull Opaque                  [   60]
// Cull Non-Opaque              [   61]
// Skip Triangles               [   62]
// Skip Procedural              [   63]
union NodePointer64
{
    struct
    {
        uint64_t type : 3;               // Hardware NODE_TYPE_*
        uint64_t aligned_addr_64b : 51;  // 64-byte aligned address
        uint64_t force_opaque : 1;
        uint64_t force_non_opaque : 1;
        uint64_t disable_triangle_cull : 1;
        uint64_t flip_facedness : 1;
        uint64_t cull_back_face_triangle : 1;
        uint64_t cull_front_face_triangle : 1;
        uint64_t cull_opaque : 1;
        uint64_t cull_non_opaque : 1;
        uint64_t skip_triangles : 1;
        uint64_t skip_procedural : 1;
    };

    uint64_t u64;
};

//=====================================================================================================================
union HwTriangleFlags
{
    struct
    {
        uint8_t i : 2;
        uint8_t j : 2;
        uint8_t double_sided : 1;
        uint8_t flip_winding : 1;
        uint8_t unused : 1;
        uint8_t opaque : 1;
    };

    uint8_t u8;
};

//=====================================================================================================================
union HwTriangleID
{
    struct
    {
        HwTriangleFlags triangle0;
        HwTriangleFlags triangle1;
        uint16_t        unused;
    };

    uint32_t u32;
};

//=====================================================================================================================
union BoxNodeChildFlags
{
    struct
    {
        uint8_t only_opaque : 1;
        uint8_t only_non_opaque : 1;
        uint8_t only_triangles : 1;
        uint8_t only_procedural : 1;
        uint8_t unused : 4;
    };

    uint8_t u8All;
};

//=====================================================================================================================
union BoxNodeFlags
{
    struct
    {
        BoxNodeChildFlags child0;
        BoxNodeChildFlags child1;
        BoxNodeChildFlags child2;
        BoxNodeChildFlags child3;
    };

    uint32_t u32All;
};
#endif  // __cplusplus

//=====================================================================================================================
// Node pointer size in bytes
#define NODE_PTR_SIZE 4

#ifdef __cplusplus
static_assert(NODE_PTR_SIZE == sizeof(NodePointer32), "Node pointer size mismatch");
#endif  // __cplusplus

//=====================================================================================================================
// Function assumes the type passed in is a valid node type
//
static uint32_t PackNodePointer(uint32_t type, uint32_t address)
{
    uint32_t nodePointer = type;  // this assumes that the type is valid
    // uint32_t pointer = type & 0x7;

    // The input address is a byte offset, and node_addr is a 64-byte offset that starts at bit 3.
    nodePointer |= (address >> 3);  // this assumes that the input address is 64-byte aligned
    // pointer |= (address >> 6) << 3;

    return nodePointer;
}

//=====================================================================================================================
// Function assumes the type passed in is a valid node type
//
static uint32_t PackLeafNodePointer(uint32_t type, uint32_t address, uint32_t numPrimitives)
{
    return PackNodePointer(type, address) | ((numPrimitives - 1) << 29);
}

//=====================================================================================================================
static uint32_t GetNodeType(uint32_t nodePointer)
{
    // From the HW raytracing spec:
    // node_type = node_pointer[ 2:0]
    return nodePointer & 0x7;
}

//=====================================================================================================================
static uint32_t ClearNodeType(uint32_t nodePointer)
{
    return nodePointer & ~0x7;
}

//=====================================================================================================================
// TODO: The highest 3 bits are not handled since they currently aren't written when building the QBVH.
static uint32_t ExtractNodePointerOffset(uint32_t nodePointer)
{
    // From the HW raytracing spec:
    // node_addr[60:0] = node_pointer[63:3]
    // Also, based on the following, the node_addr is 64-byte aligned:
    // fetch_addr0 = T#.base_address*256+node_addr*64
    return ClearNodeType(nodePointer) << 3;
}

//=====================================================================================================================
// Remove the full 4 bit node type in 3.1
static uint32_t ClearNodeType3_1(uint32_t nodePointer)
{
    return nodePointer & ~0xf;
}

//=====================================================================================================================
static uint32_t ExtractNodePointerOffset3_1(uint32_t nodePointer)
{
    return ClearNodeType3_1(nodePointer) << 3;
}

//=====================================================================================================================
// Removes temp flag (MSB) within node type set by RefitBounds when fp16 nodes mode is LEAF_NODES_IN_BLAS_AS_FP16.
static uint32_t GetNodePointerExclMsbFlag(uint32_t nodePointer)
{
    return nodePointer & (~NODE_POINTER_MASK_MSB);
}

//=====================================================================================================================
// Primitive data structure that includes the unpacked data needed to process a primitive
struct PrimitiveData
{
    uint32_t primitiveIndex;  // Primitive index used to indicate what primitive in geometry description
    uint32_t geometryIndex;   // Geometry index used to indicate what geometry description
    uint32_t geometryFlags;   // Geometry flags contains if the geometry is opaque or non opaque
};

//=====================================================================================================================
// Extract the geometry index from the bottom 24 bits
static uint32_t ExtractGeometryIndex(uint32_t geometryIndexAndFlags)
{
    return geometryIndexAndFlags & 0xFFFFFF;
}

//=====================================================================================================================
// Extract the geometry flags from bits 25-26
static uint32_t ExtractGeometryFlags(uint32_t geometryIndexAndFlags)
{
    return (geometryIndexAndFlags >> 24) & 0x3;
}

//=====================================================================================================================
// Extract the geometry index from the bottom 24 bits and geometry flags from bits 25-26
static glm::uvec2 UnpackGeometryIndexAndFlags(uint32_t geometryIndexAndFlags)
{
    return glm::uvec2(ExtractGeometryIndex(geometryIndexAndFlags), ExtractGeometryFlags(geometryIndexAndFlags));
}

//=====================================================================================================================
// Pack the geometry index in the bottom 24 bits and the geometry flags into bits 25-26
static uint32_t PackGeometryIndexAndFlags(uint32_t geometryIndex, uint32_t geometryFlags)
{
    return (geometryFlags << 24) | (geometryIndex & 0xFFFFFF);
}

//=====================================================================================================================
// Additional geometry information for bottom level acceleration structures primitives
struct GeometryInfo
{
    uint32_t geometryFlagsAndNumPrimitives;
    uint32_t geometryBufferOffset;
    uint32_t primNodePtrsOffset;  // Offset from the base of all prim node ptrs to this geometry's prim node ptrs
};

#ifndef _WIN32
#define DXGI_FORMAT_UNKNOWN 0
#define DXGI_FORMAT_R32G32B32_FLOAT 6
#endif

#define DECODE_VERTEX_STRIDE 12
#define DECODE_PRIMITIVE_STRIDE_TRIANGLE 36
#define DECODE_PRIMITIVE_STRIDE_AABB 24
#define GEOMETRY_INFO_SIZE 12
#define GEOMETRY_INFO_FLAGS_AND_NUM_PRIMS_OFFSET 0
#define GEOMETRY_INFO_GEOM_BUFFER_OFFSET 4
#define GEOMETRY_INFO_PRIM_NODE_PTRS_OFFSET 8

#define PIPELINE_FLAG_SKIP_TRIANGLES 0x100
#define PIPELINE_FLAG_SKIP_PROCEDURAL_PRIMITIVES 0x200

#ifdef __cplusplus
static_assert(GEOMETRY_INFO_SIZE == sizeof(GeometryInfo), "Geometry info structure mismatch");
static_assert(GEOMETRY_INFO_FLAGS_AND_NUM_PRIMS_OFFSET == offsetof(GeometryInfo, geometryFlagsAndNumPrimitives), "");
static_assert(GEOMETRY_INFO_GEOM_BUFFER_OFFSET == offsetof(GeometryInfo, geometryBufferOffset), "");
static_assert(GEOMETRY_INFO_PRIM_NODE_PTRS_OFFSET == offsetof(GeometryInfo, primNodePtrsOffset), "");
#endif  // __cplusplus

//=====================================================================================================================
static uint32_t ExtractGeometryInfoFlags(uint32_t packedGeometryFlagsAndNumPrimitives)
{
    return (packedGeometryFlagsAndNumPrimitives >> 29);
}

//=====================================================================================================================
static uint32_t ExtractGeometryInfoNumPrimitives(uint32_t packedGeometryFlagsAndNumPrimitives)
{
    // ((1 << 29) - 1) = 0x1fffffff
    return (packedGeometryFlagsAndNumPrimitives & 0x1FFFFFFF);
}

//=====================================================================================================================
static uint32_t PackGeometryFlagsAndNumPrimitives(uint32_t geometryFlags, uint32_t numPrimitives)
{
    return (geometryFlags << 29) | numPrimitives;
}

//=====================================================================================================================
// 64-byte aligned BVH2 node structure
struct BVHNode
{
    glm::vec3 bbox_left_min_or_v0;  /// Left Node bounding box minimum bounds or vertex 0
    uint32_t  left;                 /// Left child node pointer  (Also, primitive ID for leaves, instance ID for instances)

    glm::vec3 bbox_left_max_or_v1;  /// Left Node bounding box maximum bounds or vertex 1
    uint32_t  right;                /// Right child node pointer (Also, geometry Index for leaves)

    glm::vec3 bbox_right_min_or_v2;  /// Right Node bounding box min bounds or vertex 2
    uint32_t  flags;                 /// Bottom: geometry flags OR Top: node[0] this is used to hold num instances

    glm::vec3 bbox_right_max;  /// Right node bounding box max bounds
    uint32_t  unused;          /// Unused
};

#define BVH_NODE_SIZE 64
#define BVH_NODE_LEFT_MIN_OFFSET 0
#define BVH_NODE_V0_OFFSET BVH_NODE_LEFT_MIN_OFFSET
#define BVH_NODE_LEFT_OFFSET 12
#define BVH_NODE_PRIMITIVE_ID_OFFSET BVH_NODE_LEFT_OFFSET
#define BVH_NODE_LEFT_MAX_OFFSET 16
#define BVH_NODE_V1_OFFSET BVH_NODE_LEFT_MAX_OFFSET
#define BVH_NODE_RIGHT_OFFSET 28
#define BVH_NODE_GEOMETRY_INDEX_OFFSET BVH_NODE_RIGHT_OFFSET
#define BVH_NODE_RIGHT_MIN_OFFSET 32
#define BVH_NODE_V2_OFFSET BVH_NODE_RIGHT_MIN_OFFSET
#define BVH_NODE_FLAGS_OFFSET 44
#define BVH_NODE_RIGHT_MAX_OFFSET 48

#ifdef __cplusplus
static_assert(BVH_NODE_SIZE == sizeof(BVHNode), "BVH2Node structure mismatch");
static_assert(BVH_NODE_LEFT_MIN_OFFSET == offsetof(BVHNode, bbox_left_min_or_v0), "");
static_assert(BVH_NODE_LEFT_OFFSET == offsetof(BVHNode, left), "");
static_assert(BVH_NODE_LEFT_MAX_OFFSET == offsetof(BVHNode, bbox_left_max_or_v1), "");
static_assert(BVH_NODE_RIGHT_OFFSET == offsetof(BVHNode, right), "");
static_assert(BVH_NODE_RIGHT_MIN_OFFSET == offsetof(BVHNode, bbox_right_min_or_v2), "");
static_assert(BVH_NODE_FLAGS_OFFSET == offsetof(BVHNode, flags), "");
static_assert(BVH_NODE_RIGHT_MAX_OFFSET == offsetof(BVHNode, bbox_right_max), "");
#endif  // __cplusplus

//=====================================================================================================================
struct InstanceSidebandData1_1
{
    uint32_t  instanceIndex;
    uint32_t  blasNodePointer;  // might not point to root
    uint32_t  blasMetadataSize;
    uint32_t  padding0;
    glm::vec4 Transform[3];  // Non-inverse (original D3D12_RAYTRACING_INSTANCE_DESC.Transform)
};

#define RTIP1_1_INSTANCE_SIDEBAND_INSTANCE_INDEX_OFFSET 0
#define RTIP1_1_INSTANCE_SIDEBAND_CHILD_POINTER_OFFSET 4
#define RTIP1_1_INSTANCE_SIDEBAND_CHILD_METADATA_SIZE_OFFSET 8
#define RTIP1_1_INSTANCE_SIDEBAND_OBJECT2WORLD_OFFSET 16
#define RTIP1_1_INSTANCE_SIDEBAND_SIZE 64

//=====================================================================================================================
// 64-byte aligned structure matching D3D12_RAYTRACING_INSTANCE_DESC
struct InstanceDesc
{
    glm::vec4 Transform[3];                                   // Inverse transform for traversal
    uint32_t  InstanceID_and_Mask;                            // 24-bit instance ID and 8-bit mask
    uint32_t  InstanceContributionToHitGroupIndex_and_Flags;  // 24-bit instance contribution and 8-bit flags
    uint32_t  accelStructureAddressLo;                        // Lower part of acceleration structure base address
    uint32_t  accelStructureAddressHiAndFlags;                // Upper part of acceleration structure base address and
                                                              // HW raytracing IP 2.0 flags
};

#define INSTANCE_DESC_SIZE 64
#define INSTANCE_DESC_WORLD_TO_OBJECT_XFORM_OFFSET 0
#define INSTANCE_DESC_ID_AND_MASK_OFFSET 48
#define INSTANCE_DESC_CONTRIBUTION_AND_FLAGS_OFFSET 52
#define INSTANCE_DESC_VA_LO_OFFSET 56
#define INSTANCE_DESC_VA_HI_OFFSET 60

#ifdef __cplusplus
static_assert(INSTANCE_DESC_SIZE == sizeof(InstanceDesc), "InstanceDesc structure mismatch");
static_assert(INSTANCE_DESC_ID_AND_MASK_OFFSET == offsetof(InstanceDesc, InstanceID_and_Mask), "");
static_assert(INSTANCE_DESC_CONTRIBUTION_AND_FLAGS_OFFSET == offsetof(InstanceDesc, InstanceContributionToHitGroupIndex_and_Flags), "");
static_assert(INSTANCE_DESC_VA_LO_OFFSET == offsetof(InstanceDesc, accelStructureAddressLo), "");
static_assert(INSTANCE_DESC_VA_HI_OFFSET == offsetof(InstanceDesc, accelStructureAddressHiAndFlags), "");
#endif  // __cplusplus

#ifdef __cplusplus
static_assert(RTIP1_1_INSTANCE_SIDEBAND_SIZE == sizeof(InstanceSidebandData1_1), "Instance sideband structure mismatch");
static_assert(RTIP1_1_INSTANCE_SIDEBAND_INSTANCE_INDEX_OFFSET == offsetof(InstanceSidebandData1_1, instanceIndex), "");
static_assert(RTIP1_1_INSTANCE_SIDEBAND_CHILD_POINTER_OFFSET == offsetof(InstanceSidebandData1_1, blasNodePointer), "");
static_assert(RTIP1_1_INSTANCE_SIDEBAND_OBJECT2WORLD_OFFSET == offsetof(InstanceSidebandData1_1, Transform[0]), "");
#endif  // __cplusplus

//=====================================================================================================================
struct FusedInstanceNode
{
    InstanceDesc            desc;
    InstanceSidebandData1_1 sideband;
    Float32BoxNode          blasRootNode;
};

//=====================================================================================================================
struct InstanceNode
{
    InstanceDesc            desc;
    InstanceSidebandData1_1 sideband;
};

#define INSTANCE_NODE_DESC_OFFSET 0
#define INSTANCE_NODE_EXTRA_OFFSET 64
#define INSTANCE_NODE_SIZE 128
#define FUSED_INSTANCE_NODE_ROOT_OFFSET INSTANCE_NODE_SIZE
#define FUSED_INSTANCE_NODE_SIZE 256

#ifdef __cplusplus
static_assert(INSTANCE_NODE_SIZE == sizeof(InstanceNode), "InstanceNode structure mismatch");
static_assert(INSTANCE_NODE_DESC_OFFSET == offsetof(InstanceNode, desc), "InstanceNode structure mismatch");
static_assert(INSTANCE_NODE_EXTRA_OFFSET == offsetof(InstanceNode, sideband), "InstanceNode structure mismatch");
#endif  // __cplusplus

//=====================================================================================================================
struct HighPrecisionBoxNode
{
    uint32_t dword[16];

#ifdef __cplusplus
    HighPrecisionBoxNode(uint32_t val)
    {
        memset(this, val, sizeof(HighPrecisionBoxNode));
    }
#endif
};

//=====================================================================================================================
static uint64_t PackUint64(uint32_t lowBits, uint32_t highBits)
{
    // Note glslang doesn't like uint64_t casts
    uint64_t addr = highBits;
    addr          = (addr << 32) | lowBits;
    return addr;
}

//=====================================================================================================================
static glm::uvec2 SplitUint64(uint64_t x)
{
    return glm::uvec2(x, (x >> 32));
}

//=====================================================================================================================
// Instance base pointer layout from the HW raytracing IP 2.0 spec:
// Zero                         [ 2: 0]
// Tree Base Address (64B index)[53: 3]
// Force Opaque                 [   54]
// Force Non-Opaque             [   55]
// Disable Triangle Cull        [   56]
// Flip Facedness               [   57]
// Cull Back Facing Triangles   [   58]
// Cull Front Facing Triangles  [   59]
// Cull Opaque                  [   60]
// Cull Non-Opaque              [   61]
// Skip Triangles               [   62]
// Skip Procedural              [   63]
//
// Since GPU VAs can only be 48 bits, only 42 bits of the Tree Base Address field are used:
// Used Address                 [44: 3]
// Unused Address               [53:45]
//
// Note glslang doesn't like 64-bit integer literals
#define INSTANCE_BASE_POINTER_ZERO_MASK PackUint64(0x7, 0x0)                        //                0x7ull
#define INSTANCE_BASE_POINTER_ADDRESS_USED_MASK PackUint64(0xFFFFFFF8, 0x1FFF)      //     0x1FFFFFFFFFF8ull
#define INSTANCE_BASE_POINTER_ADDRESS_UNUSED_MASK PackUint64(0x00000000, 0x3FE000)  //   0x3FE00000000000ull
#define INSTANCE_BASE_POINTER_ADDRESS_MASK PackUint64(0xFFFFFFF8, 0x3FFFFF)         //   0x3FFFFFFFFFFFF8ull
#define INSTANCE_BASE_POINTER_FLAGS_MASK PackUint64(0x00000000, 0xFFC00000)         // 0xFFC0000000000000ull

#define NODE_POINTER_FLAGS_SHIFT 54
#define NODE_POINTER_FORCE_OPAQUE_SHIFT 54
#define NODE_POINTER_FORCE_NON_OPAQUE_SHIFT 55
#define NODE_POINTER_DISABLE_TRIANGLE_CULL_SHIFT 56
#define NODE_POINTER_FLIP_FACEDNESS_SHIFT 57
#define NODE_POINTER_CULL_BACK_FACING_SHIFT 58
#define NODE_POINTER_CULL_FRONT_FACING_SHIFT 59
#define NODE_POINTER_CULL_OPAQUE_SHIFT 60
#define NODE_POINTER_CULL_NON_OPAQUE_SHIFT 61
#define NODE_POINTER_SKIP_TRIANGLES_SHIFT 62
#define NODE_POINTER_SKIP_PROCEDURAL_SHIFT 63

#define RAY_FLAG_VALID_MASK 0x3ffu
#define RAY_FLAG_EXCLUDE_MASK (RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER)
#define RAY_FLAG_OVERRIDE_MASK (RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_FORCE_NON_OPAQUE)  // 0x3
#define RAY_FLAG_PRESERVE_MASK (RAY_FLAG_VALID_MASK & (~RAY_FLAG_OVERRIDE_MASK))    // 0x3fc

#define POINTER_FLAGS_HIDWORD_SHIFT (NODE_POINTER_FORCE_OPAQUE_SHIFT - 32)             // 22
#define POINTER_FLAGS_VALID_MASK (RAY_FLAG_VALID_MASK << POINTER_FLAGS_HIDWORD_SHIFT)  // 0x3ff << 22
#define POINTER_FLAGS_EXCLUDED_MASK ~(POINTER_FLAGS_VALID_MASK)                        // 0xFFC00000

//=====================================================================================================================
struct StackPtrs
{
    uint32_t stackPtrSrcNodeId;  // source node index in linear memory
    uint32_t stackPtrNodeDest;   // node destination in linear memory. Counts in 64B chunks (FP16 box node size)
    uint32_t numLeafsDone;
    uint32_t primCompBatchCount;
};

#define STACK_PTRS_SRC_PTR_OFFSET 0
#define STACK_PTRS_DST_PTR_OFFSET 4
#define STACK_PTRS_NUM_LEAFS_DONE_OFFSET 8
#define STACK_PTRS_PRIM_COMP_BATCH_COUNT 12
#define STACK_PTR_SIZE 16

#ifdef __cplusplus
static_assert(STACK_PTR_SIZE == sizeof(StackPtrs), "StackPtrs structure mismatch");
static_assert(STACK_PTRS_SRC_PTR_OFFSET == offsetof(StackPtrs, stackPtrSrcNodeId), "");
static_assert(STACK_PTRS_DST_PTR_OFFSET == offsetof(StackPtrs, stackPtrNodeDest), "");
static_assert(STACK_PTRS_NUM_LEAFS_DONE_OFFSET == offsetof(StackPtrs, numLeafsDone), "");
static_assert(STACK_PTRS_PRIM_COMP_BATCH_COUNT == offsetof(StackPtrs, primCompBatchCount), "");
#endif  // __cplusplus

//=====================================================================================================================
// Build Stage Counters (Debug only)
// It starts with the qbvhGlobalCounters offset, i.e.,
// qbvhGlobalStack...qbvhGlobalStackPtrs...bvhBuildDebugCounters

#define COUNTER_MORTONGEN_OFFSET 0x0
#define COUNTER_MORTON_SORT_OFFSET 0x4
#define COUNTER_SORTLEAF_OFFSET 0x8
#define COUNTER_BUILDPLOC_OFFSET 0xC
#define COUNTER_BUILDLBVH_OFFSET 0x10
#define COUNTER_REFIT_OFFSET 0x14
#define COUNTER_INITQBVH_OFFSET 0x18
#define COUNTER_BUILDQBVH_OFFSET 0x1C
#define COUNTER_EMPTYPRIM_OFFSET 0x20
#define COUNTER_EMITCOMPACTSIZE_OFFSET 0x24
#define COUNTER_BUILDFASTLBVH_OFFSET 0x28

//=====================================================================================================================
// Get leaf triangle node size in bytes
static uint32_t GetBvhNodeSizeTriangle()
{
    return TRIANGLE_NODE_SIZE;
}

//=====================================================================================================================
// Get leaf AABB node size in bytes
static uint32_t GetBvhNodeSizeProcedural()
{
    return USER_NODE_PROCEDURAL_SIZE;
}

//=====================================================================================================================
// Get leaf instance node size in bytes
static uint32_t GetBvhNodeSizeInstance(uint32_t enableFusedInstanceNode)
{
    return (enableFusedInstanceNode == 0) ? INSTANCE_NODE_SIZE : FUSED_INSTANCE_NODE_SIZE;
}

//=====================================================================================================================
// Get internal BVH node size in bytes
static uint32_t GetBvhNodeSizeInternal()
{
    return FLOAT32_BOX_NODE_SIZE;
}

//=====================================================================================================================
// Get internal BVH node size in bytes
static uint32_t GetBvhNodeSizeLeaf(uint32_t primitiveType, uint32_t enableFusedInstanceNode)
{
    uint32_t sizeInBytes = 0;
    switch (primitiveType)
    {
    case PrimitiveType::Triangle:
        sizeInBytes = GetBvhNodeSizeTriangle();
        break;
    case PrimitiveType::AABB:
        sizeInBytes = GetBvhNodeSizeProcedural();
        break;
    case PrimitiveType::Instance:
        sizeInBytes = GetBvhNodeSizeInstance(enableFusedInstanceNode);
        break;
    }

    return sizeInBytes;
}

//=====================================================================================================================
static uint32_t CalcParentPtrOffset(uint32_t nodePtr)
{
    // Subtract 1 from the index to account for negative offset calculations. I.e. index 0 is actually at -4 byte
    // offset from the end of the parent pointer memory
    const uint32_t linkIndex = (nodePtr >> 3) - 1;
    return linkIndex * NODE_PTR_SIZE;
}

//=====================================================================================================================
#define QBVH_COLLAPSE_STACK_ENTRY_SIZE 24

//=====================================================================================================================
struct QBVHTaskCollapse
{
    uint32_t nodeIndex;
    uint32_t leafIndex;
    uint32_t numPrimitives;
    uint32_t lastNodeIndex;
    uint32_t parentOfCollapseNodeIndex;
    uint32_t nodeDestIndex;
};

#ifdef __cplusplus
static_assert((QBVH_COLLAPSE_STACK_ENTRY_SIZE == sizeof(QBVHTaskCollapse)), "QBVHTaskCollapse structure mismatch");
#endif  // __cplusplus

//=====================================================================================================================
static uint32_t CalcBottomGeometryInfoSize(uint32_t numGeometries)
{
    return numGeometries * GEOMETRY_INFO_SIZE;
}

//=====================================================================================================================
struct DataOffsetAndSize
{
    uint32_t offset;
    uint32_t size;
};

//=====================================================================================================================
struct StateTaskQueueCounter
{
    uint32_t phase;
    uint32_t startPhaseIndex;
    uint32_t endPhaseIndex;
    uint32_t taskCounter;
    uint32_t numTasksDone;
};

#define STATE_TASK_QUEUE_PHASE_OFFSET 0
#define STATE_TASK_QUEUE_START_PHASE_INDEX_OFFSET 4
#define STATE_TASK_QUEUE_END_PHASE_INDEX_OFFSET 8
#define STATE_TASK_QUEUE_TASK_COUNTER_OFFSET 12
#define STATE_TASK_QUEUE_NUM_TASKS_DONE_OFFSET 16

//=====================================================================================================================
#define REF_SCRATCH_SIDE_LEFT 0
#define REF_SCRATCH_SIDE_RIGHT 1
#define REF_SCRATCH_SIDE_LEAF 2

#define USE_BLAS_PRIM_COUNT 0

struct TDRefScratch
{
    uint32_t    primitiveIndex;
    uint32_t    nodeIndex;
    glm::vec3   center;
    BoundingBox box;
    uint32_t    side;
#if USE_BVH_REBRAID
    uint32_t nodePointer;  //rebraid only
#endif
#if USE_BLAS_PRIM_COUNT
    uint32_t numPrimitives;
#endif
};

#define TD_REF_PRIM_INDEX_OFFSET 0
#define TD_REF_NODE_INDEX_OFFSET 4
#define TD_REF_CENTER_OFFSET 8
#define TD_REF_BOX_OFFSET 20
#define TD_REF_SIDE_OFFSET (TD_REF_BOX_OFFSET + sizeof(BoundingBox))
#define TD_REF_NODE_POINTER_OFFSET (TD_REF_SIDE_OFFSET + 4)
#if USE_BLAS_PRIM_COUNT
#define TD_REF_NUM_PRIM_OFFSET (TD_REF_NODE_POINTER_OFFSET + sizeof(uint))
#endif

//=====================================================================================================================
#define NUM_SPLIT_BINS 4

#define TD_NODE_REBRAID_STATE_OPEN 0
#define TD_NODE_REBRAID_STATE_CLOSED 1

struct TDBins
{
    uint64_t firstRefIndex;

    UintBoundingBox binBoxes[3][NUM_SPLIT_BINS];
    uint32_t        binPrimCount[3][NUM_SPLIT_BINS];

    uint32_t bestAxis;
    uint32_t bestSplit;
    uint32_t numLeft;
    uint32_t numRight;

#if USE_BLAS_PRIM_COUNT
    uint32_t binBLASPrimCount[3][NUM_SPLIT_BINS];
#endif
};

#define TD_BINS_FIRST_REF_INDEX_OFFSET 0
#define TD_BINS_BIN_BOXES_OFFSET (TD_BINS_FIRST_REF_INDEX_OFFSET + 8)
#define TD_BINS_BIN_PRIM_COUNT_OFFSET (TD_BINS_BIN_BOXES_OFFSET + sizeof(UintBoundingBox) * NUM_SPLIT_BINS * 3)
#define TD_BINS_BEST_AXIS_OFFSET (TD_BINS_BIN_PRIM_COUNT_OFFSET + sizeof(uint) * NUM_SPLIT_BINS * 3)
#define TD_BINS_BEST_SPLIT_OFFSET (TD_BINS_BEST_AXIS_OFFSET + 4)
#define TD_BINS_NUM_LEFT_OFFSET (TD_BINS_BEST_SPLIT_OFFSET + 4)
#define TD_BINS_NUM_RIGHT_OFFSET (TD_BINS_NUM_LEFT_OFFSET + 4)
#if USE_BLAS_PRIM_COUNT
#define TD_BINS_BLAS_PRIM_COUNT_OFFSET (TD_BINS_NUM_RIGHT_OFFSET + 4)
#endif

struct TDNode
{
    UintBoundingBox centroidBox;
    uint32_t        binsIndex;
    uint32_t        childCount;

#if USE_BVH_REBRAID
    uint32_t largestAxis;   // rebraid only
    float    largestWidth;  // rebraid only
    uint32_t rebraidState;  // rebraid only
    uint32_t primIndex;     // rebraid only
#endif
};

#define TD_NODE_CENTROID_BOX_OFFSET 0
#define TD_NODE_BINS_INDEX_OFFSET (TD_NODE_CENTROID_BOX_OFFSET + sizeof(UintBoundingBox))
#define TD_NODE_CHILD_COUNT_OFFSET (TD_NODE_BINS_INDEX_OFFSET + 4)
#define TD_NODE_LARGEST_AXIS_OFFSET (TD_NODE_CHILD_COUNT_OFFSET + 4)
#define TD_NODE_LARGEST_WIDTH_OFFSET (TD_NODE_LARGEST_AXIS_OFFSET + 4)
#define TD_NODE_REBRAID_STATE_OFFSET (TD_NODE_LARGEST_WIDTH_OFFSET + 4)
#define TD_NODE_PRIM_INDEX_OFFSET (TD_NODE_REBRAID_STATE_OFFSET + 4)

//=====================================================================================================================

#define TD_REBRAID_STATE_NO_OPEN 0
#define TD_REBRAID_STATE_NEED_OPEN 1
#define TD_REBRAID_STATE_OOM 2

#define TD_PHASE_INIT_STATE 0
#define TD_PHASE_INIT_REFS_TO_LEAVES 1
#define TD_PHASE_CHECK_NEED_ALLOC 2
#define TD_PHASE_ALLOC_ROOT_NODE 3
#define TD_PHASE_REBRAID_COUNT_OPENINGS 4
#define TD_PHASE_REBRAID_CHECK_TERMINATION 5
#define TD_PHASE_REBRAID_OPEN 6
#define TD_PHASE_REBRAID_UPDATE_NODES 7
#define TD_PHASE_BIN_REFS 8
#define TD_PHASE_FIND_BEST_SPLIT 9
#define TD_PHASE_SECOND_PASS 10
#define TD_PHASE_UPDATE_NEW_NODES 11
#define TD_PHASE_DONE 12

struct StateTDBuild
{
    uint32_t        numNodes;
    uint32_t        numProcessedNodes;
    uint32_t        numNodesAllocated;
    uint32_t        numRefs;
    uint32_t        numRefsAllocated;
    uint32_t        numInactiveInstance;
    UintBoundingBox rootCentroidBBox;
    uint32_t        numLeaves;
    uint32_t        binsCounter;

#if USE_BVH_REBRAID
    uint32_t rebraidState;
    uint32_t leafAllocOffset;
#endif
};

#define STATE_TD_NUM_NODES_OFFSET 0
#define STATE_TD_NUM_PROCESSED_NODES_OFFSET 4
#define STATE_TD_NUM_NODES_ALLOCATED_OFFSET 8
#define STATE_TD_NUM_REFS_OFFSET 12
#define STATE_TD_NUM_REFS_ALLOCATED_OFFSET 16
#define STATE_TD_NUM_INACTIVE_INSTANCE_OFFSET 20
#define STATE_TD_CENTROID_BBOX_OFFSET 24
#define STATE_TD_NUM_LEAVES_OFFSET (STATE_TD_CENTROID_BBOX_OFFSET + sizeof(UintBoundingBox))
#define STATE_TD_BINS_COUNTER_OFFSET (STATE_TD_NUM_LEAVES_OFFSET + 4)
#define STATE_TD_REBRAID_STATE_OFFSET (STATE_TD_BINS_COUNTER_OFFSET + 4)
#define STATE_TD_LEAF_ALLOC_OFFSET_OFFSET (STATE_TD_REBRAID_STATE_OFFSET + 4)

#if GPURT_BUILD_EXPERIMENTAL
//=====================================================================================================================
struct LTDNode
{
    uint32_t startIndex;
    uint32_t numClusters;
    uint32_t parentScratchNode;
    float    centroidBoxLength;
};

//=====================================================================================================================
#define LTD_NUM_CLUSTER_LISTS 2  // double buffer

struct LTDState
{
    uint32_t numLTDNodes;
    uint32_t numLTDNodesAlloc;
    uint32_t numLTDNodesDone;

    uint32_t numInternalNodeIndicesAlloc;
    uint32_t numInternalNodeIndicesUsed;

    uint32_t numAxisBits;

    uint32_t axisMasks[8];

    uint32_t numDualClusterTDNodes;

    uint32_t numClusters[LTD_NUM_CLUSTER_LISTS];
    uint32_t numClustersDone[LTD_NUM_CLUSTER_LISTS];

    uint32_t clusterListIndex;  // the oldest list that is currently being read from
};

#define LTDSTATE_NUM_LTDNODES_OFFSET (0)
#define LTDSTATE_NUM_LTDNODES_ALLOC_OFFSET (4)
#define LTDSTATE_NUM_LTDNODES_DONE_OFFSET (8)
#define LTDSTATE_NUM_ITERNAL_NODE_INDICES_ALLOC_OFFSET (12)
#define LTDSTATE_NUM_ITERNAL_NODE_INDICES_USED_OFFSET (16)
#define LTDSTATE_NUM_AXIS_BITS_OFFSET (20)
#define LTDSTATE_AXIS_MASKS_OFFSET (24)
#define LTDSTATE_NUM_DUAL_CLUSTER_LTDNODES_OFFSET (LTDSTATE_AXIS_MASKS_OFFSET + (sizeof(uint) * 8))
#define LTDSTATE_NUM_CLUSTERS_OFFSET (LTDSTATE_NUM_DUAL_CLUSTER_LTDNODES_OFFSET + 4)
#define LTDSTATE_NUM_CLUSTERS_DONE_OFFSET (LTDSTATE_NUM_CLUSTERS_OFFSET + (sizeof(uint) * LTD_NUM_CLUSTER_LISTS))
#define LTDSTATE_CLUSTER_LIST_INDEX_OFFSET (LTDSTATE_NUM_CLUSTERS_DONE_OFFSET + (sizeof(uint) * LTD_NUM_CLUSTER_LISTS))
#endif

//=====================================================================================================================
struct Flags
{
    uint32_t dataValid;
    uint32_t prefixSum;
};

#define FLAGS_DATA_VALID_OFFSET 0
#define FLAGS_PREFIX_SUM_OFFSET 4

//=====================================================================================================================

#define PLOC_PHASE_INIT 0
#define PLOC_PHASE_FIND_NEAREST_NEIGHBOUR 1
#define PLOC_PHASE_UPDATE_CLUSTER_COUNT 2
#define PLOC_PHASE_DONE 3

struct StatePLOC
{
    uint32_t numClusters;
    uint32_t internalNodesIndex;
    uint32_t clusterListIndex;
    uint32_t numClustersAlloc;
};

#define STATE_PLOC_NUM_CLUSTERS_OFFSET 0
#define STATE_PLOC_INTERNAL_NODES_INDEX_OFFSET 4
#define STATE_PLOC_CLUSTER_LIST_INDEX_OFFSET 8
#define STATE_PLOC_NUM_CLUSTERS_ALLOC_OFFSET 12

//=====================================================================================================================
#define REBRAID_PHASE_INIT 0
#define REBRAID_PHASE_CALC_SUM 1
#define REBRAID_PHASE_OPEN 2
#define REBRAID_PHASE_DONE 3

struct RebraidState
{
    float    sumValue;
    uint32_t mutex;
};

#define STATE_REBRAID_SUM_VALUE_OFFSET 0
#define STATE_REBRAID_MUTEX_OFFSET STATE_REBRAID_SUM_VALUE_OFFSET + 4

//=====================================================================================================================
#define TS_PHASE_INIT 0
#define TS_PHASE_CALC_SUM 1
#define TS_PHASE_ALLOC_REFS 2
#define TS_PHASE_SPLIT 3
#define TS_PHASE_DONE 4

struct ScratchTSRef
{
    uint32_t leafIndex;
    uint32_t numSplits;

    uint32_t splitLeafBaseIndex;

    BoundingBox bbox;
};

struct ScratchTSState
{
    uint32_t refListIndex;
    uint32_t numRefs;
    uint32_t numRefsAlloc;
    float    sum;
    uint32_t mutex;
};

#define STATE_TS_REF_LIST_INDEX_OFFSET 0
#define STATE_TS_NUM_REFS_OFFSET STATE_TS_REF_LIST_INDEX_OFFSET + 4
#define STATE_TS_NUM_REFS_ALLOC_OFFSET STATE_TS_NUM_REFS_OFFSET + 4
#define STATE_TS_SUM_OFFSET STATE_TS_NUM_REFS_ALLOC_OFFSET + 4
#define STATE_TS_MUTEX_OFFSET STATE_TS_SUM_OFFSET + 4

//=====================================================================================================================
struct IndexBufferInfo
{
    uint32_t gpuVaLo;
    uint32_t gpuVaHi;
    uint32_t byteOffset;
    uint32_t format;
};

#define INDEX_BUFFER_INFO_GPU_VA_LO_OFFSET 0
#define INDEX_BUFFER_INFO_GPU_VA_HI_OFFSET 4
#define INDEX_BUFFER_INFO_BYTE_OFFSET_OFFSET 8
#define INDEX_BUFFER_INFO_FORMAT_OFFSET 12

//=====================================================================================================================
// Update scratch memory fields
#define UPDATE_SCRATCH_STACK_NUM_ENTRIES_OFFSET 0
#define UPDATE_SCRATCH_TASK_COUNT_OFFSET 4

//=====================================================================================================================
#ifdef AMD_VULKAN
//=====================================================================================================================
///@note Enum is a reserved keyword in glslang. To workaround this limitation, define static constants to replace the
///      HLSL enums that follow for compatibility.
//=====================================================================================================================
namespace RebraidType
{
    static const uint32_t Off = 0x0;
    static const uint32_t V1  = 0x1;
    static const uint32_t V2  = 0x2;
}  // namespace RebraidType
#else
enum RebraidType : uint32_t
{
    Off = 0,  // No Rebraid
    V1  = 1,  // First version of Rebraid
    V2  = 2,  // Second version of Rebraid
};
#endif

//typedef CompileTimeBuildSettings BuildSettingsData;

#define BUILD_MODE_LINEAR 0
// BUILD_MODE_AC was 1, but it has been removed.
#define BUILD_MODE_PLOC 2
#if GPURT_BUILD_EXPERIMENTAL
#define BUILD_MODE_LTD 3
#endif

#define SAH_COST_TRIANGLE_INTERSECTION 1.5
#define SAH_COST_PRIM_RANGE_INTERSECTION 1.3
#define SAH_COST_AABBB_INTERSECTION 1

#define ENCODE_FLAG_ARRAY_OF_POINTERS 0x00000001
#define ENCODE_FLAG_UPDATE_IN_PLACE 0x00000002
#define ENCODE_FLAG_REBRAID_ENABLED 0x00000004
#define ENCODE_FLAG_ENABLE_FUSED_INSTANCE_NODE 0x00000008

//=====================================================================================================================
struct IntersectionResult
{
    IntersectionResult(int val)
    {
        memset(this, val, sizeof(IntersectionResult));
    }
    float     t;  // Relative to tMin
    uint32_t  nodeIndex;
    glm::vec2 barycentrics;
    uint32_t  geometryIndex;
    uint32_t  primitiveIndex;
    uint32_t  instNodePtr;
    uint32_t  hitkind;
    uint32_t  instanceContribution;

#if DEVELOPER
    uint32_t numIterations;
    uint32_t maxStackDepth;
    uint32_t numRayBoxTest;
    uint32_t numCandidateHits;
    uint32_t numRayTriangleTest;
    uint32_t numAnyHitInvocation;
    uint32_t instanceIntersections;
#endif
};

//=====================================================================================================================
// Commit status
typedef uint32_t COMMITTED_STATUS;

#define COMMITTED_NOTHING 0
#define COMMITTED_TRIANGLE_HIT 1
#define COMMITTED_PROCEDURAL_PRIMITIVE_HIT 2

//=====================================================================================================================
// Candidate type
typedef uint32_t CANDIDATE_STATUS;

#define CANDIDATE_NON_OPAQUE_TRIANGLE 0
#define CANDIDATE_PROCEDURAL_PRIMITIVE 1
#define CANDIDATE_NON_OPAQUE_PROCEDURAL_PRIMITIVE 2
#define CANDIDATE_EARLY_RAY_TERMINATE 4

#define INIT_LDS_STATE 0xFFFFFFFF

//=====================================================================================================================
// Data required for system value intrinsics
struct RaySystemData
{
    uint32_t  currNodePtr;
    float     rayTCurrent;
    uint32_t  instNodePtr;
    uint32_t  instanceContribution;
    uint32_t  geometryIndex;
    uint32_t  primitiveIndex;
    glm::vec2 barycentrics;
    uint32_t  frontFace;
    glm::vec3 origin;
    glm::vec3 direction;
};

//=====================================================================================================================
#if DEFINE_RAYDESC
// Ray description matching the D3D12 HLSL header
struct RayDesc
{
    glm::vec3 Origin;
    float     TMin;
    glm::vec3 Direction;
    float     TMax;
};
#endif  // DEFINE_RAYDESC

//=====================================================================================================================
// Internal RayQuery structure initialised at TraceRaysInline()
/*
struct RayQueryInternal
{
    RayQueryInternal(int val) {
        memset(this, val, sizeof(RayQueryInternal));
    }

    // Internal query data holding address of current BVH and stack information.
    // Additional data that may be required will be stored here.
    uint32_t             bvhLo;
    uint32_t             bvhHi;
    uint32_t             topLevelBvhLo;
    uint32_t             topLevelBvhHi;
    uint32_t             stackPtr;
    uint32_t             stackPtrTop;
    uint32_t             stackNumEntries;
    uint32_t             instNodePtr;
    uint32_t             currNodePtr;
    uint32_t             instanceHitContributionAndFlags;
    uint32_t             prevNodePtr;
    uint32_t             isGoingDown;
    uint32_t             lastInstanceNode;

    RayDesc          rayDesc;
    float            rayTMin;
    uint32_t             rayFlags;
    uint32_t             instanceInclusionMask;

    // Candidate system data
    CANDIDATE_STATUS candidateType;
    RaySystemData    candidate;

    // Committed system data
    COMMITTED_STATUS committedStatus;
    RaySystemData    committed;

    uint32_t             currNodePtr2; // Second node pointer for dual traversal

    // Counter data
    // @note We don't wrap these in DEVELOPER because it would result in mismatch of RayQuery struct size
    //       on the driver side when we're not using counters.
    uint32_t             numRayBoxTest;
    uint32_t             numRayTriangleTest;
    uint32_t             numIterations;
    uint32_t             maxStackDepthAndDynamicId;
    uint32_t             clocks;
    uint32_t             numCandidateHits;
    uint32_t             instanceIntersections;
    uint32_t             rayQueryObjId;
};
*/

//=====================================================================================================================
struct HitGroupInfo
{
    glm::uvec2 closestHitId;
    glm::uvec2 anyHitId;
    glm::uvec2 intersectionId;
    uint32_t   tableIndex;
};

//=====================================================================================================================
struct TriangleData
{
    glm::vec3 v0;  ///< Vertex 0
    glm::vec3 v1;  ///< Vertex 1
    glm::vec3 v2;  ///< Vertex 2
};

//=====================================================================================================================
// Note: This function was copied from Common.hlsl in GPURT.
static bool IsQuantizedBVH8BoxNode(uint32_t pointer)
{
    return (GetNodeType(pointer) == NODE_TYPE_BOX_QUANTIZED_BVH8);
}

#ifdef _WIN32
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

#endif  // RRA_BACKEND_RAYTRACING_DEF_H_

