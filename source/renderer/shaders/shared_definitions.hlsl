//=============================================================================
// Copyright (c) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Decleration for shared shader functions.
///
/// These functions and structures will be shared between shaders and included in C++ code
/// to avoid code duplication.
///
/// When using from C++, this file should be #included as usual, but a
/// namespace should be placed around the include so that the function
/// names defined here don't clash with existing function names. Also
/// make sure that there is a float4 class defined.
//=============================================================================

#ifndef RRA_SHADERS_SHARED_DEFINITIONS_
#define RRA_SHADERS_SHARED_DEFINITIONS_

/// @brief The shared SceneUBO structure that is shared by CPU and GPU
/// Note: must be aligned to 4 bytes at all times.
struct SceneUBO
{
    float4x4 view_projection;
    float4x4 inverse_camera_projection;
    float4x4 camera_rotation;
    float4   camera_position;

    float4 light_position;

    float4 box16_node_color;
    float4 box32_node_color;
    float4 instance_node_color;
    float4 procedural_node_color;
    float4 triangle_node_color;
    float4 selected_node_color;
    float4 selected_geometry_color;

    float4 wireframe_normal_color;
    float4 wireframe_selected_color;

    float4 transparent_color;
    float4 opaque_color;
    float4 positive_color;
    float4 negative_color;

    // PREFER_FAST_BUILD / PREFER_FAST_TRACE support.
    // Also support both or none, so they can be seen. Specifying both FAST_BUILD and FAST_TRACE
    // is an error case and should be shown to the user if they are doing this (accidentally).
    float4 build_algorithm_none_color;
    float4 build_algorithm_fast_build_color;
    float4 build_algorithm_fast_trace_color;
    float4 build_algorithm_both_color;

    // Instance flags opaque / no opaque support.
    // Also support both or none, so they can be seen. Specifying both force opaque and force no opaque.
    // is an error case and should be shown to the user if they are doing this (accidentally).
    float4 instance_opaque_none_color;
    float4 instance_opaque_force_opaque_color;
    float4 instance_opaque_force_no_opaque_color;
    float4 instance_opaque_force_both_color;

    float ortho_scale;

    int max_instance_count;
    int max_triangle_count;
    int max_tree_depth;

    int bvh_coloring_mode;
    int max_node_depth;
    int depth_range_upper_bound;
    int depth_range_lower_bound;

    int screen_width;
    int screen_height;

    uint  max_traversal_count_setting;
    float max_traversal_count_limit;
    float min_traversal_count_limit;
    int   traversal_counter_mode;
    int   traversal_counter_use_custom_min_max;

    int  wireframe_enabled;

    uint traversal_box_sort_heuristic;
    int  traversal_accept_first_hit;
    int  traversal_cull_back_facing_triangles;
    int  traversal_cull_front_facing_triangles;

    int count_as_fused_instances;
};

/// @brief A vertex format for geometries.
///
/// Must follow alignment rules since it's used in an SSBO in TraversalShader.hlsl.
struct RraVertex
{
    float3 position;                           ///< The position.
    float  triangle_sah_and_selected;          ///< Absolute value is SAH, positive sign means selected.
    float2 normal;                             ///< The normal.
    uint   geometry_index_depth_split_opaque;  ///< Bits 31-16 are geometry index, 15-2 are depth, 1 is split, 0 is opaque.
    uint   triangle_node;                      ///< The triangle node.

};

// The node sort heuristic values.
#define kBoxSortHeuristicClosest 1
#define kBoxSortHeuristicMidPoint 2
#define kBoxSortHeuristicLargest 3
#define kBoxSortHeuristicDisableSorting 4

// The RraVertex struct uses a vec2 for the normal to save space. We can infer the z component, but the sign is
// lost, so we add this offset to the x component (since it's bounded to [-1, 1]) to indicate that z should be
// negative. When we infer the z component we will subtract this offset off the x component again.
#define kNormalSignIndicatorOffset 3.0f

/// @brief Generate a random number between 0.0 and 1.0, inclusive.
///
/// @param seed The random seed to use.
///
/// @return a random number between 0.0 and 1.0.
float rand(uint seed);

/// @brief A simple heatmap function.
///
/// Provides a color range across the color spectrum from blue to red.
///
/// @param level The input level, from 0.0 (blue) to 1.0 (red)
///
/// @return The color as a float4, in RGBA format.
float4 heatmap(float level);

#endif
