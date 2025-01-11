//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the public ray history interface.
//=============================================================================

#ifndef RRA_BACKEND_PUBLIC_RRA_RAY_HISTORY_H_
#define RRA_BACKEND_PUBLIC_RRA_RAY_HISTORY_H_

#include "rra_error.h"

#include <vector>

#define RRA_RAY_HISTORY_TOKENS_METADATA_IDENTIFIER "HistoryMetadata"
#define RRA_RAY_HISTORY_RAW_TOKENS_IDENTIFIER "HistoryTokensRaw"

struct GlobalInvocationID
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

struct Ray
{
    uint64_t tlas_address;
    uint32_t ray_flags;
    uint32_t cull_mask;
    uint32_t sbt_record_offset;
    uint32_t sbt_record_stride;
    uint32_t miss_index;
    float    origin[3];
    float    t_min;
    float    direction[3];
    float    t_max;
    uint32_t payload;

    uint32_t wave_id;
    uint32_t id;

    uint32_t dynamic_id;
    uint32_t parent_id;
};

enum RayEventType : uint32_t
{
    kRayEventTypeTlasBoxTest,
    kRayEventTypeTlasBoxIntersection,
    kRayEventTypeInstanceIntersection,
    kRayEventTypeBlasBoxTest,
    kRayEventTypeBlasBoxIntersection,
    kRayEventTypeTriangleTest,
    kRayEventTypeTriangleIntersection,
    kRayEventTypeClosestHit,
    kRayEventTypeAnyHitInvocation,
};

/// @brief Status for each any hit function call.
enum AnyHitStatus : uint8_t
{
    kAnyHitStatusIgnoreHit,
    kAnyHitStatusAcceptHit,
    kAnyHitStatusAcceptHitAndEndSearch,
};

/// @brief Corresponds to data to be shown to user about any hit result for ray.
enum AnyHitRayResult : uint8_t
{
    kAnyHitResultNoAnyHit,
    kAnyHitResultAccept,
    kAnyHitResultReject,
};

enum DispatchType : uint32_t
{
    kRayTracingPipeline,
    kCompute,
    kGraphics,
};

struct RraIntersectionResult
{
    uint32_t        hit_kind;
    uint32_t        instance_index;
    uint32_t        geometry_index;
    uint32_t        primitive_index;
    uint32_t        num_iterations;
    uint32_t        num_instance_intersections;
    AnyHitRayResult any_hit_result;
    float           hit_t;
};

struct RraRayHistoryStats
{
    uint64_t              raygen_count                = 0;
    uint64_t              closest_hit_count           = 0;
    uint64_t              any_hit_count               = 0;
    uint64_t              intersection_count          = 0;
    uint64_t              miss_count                  = 0;
    uint64_t              ray_count                   = 0;
    uint64_t              loop_iteration_count        = 0;
    uint64_t              instance_intersection_count = 0;
    uint64_t              pixel_count                 = 0;
    std::vector<uint64_t> tlases_traversed_;  ///< All the TLASes traversed by rays in this dispatch.
};

struct RraDispatchLoadStatus
{
    bool  data_decompressed = false;
    bool  raw_data_parsed   = false;
    bool  data_indexed      = false;
    bool  loading_complete  = false;
    bool  has_errors        = false;
    bool  incomplete_data   = false;
    float load_percentage   = 0.0f;
};

struct RayDispatchBeginIdentifier
{
    uint32_t dispatch_coord_index;
    uint32_t begin_token_index;

    // Constructor needed to construct in place using std::vector::emplace_back().
    RayDispatchBeginIdentifier(uint32_t coord_index, uint32_t begin_index);
};

struct DispatchCoordinateStats
{
    uint32_t ray_count            = 0;
    uint32_t loop_iteration_count = 0;
    uint32_t intersection_count   = 0;
    uint32_t any_hit_count        = 0;
};

struct DispatchCoordinateData
{
    DispatchCoordinateStats                 stats = {};
    std::vector<RayDispatchBeginIdentifier> begin_identifiers;
};

struct RayDispatchData
{
    uint32_t dispatch_width  = 0;  ///< Dispatch width for coordinate mapping.
    uint32_t dispatch_height = 0;  ///< Dispatch height for coordinate mapping.

    std::vector<DispatchCoordinateData> dispatch_ray_indices;  ///< A buffer to keep all of the dispatch coordinate data.
    bool                                error = false;         ///< True if an error occured during loading, such as malformed data.

    /// @brief Get the dispatch coordinate data.
    /// @param x The x coord.
    /// @param y The y coord.
    /// @param z The z coord.
    /// @return A reference to the exact coordinate data.
    DispatchCoordinateData& GetCoordinate(uint32_t x, uint32_t y, uint32_t z);

    /// @brief Check if the given coordinate is valid.
    /// @param x The x coord.
    /// @param y The y coord.
    /// @param z The z coord.
    /// @return True if the coordinate is valid.
    bool CoordinateIsValid(uint32_t x, uint32_t y, uint32_t z);
};

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

/// @brief Get the number of vkCmdTraceRaysKHR() calls.
///
/// @param [out] out_count The dispatch count.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraRayGetDispatchCount(uint32_t* out_count);

/// @brief Get the dimensions passed to a vkCmdTraceRaysKHR() call.
///
/// @param [in]  dispatch_id The ID of the vkCmdTraceRaysKHR() call.
/// @param [out] out_width   The x dimension.
/// @param [out] out_height  The y dimension.
/// @param [out] out_depth   The z dimension.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraRayGetDispatchDimensions(uint32_t dispatch_id, uint32_t* out_width, uint32_t* out_height, uint32_t* out_depth);

/// @brief Get the number of rays for an invocation ID.
///
/// @param [in]  dispatch_id   The ID of the vkCmdTraceRaysKHR() call.
/// @param [in]  invocation_id The global invocation ID.
/// @param [out] out_count     The ray count.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraRayGetRayCount(uint32_t dispatch_id, GlobalInvocationID invocation_id, uint32_t* out_count);

/// @brief Get the rays for an invocation ID.
///
/// @param [in]  dispatch_id   The ID of the vkCmdTraceRaysKHR() call.
/// @param [in]  invocation_id The global invocation ID.
/// @param [out] out_rays      A pointer to a ray list allocated with the count of rays.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraRayGetRays(uint32_t dispatch_id, GlobalInvocationID invocation_id, Ray* out_rays);

/// @brief Get the number of instances intersected for a ray.
///
/// @param [in]  dispatch_id   The ID of the vkCmdTraceRaysKHR() call.
/// @param [in]  invocation_id The global invocation ID.
/// @param [in]  ray_index     The index of the ray to get the RT event count for.
/// @param [out] out_count     The number of instance intersections.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraRayGetIntersectionResult(uint32_t dispatch_id, GlobalInvocationID invocation_id, uint32_t ray_index, RraIntersectionResult* out_result);

/// @brief Get all the stats related to a dispatch.
///
/// @param [in]  dispatch_id The ID of the dispatch.
/// @param [out] out_stats   The stats to gather.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraRayGetDispatchStats(uint32_t dispatch_id, RraRayHistoryStats* out_stats);

/// @brief Get all the stats related to a dispatch.
///
/// @param [in]  dispatch_id The ID of the dispatch.
/// @param [in]  invocation_id The global invocation ID.
/// @param [out] out_stats   The stats to gather.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraRayGetDispatchCoordinateStats(uint32_t dispatch_id, GlobalInvocationID invocation_id, DispatchCoordinateStats* out_stats);

/// @brief Get data about the any hit shader invocations for this ray.
///
/// @param dispatch_id   The ID of the vkCmdTraceRaysKHR() call.
/// @param invocation_id The global invocation ID.
/// @param ray_index     The index of the ray to count the invocations of.
/// @param out_count     The number of any hit shader invocations.
/// @param result        Whether or not any of the any hit shaders accepted the hit.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraRayGetAnyHitInvocationData(uint32_t           dispatch_id,
                                           GlobalInvocationID invocation_id,
                                           uint32_t           ray_index,
                                           uint32_t*          out_count,
                                           AnyHitRayResult*   result);

/// @brief Get the number of any hit shader invocations for a dispatch coordinate.
///
/// @param dispatch_id   The ID of the vkCmdTraceRaysKHR() call.
/// @param invocation_id The global invocation ID.
/// @param out_count     The number of any hit shader invocations.
///
/// @return kRraOk if successful or an RraErrorCode if an error occurred.
RraErrorCode RraRayGetAnyHitInvocationCount(uint32_t dispatch_id, GlobalInvocationID invocation_id, uint32_t* out_count);

/// @brief Get the dispatch type
///
/// @param [in]  dispatch_id   The id of the dispatch.
/// @param [out] type          The type of the dispatch.
///
/// @return kRraOk if successful.
RraErrorCode RraRayGetDispatchType(uint32_t dispatch_id, DispatchType* type);

/// @brief The loading status of the current dispatch.
///
/// @param dispatch_id Dispatch ID to get status for.
/// @param status      The loading status of the dispatch.
///
/// @returnkRraOk if successful.
RraErrorCode RraRayGetDispatchStatus(uint32_t dispatch_id, RraDispatchLoadStatus* status);

/// @brief Get the dispatch name (user marker) if available.
///
/// @param dispatch_id Dispatch ID to get status for.
/// @param buffer      Pointer to a buffer to receive the user marker string.
/// @param buffer_size The size of the buffer, in bytes.
///
/// @returnkRraOk if successful.
RraErrorCode RraRayGetDispatchUserMarkerString(uint32_t dispatch_id, char* buffer, uint32_t buffer_size);

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus
#endif  // RRA_BACKEND_PUBLIC_RRA_RAY_HISTORY_H_
