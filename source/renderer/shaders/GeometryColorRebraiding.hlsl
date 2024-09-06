//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief
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
    float3 wireframe_color : METADATA1;
    float  wireframe_size : METADATA2;
    float3 barycentric_coords : METADATA3;
    float4 color : COLOR1;
};

struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION;

    [[vk::location(1)]] float4x4 instance_transform : TRANSFORM;
    // A float4x4 must skip 4 byte location indices 1->5.
    [[vk::location(5)]] uint   rebraided : METADATA0;
    [[vk::location(6)]] float4 wireframe_metadata : METADATA1;
};

PSInput VSMain(VSInput input, uint vertexId : SV_VertexID)
{
    PSInput result;

    result.position                         = mul(scene_ubo.view_projection, mul(input.instance_transform, float4(input.position, 1.0)));
    result.wireframe_color                  = input.wireframe_metadata.xyz;
    result.wireframe_size                   = input.wireframe_metadata.a;
    result.barycentric_coords               = float3(0.0, 0.0, 0.0);
    result.barycentric_coords[vertexId % 3] = 1.0;
    result.color                            = input.rebraided ? scene_ubo.positive_color : scene_ubo.negative_color;

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
