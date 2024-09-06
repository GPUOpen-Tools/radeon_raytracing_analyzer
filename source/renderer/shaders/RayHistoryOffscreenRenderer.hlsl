//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief
//=============================================================================

#define KERNEL_SIZE 8

#include "Heatmap.hlsl"

enum RayHistoryColorMode
{
    kRayHistoryColorModeRayCount,
    kRayHistoryColorModeTraversalCount,
    kRayHistoryColorModeInstanceIntersectionCount,
    kRayHistoryColorModeAnyHitInvocationCount,
    kRayHistoryColorModeRayDirection,
};

enum SlicePlane
{
    kSlicePlaneXY,
    kSlicePlaneXZ,
    kSlicePlaneYZ,
};

struct DispatchIdData
{
    uint ray_count;
    uint traversal_count;
    uint instance_intersection_count;
    uint any_hit_invocation_count;
    uint first_ray_index;
};

struct DispatchRayData
{
    float3 direction;
    float  placeholder;
};

StructuredBuffer<DispatchIdData>  stats : register(t0);
RWStructuredBuffer<uint>          result_image : register(u2);
StructuredBuffer<DispatchRayData> rays : register(t3);


struct PushConstant
{
    uint color_mode;
    uint reshape_width;
    uint reshape_height;
    uint reshape_depth;
    uint slice_index;
    uint slice_plane;
    uint min_traversal_count_limit;
    uint max_traversal_count_limit;
    uint ray_index;
};

[[vk::push_constant]] PushConstant constants;

uint Float4ToRGBA(float4 color)
{
    uint rgba = 0;
    rgba |= (uint)(color.r * 255) << 0;
    rgba |= (uint)(color.g * 255) << 8;
    rgba |= (uint)(color.b * 255) << 16;
    rgba |= (uint)(color.a * 255) << 24;
    return rgba;
}

[numthreads(KERNEL_SIZE, KERNEL_SIZE, 1)] void CSMain(uint3 Gid
                                                      : SV_GroupID, uint3          DTid
                                                      : SV_DispatchThreadID, uint3 GTid
                                                      : SV_GroupThreadID, uint     GI
                                                      : SV_GroupIndex) {
    // Get the pixel indices.
    int2 pixel_xy = int2(Gid.x * KERNEL_SIZE + GTid.x, Gid.y * KERNEL_SIZE + GTid.y);

    uint reshaped_x   = 0;
    uint reshaped_y   = 0;
    uint reshaped_z   = 0;
    uint image_width  = 0;
    uint image_height = 0;
    switch (constants.slice_plane)
    {
    case kSlicePlaneXY:
        reshaped_x   = pixel_xy.x;
        reshaped_y   = pixel_xy.y;
        reshaped_z   = constants.slice_index;
        image_width  = constants.reshape_width;
        image_height = constants.reshape_height;
        break;
    case kSlicePlaneXZ:
        reshaped_x   = pixel_xy.x;
        reshaped_y   = constants.slice_index;
        reshaped_z   = pixel_xy.y;
        image_width  = constants.reshape_width;
        image_height = constants.reshape_depth;
        break;
    case kSlicePlaneYZ:
        reshaped_x   = constants.slice_index;
        reshaped_y   = pixel_xy.y;
        reshaped_z   = pixel_xy.x;
        image_width  = constants.reshape_depth;
        image_height = constants.reshape_height;
        break;
    }

    if (pixel_xy.x >= image_width || pixel_xy.y >= image_height)
    {
        return;
    }

    uint stat_idx         = reshaped_x + (reshaped_y * constants.reshape_width) + (reshaped_z * constants.reshape_width * constants.reshape_height);
    uint color_buffer_idx = pixel_xy.x + (pixel_xy.y * image_width);

    // Linear equation satisfying f(max) = 1 and f(min) = 0.
    float  a           = 1.0 / (constants.max_traversal_count_limit - constants.min_traversal_count_limit);
    float  b           = 1.0 - (constants.max_traversal_count_limit / (float)(constants.max_traversal_count_limit - constants.min_traversal_count_limit));
    float4 final_color = float4(0.0, 0.0, 0.0, 1.0);

    switch (constants.color_mode)
    {
    case kRayHistoryColorModeRayCount:
        final_color = heatmap_temp(a * stats[stat_idx].ray_count + b);
        break;
    case kRayHistoryColorModeTraversalCount:
        final_color = heatmap_temp(a * stats[stat_idx].traversal_count + b);
        break;
    case kRayHistoryColorModeInstanceIntersectionCount:
        final_color = heatmap_temp(a * stats[stat_idx].instance_intersection_count + b);
        break;
    case kRayHistoryColorModeAnyHitInvocationCount:
        final_color = heatmap_temp(a * stats[stat_idx].any_hit_invocation_count + b);
        break;
    case kRayHistoryColorModeRayDirection:
        if(constants.ray_index >= stats[stat_idx].ray_count) {
            final_color = float4(0.0, 0.0, 0.0, 1.0);
        } else {
            final_color = float4((normalize(rays[stats[stat_idx].first_ray_index + constants.ray_index].direction) + 1.0) * 0.5, 1.0);
        }
        break;
    }

    result_image[color_buffer_idx] = Float4ToRGBA(final_color);
}
