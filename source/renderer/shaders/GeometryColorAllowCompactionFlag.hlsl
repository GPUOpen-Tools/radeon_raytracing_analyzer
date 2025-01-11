//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Shader for coloring by allow compaction flag.
//=============================================================================

#include "Common.hlsl"
#include "shared_definitions.hlsl"

cbuffer scene_ubo : register(b0)
{
    SceneUBO scene_ubo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

struct PSInput
{
    float4 position : SV_POSITION;
    float3 wireframe_color : COLOR0;
    float  wireframe_size : METADATA0;
    float3 barycentric_coords : METADATA1;
    float4 color : COLOR1;
};

struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION;

    [[vk::location(1)]] float4x4 instance_transform : TRANSFORM;
    // A float4x4 must skip 4 byte location indices 1->5.
    [[vk::location(5)]] uint   build_flags : METADATA0;
    [[vk::location(6)]] float4 wireframe_metadata : METADATA1;
};

#define ALLOW_COMPACTION_BIT 0x2

PSInput VSMain(VSInput input, uint vertexId : SV_VertexID)
{
    PSInput result;

    result.position                         = mul(scene_ubo.view_projection, mul(input.instance_transform, float4(input.position, 1.0)));
    result.wireframe_color                  = input.wireframe_metadata.xyz;
    result.wireframe_size                   = input.wireframe_metadata.a;
    result.barycentric_coords               = float3(0.0, 0.0, 0.0);
    result.barycentric_coords[vertexId % 3] = 1.0;

    result.color = scene_ubo.negative_color;

    if (input.build_flags & ALLOW_COMPACTION_BIT)
    {
        result.color = scene_ubo.positive_color;
    }

    return result;
}

float4 PSMain(PSInput input)
    : SV_TARGET
{
    if (input.wireframe_size == 0.0)
    {
        return input.color;
    }

    float3 fw              = fwidth(input.barycentric_coords);
    float3 smooth_scaling  = smoothstep(fw * (input.wireframe_size / -10.0f), fw * (input.wireframe_size), input.barycentric_coords);
    float  wireframe_alpha = min(min(smooth_scaling.x, smooth_scaling.y), smooth_scaling.z);

    return lerp(float4(input.wireframe_color, 1.0f), input.color, wireframe_alpha);
}