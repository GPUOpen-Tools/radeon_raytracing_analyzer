//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief   Shader for the heatmap rendering.
//=============================================================================

sampler           heatmap_sampler : register(s1);
Texture1D<float4> heatmap_buffer : register(t1);

float4 heatmap_temp(float value)
{
    return heatmap_buffer.SampleLevel(heatmap_sampler, value, 0);
}
