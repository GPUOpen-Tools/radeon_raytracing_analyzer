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
    float3 normal : NORMAL;
    float4 light_dir : LIGHT_DIR;
    uint   triangle_selected : METADATA0;
    float3 wireframe_color : METADATA1;
    float  wireframe_size : METADATA2;
    float3 barycentric_coords : METADATA3;
};

struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float2 normal : NORMAL;
    [[vk::location(2)]] float  triangle_sah_and_selected : METADATA0;

    [[vk::location(3)]] float4x4 instance_transform : METADATA1;
    // A float4x4 must skip 4 byte location indices 3->7.
    [[vk::location(7)]] float4 wireframe_metadata : METADATA2;
};

PSInput VSMain(VSInput input, uint vertexId : SV_VertexID)
{
    PSInput result;

    // calculate position
    result.position = mul(scene_ubo.view_projection, mul(input.instance_transform, float4(input.position, 1.0)));

    // work out lighting stuff
    float4 world_pos = mul(input.instance_transform, float4(input.position, 1.0));

    // The sqrt erases the sign form the z component, so we encode the sign of z in the x component
    // by adding kNormalSignIndicatorOffset which pushes it over the valid maximum of x (1.0). So when
    // x passes this threshold we know z should be negative, and we can subtract kNormalSignIndicatorOffset
    // from x to restore its original value.
    float sign_multiplier = 1.0;
    if (input.normal.x > 1.001)
    {
        input.normal.x -= kNormalSignIndicatorOffset;
        sign_multiplier = -1.0;
    }
    // Infer the z component of the normal vector. Maximum prevents floating point errors resulting in taking sqrt of negative value.
    float  z      = sqrt(max(1.0 - input.normal.x * input.normal.x - input.normal.y * input.normal.y, 0.0));
    float3 normal = float3(input.normal.x, input.normal.y, sign_multiplier * z);

    result.normal    = normalize(mul(float4(normal, 1.0), input.instance_transform).xyz);
    result.light_dir = normalize(scene_ubo.light_position - world_pos);

    result.triangle_selected                = (uint)(input.triangle_sah_and_selected > 0);
    result.wireframe_color                  = input.wireframe_metadata.xyz;
    result.wireframe_size                   = input.wireframe_metadata.a;
    result.barycentric_coords               = float3(0.0, 0.0, 0.0);
    result.barycentric_coords[vertexId % 3] = 1.0;

    return result;
}

float4 PSMain(PSInput input)
    : SV_TARGET
{
    float3 normal              = normalize(input.normal);
    float3 light               = normalize(input.light_dir).xyz;
    float4 coloring_mode_color = abs(float4(0.0f, 1.0f, 0.0f, 1.0f) * dot(normal, light)) + float4(0.2f, 0.2f, 0.2f, 0.0f);

    if (input.triangle_selected)
    {
        coloring_mode_color = scene_ubo.selected_geometry_color;
    }

    if (input.wireframe_size == 0.0)
    {
        return coloring_mode_color;
    }

    float3 fw              = fwidth(input.barycentric_coords);
    float3 smooth_scaling  = smoothstep(fw * (input.wireframe_size / -10.0f), fw * (input.wireframe_size), input.barycentric_coords);
    float  wireframe_alpha = min(min(smooth_scaling.x, smooth_scaling.y), smooth_scaling.z);

    return lerp(float4(input.wireframe_color, 1.0f), coloring_mode_color, wireframe_alpha);
}
