//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for RTA ray history counter.
//=============================================================================

#ifndef RRA_BACKEND_RAY_HISTORY_GPURT_COUNTER_H_
#define RRA_BACKEND_RAY_HISTORY_GPURT_COUNTER_H_

#ifdef __cplusplus
#include <cstdint>

namespace GpuRt
{
    typedef std ::uint16_t uint16;
    typedef std::uint32_t  uint32;
    typedef std ::uint64_t uint64;
    typedef std ::uint8_t  uint8;

    // Magic header identifier for ray tracing counters
    static constexpr std::uint32_t RtCounterInfoIdentifier = 0x46545452u;

    // Magic header identifier for general RT binary files
    static constexpr std::uint32_t RtBinaryHeaderIdentifier = 0x46425452u;
#else
#define uint32 uint
#define uint64 uint64_t
#endif  // __cplusplus

// ====================================================================================================================
#define TRACERAY_COUNTER_MODE_DISABLE 0
#define TRACERAY_COUNTER_MODE_RAYHISTORY_LIGHT 1
#define TRACERAY_COUNTER_MODE_RAYHISTORY_FULL 2
#define TRACERAY_COUNTER_MODE_TRAVERSAL 3
#define TRACERAY_COUNTER_MODE_CUSTOM 4
#define TRACERAY_COUNTER_MODE_DISPATCH 5

#ifdef __cplusplus
#pragma pack(push, 4)
#endif  // __cplusplus

    // ====================================================================================================================
    struct IndirectCounterMetadata
    {
        uint64 rayGenShaderId;         // Raygeneration shader identifier
        uint32 dispatchRayDimensionX;  // DispatchRayDimension X
        uint32 dispatchRayDimensionY;  // DispatchRayDimension Y
        uint32 dispatchRayDimensionZ;  // DispatchRayDimension Z
    };

    // =====================================================================================================================
    // Per-dispatch ray tracing counter data
    struct DispatchCounterData
    {
        uint32 numActiveRays;                   // Total number of rays that invoked traversal.
        uint32 numIterations;                   // Total number of iterations
        uint32 minIterations;                   // Minimum iterations amongst all active rays
        uint32 maxIterations;                   // Maximum iterations amongst all active rays
        uint32 activeLaneCountPerIteration;     // Active lane count within a wave per traversal iteration.
        uint32 numWaveIterations;               // Number of wave iterations.
        uint32 maxActiveLaneCountPerIteration;  // Maximum active lane count amongst unique node types for a wave iteration.
    };

    // ====================================================================================================================
    // Ray tracing counter information
    struct CounterInfo
    {
        uint32 dispatchRayDimensionX;      // DispatchRayDimension X
        uint32 dispatchRayDimensionY;      // DispatchRayDimension Y
        uint32 dispatchRayDimensionZ;      // DispatchRayDimension Z
        uint32 hitGroupShaderRecordCount;  // Hit-group shader record count
        uint32 missShaderRecordCount;      // Miss shader record count
        uint32 pipelineShaderCount;        // Pipeline per-shader count
        uint64 stateObjectHash;            // State object hash
        uint32 counterMode;                // Counter mode
        uint32 counterMask;                // Traversal counter mask
        uint32 counterStride;              // Ray tracing counter stride
        uint32 rayCounterDataSize;         // Per-ray counter data
        uint32 lostTokenBytes;             // Total lost token bytes
        uint32 counterRayIdRangeBegin;     // Partial rayID range begin
        uint32 counterRayIdRangeEnd;       // Partial rayID range end
        uint32 pipelineType;               // Pipeline type (native RT or RayQuery). RayTracing=0, Compute=1, Graphics=2
        struct
        {
            uint32 isIndirect : 1;  // Execute indirect
            uint32 reserved : 31;
        };
        uint32 padding;  // Pad to 64-bit alignment
    };
#ifdef __cplusplus
#pragma pack(pop)
#endif  // __cplusplus

// ====================================================================================================================
#define TCID_NUM_RAY_BOX_TEST 0              // Number of ray-box tests
#define TCID_NUM_RAY_TRIANGLE_TEST 1         // Number of ray-triangle tests
#define TCID_NUM_ITERATION 2                 // Traversal loop iteration count
#define TCID_MAX_TRAVERSAL_DEPTH 3           // Maximum traversal stack depth
#define TCID_NUM_ANYHIT_INVOCATION 4         // Number of anyhit shader invocations
#define TCID_SHADER_ID 5                     // Hit/Miss shader ID
#define TCID_SHADER_RECORD_INDEX 6           // Shader record index
#define TCID_TIMING_DATA 7                   // Realtime timer data
#define TCID_WAVE_ID 8                       // Wave ID
#define TCID_NUM_CANDIDATE_HITS 9            // Number of candidate leaf node hits
#define TCID_INSTANCE_INTERSECTIONS 10       // Instance nodes intersected
#define TCID_COUNT 11                        // Number of traversal counter IDs
#define TCID_STRIDE 4                        // Counter data stride
#define TCD_SIZE (TCID_STRIDE * TCID_COUNT)  // Counter size per-ray

#define TCID_SHADER_RECORD_INDEX_MISS 0x80000000
#define TCID_SHADER_RECORD_INDEX_DATA_MASK 0x7FFFFFFF

    // ====================================================================================================================
    // Per-ray traversal counter data
    struct TraversalCounter
    {
        uint32 data[TCID_COUNT];  // Counter data per-ray
    };

// ====================================================================================================================
// Ray ID
#define RAYID_CONTROL_MASK 0x80000000
#define RAYID_ID_MASK 0x3FFFFFFF

// ====================================================================================================================
// Ray history token type
#define RAY_HISTORY_TOKEN_TYPE_BEGIN 0
#define RAY_HISTORY_TOKEN_TYPE_TOP_LEVEL 1
#define RAY_HISTORY_TOKEN_TYPE_BOTTOM_LEVEL 2
#define RAY_HISTORY_TOKEN_TYPE_END 3
#define RAY_HISTORY_TOKEN_TYPE_FUNC_CALL 4
#define RAY_HISTORY_TOKEN_TYPE_GPU_TIME 5
#define RAY_HISTORY_TOKEN_TYPE_ANYHIT_STATUS 6
#define RAY_HISTORY_TOKEN_TYPE_FUNC_CALL_V2 7
#define RAY_HISTORY_TOKEN_TYPE_PROC_ISECT_STATUS 8
#define RAY_HISTORY_TOKEN_TYPE_END_V2 9
#define RAY_HISTORY_TOKEN_TYPE_BEGIN_V2 10
#define RAY_HISTORY_TOKEN_TYPE_RESERVED 0x8000

#define RAY_HISTORY_TOKEN_BEGIN_SIZE 16
#define RAY_HISTORY_TOKEN_TOP_LEVEL_SIZE 2
#define RAY_HISTORY_TOKEN_BOTTOM_LEVEL_SIZE 2
#define RAY_HISTORY_TOKEN_FUNC_CALL_SIZE 2
#define RAY_HISTORY_TOKEN_GPU_TIME_SIZE 2
#define RAY_HISTORY_TOKEN_END_SIZE 2
#define RAY_HISTORY_TOKEN_CONTROL_SIZE 2
#define RAY_HISTORY_TOKEN_FUNC_CALL_V2_SIZE 3
#define RAY_HISTORY_TOKEN_PROC_ISECT_DATA_SIZE 2
#define RAY_HISTORY_TOKEN_END_V2_SIZE 6
#define RAY_HISTORY_TOKEN_BEGIN_V2_SIZE 19

#define RAY_HISTORY_CONTROL_TOKEN_TYPE_MASK 0xFFFF
#define RAY_HISTORY_CONTROL_TOKEN_LENGTH_MASK 0xFF
#define RAY_HISTORY_CONTROL_TOKEN_DATA_MASK 0xFF
#define RAY_HISTORY_CONTROL_TOKEN_DATA_SHIFT 0
#define RAY_HISTORY_CONTROL_TOKEN_LENGTH_SHIFT 16
#define RAY_HISTORY_CONTROL_TOKEN_TYPE_SHIFT 24

// ====================================================================================================================
// Types of ray history DXR shader function calls
// NOTE: Type is stored within RayHistoryControlToken.data field, so value 0 is reserved
#define RAY_HISTORY_FUNC_CALL_TYPE_MISS 1
#define RAY_HISTORY_FUNC_CALL_TYPE_CLOSEST 2
#define RAY_HISTORY_FUNC_CALL_TYPE_ANY_HIT 3
#define RAY_HISTORY_FUNC_CALL_TYPE_INTERSECTION 4

#define RAY_TRACING_COUNTER_REQUEST_BYTE_OFFSET 0
#define RAY_TRACING_COUNTER_RAY_ID_BYTE_OFFSET 4
#define RAY_TRACING_COUNTER_RESERVED_BYTE_SIZE 8

#ifdef __cplusplus
    // ====================================================================================================================
    // Ray tracing binary file type enumeration
    enum class RayTracingBinaryFileType : uint32
    {
        Unknown = 0,
        RayHistory,
        TraversalCounter,
        BvhRaw,
        BvhDecoded,
    };

    // ====================================================================================================================
    // Flags indicating various traversal features used for ray history capture
    union RayHistoryTraversalFlags
    {
        struct
        {
            uint32 boxSortMode : 1;       /// Indicates box sort mode used by traversal
            uint32 usesNodePtrFlags : 1;  /// Indicates node pointer flag usage for culling
            uint32 reserved : 30;
        };

        uint32 u32All;
    };

    // ====================================================================================================================
    // Common header for ray tracing binary files
    struct RayTracingBinaryHeader
    {
        uint32                   identifier;  // Must be GpuRt::RayTracingFileIdentifier
        uint32                   version;     // Binary file format version
        RayTracingBinaryFileType fileType;    // Binary file type
        uint32                   ipLevel;     // Raytracing IP level (Pal::RayTracingIpLevel)
        uint32                   headerSize;  // Header size including RayTracingBinaryHeader
    };

    // ====================================================================================================================
    // Header for decoded acceleration structure binary files
    struct RayTracingBinaryHeaderDecoded : RayTracingBinaryHeader
    {
        uint32 accelStructType;  // GpuRt::AccelStructType
        uint32 buildFlags;       // API build flags
    };

    // ====================================================================================================================
    // Header for raw acceleration structure binary files
    struct RayTracingBinaryHeaderRaw : RayTracingBinaryHeader
    {
    };

    // ====================================================================================================================
    // Header for ray history binary files
    struct RayTracingBinaryHeaderRayHistory : RayTracingBinaryHeader
    {
        CounterInfo              counterInfo;
        RayHistoryTraversalFlags flags;
    };

    // ====================================================================================================================
    // Header for traversal counter binary files
    struct RayTracingBinaryHeaderTraversalCounter : RayTracingBinaryHeader
    {
        CounterInfo counterInfo;
    };

    // ====================================================================================================================
    // Ray identifier token
    struct RayID
    {
        uint32 id : 30;      // Unique identifier
        uint32 unused : 1;   // Reserved for future uses
        uint32 control : 1;  // Indicates that a control DWORD follows the RayID in token stream
    };

    // ====================================================================================================================
    // Ray history token type enumeration
    enum RayHistoryTokenType : uint16
    {
        RayHistoryTokenBegin,
        RayHistoryTokenTopLevel,
        RayHistoryTokenBottomLevel,
        RayHistoryTokenEnd,
        RayHistoryTokenFunctionCall,
        RayHistoryTokenGpuTimestamp,
        RayHistoryTokenAnyHitStatus,
        RayHistoryTokenFunctionCallV2,
        RayHistoryTokenTypeProcIsectStatus,

        // Anything with the top bit set is reserved
        RayHistoryTokenReserved = 0x8000,
        RayHistoryTokenUnknown  = 0xffff,
    };

    // ====================================================================================================================
    // Ray history token begin data format
    struct RayHistoryTokenBeginData
    {
        uint32 hwWaveId;              // Unique hardware wave identifier (SQ_WAVE_HW_ID_LEGACY)
        uint32 dispatchRaysIndex[3];  // Dispatch rays index in 3-dimensional ray grid

        uint32 accelStructAddrLo;  // API top-level acceleration structure base address (lower 32-bits)
        uint32 accelStructAddrHi;  // API top-level acceleration structure base address (upper 32-bits)
        uint32 rayFlags;           // API ray flags (see API documentation for further details)

        union
        {
            struct
            {
                uint32 instanceInclusionMask : 8;           // API instance inclusion mask (see API documentation for further details)
                uint32 rayContributionToHitGroupIndex : 4;  // API ray hit group offset (see API documentation for further details)
                uint32 geometryMultiplier : 4;              // API ray geometry stride (see API documentation for further details)
                uint32 missShaderIndex : 16;                // API ray miss shader index (see API documentation for further details)
            };

            uint32 packedTraceRayParams;  // Packed parameters
        };

        struct
        {
            float origin[3];     // Ray origin
            float tMin;          // Ray time minimum bounds
            float direction[3];  // Ray direction
            float tMax;          // Ray time maximum bounds
        } rayDesc;
    };

    // ====================================================================================================================
    // Ray history token end data format
    struct RayHistoryTokenEndData
    {
        uint32 primitiveIndex;  // Primitive index of the geometry hit by this ray (-1 for a miss)
        uint32 geometryIndex;   // Geometry index of the geometry hit by this ray (-1 for a miss)
    };

    // ====================================================================================================================
    // Ray history token procedural intersection data format
    struct RayHistoryTokenProcIsectData
    {
        float  hitT;     // Hit T as reported by the intersection shader
        uint32 hitKind;  // Hit kind as reported by the intersection shader (only lower 8-bits are valid)
    };

    // ====================================================================================================================
    // Ray history control token
    union RayHistoryControlToken
    {
        struct
        {
            RayHistoryTokenType type;         // Token type
            uint8               tokenLength;  // Length of tokens of this type, in DWORDS
            uint8               data;         // Additional data (optional)
        };

        uint32 u32;
    };
}  // namespace GpuRt
#endif  // __cplusplus
#endif  // RRA_BACKEND_RAY_HISTORY_GPURT_COUNTER_H_

