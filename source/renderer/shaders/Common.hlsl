//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Common shader functions.
//=============================================================================

#ifndef RRA_SHADERS_COMMON
#define RRA_SHADERS_COMMON

#include "shared_definitions.hlsl"
#include "shared_impl.hlsl"

#define UINT_MAX 0xFFFFFFFF

#ifndef INFINITY
#define INFINITY -log(0.0)
#endif

//=====================================================================================================================
// HLSL emulation of min3.
static float min3(float3 val)
{
    return min(min(val.x, val.y), val.z);
}

//=====================================================================================================================
// HLSL emulation of max3.
static float max3(float3 val)
{
    return max(max(val.x, val.y), val.z);
}

//=====================================================================================================================
// Intersect rays vs bbox and return intersection span.
static float4 fast_intersect_bbox(float3 ray_origin, float3 ray_inv_dir, float3 box_min, float3 box_max, float t_max)
{
    const float3 box_min_rel = box_min - ray_origin;
    const float3 box_max_rel = box_max - ray_origin;

    const float3 t_plane_min = box_min_rel * ray_inv_dir;
    const float3 t_plane_max = box_max_rel * ray_inv_dir;

    float3 min_interval, max_interval;

    min_interval.x = ray_inv_dir.x >= 0.0f ? t_plane_min.x : t_plane_max.x;
    max_interval.x = ray_inv_dir.x >= 0.0f ? t_plane_max.x : t_plane_min.x;

    min_interval.y = ray_inv_dir.y >= 0.0f ? t_plane_min.y : t_plane_max.y;
    max_interval.y = ray_inv_dir.y >= 0.0f ? t_plane_max.y : t_plane_min.y;

    min_interval.z = ray_inv_dir.z >= 0.0f ? t_plane_min.z : t_plane_max.z;
    max_interval.z = ray_inv_dir.z >= 0.0f ? t_plane_max.z : t_plane_min.z;

    // intersection interval before clamping
    float min_of_intervals_t = max3(min_interval);
    float max_of_intervals_t = min3(max_interval);

    // intersection interval after clamping
    float min_t = max(min_of_intervals_t, 0.0f);
    float max_t = min(max_of_intervals_t, t_max);

    if (isnan(min_of_intervals_t) || isnan(max_of_intervals_t))
    {
        min_t = INFINITY;
        max_t = -INFINITY;
    }

    // NaNs for values used in the closest midpoint sort algorithm are overridden to
    // maintain consistency with the other sorting heuristic.
    if (isnan(min_of_intervals_t))
    {
        min_of_intervals_t = 0;
    }

    if (isnan(max_of_intervals_t))
    {
        max_of_intervals_t = INFINITY;
    }

    return float4(min_t, max_t, min_of_intervals_t, max_of_intervals_t);
}

//=====================================================================================================================
// Intersect ray against a triangle and return whether the triangle hit is accepted or not. If hit is accepted
// hit attributes (closest distance, barycentrics and hit kind) are updated
static float4 fast_intersect_triangle(float3 origin, float3 direction, float3 v1, float3 v2, float3 v3)
{
    // Determine edge vectors for clockwise triangle vertices
    float3 e1 = v2 - v1;
    float3 e2 = v3 - v1;
    float3 e3 = origin - v1;

    float4 result;

    const float3 s1 = cross(direction, e2);
    const float3 s2 = cross(e3, e1);

    result.x = dot(e2, s2);
    result.y = dot(s1, e1);
    result.z = dot(e3, s1);
    result.w = dot(direction, s2);

    float t = result.x / result.y;
    float u = result.z / result.y;
    float v = result.w / result.y;

    // Barycentric coordinate U is outside range
    int triangle_missed = ((u < 0.f) || (u > 1.f));
    triangle_missed |= ((v < 0.f) || (u + v > 1.f));
    triangle_missed |= (t < 0.f);

    const float inf = INFINITY;
    result.x        = triangle_missed ? inf : result.x;
    result.y        = triangle_missed ? 1.0f : result.y;

    result.z = u;
    result.w = v;

    return result;
}

#endif
