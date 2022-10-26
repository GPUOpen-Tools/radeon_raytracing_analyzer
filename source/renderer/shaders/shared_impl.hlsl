//=============================================================================
// Copyright (c) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for shared shader functions.
///
/// These functions will be shared between shaders and included in C++ code
/// to avoid code duplication.
///
/// When using in c++ shared.h file must be included before this file.
//=============================================================================

float rand(uint seed)
{
    seed ^= 2747636419u;
    seed *= 2654435769u;
    seed ^= seed >> 16;
    seed *= 2654435769u;
    seed ^= seed >> 16;
    seed *= 2654435769u;

    return float(seed) / 4294967295.0f;  // 2^32-1
}

float4 heatmap(float level)
{
    level *= (3.14159265f * 0.5f);
    float r = float(sin(level));
    float g = float(sin(level * 2.0f));
    float b = float(cos(level));

    return float4(r, g, b, 1.0f);
}
