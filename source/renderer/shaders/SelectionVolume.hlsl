//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Shader for the selection volume.
//=============================================================================

#include "Common.hlsl"

#define DASH_SIZE 30.0f

cbuffer scene_ubo : register(b0)
{
    SceneUBO scene_ubo;
}

struct PSInput
{
    uint                   isTransform : METADATA0;
    nointerpolation float3 startPosition : METADATA1;
    float3                 currentPosition : METADATA2;
    float4                 position : SV_POSITION;
};

struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION;

    // instance step 1
    [[vk::location(1)]] float3   instanceMin : INSTANCE_MIN;
    [[vk::location(2)]] float3   instanceMax : INSTANCE_MAX;
    [[vk::location(3)]] uint     isTransform : METADATA0;
    [[vk::location(4)]] float4x4 transform : TRANSFORM;
};

PSInput VSMain(VSInput input)
{
    PSInput result;

    float  boxX            = lerp(input.instanceMin.x, input.instanceMax.x, max(input.position.x, 0.0f));
    float  boxY            = lerp(input.instanceMin.y, input.instanceMax.y, max(input.position.y, 0.0f));
    float  boxZ            = lerp(input.instanceMin.z, input.instanceMax.z, max(input.position.z, 0.0f));
    float4 vertex_position = mul(scene_ubo.view_projection, mul(input.transform, float4(boxX, boxY, boxZ, 1.0f)));

    result.isTransform     = input.isTransform;
    result.currentPosition = vertex_position.xyz / vertex_position.w;
    result.startPosition   = result.currentPosition;
    result.position        = vertex_position;

    return result;
}

float4 PSMain(PSInput input, float4 screenSpace
              : SV_Position)
    : SV_TARGET
{
    float4 color = scene_ubo.selected_node_color;

    if (input.isTransform)
    {
        float2 direction = (input.currentPosition.xy - input.startPosition.xy) * float2(scene_ubo.screen_width, scene_ubo.screen_height) / 2.0f;
        float  distance  = length(direction);

        if (frac(distance / DASH_SIZE) > 0.5f)
        {
            color.a = 0.0f;
        }
    }

    return color;
}
