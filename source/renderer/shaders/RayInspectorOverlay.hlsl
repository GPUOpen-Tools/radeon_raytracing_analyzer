//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Shader for the ray inspector overlay.
//=============================================================================

#include "Common.hlsl"
#include "shared_definitions.hlsl"

cbuffer scene_ubo : register(b0)
{
    SceneUBO scene_ubo;
}

struct PushConstant
{
    uint ray_count;
};

[[vk::push_constant]] PushConstant push_constant;

struct RayInspectorRay
{
    float3 origin;
    float  tmin;
    float3 direction;
    float  tmax;
    float4 color;
    int    is_outline;
    float  hit_distance;
    int    ray_flags;
    float4 placeholder;
};

StructuredBuffer<RayInspectorRay> rays : register(t1);

#ifdef PIXEL_SHADER
[[vk::input_attachment_index(0)]] SubpassInputMS input_depth : register(t2);
#endif

struct PSInput
{
    float4 position : SV_POSITION;
    float  alpha : METADATA1;
    float  normalized_hit_distance : METADATA2;
    float3 origin : METADATA3;
    float3 end_point : METADATA4;
    float4 color : METADATA5;
};

#ifdef VERTEX_SHADER
PSInput VSMain(uint vertex_index : SV_VertexID, uint ray_index : SV_InstanceID)
{
    PSInput result;

    RayInspectorRay ray = rays[ray_index];

    float tmax                     = min(ray.tmax, 1e32);
    result.normalized_hit_distance = ray.hit_distance / tmax;

    float4 mapped_origin = mul(scene_ubo.view_projection, float4(ray.origin.xyz, 1.0));
    result.origin        = mapped_origin.xyz / mapped_origin.w;
    result.origin.y      = -result.origin.y;

    float3 end_point        = ray.origin + ray.direction * tmax;
    float4 mapped_end_point = mul(scene_ubo.view_projection, float4(end_point, 1.0));
    result.end_point        = mapped_end_point.xyz / mapped_end_point.w;
    result.end_point.y      = -result.end_point.y;

    if (vertex_index % 2 == 0)
    {
        result.position = mapped_origin;
        result.alpha    = 0.0;
    }
    else
    {
        result.position = mul(scene_ubo.view_projection, float4(end_point, 1.0));
        result.alpha    = 1.0;
    }

    result.color = ray.color;

    return result;
}

#elif defined(PIXEL_SHADER)
#define DASH_SIZE 0.01f
float4 PSMain(PSInput input)
    : SV_TARGET
{
    float alpha = 1.0;

    float4 color        = input.color;
    float2 unmapped_pos = ((input.position.xy / float2(scene_ubo.screen_width, scene_ubo.screen_height)) - 0.5) * 2.0;

    float2 diff     = (unmapped_pos - input.origin.xy);
    float  distance = length(diff);

    alpha = frac(distance / (2.0 * DASH_SIZE)) >= 0.5 ? 1.0 : 0.0;

    float depth = input_depth.SubpassLoad(0).r;

    // If ray is not occluded, make it drawn as a solid line.
    if (input.position.z < depth)
    {
        alpha = 1.0;
    }

    return float4(color.xyz, alpha);
}
#endif
