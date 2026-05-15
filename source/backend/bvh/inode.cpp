//=============================================================================
// Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the node class interface.
//=============================================================================

#include "bvh/inode.h"

#include "bvh/rtip31/internal_node.h"
#include "bvh/rtip_common/encoded_bottom_level_bvh.h"

namespace rta
{
    glm::mat3 INode::GetNodeBoundingVolumeOrientation(uint32_t node_id, const IBvh* bvh) const
    {
        uint32_t obb_index = GetNodeObbIndex(node_id, bvh);
        return obb_index >= ObbDisabled ? glm::mat3(1.0f) : DecodeRotationMatrix(obb_index);
    }

    glm::mat3 INode::DecodeRotationMatrix(uint32_t id) const
    {
        constexpr uint32_t NUM_OBB_TRANSFORMS{104};
        assert(id < NUM_OBB_TRANSFORMS);

        // The stage 1 lookup table containing the indices of the floats constituting the rotation
        // matrices.
        // Each value is 6-bits, with the lower 5 bits indicating the float and highest bit
        // indicating the sign of the float.
        static const uint32_t s1_lut[NUM_OBB_TRANSFORMS][9] = {
            {25, 0, 0, 0, 22, 43, 0, 11, 22},     {25, 0, 0, 0, 22, 11, 0, 43, 22},     {25, 0, 0, 0, 17, 49, 0, 17, 17},
            {25, 0, 0, 0, 17, 17, 0, 49, 17},     {25, 0, 0, 0, 11, 54, 0, 22, 11},     {25, 0, 0, 0, 11, 22, 0, 54, 11},
            {25, 0, 0, 0, 0, 57, 0, 25, 0},       {25, 0, 0, 0, 0, 25, 0, 57, 0},       {22, 0, 11, 0, 25, 0, 43, 0, 22},
            {22, 0, 43, 0, 25, 0, 11, 0, 22},     {17, 0, 17, 0, 25, 0, 49, 0, 17},     {17, 0, 49, 0, 25, 0, 17, 0, 17},
            {11, 0, 22, 0, 25, 0, 54, 0, 11},     {11, 0, 54, 0, 25, 0, 22, 0, 11},     {0, 0, 25, 0, 25, 0, 57, 0, 0},
            {0, 0, 57, 0, 25, 0, 25, 0, 0},       {22, 43, 0, 11, 22, 0, 0, 0, 25},     {22, 11, 0, 43, 22, 0, 0, 0, 25},
            {17, 49, 0, 17, 17, 0, 0, 0, 25},     {17, 17, 0, 49, 17, 0, 0, 0, 25},     {11, 54, 0, 22, 11, 0, 0, 0, 25},
            {11, 22, 0, 54, 11, 0, 0, 0, 25},     {0, 57, 0, 25, 0, 0, 0, 0, 25},       {0, 25, 0, 57, 0, 0, 0, 0, 25},
            {22, 38, 6, 6, 24, 1, 38, 1, 24},     {22, 6, 38, 38, 24, 1, 6, 1, 24},     {17, 44, 12, 12, 20, 2, 44, 2, 20},
            {17, 12, 44, 44, 20, 2, 12, 2, 20},   {11, 47, 15, 15, 16, 7, 47, 7, 16},   {11, 15, 47, 47, 16, 7, 15, 7, 16},
            {0, 49, 17, 17, 12, 12, 49, 12, 12},  {0, 17, 49, 49, 12, 12, 17, 12, 12},  {22, 38, 38, 6, 24, 33, 6, 33, 24},
            {22, 6, 6, 38, 24, 33, 38, 33, 24},   {17, 44, 44, 12, 20, 34, 12, 34, 20}, {17, 12, 12, 44, 20, 34, 44, 34, 20},
            {11, 47, 47, 15, 16, 39, 15, 39, 16}, {11, 15, 15, 47, 16, 39, 47, 39, 16}, {0, 49, 49, 17, 12, 44, 17, 44, 12},
            {0, 17, 17, 49, 12, 44, 49, 44, 12},  {24, 38, 1, 6, 22, 38, 1, 6, 24},     {24, 6, 1, 38, 22, 6, 1, 38, 24},
            {20, 44, 2, 12, 17, 44, 2, 12, 20},   {20, 12, 2, 44, 17, 12, 2, 44, 20},   {16, 47, 7, 15, 11, 47, 7, 15, 16},
            {16, 15, 7, 47, 11, 15, 7, 47, 16},   {12, 49, 12, 17, 0, 49, 12, 17, 12},  {12, 17, 12, 49, 0, 17, 12, 49, 12},
            {24, 6, 33, 38, 22, 38, 33, 6, 24},   {24, 38, 33, 6, 22, 6, 33, 38, 24},   {20, 12, 34, 44, 17, 44, 34, 12, 20},
            {20, 44, 34, 12, 17, 12, 34, 44, 20}, {16, 15, 39, 47, 11, 47, 39, 15, 16}, {16, 47, 39, 15, 11, 15, 39, 47, 16},
            {12, 17, 44, 49, 0, 49, 44, 17, 12},  {12, 49, 44, 17, 0, 17, 44, 49, 12},  {24, 1, 6, 1, 24, 38, 38, 6, 22},
            {24, 1, 38, 1, 24, 6, 6, 38, 22},     {20, 2, 12, 2, 20, 44, 44, 12, 17},   {20, 2, 44, 2, 20, 12, 12, 44, 17},
            {16, 7, 15, 7, 16, 47, 47, 15, 11},   {16, 7, 47, 7, 16, 15, 15, 47, 11},   {12, 12, 17, 12, 12, 49, 49, 17, 0},
            {12, 12, 49, 12, 12, 17, 17, 49, 0},  {24, 33, 6, 33, 24, 6, 38, 38, 22},   {24, 33, 38, 33, 24, 38, 6, 6, 22},
            {20, 34, 12, 34, 20, 12, 44, 44, 17}, {20, 34, 44, 34, 20, 44, 12, 12, 17}, {16, 39, 15, 39, 16, 15, 47, 47, 11},
            {16, 39, 47, 39, 16, 47, 15, 15, 11}, {12, 44, 17, 44, 12, 17, 49, 49, 0},  {12, 44, 49, 44, 12, 49, 17, 17, 0},
            {23, 35, 5, 5, 23, 35, 35, 5, 23},    {23, 5, 35, 35, 23, 5, 5, 35, 23},    {19, 40, 13, 13, 19, 40, 40, 13, 19},
            {19, 13, 40, 40, 19, 13, 13, 40, 19}, {14, 41, 18, 18, 14, 41, 41, 18, 14}, {14, 18, 41, 41, 14, 18, 18, 41, 14},
            {10, 36, 21, 21, 10, 36, 36, 21, 10}, {10, 21, 36, 36, 10, 21, 21, 36, 10}, {23, 37, 3, 3, 23, 5, 37, 35, 23},
            {23, 3, 37, 37, 23, 35, 3, 5, 23},    {19, 45, 8, 8, 19, 13, 45, 40, 19},   {19, 8, 45, 45, 19, 40, 8, 13, 19},
            {14, 50, 9, 9, 14, 18, 50, 41, 14},   {14, 9, 50, 50, 14, 41, 9, 18, 14},   {10, 53, 4, 4, 10, 21, 53, 36, 10},
            {10, 4, 53, 53, 10, 36, 4, 21, 10},   {23, 37, 35, 3, 23, 37, 5, 3, 23},    {23, 3, 5, 37, 23, 3, 35, 37, 23},
            {19, 45, 40, 8, 19, 45, 13, 8, 19},   {19, 8, 13, 45, 19, 8, 40, 45, 19},   {14, 50, 41, 9, 14, 50, 18, 9, 14},
            {14, 9, 18, 50, 14, 9, 41, 50, 14},   {10, 53, 36, 4, 10, 53, 21, 4, 10},   {10, 4, 21, 53, 10, 4, 36, 53, 10},
            {23, 35, 37, 5, 23, 3, 3, 37, 23},    {23, 5, 3, 35, 23, 37, 37, 3, 23},    {19, 40, 45, 13, 19, 8, 8, 45, 19},
            {19, 13, 8, 40, 19, 45, 45, 8, 19},   {14, 41, 50, 18, 14, 9, 9, 50, 14},   {14, 18, 9, 41, 14, 50, 50, 9, 14},
            {10, 36, 53, 21, 10, 4, 4, 53, 10},   {10, 21, 4, 36, 10, 53, 53, 4, 10}};
        // The stage 2 lookup table containing the floating point data.
        // Each value is 30-bits as the sign bit and highest exponent bit are unused.
        static const uint32_t s2_lut[] = {
            0x00000000, 0x3d1be50c, 0x3e15f61a, 0x3e484336, 0x3e79df93, 0x3e7c3a3a, 0x3e8a8bd4, 0x3e9e0875, 0x3e9f0938,
            0x3ea7bf1b, 0x3eaaaaab, 0x3ec3ef15, 0x3f000000, 0x3f01814f, 0x3f16a507, 0x3f273d75, 0x3f30fbc5, 0x3f3504f3,
            0x3f3d3a87, 0x3f4e034d, 0x3f5a827a, 0x3f692290, 0x3f6c835e, 0x3f73023f, 0x3f7641af, 0x3f800000,
        };

        glm::mat3 out = {};
        uint32_t  float_id;

        // Loop over all 9 values in the relevant S1 LUT entry, using the lower 5-bits to index
        // the float from the S2 LUT and the highest bit to insert the sign bit.
        for (uint32_t j = 0; j < 3; ++j)
        {
            for (uint32_t i = 0; i < 3; ++i)
            {
                float_id   = s1_lut[id][(j * 3) + i];
                uint32_t u = s2_lut[float_id & 0x1f] | ((float_id >> 5) ? 0x80000000 : 0);
                std::memcpy(&out[j][i], &u, sizeof(u));
            }
        }

        return out;
    }

    bool INode::GetIsInstanceNode(uint32_t node_id, const IBvh* bvh) const
    {
        RRA_UNUSED(bvh);
        const dxr::amd::NodeType node_type = reinterpret_cast<dxr::amd::NodePointer*>(&node_id)->GetType();
        return node_type == dxr::amd::NodeType::kAmdNodeInstance;
    }

    bool INode::GetIsBoxNode(uint32_t node_id, const IBvh* bvh) const
    {
        RRA_UNUSED(bvh);
        const dxr::amd::NodeType node_type = reinterpret_cast<dxr::amd::NodePointer*>(&node_id)->GetType();
        return node_type == dxr::amd::NodeType::kAmdNodeBoxFp32 || node_type == dxr::amd::NodeType::kAmdNodeBoxFp16;
    }

    bool INode::GetIsBox32Node(uint32_t node_id, const IBvh* bvh) const
    {
        RRA_UNUSED(bvh);
        const dxr::amd::NodeType node_type = reinterpret_cast<dxr::amd::NodePointer*>(&node_id)->GetType();
        return node_type == dxr::amd::NodeType::kAmdNodeBoxFp32;
    }

    bool INode::GetIsBox16Node(uint32_t node_id, const IBvh* bvh) const
    {
        RRA_UNUSED(bvh);
        const dxr::amd::NodeType node_type = reinterpret_cast<dxr::amd::NodePointer*>(&node_id)->GetType();
        return node_type == dxr::amd::NodeType::kAmdNodeBoxFp16;
    }

    bool INode::GetHasChildren(uint32_t node_id, const IBvh* bvh) const
    {
        return GetIsBoxNode(node_id, bvh);
    }

    bool INode::GetIsProceduralNode(uint32_t node_id, const IBvh* bvh) const
    {
        RRA_UNUSED(bvh);
        const dxr::amd::NodeType node_type = reinterpret_cast<dxr::amd::NodePointer*>(&node_id)->GetType();
        return node_type == dxr::amd::NodeType::kAmdNodeProcedural;
    }

    bool INode::GetIsTriangleNode(uint32_t node_id, const IBvh* bvh) const
    {
        const rta::EncodedBottomLevelBvh* blas = dynamic_cast<const rta::EncodedBottomLevelBvh*>(bvh);
        if (blas && blas->IsProcedural())
        {
            return false;
        }

        const dxr::amd::NodePointer* node = reinterpret_cast<dxr::amd::NodePointer*>(&node_id);

        switch ((uint32_t)node->GetType())
        {
        case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle0:
        case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle1:
        case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle2:
        case (uint32_t)dxr::amd::NodeType::kAmdNodeTriangle3:
        case NODE_TYPE_TRIANGLE_4:
        case NODE_TYPE_TRIANGLE_5:
        case NODE_TYPE_TRIANGLE_6:
        case NODE_TYPE_TRIANGLE_7:
            // Return true if the node is any of the valid triangle node types.
            return true;
        default:
            // If it's not a triangle node type, return false.
            return false;
        }
    }

}  // namespace rta

