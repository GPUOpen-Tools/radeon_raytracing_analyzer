//=============================================================================
// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Shader for the Traversal counter rendering.
//=============================================================================

#include "../Common.hlsl"
#include "../Heatmap.hlsl"

#define KERNEL_SIZE 8
#define TRAVERAL_LOOP_LIMIT 6000  /// The maximum upper limit for traversal. An expected range of loop count in modern titles is around 500 to 2000.
#define COUNTER_MIN_INDEX 0
#define COUNTER_MAX_INDEX 1

#define IDENTITY_MATRIX float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)

cbuffer scene_ubo : register(b0)
{
    SceneUBO scene_ubo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/// Types of the volumes that we can traverse.
#define kTraversalVolumeTypeDisabled 0
#define kTraversalVolumeTypeBox 1
#define kTraversalVolumeTypeInstance 2
#define kTraversalVolumeTypeTriangle 3

/// The counter types that we use to create a final image.
#define kTraversalCounterModeTraversalLoopCount 0
#define kTraversalCounterModeInstanceHit 1
#define kTraversalCounterModeBoxVolumeHit 2
#define kTraversalCounterModeBoxVolumeMiss 3
#define kTraversalCounterModeBoxVolumeTest 4
#define kTraversalCounterModeTriangleHit 5
#define kTraversalCounterModeTriangleMiss 6
#define kTraversalCounterModeTriangleTest 7

// The hit flags of the result.
#define kTraversalResultHitFlagBlasHit 0x1
#define kTraversalResultHitFlagTlasHit 0x2

#define kInstanceFlagTriangleCullDisable 0x1
#define kInstanceFlagTriangleFrontCounterClockwise 0x2

#define kObbInvalid 0x7f

/// Structures provided by the CPU.
struct TraversalVolume
{
    float3 volume_min;
    uint   parent;
    float3 volume_max;
    int    index_at_parent;

    uint volume_type;
    uint leaf_start;
    uint leaf_end;

    int  child_masks;
    uint child_nodes[8];

    float4 child_nodes_min[8];
    float4 child_nodes_max[8];
    
    uint obb_index;
    float3 padding;
};

struct TraversalInstance
{
    float4x4 transform;
    float4x4 inverse_transform;
    uint     blas_index;
    uint     geometry_index;
    uint     selected;
    uint     flags;
};

struct TraversalResult
{
    uint counter;
    uint hit_flags;
    uint instance_index;
    uint triangle_index;
    uint blas_index;
    uint exit_properly;
    uint palceholder_2;
    uint palceholder_3;
};

/// Storage buffers of tlas and blas.
StructuredBuffer<TraversalVolume>   tlas_volumes : register(t11);
StructuredBuffer<RraVertex>         tlas_vertices : register(t12);
StructuredBuffer<TraversalInstance> instances : register(t13);

[[vk::binding(14, 0)]] StructuredBuffer<TraversalVolume> blas_volumes[];
[[vk::binding(15, 0)]] StructuredBuffer<RraVertex>       blas_vertices[];

/// The data storage.
RWStructuredBuffer<TraversalResult> result_data : register(u16);
RWStructuredBuffer<uint> histogram_data : register(u17);

/// Pixel coord to index mapper.
int getPixelIndex(int2 pixel_xy)
{
    // We add to 2 because the first two values are used for min and max.
    return 2 + (pixel_xy.x * scene_ubo.screen_height + pixel_xy.y);
}

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

/// Checks if the given intersection result was a hit.
bool is_hit(float4 box_intersection_result)
{
    const float eps = 5.960464478e-8f;  // 2^-24;
    return (box_intersection_result.x <= (box_intersection_result.y * (1 + 6 * eps)));
}

/// Check if the mask in the node has the child node enabled.
bool has_child(int masks, int index)
{
    return masks & (0x1U << index);
}

/// The scene stats to keep track of.
struct SceneStats
{
    TraversalResult result;

    float closest_hit;
    int   traversal_loop_count;
    int   instance_hit_count;
    int   box_test_count;
    int   box_hit_count;
    int   triangle_test_count;
    int   triangle_hit_count;
};

/// Get volume from tlas or blas by it's id.
TraversalVolume get_volume(uint id, bool tlas, uint blas_index_if_blas)
{
    if (tlas)
    {
        return tlas_volumes[id];
    }
    else
    {
        return blas_volumes[NonUniformResourceIndex(blas_index_if_blas)][id];
    }
}

RraVertex get_vertex(uint id, bool tlas, uint blas_index_if_blas)
{
    if (tlas)
    {
        return tlas_vertices[id];
    }
    else
    {
        return blas_vertices[NonUniformResourceIndex(blas_index_if_blas)][id];
    }
}

static void OBBTransform(
    in float3x3 transform,
    in float3 origin,
    in float3 direction,
    out float3 new_origin,
    out float3 new_direction)
{
    float3 t0 = transform[0];
    float3 t1 = transform[1];
    float3 t2 = transform[2];

    float r0x = mad(origin.z, t0.z, 0.0);
    float r0y = mad(origin.z, t1.z, 0.0);
    float r0z = mad(origin.z, t2.z, 0.0);

    float r1x = mul(direction.z, t0.z);
    float r1y = mul(direction.z, t1.z);
    float r1z = mul(direction.z, t2.z);

    r0x = mad(origin.y, t0.y, r0x);
    r0y = mad(origin.y, t1.y, r0y);
    r0z = mad(origin.y, t2.y, r0z);

    r1x = mad(direction.y, t0.y, r1x);
    r1y = mad(direction.y, t1.y, r1y);
    r1z = mad(direction.y, t2.y, r1z);

    r0x = mad(origin.x, t0.x, r0x);
    r0y = mad(origin.x, t1.x, r0y);
    r0z = mad(origin.x, t2.x, r0z);

    r1x = mad(direction.x, t0.x, r1x);
    r1y = mad(direction.x, t1.x, r1y);
    r1z = mad(direction.x, t2.x, r1z);

    new_origin = float3(r0x, r0y, r0z);
    new_direction = float3(r1x, r1y, r1z);
}

static const uint64_t ObbStage1Lut[104] =
{
    0x162c0ad6000019, 0x16ac02d6000019, 0x11440c51000019, 0x11c40451000019, 0xb580d8b000019,
    0xbd8058b000019, 0x640e40000019, 0xe40640000019, 0x1602b01900b016, 0x1600b01902b016,
    0x11031019011011, 0x11011019031011, 0xb03601901600b, 0xb01601903600b, 0x39019019000,
    0x19019039000, 0x190000162c0ad6, 0x19000016ac02d6, 0x19000011440c51, 0x19000011c40451,
    0x1900000b580d8b, 0x1900000bd8058b, 0x19000000640e40, 0x19000000e40640, 0x18066058186996,
    0x180460589a6196, 0x140ac09430cb11, 0x1408c094b2c311, 0x101ef1d03cfbcb, 0x101cf1d0bef3cb,
    0xc33130c451c40, 0xc31130cc71440, 0x188468581a6996, 0x18866858986196, 0x1488c89432cb11,
    0x148ac894b0c311, 0x109cf9d03efbcb, 0x109ef9d0bcf3cb, 0xcb11b0c471c40, 0xcb31b0cc51440,
    0x18181996181998, 0x18981196981198, 0x14302b11302b14, 0x14b02311b02314, 0x103c7bcb3c7bd0,
    0x10bc73cbbc73d0, 0xc44cc4044cc4c, 0xcc4c440c4c44c, 0x181a19969a1198, 0x189a11961a1998,
    0x14322b11b22314, 0x14b22311322b14, 0x103e7bcbbe73d0, 0x10be73cb3e7bd0, 0xc46cc40c6c44c,
    0xcc6c44046cc4c, 0x161a6998046058, 0x16986198066058, 0x1132cb1408c094, 0x11b0c3140ac094,
    0xb3efbd01cf1d0, 0xbbcf3d01ef1d0, 0x471c4c31130c, 0xc5144c33130c, 0x169a6198846858,
    0x16186998866858, 0x11b2c31488c894, 0x1130cb148ac894, 0xbbef3d09cf9d0, 0xb3cfbd09ef9d0,
    0xc7144cb11b0c, 0x451c4cb31b0c, 0x171638d71458d7, 0x178c51578e3157, 0x13368a1334da13,
    0x13a0d353a28353, 0xe4a9a4e492a4e, 0xea5248ea6948e, 0xa56490a55590a, 0xa91554a92454a,
    0x178e51570c3957, 0x171438d79650d7, 0x13a2d353208b53, 0x13348a13b6d213, 0xea7248e249c8e,
    0xe489a4ecb224e, 0xa93554a104d4a, 0xa54490ad7510a, 0x170c59570e3957, 0x179630d79450d7,
    0x1320db53228b53, 0x13b68213b4d213, 0xe252c8e269c8e, 0xeca924ec9224e, 0xa115d4a124d4a,
    0xad6410ad5510a, 0x179430d71658d7, 0x170e59578c3157, 0x13b4821336da13, 0x1322db53a08353,
    0xec8924e4b2a4e, 0xe272c8ea4948e, 0xad4410a57590a, 0xa135d4a90454a
};

static const uint ObbStage2Lut[32] =
{
    0x00000000, 0x3d1be50c, 0x3e15f61a, 0x3e484336, 0x3e79df93, 0x3e7c3a3a, 0x3e8a8bd4, 0x3e9e0875,
    0x3e9f0938, 0x3ea7bf1b, 0x3eaaaaab, 0x3ec3ef15, 0x3f000000, 0x3f01814f, 0x3f16a507, 0x3f273d75,
    0x3f30fbc5, 0x3f3504f3, 0x3f3d3a87, 0x3f4e034d, 0x3f5a827a, 0x3f692290, 0x3f6c835e, 0x3f73023f,
    0x3f7641af, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};

//=====================================================================================================================
// Map LUT matrix indices to locations in a 3x3 matrix.
static float MatrixIndexToFloat(uint index, uint row, uint col)
{
    // Each entry in the stage-1 lookup table contains 9 packed 6-bit indices corresponding
    // to the elements of the matrix.
    uint shift = (18 * row) + (6 * col);
    uint stage2Index = uint((ObbStage1Lut[index] >> shift) & 0x3f);

    // Bits [29:0] of the float are encoded in the stage-2 lookup table.
    // Bits [4:0] of the stage-2 index are used to access the stage-2 lookup table.
    // The sign bit is encoded in bit 5 of the stage2Index.
    // Bit 30 of the float will always be zero as all exponents are less than 128.
    uint outValue = ObbStage2Lut[stage2Index & 0x1f] | ((stage2Index >> 5) << 31);
    return asfloat(outValue);
}

//=====================================================================================================================
// Convert a discretized OBB index to a float3x3 matrix.
static float3x3 OBBIndexToMatrix(uint index)
{
    // If this box node is an OBB, decode the matrix and transform the ray origin and
    // direction before proceeding to the box test.
    float3x3 m =
    {
        MatrixIndexToFloat(index, 0, 0),
                   MatrixIndexToFloat(index, 0, 1),
                   MatrixIndexToFloat(index, 0, 2),
                   MatrixIndexToFloat(index, 1, 0),
                   MatrixIndexToFloat(index, 1, 1),
                   MatrixIndexToFloat(index, 1, 2),
                   MatrixIndexToFloat(index, 2, 0),
                   MatrixIndexToFloat(index, 2, 1),
                   MatrixIndexToFloat(index, 2, 2)
    };

    return m;
}

/// Returns the intersection count.
SceneStats traverse_scene(float4 original_ray_origin, float3 original_ray_direction)
{
    // Set up stats with all 0s.
    SceneStats stats = (SceneStats)0;

    // Initially make triangle_index invalid.
    stats.result.triangle_index = -1;

    // Set up the state variables for traversal.
    bool tlas_mode               = true;  // When we hit an instance we flip this to get volumes from blasses.
    uint current_blas_index      = 0;     // When we start traversing an instance we need to remember which blas it points to.
    uint current_node            = 0;     // The current node we are traversing.
    int  next_index              = 0;     // The next node index we'll traverse.
    uint transform_raise_node_id = 0;     // When an instance traversal starts, we keep track of our last step in the tlas to go back to.
    bool miss                    = true;

    // Current ray data.
    float4 ray_origin    = original_ray_origin;
    float3 ray_direction = original_ray_direction;
    float3 ray_inv_dir   = 1.0f / ray_direction;
    stats.closest_hit    = INFINITY;

    uint current_instance_index;
    uint current_instance_flags = 0;  // In the BLAS tab, it will use the default value of no instance flags.

    TraversalVolume volume = get_volume(current_node, tlas_mode, current_blas_index);  // Initial volume.

    // Initial check for a hit.
    stats.box_test_count += 1;
    if (!is_hit(fast_intersect_bbox(ray_origin.xyz, ray_inv_dir, volume.volume_min, volume.volume_max, stats.closest_hit)))
    {
        return stats;
    }

    stats.box_hit_count += 1;  // The scene bounds are hit so we count it as one.

    bool reverse_lookup = false;  // When we are going back from a branch we use this to do a reverse lookup of the closest box.

    while (stats.traversal_loop_count < TRAVERAL_LOOP_LIMIT)
    {
        stats.traversal_loop_count += 1;  // Count every loop here.

        bool rise_up = false;  // If the traversal needs to go up on the branch, we set this to true.
        
        if (volume.volume_type == kTraversalVolumeTypeBox)
        {
            /// Explanation:
            ///     The traversal is accelerated by checking for the closest child node.
            ///     We do this by checking for the closest hit on all child volumes.
            ///     A sorting network is then used to determine which node to step into next.
            ///     When we go back up on the tree, we do a reverse lookup.

            // Intersection data.
            int   intersection_mapping[8];
            bool  intersection_results[8];
            float intersection_keys[8];

            int intersection_count = 0;  // Local intersection counter.

            for (int i = 0; i < 8; i++)
            {
                // OBB Transform.
                float3 local_ray_origin = ray_origin.xyz;
                float3 local_ray_dir = ray_direction;
                if (volume.obb_index != kObbInvalid)
                {
                    float3x3 obb_matrix = OBBIndexToMatrix(volume.obb_index);
                
                    OBBTransform(obb_matrix,
                             ray_origin.xyz,
                             ray_direction,
                             local_ray_origin,
                             local_ray_dir);
                }
                
                // Intersect box and set state.
                // Notice that we are using closest_hit on the intersection.
                // Closest hit allows us to discard volumes that are further away than a confirmed triangle hit.
                float4 intersection =
                    fast_intersect_bbox(local_ray_origin, 1.0 / local_ray_dir, volume.child_nodes_min[i].xyz, volume.child_nodes_max[i].xyz, stats.closest_hit);
                intersection_mapping[i] = i;
                intersection_results[i] = has_child(volume.child_masks, i) && is_hit(intersection);

                if (scene_ubo.traversal_box_sort_heuristic == kBoxSortHeuristicClosest)
                {
                    intersection_keys[i] = intersection.x;
                }
                else if (scene_ubo.traversal_box_sort_heuristic == kBoxSortHeuristicMidPoint)
                {
                    intersection_keys[i] = (intersection.z + intersection.w);
                }
                else if (scene_ubo.traversal_box_sort_heuristic == kBoxSortHeuristicLargest)
                {
                    intersection_keys[i] = intersection.x - intersection.y;
                }

                if (intersection_results[i])
                {
                    intersection_count += 1;  // Count intersections.
                }
            }

            // The sorting algorithm to sort the states.
#define SORT(A, B)                                                                                            \
    if ((intersection_results[B] && intersection_keys[B] < intersection_keys[A]) || !intersection_results[A]) \
    {                                                                                                         \
        int mapping_temp        = intersection_mapping[A];                                                    \
        intersection_mapping[A] = intersection_mapping[B];                                                    \
        intersection_mapping[B] = mapping_temp;                                                               \
        bool results_temp       = intersection_results[A];                                                    \
        intersection_results[A] = intersection_results[B];                                                    \
        intersection_results[B] = results_temp;                                                               \
        float key_temp          = intersection_keys[A];                                                       \
        intersection_keys[A]    = intersection_keys[B];                                                       \
        intersection_keys[B]    = key_temp;                                                                   \
    }

            // Sort the intersections by their closest hit.
            // Stage 1.
            SORT(0, 2);
            SORT(1, 3);
            SORT(4, 6);
            SORT(5, 7);
            
            // Stage 2.
            SORT(0, 1);
            SORT(2, 3);
            SORT(4, 5);
            SORT(6, 7);
            
            // Stage 3.
            SORT(1, 2);
            SORT(5, 6);
            
            // Stage 4.
            SORT(0, 4);
            SORT(1, 5);
            SORT(2, 6);
            SORT(3, 7);
            
            // Stage 5.
            SORT(2, 5);
            SORT(3, 5);
            
            // Stage 6.
            SORT(1, 2);
            SORT(3, 4);
            SORT(5, 6);

            // If the traversal is going up the tree, we use the reverse lookup.
            // We do not count intersections or tests when going up.
            // The next_index must map to the next closest hit.
            if (reverse_lookup)
            {
                for (int i = 0; i < 8; i++)
                {
                    if (intersection_mapping[i] == next_index)
                    {
                        next_index = i + 1;
                        break;
                    }
                }
            }
            else
            {
                // Only count the tests and hits when going down into a node.
                stats.box_test_count += 1;
                if (intersection_count > 0)
                {
                    stats.box_hit_count += 1;
                }
            }

            // Boundary checks.
            // Find the next node to step into after mapping. If we run out of nodes, then we need to go back up the tree.
            if (next_index < 8)
            {
                while (true)
                {
                    if (intersection_results[next_index])
                    {
                        current_node = volume.child_nodes[intersection_mapping[next_index]];
                        volume       = get_volume(current_node, tlas_mode, current_blas_index);
                        next_index   = 0;
                        break;
                    }
                    else
                    {
                        next_index = next_index + 1;

                        if (next_index >= 4)
                        {
                            rise_up = true;
                            break;
                        }
                    }
                }
            }
            else
            {
                rise_up = true;
            }
        }
        else if (volume.volume_type == kTraversalVolumeTypeTriangle)
        {
            // Process the triangle node.
            // A triangle node may be split into multiple triangles.
            // Test each triangle fan.

            for (uint i = volume.leaf_start; i < volume.leaf_end - 2; i++)
            {
                stats.triangle_test_count += 1;  // Count the triangle test.

                // Intersect triangle.
                float4 result = fast_intersect_triangle(ray_origin.xyz,
                                                        ray_direction,
                                                        get_vertex(i, tlas_mode, current_blas_index).position,
                                                        get_vertex(i + 1, tlas_mode, current_blas_index).position,
                                                        get_vertex(i + 2, tlas_mode, current_blas_index).position);

                bool cull_disable          = current_instance_flags & kInstanceFlagTriangleCullDisable;
                bool flip_facing           = current_instance_flags & kInstanceFlagTriangleFrontCounterClockwise;
                bool back_face_culling     = scene_ubo.traversal_cull_back_facing_triangles;
                bool front_face_culling    = scene_ubo.traversal_cull_front_facing_triangles;
                bool clockwise             = result.y > 0.0;
                bool front_facing_triangle = clockwise ^ flip_facing;
                bool face_culled           = !cull_disable && ((front_facing_triangle && front_face_culling) || (!front_facing_triangle && back_face_culling));

                // Check if the candidate is the closest hit.
                float candidate_t = result.x / result.y;
                if (!face_culled && candidate_t < stats.closest_hit)
                {
                    stats.closest_hit = candidate_t;  // Assign closest hit.
                    stats.triangle_hit_count += 1;    // Count triangle hit.

                    stats.result.instance_index = current_instance_index;
                    stats.result.triangle_index = i;
                    stats.result.hit_flags      = tlas_mode ? kTraversalResultHitFlagTlasHit : kTraversalResultHitFlagBlasHit;

                    stats.result.blas_index = current_blas_index;
                    miss                    = false;
                }
            }

            rise_up = true;
        }
        else if (volume.volume_type == kTraversalVolumeTypeInstance)
        {
            // Process an instance.
            stats.instance_hit_count += 1;  // Count the instance hit.

            if (scene_ubo.count_as_fused_instances)
            {
                stats.traversal_loop_count--;
                stats.box_test_count--;
                stats.box_hit_count--;
            }

            TraversalInstance instance = instances[volume.leaf_start];  // Get the instance information.

            // Transform the ray data into the instance space.
            ray_origin    = mul(ray_origin, instance.inverse_transform);
            ray_direction = mul(ray_direction, (float3x3)(instance.inverse_transform));
            ray_inv_dir   = 1.0f / ray_direction;

            current_instance_index = volume.leaf_start;
            current_instance_flags = instance.flags;

            tlas_mode               = false;                // An instance must point to a blas.
            transform_raise_node_id = current_node;         // Keep track our last location. We'll need this when we are done traversing the instance.
            current_node            = 0;                    // Swap the current node to the blas root.
            current_blas_index      = instance.blas_index;  // Update the blas index.
            volume                  = get_volume(current_node, tlas_mode, current_blas_index);  // Get the node using latest state.
            next_index              = 0;                                                        // Set the next branch id to step into.
        }
        else
        {
            // We don't know how to process this node so we ignore.
            rise_up = true;
        }

        if (scene_ubo.traversal_accept_first_hit && !miss)
        {
            // We finished traversal.
            stats.result.exit_properly = 1;
            break;
        }

        reverse_lookup = false;  // Turn off reverse lookup. The next state will be determined depending on rise_up.

        if (rise_up)
        {
            if (volume.index_at_parent == -1 && !tlas_mode)
            {
                // We are coming out of a blas instance back into tlas.

                // Transform ray back into tlas space.
                ray_origin    = original_ray_origin;
                ray_direction = original_ray_direction;
                ray_inv_dir   = 1.0f / ray_direction;

                tlas_mode    = true;                     // Set traversal mode to tlas.
                current_node = transform_raise_node_id;  // Set the current node to the last node we were on before going into the instance.
                volume       = get_volume(current_node, tlas_mode, current_blas_index);  // Load the node.
            }
            else if (volume.index_at_parent == -1 && tlas_mode)
            {
                // We finished traversal.
                stats.result.exit_properly = 1;
                break;
            }

            // Go back up to traverse next set of nodes.
            // We set the next index as our index and we tell traversal to do a reverse lookup since we use a sorting network.
            current_node   = volume.parent;
            next_index     = volume.index_at_parent;
            reverse_lookup = true;

            volume = get_volume(current_node, tlas_mode, current_blas_index);  // Load the node.
        }
    }

    return stats;
}

[numthreads(KERNEL_SIZE, KERNEL_SIZE, 1)] void CSMain(uint3 Gid
                                                      : SV_GroupID, uint3          DTid
                                                      : SV_DispatchThreadID, uint3 GTid
                                                      : SV_GroupThreadID, uint     GI
                                                      : SV_GroupIndex) {
    // Get the pixel indexes.
    int2 pixel_xy = int2(Gid.x * KERNEL_SIZE + GTid.x, Gid.y * KERNEL_SIZE + GTid.y);
    if (pixel_xy.x >= scene_ubo.screen_width || pixel_xy.y >= scene_ubo.screen_height)
    {
        return;
    }

    float4 ray_origin;
    float3 ray_direction;

    float2 normalized_coords = pixel_xy / float2(scene_ubo.screen_width, scene_ubo.screen_height);
    normalized_coords        = (normalized_coords - 0.5f) * 2.0f;
    normalized_coords.y *= -1.0f;  // Flipped to match bitmap coordinate system.

    // If camera is set to use orthographic projection, handle this case separately.
    if (scene_ubo.ortho_scale > 0.0f)
    {
        float  near_plane   = 1000.0f;
        float  aspect_ratio = (float)scene_ubo.screen_width / (float)scene_ubo.screen_height;
        float  offset_x     = normalized_coords.x * scene_ubo.ortho_scale * aspect_ratio;
        float  offset_y     = normalized_coords.y * scene_ubo.ortho_scale;
        float4 offset       = mul(float4(offset_x, offset_y, near_plane, 0.0f), scene_ubo.camera_rotation);
        ray_origin          = scene_ubo.camera_position + offset;

        ray_direction = mul(float3(0.0f, 0.0f, -1.0f), (float3x3)scene_ubo.camera_rotation);
    }
    else
    {
        // Construct ray direction by camera parameters.
        float4 perspective_direction = mul(float4(normalized_coords, 1.0f, 1.0f), scene_ubo.inverse_camera_projection);
        perspective_direction        = perspective_direction / perspective_direction.w;

        ray_origin    = scene_ubo.camera_position;
        ray_direction = mul(normalize((float3)perspective_direction), (float3x3)scene_ubo.camera_rotation);
    }

    // Traverse the scene and collect traversal stats.
    SceneStats stats = traverse_scene(ray_origin, ray_direction);

    // Load the requested counter.
    uint counter = 0;
    switch (scene_ubo.traversal_counter_mode)
    {
    case kTraversalCounterModeTraversalLoopCount:
        counter = stats.traversal_loop_count;
        break;
    case kTraversalCounterModeInstanceHit:
        counter = stats.instance_hit_count;
        break;
    case kTraversalCounterModeBoxVolumeHit:
        counter = stats.box_hit_count;
        break;
    case kTraversalCounterModeBoxVolumeMiss:
        counter = stats.box_test_count - stats.box_hit_count;
        break;
    case kTraversalCounterModeBoxVolumeTest:
        counter = stats.box_test_count;
        break;
    case kTraversalCounterModeTriangleHit:
        counter = stats.triangle_hit_count;
        break;
    case kTraversalCounterModeTriangleMiss:
        counter = stats.triangle_test_count - stats.triangle_hit_count;
        break;
    case kTraversalCounterModeTriangleTest:
        counter = stats.triangle_test_count;
        break;
    }

    InterlockedAdd(histogram_data[(counter < scene_ubo.max_traversal_count_setting) ? counter : scene_ubo.max_traversal_count_setting - 1], 1);

    TraversalResult result = stats.result;
    result.counter         = counter;

    // Save final color onto texture.
    result_data[getPixelIndex(pixel_xy)] = result;

    if (pixel_xy.x == 0 && pixel_xy.y == 0)
    {
        result_data[COUNTER_MAX_INDEX].counter = 0;
        result_data[COUNTER_MIN_INDEX].counter = TRAVERAL_LOOP_LIMIT;
    }
}

    [numthreads(KERNEL_SIZE, KERNEL_SIZE, 1)] void CSSubsample(uint3 Gid
                                                               : SV_GroupID, uint3          DTid
                                                               : SV_DispatchThreadID, uint3 GTid
                                                               : SV_GroupThreadID, uint     GI
                                                               : SV_GroupIndex)
{
    int2 pixel_xy = int2(Gid.x * KERNEL_SIZE + GTid.x, Gid.y * KERNEL_SIZE + GTid.y);
    if (pixel_xy.x >= scene_ubo.screen_width || pixel_xy.y >= scene_ubo.screen_height)
    {
        return;
    }

    uint counter = result_data[getPixelIndex(pixel_xy)].counter;

    // Atomic operations, the counter data is offset by 2 since the first two indices are reserved for min and max.
    InterlockedMax(result_data[COUNTER_MAX_INDEX].counter, counter);
    InterlockedMin(result_data[COUNTER_MIN_INDEX].counter, counter);
}

PSInput VSMain(uint index : SV_VertexID)
{
    // Draw big triangle to saturate the fragment shader.
    PSInput result;
    float   x         = float(index / 2) * 4.0 - 1.0f;
    float   y         = float(index % 2) * 4.0 - 1.0f;
    result.position   = float4(x, y, 0.0f, 1.0f);
    result.texcoord.x = float(index / 2) * 2.0f;
    result.texcoord.y = 1.0f - float(index % 2) * 2.0f;
    return result;
}

float line_draw(float2 p1, float2 p2, float2 fragCoord)
{
    const float width = 0.0005;

    float distance = abs((p2.y - p1.y) * fragCoord.x - (p2.x - p1.x) * fragCoord.y + p2.x * p1.y - p2.y * p1.x);
    distance /= sqrt(pow(p2.y - p1.y, 2.) + pow(p2.x - p1.x, 2.));

    return clamp(distance, 0., width) / width;
}

float wireframe_mix(float4x4 view_projection, float4x4 transform, float3 triangle_a, float3 triangle_b, float3 triangle_c, float2 frag_coord)
{
    float4 t_a = mul(float4(triangle_a, 1.0), transform);
    float4 t_b = mul(float4(triangle_b, 1.0), transform);
    float4 t_c = mul(float4(triangle_c, 1.0), transform);

    t_a = mul(view_projection, t_a);
    t_b = mul(view_projection, t_b);
    t_c = mul(view_projection, t_c);

    t_a = t_a / t_a.a;
    t_b = t_b / t_b.a;
    t_c = t_c / t_c.a;

    t_a = (t_a + 1.0) / 2.0;
    t_b = (t_b + 1.0) / 2.0;
    t_c = (t_c + 1.0) / 2.0;

    frag_coord.y = 1.0 - frag_coord.y;

    float alpha = 1.0;
    alpha *= line_draw(t_a.xy, t_b.xy, frag_coord);
    alpha *= line_draw(t_b.xy, t_c.xy, frag_coord);
    alpha *= line_draw(t_c.xy, t_a.xy, frag_coord);

    return alpha;
}

float4 sample_wireframe(float4 final_color, int2 pixel_xy, TraversalResult result)
{
    if (!scene_ubo.wireframe_enabled)
    {
        return final_color;
    }

    if (result.hit_flags == 0)
    {
        return final_color;
    }

    TraversalInstance instance;
    RraVertex         tv_a;
    RraVertex         tv_b;
    RraVertex         tv_c;

    if (result.hit_flags == kTraversalResultHitFlagBlasHit)
    {
        instance = instances[result.instance_index];
        tv_a     = get_vertex(result.triangle_index, false, result.blas_index);
        tv_b     = get_vertex(result.triangle_index + 1, false, result.blas_index);
        tv_c     = get_vertex(result.triangle_index + 2, false, result.blas_index);
    }
    else
    {
        instance.transform = IDENTITY_MATRIX;
        tv_a               = get_vertex(result.triangle_index, true, result.blas_index);
        tv_b               = get_vertex(result.triangle_index + 1, true, result.blas_index);
        tv_c               = get_vertex(result.triangle_index + 2, true, result.blas_index);
        instance.selected  = tv_a.triangle_sah_and_selected > 0.0f;
    }

    float2 frag_coords = float2(pixel_xy.x / float(scene_ubo.screen_width), pixel_xy.y / float(scene_ubo.screen_height));

    float alpha = wireframe_mix(scene_ubo.view_projection, instance.transform, tv_a.position, tv_b.position, tv_c.position, frag_coords);

    return lerp(instance.selected ? scene_ubo.wireframe_selected_color : scene_ubo.wireframe_normal_color, final_color, alpha);
}

float4 PSMain(PSInput input)
    : SV_TARGET
{
    int2 pixel_xy = int2(input.texcoord.x * scene_ubo.screen_width, input.texcoord.y * scene_ubo.screen_height);

    // Get counter data.
    TraversalResult result = result_data[getPixelIndex(pixel_xy)];

    // Save unedited counter value.
    // Bound the counter by limits.

    float4 final_color;

    if (scene_ubo.traversal_counter_use_custom_min_max)
    {
        result.counter = min(result.counter, scene_ubo.max_traversal_count_limit);
        result.counter = max(result.counter, scene_ubo.min_traversal_count_limit);
        result.counter -= scene_ubo.min_traversal_count_limit;

        // Set the final color with alpha components as the counter.
        final_color = heatmap_temp(result.counter / (scene_ubo.max_traversal_count_limit - scene_ubo.min_traversal_count_limit));
    }
    else
    {
        result.counter = min(result.counter, result_data[COUNTER_MAX_INDEX].counter);
        result.counter = max(result.counter, result_data[COUNTER_MIN_INDEX].counter);
        result.counter -= result_data[COUNTER_MIN_INDEX].counter;

        // Set the final color with alpha components as the counter.
        final_color = heatmap_temp(result.counter / float(result_data[COUNTER_MAX_INDEX].counter - result_data[COUNTER_MIN_INDEX].counter));
    }

    // Highlight selected triangle in the BLAS pane.
    bool selected = get_vertex(result.triangle_index, true, result.blas_index).triangle_sah_and_selected > 0.0;
    final_color   = selected ? scene_ubo.selected_geometry_color : final_color;

    // Sample wireframe for the given coord.
    return sample_wireframe(final_color, pixel_xy, result);
}
