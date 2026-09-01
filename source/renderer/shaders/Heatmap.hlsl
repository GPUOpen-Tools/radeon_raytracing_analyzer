//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
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
