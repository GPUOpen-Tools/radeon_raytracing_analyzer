//=============================================================================
//  Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 3.1 (Navi4x) specific child node definition.
//=============================================================================

#ifndef RRA_BACKEND_CHILD_INFO_H_
#define RRA_BACKEND_CHILD_INFO_H_

#include <cstdint>
#include <cstring>
#include "glm/glm/glm.hpp"
#include "math.h"

#ifndef _WIN32
// Switch off Linux warnings-as-errors for now.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

//=====================================================================================================================
// Quantized child node information
#pragma pack(push, 4)
struct ChildInfo
{
    union
    {
        struct
        {
            uint32_t minX : 12;
            uint32_t minY : 12;
            uint32_t cullingFlags : 4;
            uint32_t unused : 4;
        };

        uint32_t minXMinYAndCullingFlags;
    };

    union
    {
        struct
        {
            uint32_t minZ : 12;
            uint32_t maxX : 12;
            uint32_t instanceMask : 8;
        };

        uint32_t minZMaxXAndInstanceMask;
    };

    union
    {
        struct
        {
            uint32_t maxY : 12;
            uint32_t maxZ : 12;
            uint32_t nodeType : 4;
            uint32_t nodeRange : 4;
        };

        uint32_t maxYMaxZNodeTypeAndNodeRange;
    };

    ChildInfo()
        : ChildInfo(0)
    {
    }

    ChildInfo(uint32_t val)
    {
        memset(this, val, sizeof(ChildInfo));
    }

    static const glm::uvec3 Invalid;

    static glm::uvec3 BuildPacked(glm::uvec3 quantMin,
                                  glm::uvec3 quantMax,
                                  uint32_t   boxNodeFlags,
                                  uint32_t   instanceMask,
                                  uint32_t   nodeType,
                                  uint32_t   nodeRangeLength);

    void        Load(glm::uvec3 packedData);
    void        Init();
    void        Invalidate();
    glm::uvec3  Min();
    glm::uvec3  Max();
    void        SetMin(glm::uvec3 min);
    void        SetMax(glm::uvec3 max);
    uint32_t    CullingFlags();
    void        SetCullingFlags(uint32_t cullingFlags);
    uint32_t    InstanceMask();
    void        SetInstanceMask(uint32_t instanceMask);
    uint32_t    NodeType() const;
    void        SetNodeType(uint32_t nodeType);
    uint32_t    NodeRangeLength() const;
    void        SetNodeRangeLength(uint32_t nodeBoundaries);
    bool        Valid();
    BoundingBox DecodeBounds(glm::vec3 origin, glm::uvec3 exponents);
};
#pragma pack(pop)

#define QUANTIZED_NODE_CHILD_INFO_OFFSET_MINX_MINY_CULLING_FLAGS 0        // offsetof(ChildInfo, minXMinYAndCullingFlags);
#define QUANTIZED_NODE_CHILD_INFO_OFFSET_MINZ_MAXX_INSTANCE_MASK 4        // offsetof(ChildInfo, minZMaxXAndInstanceMask);
#define QUANTIZED_NODE_CHILD_INFO_OFFSET_MAXY_MAXZ_NODE_TYPE_AND_RANGE 8  // offsetof(ChildInfo, maxYMaxZNodeTypeAndNodeRange);
#define QUANTIZED_NODE_CHILD_INFO_STRIDE 12                               // sizeof(ChildInfo) / sizeof(uint32_t);

static_assert(sizeof(ChildInfo) == 12);

//=====================================================================================================================
inline const glm::uvec3 ChildInfo::Invalid = glm::uvec3(0xFFFFFFFF, 0, 0);

//=====================================================================================================================
inline glm::uvec3 ChildInfo::BuildPacked(glm::uvec3 quantMin,
                                         glm::uvec3 quantMax,
                                         uint32_t   boxNodeFlags,
                                         uint32_t   instanceMask,
                                         uint32_t   nodeType,
                                         uint32_t   nodeRangeLength)
{
    glm::uvec3 childInfo = glm::uvec3(0, 0, 0);
    childInfo.x          = bitFieldInsert(childInfo.x, 0, 12, quantMin.x);
    childInfo.x          = bitFieldInsert(childInfo.x, 12, 12, quantMin.y);
    childInfo.x          = bitFieldInsert(childInfo.x, 24, 4, boxNodeFlags);
    childInfo.y          = bitFieldInsert(childInfo.y, 0, 12, quantMin.z);
    childInfo.y          = bitFieldInsert(childInfo.y, 12, 12, quantMax.x);
    childInfo.y          = bitFieldInsert(childInfo.y, 24, 8, instanceMask);
    childInfo.z          = bitFieldInsert(childInfo.z, 0, 12, quantMax.y);
    childInfo.z          = bitFieldInsert(childInfo.z, 12, 12, quantMax.z);
    childInfo.z          = bitFieldInsert(childInfo.z, 24, 4, nodeType);
    childInfo.z          = bitFieldInsert(childInfo.z, 28, 4, nodeRangeLength);

    return childInfo;
}

//=====================================================================================================================
inline void ChildInfo::Load(glm::uvec3 packedData)
{
    minXMinYAndCullingFlags      = packedData.x;
    minZMaxXAndInstanceMask      = packedData.y;
    maxYMaxZNodeTypeAndNodeRange = packedData.z;
}

//=====================================================================================================================
inline void ChildInfo::Init()
{
    minXMinYAndCullingFlags      = 0;
    minZMaxXAndInstanceMask      = 0;
    maxYMaxZNodeTypeAndNodeRange = 0;
}

//=====================================================================================================================
inline void ChildInfo::Invalidate()
{
    minXMinYAndCullingFlags      = 0xffffffff;
    maxYMaxZNodeTypeAndNodeRange = 0;
}

//=====================================================================================================================
inline glm::uvec3 ChildInfo::Min()
{
    glm::uvec3 min;
    min.x = bitFieldExtract(minXMinYAndCullingFlags, 0, 12);
    min.y = bitFieldExtract(minXMinYAndCullingFlags, 12, 12);
    min.z = bitFieldExtract(minZMaxXAndInstanceMask, 0, 12);
    return min;
}

//=====================================================================================================================
inline glm::uvec3 ChildInfo::Max()
{
    glm::uvec3 max;
    max.x = bitFieldExtract(minZMaxXAndInstanceMask, 12, 12);
    max.y = bitFieldExtract(maxYMaxZNodeTypeAndNodeRange, 0, 12);
    max.z = bitFieldExtract(maxYMaxZNodeTypeAndNodeRange, 12, 12);
    return max;
}

//=====================================================================================================================
inline void ChildInfo::SetMin(glm::uvec3 min)
{
    minXMinYAndCullingFlags = bitFieldInsert(minXMinYAndCullingFlags, 0, 12, min.x);
    minXMinYAndCullingFlags = bitFieldInsert(minXMinYAndCullingFlags, 12, 12, min.y);
    minZMaxXAndInstanceMask = bitFieldInsert(minZMaxXAndInstanceMask, 0, 12, min.z);
}

//=====================================================================================================================
inline void ChildInfo::SetMax(glm::uvec3 max)
{
    minZMaxXAndInstanceMask      = bitFieldInsert(minZMaxXAndInstanceMask, 12, 12, max.x);
    maxYMaxZNodeTypeAndNodeRange = bitFieldInsert(maxYMaxZNodeTypeAndNodeRange, 0, 12, max.y);
    maxYMaxZNodeTypeAndNodeRange = bitFieldInsert(maxYMaxZNodeTypeAndNodeRange, 12, 12, max.z);
}

//=====================================================================================================================
inline uint32_t ChildInfo::CullingFlags()
{
    return bitFieldExtract(minXMinYAndCullingFlags, 24, 4);
}

//=====================================================================================================================
inline void ChildInfo::SetCullingFlags(uint32_t cullingFlagsBits)
{
    minXMinYAndCullingFlags = bitFieldInsert(minXMinYAndCullingFlags, 24, 4, cullingFlagsBits);
}

//=====================================================================================================================
inline uint32_t ChildInfo::InstanceMask()
{
    return bitFieldExtract(minZMaxXAndInstanceMask, 24, 8);
}

//=====================================================================================================================
inline void ChildInfo::SetInstanceMask(uint32_t instanceMaskBits)
{
    minZMaxXAndInstanceMask = bitFieldInsert(minZMaxXAndInstanceMask, 24, 8, instanceMaskBits);
}

//=====================================================================================================================
inline uint32_t ChildInfo::NodeType() const
{
    return bitFieldExtract(maxYMaxZNodeTypeAndNodeRange, 24, 4);
}

//=====================================================================================================================
inline void ChildInfo::SetNodeType(uint32_t nodeTypeBits)
{
    maxYMaxZNodeTypeAndNodeRange = bitFieldInsert(maxYMaxZNodeTypeAndNodeRange, 24, 4, nodeTypeBits);
}

//=====================================================================================================================
inline uint32_t ChildInfo::NodeRangeLength() const
{
    return bitFieldExtract(maxYMaxZNodeTypeAndNodeRange, 28, 4);
}

//=====================================================================================================================
inline void ChildInfo::SetNodeRangeLength(uint32_t nodeBoundariesBits)
{
    maxYMaxZNodeTypeAndNodeRange = bitFieldInsert(maxYMaxZNodeTypeAndNodeRange, 28, 4, nodeBoundariesBits);
}

//=====================================================================================================================
inline bool ChildInfo::Valid()
{
    glm::uvec3 min = Min();
    glm::uvec3 max = Max();
    return !(min.y == 0xfff && min.x == 0xfff && max.y == 0 && max.z == 0);
}

//=====================================================================================================================
inline BoundingBox ChildInfo::DecodeBounds(glm::vec3 origin, glm::uvec3 exponents)
{
    glm::uvec3  qmin = Min();
    glm::uvec3  qmax = Max();
    BoundingBox result;
    result.min =
        glm::vec3(Dequantize(origin.x, exponents.x, qmin.x, 12), Dequantize(origin.y, exponents.y, qmin.y, 12), Dequantize(origin.z, exponents.z, qmin.z, 12));
    result.max = glm::vec3(Dequantize(origin.x, exponents.x, qmax.x + 1, 12),
                           Dequantize(origin.y, exponents.y, qmax.y + 1, 12),
                           Dequantize(origin.z, exponents.z, qmax.z + 1, 12));
    return result;
}

#ifndef _WIN32
#pragma GCC diagnostic pop
#endif

#endif  // RRA_BACKEND_CHILD_INFO_H_
