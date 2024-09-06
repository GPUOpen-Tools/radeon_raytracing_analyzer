//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief
//=============================================================================

struct CheckerColors
{
    float4 checkerColor1;
    float4 checkerColor2;
};

[[vk::push_constant]] CheckerColors checkerColors;

struct PSInput
{
    float4 position : SV_POSITION;
};

PSInput VSMain(uint index : SV_VertexID)
{
    PSInput result;
    float   x         = float(index / 2) * 4.0 - 1.0f;
    float   y         = float(index % 2) * 4.0 - 1.0f;
    result.position   = float4(x, y, 0.0f, 1.0f);
    return result;
}

float4 PSMain(PSInput input)
    : SV_TARGET
{
    float2 xy      = floor(input.position / 64.0f).xy;
    float  checker = fmod(xy.x + xy.y, 2.0f);
    return lerp(checkerColors.checkerColor1, checkerColors.checkerColor2, checker);
}
