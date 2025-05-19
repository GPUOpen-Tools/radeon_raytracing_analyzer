//=============================================================================
//  Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 3.1 (Navi4x) specific internal node definition.
//=============================================================================

#ifndef RRA_BACKEND_INTERNAL_NODE_H_
#define RRA_BACKEND_INTERNAL_NODE_H_

#include <algorithm>
#include <cstdint>
#include <limits>

#include "glm/glm/glm.hpp"

#include "bvh/rtip31/child_info.h"
#include "bvh/rtip_common/math.h"

#ifndef _WIN32
// Switch off Linux warnings-as-errors for now.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

static const uint32_t ObbDisabled = 0x7f;

#define QUANTIZED_BVH8_NODE_OFFSET_INTERNAL_NODE_BASE_OFFSET 0       // offsetof(QuantizedBVH8BoxNode, internalNodeBaseOffset);
#define QUANTIZED_BVH8_NODE_OFFSET_LEAF_NODE_BASE_OFFSET 4           // offsetof(QuantizedBVH8BoxNode, leafNodeBaseOffset);
#define QUANTIZED_BVH8_NODE_OFFSET_PARENT_POINTER 8                  // offsetof(QuantizedBVH8BoxNode, parentPointer);
#define QUANTIZED_BVH8_NODE_OFFSET_ORIGIN 12                         // offsetof(QuantizedBVH8BoxNode, origin);
#define QUANTIZED_BVH8_NODE_OFFSET_EXP_CHILD_IDX_AND_VALID_COUNT 24  // offsetof(QuantizedBVH8BoxNode, exponentsChildIndexAndChildCount);
#define QUANTIZED_BVH8_NODE_OFFSET_OBB_MATRIX_INDEX 28               // offsetof(QuantizedBVH8BoxNode, obbMatrixIndex);
#define QUANTIZED_BVH8_NODE_OFFSET_CHILD_INFO_0 32                   // offsetof(QuantizedBVH8BoxNode, childInfos);
#define QUANTIZED_BVH8_NODE_SIZE 128

#define QUANTIZED_BVH4_NODE_OFFSET_ORIGIN 0
#define QUANTIZED_BVH4_NODE_OFFSET_EXP_CHILD_IDX_AND_VALID_COUNT 12
#define QUANTIZED_BVH4_NODE_OFFSET_CHILD_INFO_0 16

#define INVALID_NODE 0xffffffff

//=====================================================================================================================
// Quantized internal BVH8 node information
//=====================================================================================================================
struct QuantizedBVH8BoxNode
{
    uint32_t  internalNodeBaseOffset;
    uint32_t  leafNodeBaseOffset;
    uint32_t  parentPointer;
    glm::vec3 origin;

#ifdef __cplusplus
    union
    {
        struct
        {
            uint32_t xExponent : 8;
            uint32_t yExponent : 8;
            uint32_t zExponent : 8;
            uint32_t childIndex : 4;
            uint32_t childCount : 4;
        };

        uint32_t exponentsChildIndexAndChildCount;
    };
#else
    uint32_t exponentsChildIndexAndChildCount;
#endif

    uint32_t  obbMatrixIndex;
    ChildInfo childInfos[8];

#ifdef __cplusplus
    QuantizedBVH8BoxNode()
        : QuantizedBVH8BoxNode(0)
    {
    }

    QuantizedBVH8BoxNode(uint32_t val)
    {
        memset(this, val, sizeof(QuantizedBVH8BoxNode));
    }
#endif

    static uint32_t PackExpChildIdxAndCount(glm::uvec3 exponents, uint32_t disableBoxSort, uint32_t indexInParent, uint32_t validChildCount);

    void       Init();
    uint32_t   InternalNodeBaseOffset();
    void       SetInternalNodeBaseOffset(uint32_t internalNodeBaseOffsetVal);
    uint32_t   OBBMatrixIndex();
    void       SetOBBMatrixIndex(uint32_t idx);
    uint32_t   LeafNodeBaseOffset();
    void       SetLeafNodeBaseOffset(uint32_t leafNodeBaseOffsetVal);
    uint32_t   ParentPointer();
    void       SetParentPointer(uint32_t parentPointerVal);
    glm::vec3  Origin();
    void       SetOrigin(glm::vec3 originVal);
    glm::uvec3 Exponents() const;
    void       SetExponents(glm::uvec3 exponents);
    uint32_t   IndexInParent();
    void       SetIndexInParent(uint32_t indexInParent);
    uint32_t   ValidChildCount() const;
    void       SetValidChildCount(uint32_t childCount);
    void       SetLeafPointer(uint32_t childIndex, uint32_t startTriStructOffset, uint32_t startType);
    void       SetInternalNodeChildInfo(uint32_t childIndex, uint32_t startInternalNodeOffset);

#if GPURT_DEVELOPER
    bool ValidatePlanes(BoundingBox aabbs[8], uint32_t aabbCount);

#ifdef __cplusplus
    void Print();
#endif  // __cplusplus
#endif  // GPURT_DEVELOPER

    void Decode(Float32BoxNode& f32BoxNode0, Float32BoxNode& f32BoxNode1, uint32_t& parentPointer);

    void DecodeChildrenOffsets(uint32_t childPointers[8]) const;

    void Encode(Float32BoxNode f32BoxNode0, Float32BoxNode f32BoxNode1, uint32_t parentPointer, uint32_t indexInParent);

    void EncodeObbOnly(Float32BoxNode f32BoxNode0, Float32BoxNode f32BoxNode1);
};

#if __cplusplus
static_assert(sizeof(QuantizedBVH8BoxNode) == 128);
#endif  /// __cplusplus

//=====================================================================================================================
inline uint32_t QuantizedBVH8BoxNode::PackExpChildIdxAndCount(glm::uvec3 exponents, uint32_t disableBoxSort, uint32_t indexInParent, uint32_t validChildCount)
{
    uint32_t packedData = 0;
    packedData          = bitFieldInsert(packedData, 0, 8, exponents.x);
    packedData          = bitFieldInsert(packedData, 8, 8, exponents.y);
    packedData          = bitFieldInsert(packedData, 16, 8, exponents.z);
    packedData          = bitFieldInsert(packedData, 24, 1, disableBoxSort);
    packedData          = bitFieldInsert(packedData, 25, 3, indexInParent);
    packedData          = bitFieldInsert(packedData, 28, 3, validChildCount - 1);

    return packedData;
}

//=====================================================================================================================
inline void QuantizedBVH8BoxNode::Init()
{
    internalNodeBaseOffset           = 0;
    leafNodeBaseOffset               = 0;
    parentPointer                    = INVALID_NODE;
    origin                           = glm::vec3(0, 0, 0);
    exponentsChildIndexAndChildCount = 0;
    obbMatrixIndex                   = ObbDisabled;

    for (uint32_t i = 0; i < 8; i++)
    {
        childInfos[i].Init();
    }
}

//=====================================================================================================================
inline uint32_t QuantizedBVH8BoxNode::InternalNodeBaseOffset()
{
    return internalNodeBaseOffset;
}

//=====================================================================================================================
inline void QuantizedBVH8BoxNode::SetInternalNodeBaseOffset(uint32_t internalNodeBaseOffsetVal)
{
    internalNodeBaseOffset = internalNodeBaseOffsetVal;
}

//=====================================================================================================================
inline uint32_t QuantizedBVH8BoxNode::OBBMatrixIndex()
{
    return obbMatrixIndex;
}

//=====================================================================================================================
inline void QuantizedBVH8BoxNode::SetOBBMatrixIndex(uint32_t idx)
{
    obbMatrixIndex = idx;
}

//=====================================================================================================================
inline uint32_t QuantizedBVH8BoxNode::LeafNodeBaseOffset()
{
    return leafNodeBaseOffset;
}

//=====================================================================================================================
inline void QuantizedBVH8BoxNode::SetLeafNodeBaseOffset(uint32_t leafNodeBaseOffsetVal)
{
    leafNodeBaseOffset = leafNodeBaseOffsetVal;
}

//=====================================================================================================================
inline uint32_t QuantizedBVH8BoxNode::ParentPointer()
{
    return parentPointer;
}

//=====================================================================================================================
inline void QuantizedBVH8BoxNode::SetParentPointer(uint32_t parentPointerVal)
{
    parentPointer = parentPointerVal;
}

//=====================================================================================================================
inline glm::vec3 QuantizedBVH8BoxNode::Origin()
{
    return origin;
}

//=====================================================================================================================
inline void QuantizedBVH8BoxNode::SetOrigin(glm::vec3 originVal)
{
    origin = originVal;
}

//=====================================================================================================================
inline glm::uvec3 QuantizedBVH8BoxNode::Exponents() const
{
    glm::uvec3 exponents;
    exponents.x = bitFieldExtract(exponentsChildIndexAndChildCount, 0, 8);
    exponents.y = bitFieldExtract(exponentsChildIndexAndChildCount, 8, 8);
    exponents.z = bitFieldExtract(exponentsChildIndexAndChildCount, 16, 8);

    return exponents;
}

//=====================================================================================================================
inline void QuantizedBVH8BoxNode::SetExponents(glm::uvec3 exponents)
{
    exponentsChildIndexAndChildCount = bitFieldInsert(exponentsChildIndexAndChildCount, 0, 8, exponents.x);
    exponentsChildIndexAndChildCount = bitFieldInsert(exponentsChildIndexAndChildCount, 8, 8, exponents.y);
    exponentsChildIndexAndChildCount = bitFieldInsert(exponentsChildIndexAndChildCount, 16, 8, exponents.z);
}

//=====================================================================================================================
inline uint32_t QuantizedBVH8BoxNode::IndexInParent()
{
    return bitFieldExtract(exponentsChildIndexAndChildCount, 25, 3);
}

//=====================================================================================================================
inline void QuantizedBVH8BoxNode::SetIndexInParent(uint32_t indexInParent)
{
    indexInParent                    = indexInParent & 0x7;
    exponentsChildIndexAndChildCount = bitFieldInsert(exponentsChildIndexAndChildCount, 25, 3, indexInParent);
}

//=====================================================================================================================
inline uint32_t QuantizedBVH8BoxNode::ValidChildCount() const
{
    return 1 + bitFieldExtract(exponentsChildIndexAndChildCount, 28, 3);
}

//=====================================================================================================================
inline void QuantizedBVH8BoxNode::SetValidChildCount(uint32_t childCountBits)
{
    exponentsChildIndexAndChildCount = bitFieldInsert(exponentsChildIndexAndChildCount, 28, 3, (childCountBits - 1));
}

//=====================================================================================================================
inline void QuantizedBVH8BoxNode::SetLeafPointer(uint32_t childIdx, uint32_t startTriStructOffset, uint32_t startType)
{
    /* The internal nodes conceptually encode up to 8 primitive ranges. Each primitive range
    * is described using a single start pointer (offset + node type). To generate 8 stack offsets
    * it must be known how many node boundaries each primitive range crosses.
    *
    * To simplify, the actual value for node boundaries is known given 3 pieces of information
    * 1. The internal node base leaf offset
    * 2. The number of node boundaries crossed by earlier leaf children
    * 3. The offset of the node that is beginning this primitive range
    *
    * 1 and 2 determine the offset of the previous primitive range and 3 determines where this primitive range starts.
    * Subtracting where this prim range starts from where the last starts allows us to determine the necessary value
    * for node boundaries without storing any extra state.
    */

    if (leafNodeBaseOffset == INVALID_NODE)
    {
        leafNodeBaseOffset = startTriStructOffset;
    }

#if defined(GPURT_DEVELOPER) && defined(__cplusplus)
    assert(startTriStructOffset >= leafNodeBaseOffset);
#endif

    uint32_t runningOffset = leafNodeBaseOffset;
    uint32_t lastPrimIndex = 0xFFFFFFFF;
    for (uint32_t i = 0; i < childIdx; i++)
    {
        if (childInfos[i].Valid())
        {
            if (childInfos[i].NodeType() != NODE_TYPE_BOX_QUANTIZED_BVH8)
            {
                lastPrimIndex = i;
                runningOffset += childInfos[i].NodeRangeLength() * 128;
            }
        }
    }
    uint32_t nodeBoundaries = (startTriStructOffset - runningOffset) / 128;

    if (lastPrimIndex != 0xFFFFFFFF)
    {
        childInfos[lastPrimIndex].SetNodeRangeLength(nodeBoundaries);
    }
    childInfos[childIdx].SetNodeType(startType);
}

//=====================================================================================================================
inline void QuantizedBVH8BoxNode::SetInternalNodeChildInfo(uint32_t childIdx, uint32_t startInternalNodeOffset)
{
    uint32_t lastInternalIndex = 0xFFFFFFFF;
    uint32_t runningOffset     = internalNodeBaseOffset;
    for (uint32_t i = 0; i < childIdx; i++)
    {
        if (childInfos[i].Valid())
        {
            if (childInfos[i].NodeType() == NODE_TYPE_BOX_QUANTIZED_BVH8)
            {
                runningOffset += childInfos[i].NodeRangeLength() * 128;
                lastInternalIndex = i;
            }
        }
    }
    uint32_t nodeBoundaries = (startInternalNodeOffset - runningOffset) / 128;

    if (lastInternalIndex != 0xFFFFFFFF)
    {
        childInfos[lastInternalIndex].SetNodeRangeLength(nodeBoundaries);
    }
    childInfos[childIdx].SetNodeType(NODE_TYPE_BOX_QUANTIZED_BVH8);
}

//=====================================================================================================================
inline void QuantizedBVH8BoxNode::Decode(Float32BoxNode& f32BoxNode0, Float32BoxNode& f32BoxNode1, uint32_t& outParentPointer)
{
    BoundingBox aabbs[8];
    uint32_t    childPointers[8];
    uint32_t    childReuse[8];

    outParentPointer = ParentPointer();

    // Take local copies of base offsets as these are updated as children are decoded.
    // Lower 4 bits of internal offset will always be 5 as this is the only valid internal
    // node type.
    uint32_t internalOffset  = internalNodeBaseOffset | NODE_TYPE_BOX_QUANTIZED_BVH8;
    uint32_t primitiveOffset = leafNodeBaseOffset;

    uint32_t aabbCount = ValidChildCount();

    // Track the primitive range and node type of the previous child.
    // Used to determine if child reuse is active for current box.
    uint32_t previousNodeType  = 0;
    bool     previousRangeZero = false;

    glm::uvec3 exponents = Exponents();

    // Valid children are always packed from the start of the node.
    for (uint32_t i = 0; i < 8; i++)
    {
        if (i < aabbCount)
        {
            // Decode the child offset.
            // There are two running offsets: one for internal node children (boxes), and one for
            // primitive node children (all other types).
            // For primitive node children, the node type must be placed into the lower 4 bits.
            // The appropriate running offset must be incremented by the number of consecutive
            // 128-byte nodes the box points to.
            if (childInfos[i].NodeType() == NODE_TYPE_BOX_QUANTIZED_BVH8)
            {
                childPointers[i] = internalOffset;
                internalOffset += (childInfos[i].NodeRangeLength() << 4);
            }
            else
            {
                childPointers[i] = primitiveOffset | childInfos[i].NodeType();
                primitiveOffset += (childInfos[i].NodeRangeLength() << 4);
            }

            // Dequantize the box planes.
            // Min planes range from 0-4095, and max planes from 1-4096.
            // This will result in output planes ranging from 0 - 2^(exponent-127).
            aabbs[i] = childInfos[i].DecodeBounds(origin, exponents);

            // This box is reusing the child offset from the previous box if the node type is the
            // same and the previous box had zero primitive range.
            childReuse[i]     = (previousNodeType == childInfos[i].NodeType()) && (previousRangeZero == true);
            previousNodeType  = childInfos[i].NodeType();
            previousRangeZero = (childInfos[i].NodeRangeLength() == 0);
        }
        else
        {
            // Invalid children are always packed at the end of the node.
            childPointers[i] = INVALID_NODE;

            // Set the box planes to NaN to force a miss.
            aabbs[i].min =
                glm::vec3(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());
            aabbs[i].max =
                glm::vec3(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());

            childReuse[i] = 0;
        }
    }

    f32BoxNode0 = (Float32BoxNode)0;
    f32BoxNode1 = (Float32BoxNode)0;

    f32BoxNode0.child0 = childPointers[0];
    f32BoxNode0.child1 = childPointers[1];
    f32BoxNode0.child2 = childPointers[2];
    f32BoxNode0.child3 = childPointers[3];
    f32BoxNode1.child0 = childPointers[4];
    f32BoxNode1.child1 = childPointers[5];
    f32BoxNode1.child2 = childPointers[6];
    f32BoxNode1.child3 = childPointers[7];

    f32BoxNode0.bbox0_min      = aabbs[0].min;
    f32BoxNode0.bbox0_max      = aabbs[0].max;
    f32BoxNode0.bbox1_min      = aabbs[1].min;
    f32BoxNode0.bbox1_max      = aabbs[1].max;
    f32BoxNode0.bbox2_min      = aabbs[2].min;
    f32BoxNode0.bbox2_max      = aabbs[2].max;
    f32BoxNode0.bbox3_min      = aabbs[3].min;
    f32BoxNode0.bbox3_max      = aabbs[3].max;
    f32BoxNode0.obbMatrixIndex = obbMatrixIndex;

    f32BoxNode1.bbox0_min = aabbs[4].min;
    f32BoxNode1.bbox0_max = aabbs[4].max;
    f32BoxNode1.bbox1_min = aabbs[5].min;
    f32BoxNode1.bbox1_max = aabbs[5].max;
    f32BoxNode1.bbox2_min = aabbs[6].min;
    f32BoxNode1.bbox2_max = aabbs[6].max;
    f32BoxNode1.bbox3_min = aabbs[7].min;
    f32BoxNode1.bbox3_max = aabbs[7].max;

    for (uint32_t j = 0; j < 4; j++)
    {
        f32BoxNode0.flags |= (childInfos[j].CullingFlags() << (j * 8));
        f32BoxNode1.flags |= (childInfos[j + 4].CullingFlags() << (j * 8));

        f32BoxNode0.instanceMask |= (childInfos[j].InstanceMask() << (j * 8));
        f32BoxNode1.instanceMask |= (childInfos[j + 4].InstanceMask() << (j * 8));
    }
}

//=====================================================================================================================
inline void QuantizedBVH8BoxNode::DecodeChildrenOffsets(uint32_t childPointers[8]) const
{
    // Take local copies of base offsets as these are updated as children are decoded.
    // Lower 4 bits of internal offset will always be 5 as this is the only valid internal
    // node type.
    uint32_t internalOffset  = internalNodeBaseOffset | NODE_TYPE_BOX_QUANTIZED_BVH8;
    uint32_t primitiveOffset = leafNodeBaseOffset;

    uint32_t aabbCount = ValidChildCount();

    // Track the primitive range and node type of the previous child.
    // Used to determine if child reuse is active for current box.
    uint32_t previousNodeType  = 0;
    bool     previousRangeZero = false;

    glm::uvec3 exponents = Exponents();

    // Valid children are always packed from the start of the node.
    for (uint32_t i = 0; i < 8; i++)
    {
        if (i < aabbCount)
        {
            // Decode the child offset.
            // There are two running offsets: one for internal node children (boxes), and one for
            // primitive node children (all other types).
            // For primitive node children, the node type must be placed into the lower 4 bits.
            // The appropriate running offset must be incremented by the number of consecutive
            // 128-byte nodes the box points to.
            if (childInfos[i].NodeType() == NODE_TYPE_BOX_QUANTIZED_BVH8)
            {
                childPointers[i] = internalOffset;
                internalOffset += (childInfos[i].NodeRangeLength() << 4);
            }
            else
            {
                childPointers[i] = primitiveOffset | childInfos[i].NodeType();
                primitiveOffset += (childInfos[i].NodeRangeLength() << 4);
            }

            // This box is reusing the child offset from the previous box if the node type is the
            // same and the previous box had zero primitive range.
            //childReuse[i] = (previousNodeType == childInfos[i].NodeType()) && (previousRangeZero == true);
            previousNodeType  = childInfos[i].NodeType();
            previousRangeZero = (childInfos[i].NodeRangeLength() == 0);
        }
        else
        {
            // Invalid children are always packed at the end of the node.
            childPointers[i] = INVALID_NODE;
        }
    }
}

//=====================================================================================================================
inline void QuantizedBVH8BoxNode::Encode(Float32BoxNode f32BoxNode0, Float32BoxNode f32BoxNode1, uint32_t parentPtr, uint32_t indexInParent)
{
    BoundingBox aabbs[8];
    uint32_t    childPointers[8];

    childPointers[0] = f32BoxNode0.child0;
    childPointers[1] = f32BoxNode0.child1;
    childPointers[2] = f32BoxNode0.child2;
    childPointers[3] = f32BoxNode0.child3;
    childPointers[4] = f32BoxNode1.child0;
    childPointers[5] = f32BoxNode1.child1;
    childPointers[6] = f32BoxNode1.child2;
    childPointers[7] = f32BoxNode1.child3;

    aabbs[0].min = f32BoxNode0.bbox0_min;
    aabbs[0].max = f32BoxNode0.bbox0_max;
    aabbs[1].min = f32BoxNode0.bbox1_min;
    aabbs[1].max = f32BoxNode0.bbox1_max;
    aabbs[2].min = f32BoxNode0.bbox2_min;
    aabbs[2].max = f32BoxNode0.bbox2_max;
    aabbs[3].min = f32BoxNode0.bbox3_min;
    aabbs[3].max = f32BoxNode0.bbox3_max;
    aabbs[4].min = f32BoxNode1.bbox0_min;
    aabbs[4].max = f32BoxNode1.bbox0_max;
    aabbs[5].min = f32BoxNode1.bbox1_min;
    aabbs[5].max = f32BoxNode1.bbox1_max;
    aabbs[6].min = f32BoxNode1.bbox2_min;
    aabbs[6].max = f32BoxNode1.bbox2_max;
    aabbs[7].min = f32BoxNode1.bbox3_min;
    aabbs[7].max = f32BoxNode1.bbox3_max;

    for (uint32_t i = 0; i < 4; i++)
    {
        childInfos[i].SetCullingFlags(bitFieldExtract(f32BoxNode0.flags, (i * 8), 4));
        childInfos[i + 4].SetCullingFlags(bitFieldExtract(f32BoxNode1.flags, (i * 8), 4));
        childInfos[i].SetInstanceMask(bitFieldExtract(f32BoxNode0.instanceMask, i * 8, 8));
        childInfos[i + 4].SetInstanceMask(bitFieldExtract(f32BoxNode1.instanceMask, i * 8, 8));
    }

    uint32_t aabbCount          = 0;
    uint32_t internalBaseOffset = 0;
    uint32_t leafBaseOffset     = 0;

    // Valid children are always packed from the start of the node.
    for (; (aabbCount < 8) && (childPointers[aabbCount] != INVALID_NODE); ++aabbCount)
    {
        if (IsQuantizedBVH8BoxNode(childPointers[aabbCount]))
        {
            if (internalBaseOffset == 0)
            {
                internalBaseOffset = ClearNodeType(childPointers[aabbCount]);
            }
        }
        else if (leafBaseOffset == 0)
        {
            leafBaseOffset = ClearNodeType(childPointers[aabbCount]);
        }

        childInfos[aabbCount].SetNodeType(GetNodeType(childPointers[aabbCount]));
        childInfos[aabbCount].SetNodeRangeLength(1);
    }
    SetOBBMatrixIndex(f32BoxNode0.obbMatrixIndex);
    SetInternalNodeBaseOffset(internalBaseOffset);
    SetLeafNodeBaseOffset(leafBaseOffset);
    SetParentPointer(parentPtr);
    SetIndexInParent(indexInParent);

    // Next section is all about encoding the child bounding boxes

    glm::vec3 minOfMins = aabbs[0].min;
    glm::vec3 maxOfMaxs = aabbs[0].max;
    for (uint32_t j = 1; j < aabbCount; j++)
    {
        // For degenerate triangles and instances, we write inverted bounds in scratch nodes.
        // Inverted bounds don't get encoded and decoded correctly. For example, when min is FLT_MAX and max is
        // -FLT_MAX, max gets decoded as INFINITY, which is a problem during intersection.
        // We set instance inclusion mask as 0 instead to cull them out.
        minOfMins.x = std::min(minOfMins.x, aabbs[j].min.x);
        minOfMins.y = std::min(minOfMins.y, aabbs[j].min.y);
        minOfMins.z = std::min(minOfMins.z, aabbs[j].min.z);
        maxOfMaxs.x = std::max(maxOfMaxs.x, aabbs[j].max.x);
        maxOfMaxs.y = std::max(maxOfMaxs.y, aabbs[j].max.y);
        maxOfMaxs.z = std::max(maxOfMaxs.z, aabbs[j].max.z);
    }

    const glm::uvec3 exponents = ComputeCommonExponent(minOfMins, maxOfMaxs);

    const glm::vec3 rcpExponents = ComputeFastExpReciprocal(exponents);

    SetOrigin(minOfMins);
    SetExponents(exponents);

    for (uint32_t k = 0; k < aabbCount; k++)
    {
        const glm::uvec3 quantMax = ComputeQuantizedMax(aabbs[k].max, minOfMins, rcpExponents, 12);
        childInfos[k].SetMax(quantMax);
    }

    for (uint32_t l = 0; l < aabbCount; l++)
    {
        const glm::uvec3 quantMin = ComputeQuantizedMin(aabbs[l].min, minOfMins, rcpExponents, 12);
        childInfos[l].SetMin(quantMin);
    }

    for (uint32_t m = aabbCount; m < 8; m++)
    {
        childInfos[m].Invalidate();
    }

    SetValidChildCount(aabbCount);
}

//=====================================================================================================================
inline void QuantizedBVH8BoxNode::EncodeObbOnly(Float32BoxNode f32BoxNode0, Float32BoxNode f32BoxNode1)
{
    BoundingBox aabbs[8];
    uint32_t    childPointers[8];

    childPointers[0] = f32BoxNode0.child0;
    childPointers[1] = f32BoxNode0.child1;
    childPointers[2] = f32BoxNode0.child2;
    childPointers[3] = f32BoxNode0.child3;
    childPointers[4] = f32BoxNode1.child0;
    childPointers[5] = f32BoxNode1.child1;
    childPointers[6] = f32BoxNode1.child2;
    childPointers[7] = f32BoxNode1.child3;

    aabbs[0].min = f32BoxNode0.bbox0_min;
    aabbs[0].max = f32BoxNode0.bbox0_max;
    aabbs[1].min = f32BoxNode0.bbox1_min;
    aabbs[1].max = f32BoxNode0.bbox1_max;
    aabbs[2].min = f32BoxNode0.bbox2_min;
    aabbs[2].max = f32BoxNode0.bbox2_max;
    aabbs[3].min = f32BoxNode0.bbox3_min;
    aabbs[3].max = f32BoxNode0.bbox3_max;
    aabbs[4].min = f32BoxNode1.bbox0_min;
    aabbs[4].max = f32BoxNode1.bbox0_max;
    aabbs[5].min = f32BoxNode1.bbox1_min;
    aabbs[5].max = f32BoxNode1.bbox1_max;
    aabbs[6].min = f32BoxNode1.bbox2_min;
    aabbs[6].max = f32BoxNode1.bbox2_max;
    aabbs[7].min = f32BoxNode1.bbox3_min;
    aabbs[7].max = f32BoxNode1.bbox3_max;

    uint32_t aabbCount          = 0;
    uint32_t internalBaseOffset = 0;
    uint32_t leafBaseOffset     = 0;

    // Valid children are always packed from the start of the node.
    for (; (aabbCount < 8) && (childPointers[aabbCount] != INVALID_NODE); ++aabbCount)
    {
        if (IsQuantizedBVH8BoxNode(childPointers[aabbCount]))
        {
            if (internalBaseOffset == 0)
            {
                internalBaseOffset = ClearNodeType(childPointers[aabbCount]);
            }
        }
        else if (leafBaseOffset == 0)
        {
            leafBaseOffset = ClearNodeType(childPointers[aabbCount]);
        }
    }
    SetOBBMatrixIndex(f32BoxNode0.obbMatrixIndex);

    // Next section is all about encoding the child bounding boxes

    glm::vec3 minOfMins = aabbs[0].min;
    glm::vec3 maxOfMaxs = aabbs[0].max;
    for (uint32_t j = 1; j < aabbCount; j++)
    {
        // For degenerate triangles and instances, we write inverted bounds in scratch nodes.
        // Inverted bounds don't get encoded and decoded correctly. For example, when min is FLT_MAX and max is
        // -FLT_MAX, max gets decoded as INFINITY, which is a problem during intersection.
        // We set instance inclusion mask as 0 instead to cull them out.
        minOfMins.x = std::min(minOfMins.x, aabbs[j].min.x);
        minOfMins.y = std::min(minOfMins.y, aabbs[j].min.y);
        minOfMins.z = std::min(minOfMins.z, aabbs[j].min.z);
        maxOfMaxs.x = std::max(maxOfMaxs.x, aabbs[j].max.x);
        maxOfMaxs.y = std::max(maxOfMaxs.y, aabbs[j].max.y);
        maxOfMaxs.z = std::max(maxOfMaxs.z, aabbs[j].max.z);
    }

    const glm::uvec3 exponents = ComputeCommonExponent(minOfMins, maxOfMaxs);

    const glm::vec3 rcpExponents = ComputeFastExpReciprocal(exponents);

    SetOrigin(minOfMins);
    SetExponents(exponents);

    for (uint32_t k = 0; k < aabbCount; k++)
    {
        const glm::uvec3 quantMax = ComputeQuantizedMax(aabbs[k].max, minOfMins, rcpExponents, 12);
        childInfos[k].SetMax(quantMax);
    }

    for (uint32_t l = 0; l < aabbCount; l++)
    {
        const glm::uvec3 quantMin = ComputeQuantizedMin(aabbs[l].min, minOfMins, rcpExponents, 12);
        childInfos[l].SetMin(quantMin);
    }

    for (uint32_t m = aabbCount; m < 8; m++)
    {
        childInfos[m].Invalidate();
    }

    SetValidChildCount(aabbCount);
}

#ifndef _WIN32
// Switch off Linux warnings-as-errors for now.
#pragma GCC diagnostic pop
#endif

#endif  // RRA_BACKEND_INTERNAL_NODE_H_

