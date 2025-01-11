//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Shader to draw the BVH wireframe.
//=============================================================================

#include "Common.hlsl"

cbuffer scene_ubo : register(b0)
{
    SceneUBO scene_ubo;
}

sampler           heatmap_sampler : register(s1);
Texture1D<float4> heatmap_buffer : register(t1);

float4 heatmap_temp(float value)
{
    return heatmap_buffer.SampleLevel(heatmap_sampler, value, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#define COLORING_MODE_NODE_TYPE (0)
#define COLORING_MODE_TREE_DEPTH (1)

/// Look at scene_node.cpp AppendBoundingVolumesTo function.
#define NODE_TYPE_SELECTED (0)
#define NODE_TYPE_BOX16 (1)
#define NODE_TYPE_BOX32 (2)
#define NODE_TYPE_INSTANCE (3)
#define NODE_TYPE_PROCEDURAL (4)
#define NODE_TYPE_TRIANGLE (5)

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION;

    // instance step 1
    [[vk::location(1)]] float4 instanceMin : INSTANCE_MIN;
    [[vk::location(2)]] float3 instanceMax : INSTANCE_MAX;
    [[vk::location(3)]] float4 color : METADATA;
    [[vk::location(4)]] float3x3 rotation : ROTATION;
};

PSInput VSMain(VSInput input, uint currentInstance : SV_InstanceID)
{
    PSInput result;

    float currentBvhLevel = input.instanceMin.w;
    if (scene_ubo.depth_range_lower_bound <= currentBvhLevel && currentBvhLevel <= scene_ubo.depth_range_upper_bound)
    {
        float boxX      = lerp(input.instanceMin.x, input.instanceMax.x, max(input.position.x, 0.0f));
        float boxY      = lerp(input.instanceMin.y, input.instanceMax.y, max(input.position.y, 0.0f));
        float boxZ      = lerp(input.instanceMin.z, input.instanceMax.z, max(input.position.z, 0.0f));
        
        float3 rotated_box = mul(transpose(input.rotation), float3(boxX, boxY, boxZ));
        result.position = mul(scene_ubo.view_projection, float4(rotated_box, 1.0f));
    }
    else
    {
        result.position = float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    switch (scene_ubo.bvh_coloring_mode)
    {
    case COLORING_MODE_NODE_TYPE:
        switch (int(ceil(input.color.x)))
        {
        case NODE_TYPE_SELECTED:
            result.color = scene_ubo.selected_node_color;
            break;
        case NODE_TYPE_BOX16:
            result.color = scene_ubo.box16_node_color;
            break;
        case NODE_TYPE_BOX32:
            result.color = scene_ubo.box32_node_color;
            break;
        case NODE_TYPE_INSTANCE:
            result.color = scene_ubo.instance_node_color;
            break;
        case NODE_TYPE_PROCEDURAL:
            result.color = scene_ubo.procedural_node_color;
            break;
        case NODE_TYPE_TRIANGLE:
            result.color = scene_ubo.triangle_node_color;
            break;
        default:
            result.color = float4(0.0f, 0.0f, 0.0f, 1.0f);
            break;
        }
        break;
    case COLORING_MODE_TREE_DEPTH:
        if (scene_ubo.depth_range_lower_bound == scene_ubo.depth_range_upper_bound)
        {
            result.color = heatmap_temp(input.color.y / float(scene_ubo.max_node_depth));
        }
        else
        {
            result.color =
                heatmap_temp((input.color.y - scene_ubo.depth_range_lower_bound) / (scene_ubo.depth_range_upper_bound - scene_ubo.depth_range_lower_bound));
        }
        break;
    default:
        result.color = float4(0.0f, 0.0f, 0.0f, 1.0f);
        break;
    }

    return result;
}

float4 PSMain(PSInput input)
    : SV_TARGET
{
    return input.color;
}
