//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Shader for the ray inspector overlay icons,
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

struct IconDescription
{
    float2 position;
    uint   type;
    float  aspect_ratio;
    float4 color;
    float2 size;
    float  angle;
    float  placeholder;
};

StructuredBuffer<IconDescription> icons : register(t1);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : UV;
    uint   type : ICON_TYPE;
    float  angle : METADATA;
    float3 color : COLOR;
};

PSInput VSMain(uint vertex_index : SV_VertexID, uint icon_index : SV_InstanceID)
{
    PSInput result;

    IconDescription desc = icons[icon_index];
    float2          span = desc.size / 2.0;
    span.y *= desc.aspect_ratio;

    result.type  = desc.type;
    result.angle = desc.angle;

    if (vertex_index == 0)
    {
        result.position = float4(-span.x, span.y, 0.0, 0.0);
        result.uv       = float2(-1.0, 1.0);
    }
    else if (vertex_index == 1)
    {
        result.position = float4(span.x, span.y, 0.0, 0.0);
        result.uv       = float2(1.0, 1.0);
    }
    else if (vertex_index == 2)
    {
        result.position = float4(span.x, -span.y, 0.0, 0.0);
        result.uv       = float2(1.0, -1.0);
    }
    else if (vertex_index == 3)
    {
        result.position = float4(span.x, -span.y, 0.0, 0.0);
        result.uv       = float2(1.0, -1.0);
    }
    else if (vertex_index == 4)
    {
        result.position = float4(-span.x, -span.y, 0.0, 0.0);
        result.uv       = float2(-1.0, -1.0);
    }
    else if (vertex_index == 5)
    {
        result.position = float4(-span.x, span.y, 0.0, 0.0);
        result.uv       = float2(-1.0, 1.0);
    }

    float ct = cos(desc.angle);
    float st = sin(desc.angle);

    float2 tp         = result.position.xy;
    result.position.x = tp.x * ct - tp.y * st;
    result.position.y = tp.x * st + tp.y * ct;

    result.position += float4(desc.position, 0.0, 1.0);

    result.color = desc.color.xyz;

    return result;
}

float4 PSMain(PSInput input)
    : SV_TARGET
{
    float3 color = input.color;

    // Circle
    if (input.type == 0)
    {
        // Smooth out the edges.
        float len = length(input.uv);
        len -= 0.5;
        len *= 25.0;
        float alpha = lerp(1.0, 0.0, clamp(len, 0.0, 1.0));
        return float4(color, alpha);
    }

    // Triangle
    if (input.type == 1)
    {
        float len = abs(input.uv.x) * 1.0 + abs(input.uv.y) * 3.0;
        len -= 0.9;
        len *= 25.0;

        float alpha = lerp(1.0, 0.0, clamp(len, 0.0, 1.0));

        float flip = input.uv.x;
        if (flip < 0.0)
        {
            alpha = 0.0;
        }

        return float4(color, alpha);
    }

    // Plus sign
    if (input.type == 2)
    {
        float alpha = 0.0;
        float size  = 0.25;

        bool vertical_pass   = input.uv.x > -size && input.uv.x < size;
        bool horizontal_pass = input.uv.y > -size && input.uv.y < size;

        if (vertical_pass || horizontal_pass)
        {
            alpha = 1.0;
        }

        return float4(color, alpha);
    }

    // Error color.
    return float4(1.0, 0.0, 1.0, 1.0);
}
