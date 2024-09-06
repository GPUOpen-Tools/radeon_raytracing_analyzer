//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief
//=============================================================================

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float  fadeFactor : FADE_FACTOR;
};

struct VSInput
{
    // Vertex attributes
    [[vk::location(0)]] float3 position : SV_POSITION;

    // Instance attributes
    [[vk::location(1)]] float4   color : METADATA0;
    [[vk::location(2)]] float    fadeFactor : METADATA1;
    [[vk::location(3)]] float4x4 transform : TRANSFORM;
};

PSInput VSMain(VSInput input)
{
    PSInput result;

    result.position   = mul(float4(input.position, 1.0), input.transform);
    result.color      = input.color;
    result.fadeFactor = input.fadeFactor;

    return result;
}

float4 PSMain(PSInput input)
    : SV_TARGET
{
    float lineHeight = 0.05;
    float darkestMul = 0.8;

    // Get 1.0 when gizmo is closest to camera, and darkestMul when it's furthest
    float depthBlend = ((1.0 - input.position.z) - 0.5 + lineHeight) * ((1.0 - darkestMul) / (2.0 * lineHeight)) + darkestMul;
    depthBlend       = lerp(1.0, depthBlend, input.fadeFactor);

    input.color.xyz *= depthBlend;
    return input.color;
}
