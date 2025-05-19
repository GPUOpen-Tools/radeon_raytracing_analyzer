//=============================================================================
//  Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 3.1 (Navi4x) specific primitive node definition.
//=============================================================================

#ifndef RRA_BACKEND_PRIMITIVE_NODE_H_
#define RRA_BACKEND_PRIMITIVE_NODE_H_

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4505)  // Disable unreferenced local function warning.
#pragma warning(disable : 4189)  // Disable unreferenced local variable warning.
#pragma warning(disable : 4100)  // Disable unreferenced formal parameter warning.
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <cstdint>

#include "glm/glm/glm.hpp"

#include "public/rra_assert.h"

#include "bvh/rtip_common/math.h"

//=====================================================================================================================
// RTIP3.x hardware instance transform node
struct HwInstanceTransformNode
{
    float    worldToObject[3][4];       // World-to-object transformation matrix (float4x3)
    uint64_t childBasePtr;              // Child acceleration structure base pointer
    uint32_t childRootNodeOrParentPtr;  // Child root node pointer (RTIP3.0) or parent pointer (RTIP3.1)
    uint32_t userDataAndInstanceMask;   // [0:23] Instance contribution to hit group index, [24:31] Instance mask
};

//=====================================================================================================================
// Instance sideband data for RTIP3.x instance nodes
struct InstanceSidebandData
{
    uint32_t instanceIndex;        // Auto generated instance index
    uint32_t instanceIdAndFlags;   // [0:23] Instance ID, [24:31] Instance Flags
    uint32_t blasMetadataSize;     // Child acceleration structure metadata size
    uint32_t padding0;             // Unused
    float    objectToWorld[3][4];  // Object-to-world transformation matrix (float4x3)
};

//=====================================================================================================================
struct ChildInfoRRA
{
    union
    {
        struct
        {
            uint32_t quantized_x_min : 12;
            uint32_t quantized_y_min : 12;
            uint32_t node_culling_flags : 4;
            uint32_t force_hit : 1;
            uint32_t unused : 3;
        };
        uint32_t culling_flags_and_force_hit;
    };

    union
    {
        struct
        {
            uint32_t quantized_z_min : 12;
            uint32_t quantized_x_max : 12;
            uint32_t instance_mask : 8;
        };
        uint32_t quantized_min_max_and_instance_mask;
    };

    union
    {
        struct
        {
            uint32_t quantized_y_max : 12;
            uint32_t quantized_z_max : 12;
            uint32_t node_type : 4;
            uint32_t node_range_length : 4;
        };
        uint32_t node_type_and_node_range_length;
    };
};

struct InstanceChildInfoRRA
{
    uint32_t min_x;
    uint32_t min_y;
    uint32_t min_z;
    union
    {
        struct
        {
            uint32_t exponent_x : 8;
            uint32_t exponent_y : 8;
            uint32_t exponent_z : 8;
            uint32_t disable_box_sort : 1;   // For just this node.
            uint32_t child_index : 3;        // Used by software, index of this child within the parent.
            uint32_t valid_child_count : 2;  // Add 1 to decode.
            uint32_t unused : 2;
        };
        uint32_t exponents_and_child_count;
    };
    ChildInfoRRA child_info_0;
    ChildInfoRRA child_info_1;
    ChildInfoRRA child_info_2;
    ChildInfoRRA child_info_3;
};

struct HwInstanceNodeRRA
{
    HwInstanceTransformNode data;
    InstanceChildInfoRRA    child_info;
};

struct InstanceNodeDataRRA
{
    HwInstanceNodeRRA    hw_instance_node;
    InstanceSidebandData sideband;
};

//=====================================================================================================================
// Primitive Structure triangle size: 128 Byte / 32 Dwords
#define PRIMITIVE_STRUCT_SIZE_IN_BYTE 128
#define PRIMITIVE_STRUCT_SIZE_IN_DW 32
#define TRIANGLE_PAIR_COUNT_OFFSET 28
#define TRI_PAIR_DESC_SIZE 29

#define PRIM_RANGE_UPDATE 0xfffffff8
#define END_OF_PAIR 0   // Increment node type
#define END_OF_NODE 1   // Increment node offset and clear node type
#define END_OF_RANGE 3  // Return invalid node

#define TRI_VERTEX_IDX_NUM_BITS 4
#define OPAQUE_NUM_BITS 1
#define DOUBLE_SIDED_NUM_BITS 1
#define PRIM_RANGE_STOP_NUM_BITS 1

#define TRI_DESC_PRIM_RANGE_STOP_BIT_SHIFT 0
#define TRI_DESC_TRI1_DOUBLE_SIDED_SHIFT 1
#define TRI_DESC_TRI1_OPAQUE_SHIFT 2
#define TRI_DESC_TRI1_V0_IDX_SHIFT 3
#define TRI_DESC_TRI1_V1_IDX_SHIFT 7
#define TRI_DESC_TRI1_V2_IDX_SHIFT 11
#define TRI_DESC_TRI0_DOUBLE_SIDED_SHIFT 15
#define TRI_DESC_TRI0_OPAQUE_SHIFT 16
#define TRI_DESC_TRI0_V0_IDX_SHIFT 17
#define TRI_DESC_TRI0_V1_IDX_SHIFT 21
#define TRI_DESC_TRI0_V2_IDX_SHIFT 25

//=====================================================================================================================
static void PackedTriDescSetTriangle(uint32_t triangleIdx, uint32_t i0, uint32_t i1, uint32_t i2, uint32_t opaque, uint32_t& triDesc)
{
    if (triangleIdx == 0)
    {
        triDesc = bitFieldInsert(triDesc, TRI_DESC_TRI0_V0_IDX_SHIFT, TRI_VERTEX_IDX_NUM_BITS, i0);
        triDesc = bitFieldInsert(triDesc, TRI_DESC_TRI0_V1_IDX_SHIFT, TRI_VERTEX_IDX_NUM_BITS, i1);
        triDesc = bitFieldInsert(triDesc, TRI_DESC_TRI0_V2_IDX_SHIFT, TRI_VERTEX_IDX_NUM_BITS, i2);
        triDesc = bitFieldInsert(triDesc, TRI_DESC_TRI0_OPAQUE_SHIFT, OPAQUE_NUM_BITS, opaque);
    }
    else
    {
        triDesc = bitFieldInsert(triDesc, TRI_DESC_TRI1_V0_IDX_SHIFT, TRI_VERTEX_IDX_NUM_BITS, i0);
        triDesc = bitFieldInsert(triDesc, TRI_DESC_TRI1_V1_IDX_SHIFT, TRI_VERTEX_IDX_NUM_BITS, i1);
        triDesc = bitFieldInsert(triDesc, TRI_DESC_TRI1_V2_IDX_SHIFT, TRI_VERTEX_IDX_NUM_BITS, i2);
        triDesc = bitFieldInsert(triDesc, TRI_DESC_TRI1_OPAQUE_SHIFT, OPAQUE_NUM_BITS, opaque);
    }
}

//=====================================================================================================================
static void PackedTriDescSetInvalidTriangle(uint32_t triangleIdx, uint32_t& triDesc)
{
    PackedTriDescSetTriangle(triangleIdx, 0, 0, 0, 0, triDesc);
}

//=====================================================================================================================
static uint32_t ComputeIndexSectionMidpoint(uint32_t triPairCount, uint32_t primIdPayloadLength, uint32_t primIdAnchorLength)
{
    return 1024u - triPairCount * (TRI_PAIR_DESC_SIZE + 2 * primIdPayloadLength) - primIdAnchorLength;
}

//=====================================================================================================================
static uint32_t PackMetadataHeaderBitsLo(uint32_t payloadXLengthMinusOne,
                                         uint32_t payloadYLengthMinusOne,
                                         uint32_t payloadZLengthMinusOne,
                                         uint32_t trailingZeroLength,
                                         uint32_t geoIdAnchorSizeDivTwo,
                                         uint32_t geoIdPayloadSizeDivTwo,
                                         uint32_t triPairCountMinusOne,
                                         uint32_t floatOrUnorm16)
{
    return (((payloadXLengthMinusOne & 0x1F) | ((payloadYLengthMinusOne & 0x1F) << 5) | ((payloadZLengthMinusOne & 0x1F) << 10) |
             ((trailingZeroLength & 0x1F) << 15) | ((geoIdAnchorSizeDivTwo & 0xF) << 20) | ((geoIdPayloadSizeDivTwo & 0xF) << 24) |
             ((triPairCountMinusOne & 0x7) << 28) | ((floatOrUnorm16 & 0x1) << 31)));
}

//=====================================================================================================================
static uint32_t PackMetadataHeaderBitsHi(uint32_t primIdAnchorSize, uint32_t primIdPayloadSize, uint32_t indexSectionMidpoint)
{
    return ((primIdAnchorSize & 0x1F) | ((primIdPayloadSize & 0x1F) << 5) | ((indexSectionMidpoint & 0x3FF) << 10));
}

//=====================================================================================================================
// Packed 32-bit hardware triangle pair descriptor
struct HwTrianglePairDesc
{
    uint32_t triDesc;  // 32-bit packed triangle pair descriptor

    void Init()
    {
        triDesc = 0;
    }

    uint32_t GetData()
    {
        return triDesc;
    }

    void SetData(uint32_t packedData)
    {
        triDesc = packedData;
    }

    uint32_t Tri1V0()
    {
        return bitFieldExtract(triDesc, TRI_DESC_TRI1_V0_IDX_SHIFT, TRI_VERTEX_IDX_NUM_BITS);
    }

    uint32_t Tri1V1()
    {
        return bitFieldExtract(triDesc, TRI_DESC_TRI1_V1_IDX_SHIFT, TRI_VERTEX_IDX_NUM_BITS);
    }

    uint32_t Tri1V2()
    {
        return bitFieldExtract(triDesc, TRI_DESC_TRI1_V2_IDX_SHIFT, TRI_VERTEX_IDX_NUM_BITS);
    }

    bool Tri1Opaque()
    {
        return bitFieldExtract(triDesc, TRI_DESC_TRI1_OPAQUE_SHIFT, 1);
    }

    bool Tri1DoubleSided()
    {
        return bitFieldExtract(triDesc, TRI_DESC_TRI1_DOUBLE_SIDED_SHIFT, 1);
    }

    bool Tri1Valid()
    {
        return Tri1V0() != 0 || Tri1V1() != 0 || Tri1V2() != 0;
    }

    uint32_t Tri0V0()
    {
        return bitFieldExtract(triDesc, TRI_DESC_TRI0_V0_IDX_SHIFT, TRI_VERTEX_IDX_NUM_BITS);
    }

    uint32_t Tri0V1()
    {
        return bitFieldExtract(triDesc, TRI_DESC_TRI0_V1_IDX_SHIFT, TRI_VERTEX_IDX_NUM_BITS);
    }

    uint32_t Tri0V2()
    {
        return bitFieldExtract(triDesc, TRI_DESC_TRI0_V2_IDX_SHIFT, TRI_VERTEX_IDX_NUM_BITS);
    }

    bool Tri0Opaque()
    {
        return bitFieldExtract(triDesc, TRI_DESC_TRI0_OPAQUE_SHIFT, OPAQUE_NUM_BITS);
    }

    bool Tri0DoubleSided()
    {
        return bitFieldExtract(triDesc, TRI_DESC_TRI0_DOUBLE_SIDED_SHIFT, DOUBLE_SIDED_NUM_BITS);
    }

    uint32_t PrimRangeStopBit()
    {
        return bitFieldExtract(triDesc, TRI_DESC_PRIM_RANGE_STOP_BIT_SHIFT, PRIM_RANGE_STOP_NUM_BITS);
    }

    void SetPrimRangeStopBit(uint32_t value)
    {
        triDesc = bitFieldInsert(triDesc, TRI_DESC_PRIM_RANGE_STOP_BIT_SHIFT, PRIM_RANGE_STOP_NUM_BITS, value);
    }

    void SetTri0(uint32_t i0, uint32_t i1, uint32_t i2, bool opaque, uint32_t primId, uint32_t geoId)
    {
        PackedTriDescSetTriangle(0, i0, i1, i2, opaque, triDesc);
    }

    void SetTri1(uint32_t i0, uint32_t i1, uint32_t i2, bool opaque, uint32_t primId, uint32_t geoId)
    {
        PackedTriDescSetTriangle(1, i0, i1, i2, opaque, triDesc);
    }
};

//=====================================================================================================================
// Unpacked triangle pair descriptor
struct TrianglePairDesc : HwTrianglePairDesc
{
    uint32_t tri0PrimId;
    uint32_t tri0GeoId;
    uint32_t tri1PrimId;
    uint32_t tri1GeoId;

    void InitTri0(glm::uvec3 index, bool opaque, uint32_t primId, uint32_t geoId)
    {
        SetTri0(index[0], index[1], index[2], opaque, primId, geoId);

        tri0PrimId = primId;
        tri0GeoId  = geoId;
    }

    void InitTri1(glm::uvec3 index, bool opaque, uint32_t primId, uint32_t geoId)
    {
        SetTri1(index[0], index[1], index[2], opaque, primId, geoId);

        tri1PrimId = primId;
        tri1GeoId  = geoId;
    }

    void InvalidateTri1()
    {
        SetTri1(0, 0, 0, false, 0, 0);
    }
};

//=====================================================================================================================
// TODO - combine TryInitTriStructHeader & Encode
//        Simplify "UnpackedPrimStructHeader" to reduce from 16DW
struct UnpackedPrimStructHeader
{
    float    prefixSourceX;
    float    prefixSourceY;
    float    prefixSourceZ;
    uint32_t anchorPrimId;
    uint32_t anchorGeoId;
    uint32_t indexSectionMidpoint;
    uint32_t primIdAnchorSize;
    uint32_t primIdPayloadSize;
    uint32_t geoIdAnchorSizeDivTwo;
    uint32_t geoIdPayloadSizeDivTwo;
    uint32_t payloadXLengthMinusOne;
    uint32_t payloadYLengthMinusOne;
    uint32_t payloadZLengthMinusOne;
    uint32_t trailingZeroLength;
    uint32_t triPairCountMinusOne;
    bool     floatOrUnorm16;
};

//=====================================================================================================================
#define TRI_STRUCT_HEADER_SIZE_BITS 52
#define TRI_STRUCT_DATA_SECTION_SIZE_BITS (1024 - TRI_STRUCT_HEADER_SIZE_BITS)

#define PAYLOAD_NUM_BITS 5
#define TRIALING_ZERO_NUM_BITS 5
#define GEO_ID_ANCHOR_NUM_BITS 4
#define GEO_ID_PAYLOAD_NUM_BITS 4
#define TRIPAIR_COUNT_NUM_BITS 3
#define VERTEX_TYPE_NUM_BITS 1
#define PRIM_ID_ANCHOR_NUM_BITS 5
#define PRIM_ID_PAYLOAD_NUM_BITS 5
#define INDEX_SECTION_MID_POINT_NUM_BITS 10

#define PAYLOAD_X_BIT_SHIFT_IN_DW0 0
#define PAYLOAD_Y_BIT_SHIFT_IN_DW0 5
#define PAYLOAD_Z_BIT_SHIFT_IN_DW0 10
#define TRAILING_ZERO_BIT_SHIFT_IN_DW0 15
#define GEO_ID_ANCHOR_SIZE_BIT_SHIFT_IN_DW0 20
#define GEO_ID_PAYLOAD_SIZE_BIT_SHIFT_IN_DW0 24
#define TRIANGLE_PAIR_COUNT_BIT_SHIFT_IN_DW0 28
#define VERTEX_TYPE_BIT_SHIFT_IN_DW0 31

#define PRIM_ID_ANCOR_SIZE_BIT_SHIFT_IN_DW1 0
#define PRIM_ID_PAYLOAD_SIZE_BIT_SHIFT_IN_DW1 5
#define INDEX_SECTION_MIDPOINT_SHIFT_IN_DW1 10

//=====================================================================================================================
// 128-byte primitive structure
struct PrimitiveStructure
{
    // PrimitiveStructure static header data section layout:
    //    uint32_t payloadXLength;        // 5 bit
    //    uint32_t payloadYLength;        // 5 bit
    //    uint32_t payloadZLength;        // 5 bit
    //    uint32_t trailingZeroLength;    // 5 bit
    //    uint32_t geoIndexAnchorSize;    // 4 bit
    //    uint32_t geoIndexPayloadSize;   // 4 bit
    //    uint32_t trianglePairCount;     // 3 bit
    //    uint32_t vertexType;            // 1 bit
    //    uint32_t primIndexAnchorSize;   // 5 bit
    //    uint32_t primIndexPayloadSize;  // 5 bit
    //    uint32_t indexSectionMidPoint;  // 10 bit

    uint32_t primData[PRIMITIVE_STRUCT_SIZE_IN_DW];

    void Init()
    {
        uint32_t i = 0;
        for (i = 0; i < PRIMITIVE_STRUCT_SIZE_IN_DW; i++)
        {
            primData[i] = 0;
        }
    }

    void Init(uint32_t d[PRIMITIVE_STRUCT_SIZE_IN_DW])
    {
        uint32_t i = 0;
        for (i = 0; i < PRIMITIVE_STRUCT_SIZE_IN_DW; i++)
        {
            primData[i] = d[i];
        }
    }

    uint32_t PayloadXLength()
    {
        return 1 + bitFieldExtract(primData[0], PAYLOAD_X_BIT_SHIFT_IN_DW0, PAYLOAD_NUM_BITS);
    }

    uint32_t PayloadYLength()
    {
        return 1 + bitFieldExtract(primData[0], PAYLOAD_Y_BIT_SHIFT_IN_DW0, PAYLOAD_NUM_BITS);
    }

    uint32_t PayloadZLength()
    {
        return 1 + bitFieldExtract(primData[0], PAYLOAD_Z_BIT_SHIFT_IN_DW0, PAYLOAD_NUM_BITS);
    }

    uint32_t TrailingZeroLength()
    {
        return bitFieldExtract(primData[0], TRAILING_ZERO_BIT_SHIFT_IN_DW0, TRIALING_ZERO_NUM_BITS);
    }

    uint32_t GeoIdAnchorSize()
    {
        return 2 * bitFieldExtract(primData[0], GEO_ID_ANCHOR_SIZE_BIT_SHIFT_IN_DW0, GEO_ID_ANCHOR_NUM_BITS);
    }

    uint32_t GeoIdPayloadSize()
    {
        return 2 * bitFieldExtract(primData[0], GEO_ID_PAYLOAD_SIZE_BIT_SHIFT_IN_DW0, GEO_ID_PAYLOAD_NUM_BITS);
    }

    uint32_t TrianglePairCount()
    {
        return 1 + bitFieldExtract(primData[0], TRIANGLE_PAIR_COUNT_BIT_SHIFT_IN_DW0, TRIPAIR_COUNT_NUM_BITS);
    }

    uint32_t VertexType()
    {
        return bitFieldExtract(primData[0], VERTEX_TYPE_BIT_SHIFT_IN_DW0, VERTEX_TYPE_NUM_BITS);
    }

    uint32_t PrimIdAnchorSize()
    {
        return bitFieldExtract(primData[1], PRIM_ID_ANCOR_SIZE_BIT_SHIFT_IN_DW1, PRIM_ID_ANCHOR_NUM_BITS);
    }

    uint32_t PrimIdPayloadSize()
    {
        return bitFieldExtract(primData[1], PRIM_ID_PAYLOAD_SIZE_BIT_SHIFT_IN_DW1, PRIM_ID_PAYLOAD_NUM_BITS);
    }

    uint32_t IndexSectionMidpoint()
    {
        return bitFieldExtract(primData[1], INDEX_SECTION_MIDPOINT_SHIFT_IN_DW1, INDEX_SECTION_MID_POINT_NUM_BITS);
    }

    uint32_t PrefixXLength()
    {
        return 32 - TrailingZeroLength() - PayloadXLength();
    }

    uint32_t PrefixYLength()
    {
        return 32 - TrailingZeroLength() - PayloadYLength();
    }

    uint32_t PrefixZLength()
    {
        return 32 - TrailingZeroLength() - PayloadZLength();
    }

    uint32_t VertexPayloadLength()
    {
        return PayloadXLength() + PayloadYLength() + PayloadZLength();
    }

    uint32_t VertexPrefixesLength()
    {
        return PrefixXLength() + PrefixYLength() + PrefixZLength();
    }

    uint32_t TriangleCount()
    {
        uint32_t triCount = 0;
        uint32_t i        = 0;
        for (i = 0; i < TrianglePairCount(); i++)
        {
            TrianglePairDesc desc = ReadTrianglePairDesc(i);
            triCount += 1 + desc.Tri1Valid();
        }
        return triCount;
    }

    // Write PackedBits (up to 32 bits) from metadata
    void WritePackedBits32(uint32_t startBitOffset,  // offset of the bit from the beginning of the node
                           uint32_t length,          // length of the bits to write
                           uint32_t value)           // value to store in memory
    {
        uint32_t startDwordIdx = startBitOffset / 32;

        uint32_t data32      = primData[startDwordIdx];
        uint32_t dwordOffset = startBitOffset & 31;
        data32               = bitFieldInsert(data32, dwordOffset, length, value);

        primData[startDwordIdx] = data32;
    }

    // Clear bits first, then update with the new data
    void WritePackedBits64(uint32_t startBitOffset,  // offset of the bit from the beginning of the node
                           uint32_t length,          // length of the bits to write
                           uint64_t value)           // value to store in memory
    {
        if (length > 0)
        {
            uint32_t startDwordIdx = startBitOffset / 32;
            if (startDwordIdx == 0x1f)
            {
                // only can read in one DW from primData, switch to 32bit mode
                uint32_t value32 = uint32_t(value & 0xFFFFFFFF);
                WritePackedBits32(startBitOffset, length, value32);
            }
            else
            {
                uint64_t dataLo = primData[startDwordIdx];
                uint64_t dataHi = primData[startDwordIdx + 1];
                uint64_t data64 = (dataHi << 32) | dataLo;

                uint32_t dwordOffset = startBitOffset & 31;
                data64               = bitFieldInsert64(data64, dwordOffset, length, value);

                // Update data packed in DW#
                uint32_t data64Lo           = uint32_t(data64 & 0xFFFFFFFF);
                uint32_t data64Hi           = uint32_t((data64 & 0xFFFFFFFF00000000) >> 32);
                primData[startDwordIdx]     = data64Lo;
                primData[startDwordIdx + 1] = data64Hi;
            }
        }
    }

    uint32_t PackPrimitiveStructureHeaderLo(UnpackedPrimStructHeader header)
    {
        uint32_t dataLo = PackMetadataHeaderBitsLo(header.payloadXLengthMinusOne,
                                                   header.payloadYLengthMinusOne,
                                                   header.payloadZLengthMinusOne,
                                                   header.trailingZeroLength,
                                                   header.geoIdAnchorSizeDivTwo,
                                                   header.geoIdPayloadSizeDivTwo,
                                                   header.triPairCountMinusOne,
                                                   header.floatOrUnorm16);
        return dataLo;
    }

    // Pack the High 32bit of PrimitiveStructure Header
    uint32_t PackPrimitiveStructureHeaderHi(UnpackedPrimStructHeader header)
    {
        uint32_t dataHi = PackMetadataHeaderBitsHi(header.primIdAnchorSize, header.primIdPayloadSize, header.indexSectionMidpoint);
        return dataHi;
    }

    void WriteVertexPrefixes(glm::vec3 prefixes)
    {
        const uint32_t trailingZeroLength = TrailingZeroLength();

        const uint32_t xLength = PayloadXLength();
        const uint32_t yLength = PayloadYLength();
        const uint32_t zLength = PayloadZLength();

        const uint32_t prefixXLength = PrefixXLength();
        const uint32_t prefixYLength = PrefixYLength();
        const uint32_t prefixZLength = PrefixZLength();

        const uint32_t x = (asuint(prefixes.x) >> (32 - prefixXLength));
        const uint32_t y = (asuint(prefixes.y) >> (32 - prefixYLength));
        const uint32_t z = (asuint(prefixes.z) >> (32 - prefixZLength));

        WritePackedBits64(TRI_STRUCT_HEADER_SIZE_BITS, prefixXLength, uint64_t(x));
        WritePackedBits64(TRI_STRUCT_HEADER_SIZE_BITS + prefixXLength, prefixYLength, uint64_t(y));
        WritePackedBits64(TRI_STRUCT_HEADER_SIZE_BITS + prefixXLength + prefixYLength, prefixZLength, uint64_t(z));
    }

    void WritePrimIdAnchor(uint32_t anchor)
    {
        const uint32_t primIdAnchorSize = PrimIdAnchorSize();
        const uint64_t mask             = (1ull << primIdAnchorSize) - 1ull;
        const uint32_t midPoint         = IndexSectionMidpoint();
        const uint64_t anchor64         = uint64_t(anchor);

        WritePackedBits64(midPoint, primIdAnchorSize, (anchor64 & mask));
    }

    void WriteGeoIdAnchor(uint32_t anchor)
    {
        const uint32_t geoIdAnchorSize = GeoIdAnchorSize();
        const uint64_t mask            = (1ull << geoIdAnchorSize) - 1ull;
        const uint32_t midPoint        = IndexSectionMidpoint();
        const uint64_t anchor64        = uint64_t(anchor);

        WritePackedBits64((midPoint - geoIdAnchorSize), geoIdAnchorSize, (anchor64 & mask));
    }

    void WriteGeoIdPayload(uint32_t triIndex, uint32_t geoId)
    {
        if (triIndex > 0)
        {
            uint32_t geoIdAnchorSize  = GeoIdAnchorSize();
            uint32_t geoIdPayloadSize = GeoIdPayloadSize();

            const uint32_t startOffset = IndexSectionMidpoint() - geoIdAnchorSize - (triIndex)*geoIdPayloadSize;

            uint64_t geoId64 = uint64_t(geoId);
            if (geoIdPayloadSize < geoIdAnchorSize)
            {
                geoId64 = geoId64 & ((1ull << geoIdPayloadSize) - 1ull);
            }

            WritePackedBits64(startOffset, geoIdPayloadSize, geoId64);
        }
    }

    void WritePrimIdPayload(uint32_t triIndex, uint32_t primId)
    {
        if (triIndex > 0)
        {
            const uint32_t primIdAnchorSize  = PrimIdAnchorSize();
            const uint32_t primIdPayloadSize = PrimIdPayloadSize();

            const uint32_t startOffset = IndexSectionMidpoint() + primIdAnchorSize + (triIndex - 1) * primIdPayloadSize;

            uint64_t primId64 = uint64_t(primId);
            if (primIdPayloadSize < primIdAnchorSize)
            {
                primId64 = primId64 & ((1ull << primIdPayloadSize) - 1ull);
            }  // else no compression
            WritePackedBits64(startOffset, primIdPayloadSize, primId64);
        }
    }

    void WriteFloat(const uint32_t startBitOffset, const uint32_t payloadLength, float f)
    {
        uint32_t component   = asuint(f) >> TrailingZeroLength();
        uint64_t component64 = (uint64_t(component)) & ((1ull << payloadLength) - 1ull);
        WritePackedBits64(startBitOffset, payloadLength, component64);
    }

    void WriteVertex(uint32_t verticeIndex, glm::vec3 vertex, bool isProcedural)
    {
        const uint32_t sectionOffset  = TRI_STRUCT_HEADER_SIZE_BITS + VertexPrefixesLength();
        const uint32_t payloadXLength = PayloadXLength();
        const uint32_t payloadYLength = PayloadYLength();
        const uint32_t payloadZLength = PayloadZLength();

        if (isProcedural)
        {
            // store full vertex for Procedural nodes
            const uint32_t dwBitLength = 0x20;
            uint32_t       offset      = TRI_STRUCT_HEADER_SIZE_BITS + (verticeIndex)*dwBitLength * 3;
            WriteFloat(offset, dwBitLength, vertex.x);
            offset += dwBitLength;
            WriteFloat(offset, dwBitLength, vertex.y);
            offset += dwBitLength;
            WriteFloat(offset, dwBitLength, vertex.z);
        }
        else
        {
            uint32_t startBitOffset = sectionOffset + (verticeIndex)*VertexPayloadLength();
            WriteFloat(startBitOffset, payloadXLength, vertex.x);

            startBitOffset += payloadXLength;
            WriteFloat(startBitOffset, payloadYLength, vertex.y);

            startBitOffset += payloadYLength;
            WriteFloat(startBitOffset, payloadZLength, vertex.z);
        }
    }

    void WriteTriPairDesc(uint32_t triPairIndex, TrianglePairDesc desc)
    {
        const uint32_t startOffset = 1024 - TRI_PAIR_DESC_SIZE * (triPairIndex + 1);
        WritePackedBits64(startOffset, TRI_PAIR_DESC_SIZE, uint64_t(desc.GetData()));
    }

    // Pack Metadata
    void SetMetadata(UnpackedPrimStructHeader header)
    {
        primData[0] = PackPrimitiveStructureHeaderLo(header);
        primData[1] = PackPrimitiveStructureHeaderHi(header);

        glm::vec3 perfixSource = glm::vec3(header.prefixSourceX, header.prefixSourceY, header.prefixSourceZ);
        WriteVertexPrefixes(perfixSource);
        WritePrimIdAnchor(header.anchorPrimId);
        WriteGeoIdAnchor(header.anchorGeoId);
    }

    uint32_t ReadPackedBits(uint32_t startBitOffset, uint32_t length)
    {
        uint32_t startByteOffset = startBitOffset / 8;
        uint32_t startDWOffset   = startBitOffset / 32;

        // Reading in the data and the one Byte after it
        uint64_t currData = uint64_t(primData[startDWOffset]);
        uint64_t nextData = 0;
        if (startDWOffset < 0x1f)
        {
            nextData = uint64_t(primData[startDWOffset + 1]);
        }
        uint64_t packedData = (nextData << 32) | currData;

        uint32_t shiftBit      = (startBitOffset % 8);
        uint32_t shiftBitsInDW = (startBitOffset % 32u);

        uint64_t packedDataRSH = packedData >> shiftBitsInDW;

        uint64_t dataOut64 = (packedDataRSH & ((uint64_t(1) << length) - uint64_t(1)));
        uint32_t dataOut   = uint32_t(dataOut64 & 0xFFFFFFFF);

        return dataOut;
    }

    glm::uvec3 VertexPrefixes()
    {
        uint32_t prefixXLength = PrefixXLength();
        uint32_t prefixYLength = PrefixYLength();
        uint32_t prefixZLength = PrefixZLength();

        uint32_t prefixX = (prefixXLength > 0) ? ReadPackedBits(TRI_STRUCT_HEADER_SIZE_BITS, prefixXLength) : 0;
        uint32_t prefixY = (prefixYLength > 0) ? ReadPackedBits(TRI_STRUCT_HEADER_SIZE_BITS + prefixXLength, prefixYLength) : 0;
        uint32_t prefixZ = (prefixZLength > 0) ? ReadPackedBits(TRI_STRUCT_HEADER_SIZE_BITS + prefixXLength + prefixYLength, prefixZLength) : 0;

        glm::uvec3 prefixes = {prefixX, prefixY, prefixZ};

        prefixes.x <<= 32 - prefixXLength;
        prefixes.y <<= 32 - prefixYLength;
        prefixes.z <<= 32 - prefixZLength;

        return prefixes;
    }

    glm::vec3 ReadVertex(uint32_t index, bool isProcedural)
    {
        uint32_t x = 0;
        uint32_t y = 0;
        uint32_t z = 0;

        if (isProcedural)
        {
            const uint32_t dwBitLength = 0x20;
            uint32_t       offset      = TRI_STRUCT_HEADER_SIZE_BITS + (index)*dwBitLength * 3;

            x = ReadPackedBits(offset, dwBitLength);
            offset += dwBitLength;
            y = ReadPackedBits(offset, dwBitLength);
            offset += dwBitLength;
            z = ReadPackedBits(offset, dwBitLength);
        }
        else
        {
            glm::uvec3     prefixes      = VertexPrefixes();
            const uint32_t sectionOffset = TRI_STRUCT_HEADER_SIZE_BITS + VertexPrefixesLength();
            uint32_t       offset        = sectionOffset + (index)*VertexPayloadLength();

            uint32_t payloadXLength     = PayloadXLength();
            uint32_t payloadYLength     = PayloadYLength();
            uint32_t payloadZLength     = PayloadZLength();
            uint32_t trailingZeroLength = TrailingZeroLength();

            x = prefixes.x | (ReadPackedBits(offset, payloadXLength) << trailingZeroLength);
            offset += payloadXLength;
            y = prefixes.y | (ReadPackedBits(offset, payloadYLength) << trailingZeroLength);
            offset += payloadYLength;
            z = prefixes.z | (ReadPackedBits(offset, payloadZLength) << trailingZeroLength);
        }
        float xf = asfloat(x);
        float yf = asfloat(y);
        float zf = asfloat(z);

        return glm::vec3(xf, yf, zf);
    }

    TrianglePairDesc ReadTrianglePairDesc(uint32_t triPairIndex)
    {
        const uint32_t startOffset = 1024 - TRI_PAIR_DESC_SIZE * (triPairIndex + 1);
        uint32_t       packedData  = ReadPackedBits(startOffset, TRI_PAIR_DESC_SIZE);

        TrianglePairDesc triDesc;
        triDesc.SetData(packedData);

        return triDesc;
    }

    uint32_t PrimIdAnchor()
    {
        uint32_t indexSectionMidpoint = IndexSectionMidpoint();
        uint32_t primIdAnchorSize     = PrimIdAnchorSize();
        return ReadPackedBits(indexSectionMidpoint, primIdAnchorSize);
    }

    uint32_t ReadPrimIdPayload(uint32_t triIndex)
    {
        uint32_t anchor               = PrimIdAnchor();
        uint32_t indexSectionMidpoint = IndexSectionMidpoint();
        uint32_t ret                  = 0;

        if (triIndex == 0)
        {
            ret = anchor;
        }
        else
        {
            uint32_t       primIdAnchorSize  = PrimIdAnchorSize();
            uint32_t       primIdPayloadSize = PrimIdPayloadSize();
            const uint32_t startOffset       = indexSectionMidpoint + primIdAnchorSize + (triIndex - 1) * primIdPayloadSize;
            uint32_t       packedData        = ReadPackedBits(startOffset, primIdPayloadSize);
            if (primIdPayloadSize >= primIdAnchorSize)
            {
                ret = packedData;
            }
            else
            {
                const uint32_t removeMask = ~((1u << primIdPayloadSize) - 1u);
                ret                       = (anchor & removeMask) | packedData;
            }
        }

        return ret;
    }

    uint32_t GeoIdAnchor()
    {
        uint32_t indexSectionMidpoint = IndexSectionMidpoint();
        uint32_t geoIdAnchorSize      = GeoIdAnchorSize();

        return ReadPackedBits(indexSectionMidpoint - geoIdAnchorSize, geoIdAnchorSize);
    }

    uint32_t ReadGeoIdPayload(uint32_t triIndex)
    {
        uint32_t anchor               = GeoIdAnchor();
        uint32_t indexSectionMidpoint = IndexSectionMidpoint();
        uint32_t geoIdAnchorSize      = GeoIdAnchorSize();
        uint32_t geoIdPayloadSize     = GeoIdPayloadSize();

        uint32_t ret = 0;

        if (triIndex == 0)
        {
            ret = anchor;
        }
        else
        {
            const uint32_t startOffset = indexSectionMidpoint - geoIdAnchorSize - (triIndex)*GeoIdPayloadSize();
            uint32_t       packedData  = ReadPackedBits(startOffset, geoIdPayloadSize);
            if (geoIdPayloadSize >= geoIdAnchorSize)
            {
                ret = packedData;
            }
            else
            {
                const uint32_t removeMask = ~((1u << geoIdPayloadSize) - 1u);
                ret                       = (anchor & removeMask) | packedData;
            }
        }

        return ret;
    }

    TriangleData UnpackTriangleVertices(uint32_t pair, uint32_t triIndex)
    {
        TrianglePairDesc pairDesc = ReadTrianglePairDesc(pair);

        bool isProceduralNode = (pairDesc.Tri0V0() == 15) && (pairDesc.Tri0V1() == 15);

        TriangleData tri;
        if (isProceduralNode)
        {
            tri.v0 = ReadVertex(0, true);
            tri.v1 = ReadVertex(1, true);
            tri.v2 = ReadVertex(2, true);
        }
        else
        {
            tri.v0 = (triIndex == 0) ? ReadVertex(pairDesc.Tri0V0(), false) : ReadVertex(pairDesc.Tri1V0(), false);
            tri.v1 = (triIndex == 0) ? ReadVertex(pairDesc.Tri0V1(), false) : ReadVertex(pairDesc.Tri1V1(), false);
            tri.v2 = (triIndex == 0) ? ReadVertex(pairDesc.Tri0V2(), false) : ReadVertex(pairDesc.Tri1V2(), false);
        }

        return tri;
    }

    bool IsOpaque(uint32_t pair, uint32_t triIndex)
    {
        TrianglePairDesc pairDesc = ReadTrianglePairDesc(pair);
        return (triIndex == 0) ? pairDesc.Tri0Opaque() : pairDesc.Tri1Opaque();
    }

    bool IsProcedural(uint32_t pair, uint32_t triIndex)
    {
        TrianglePairDesc pairDesc = ReadTrianglePairDesc(pair);

        const uint32_t i0 = (triIndex == 0) ? pairDesc.Tri0V0() : pairDesc.Tri1V0();
        const uint32_t i1 = (triIndex == 0) ? pairDesc.Tri0V1() : pairDesc.Tri1V1();

        return ((i0 == 0xF) && (i1 == 0xF));
    }

    uint32_t UnpackGeometryIndex(uint32_t pair, uint32_t triIndex)
    {
        return ReadGeoIdPayload(pair * 2 + triIndex);
    }

    uint32_t UnpackPrimitiveIndex(uint32_t pair, uint32_t triIndex)
    {
        return ReadPrimIdPayload(pair * 2 + triIndex);
    }

    uint32_t CalcNavigationBits(uint32_t pair)
    {
        TrianglePairDesc pairDesc          = ReadTrianglePairDesc(pair);
        const uint32_t   trianglePairCount = TrianglePairCount();

        uint32_t navigationBits = END_OF_RANGE;

        if (!pairDesc.PrimRangeStopBit())
        {
            // prim range continues
            if (pair == (trianglePairCount - 1))
            {
                // this is the last triangle pair in node, continue in next node
                navigationBits = END_OF_NODE;
            }
            else
            {
                // continue to next pair in node
                navigationBits = END_OF_PAIR;
            }
        }

        return navigationBits;
    }
};

//=====================================================================================================================
static bool TryInitTriStructHeader(glm::vec3                 vertex0,
                                   glm::uvec3                planeDiffs,
                                   uint32_t                  planeUnion,
                                   uint32_t                  primIdDiff,
                                   uint32_t                  geoIdDiff,
                                   uint32_t                  primIdAnchor,
                                   uint32_t                  geoIdAnchor,
                                   uint32_t                  uniqueVertexCount,
                                   uint32_t                  triPairCount,
                                   bool                      isProcedural,
                                   bool                      enableTrailingZeroCompression,
                                   UnpackedPrimStructHeader& header)
{
    const uint32_t trailingZeroLength = (isProcedural || !enableTrailingZeroCompression) ? 0 : CommonTrailingZeroBits(planeUnion);
    glm::uvec3     prefixLengths      = (isProcedural || !enableTrailingZeroCompression) ? glm::uvec3(0, 0, 0) : CommonPrefixBits(planeDiffs);
    prefixLengths                     = min(prefixLengths, glm::uvec3(31 - trailingZeroLength, 31 - trailingZeroLength, 31 - trailingZeroLength));
    const uint32_t vertexSize         = 96 - (prefixLengths.x + prefixLengths.y + prefixLengths.z + 3 * trailingZeroLength);
    const uint32_t primPayloadLength  = 32 - LeadingZeroBits(primIdDiff);
    const uint32_t primAnchorLength   = 32 - LeadingZeroBits(primIdAnchor);

    // Note, geometry ID payload and anchor lengths are encoded as multiple of 2. We need to align the lengths here
    // to account for that
    const uint32_t geoPayloadLength = Pow2Align(32 - LeadingZeroBits(geoIdDiff), 2);
    const uint32_t geoAnchorLength  = Pow2Align(32 - LeadingZeroBits(geoIdAnchor), 2);

    const uint32_t triPairDataSize = 2 * primPayloadLength + 2 * geoPayloadLength + TRI_PAIR_DESC_SIZE;
    const uint32_t vertexDataSize  = uniqueVertexCount * vertexSize;
    const uint32_t anchorSizes     = prefixLengths.x + prefixLengths.y + prefixLengths.z + primAnchorLength + geoAnchorLength;
    const uint32_t requisiteBits   = triPairDataSize * triPairCount + vertexDataSize + anchorSizes;
    if (requisiteBits > TRI_STRUCT_DATA_SECTION_SIZE_BITS || uniqueVertexCount > 16 || triPairCount > 8)
    {
        // One pair should always fit
        RRA_ASSERT(triPairCount > 1);
        return false;
    }

    RRA_ASSERT(prefixLengths.x + trailingZeroLength <= 31);
    RRA_ASSERT(prefixLengths.y + trailingZeroLength <= 31);
    RRA_ASSERT(prefixLengths.z + trailingZeroLength <= 31);

    header.anchorGeoId          = geoIdAnchor;
    header.anchorPrimId         = primIdAnchor;
    header.floatOrUnorm16       = false;
    header.prefixSourceX        = vertex0.x;
    header.prefixSourceY        = vertex0.y;
    header.prefixSourceZ        = vertex0.z;
    header.triPairCountMinusOne = (triPairCount - 1);

    if (isProcedural)
    {
        // store full vertex instead of compressed format
        //
        // each vertex is stored as 1 DW (32bit)
        // min.x
        // min.y
        // min.z
        // max.x
        // max.y
        // max.z
        //
        // geometryPayload---  0 bit
        // geometryIdAncor--- 15 bit
        //------------------- index_section_midpoint
        // primIdAnchor ----- 32 bit
        // primIdPayload-----  0 bit
        // triDesc----------- 29 bit
        //------------------- 1024bit

        header.payloadXLengthMinusOne = 0;  // decode as 1
        header.payloadYLengthMinusOne = 0;  // decode as 1
        header.payloadZLengthMinusOne = 0;  // decode as 1
        header.trailingZeroLength     = 0;

        header.geoIdPayloadSizeDivTwo = 0x0;
        header.geoIdAnchorSizeDivTwo  = 0xF;  // decode as 30
        header.indexSectionMidpoint   = ComputeIndexSectionMidpoint(1, 0, 32);
        header.primIdAnchorSize       = 0x1F;
        header.primIdPayloadSize      = 0;
    }
    else
    {
        header.payloadXLengthMinusOne = (32 - prefixLengths.x - trailingZeroLength) - 1;
        header.payloadYLengthMinusOne = (32 - prefixLengths.y - trailingZeroLength) - 1;
        header.payloadZLengthMinusOne = (32 - prefixLengths.z - trailingZeroLength) - 1;
        header.trailingZeroLength     = trailingZeroLength;
        header.geoIdAnchorSizeDivTwo  = geoAnchorLength / 2;
        header.geoIdPayloadSizeDivTwo = geoPayloadLength / 2;
        header.indexSectionMidpoint   = ComputeIndexSectionMidpoint(triPairCount, primPayloadLength, primAnchorLength);
        header.primIdAnchorSize       = primAnchorLength;
        header.primIdPayloadSize      = primPayloadLength;
    }

    return true;
}

//=====================================================================================================================
static uint32_t GetPairIndex(uint32_t nodePointer)
{
    return (nodePointer & 0x3) | ((nodePointer & 8) >> 1);
}

#ifdef _WIN32
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

#endif  // _GFX12_PRIMITIVE_NODE_H

