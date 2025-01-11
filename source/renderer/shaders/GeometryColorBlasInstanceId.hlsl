//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Shader for coloring by instance ID.
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
    uint   blas_index : METADATA0;
    float3 wireframe_color : METADATA1;
    float  wireframe_size : METADATA2;
    float3 barycentric_coords : METADATA3;
};

struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION;

    [[vk::location(1)]] float4x4 instance_transform : TRANSFORM;
    // A float4x4 must skip 4 byte location indices 1->5.
    [[vk::location(5)]] uint   blas_index : METADATA0;
    [[vk::location(6)]] float4 wireframe_metadata : METADATA1;
};

PSInput VSMain(VSInput input, uint vertexId : SV_VertexID)
{
    PSInput result;

    result.position                         = mul(scene_ubo.view_projection, mul(input.instance_transform, float4(input.position, 1.0)));
    result.blas_index                       = input.blas_index;
    result.wireframe_color                  = input.wireframe_metadata.xyz;
    result.wireframe_size                   = input.wireframe_metadata.a;
    result.barycentric_coords               = float3(0.0, 0.0, 0.0);
    result.barycentric_coords[vertexId % 3] = 1.0;

    return result;
}

float4 PSMain(PSInput input)
    : SV_TARGET
{
    float  r                   = rand(input.blas_index);
    float  g                   = rand(input.blas_index * r);
    float  b                   = rand(input.blas_index * g);
    float4 coloring_mode_color = float4(r, g, b, 1.0f);

    if (input.wireframe_size == 0.0)
    {
        return coloring_mode_color;
    }

    float3 fw              = fwidth(input.barycentric_coords);
    float3 smooth_scaling  = smoothstep(fw * (input.wireframe_size / -10.0f), fw * (input.wireframe_size), input.barycentric_coords);
    float  wireframe_alpha = min(min(smooth_scaling.x, smooth_scaling.y), smooth_scaling.z);

    return lerp(float4(input.wireframe_color, 1.0f), coloring_mode_color, wireframe_alpha);
}
