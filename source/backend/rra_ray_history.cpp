//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the ray history interface.
///
/// Contains public functions specific to ray history.
//=============================================================================

#include "public/rra_ray_history.h"

#include "rra_data_set.h"
#include "ray_history/raytracing_counter.h"

// External reference to the global dataset.
extern RraDataSet data_set_;

DispatchCoordinateData& RayDispatchData::GetCoordinate(uint32_t x, uint32_t y, uint32_t z)
{
    auto idx = x + (y * dispatch_width) + (z * dispatch_width * dispatch_height);
    return dispatch_ray_indices[idx];
}

bool RayDispatchData::CoordinateIsValid(uint32_t x, uint32_t y, uint32_t z)
{
    auto idx = x + (y * dispatch_width) + (z * dispatch_width * dispatch_height);
    return idx < dispatch_ray_indices.size();
}

RraErrorCode RraRayGetDispatchCount(uint32_t* out_count)
{
    *out_count = (uint32_t)data_set_.async_ray_histories.size();
    return kRraOk;
}

RraErrorCode RraRayGetDispatchDimensions(uint32_t dispatch_id, uint32_t* out_width, uint32_t* out_height, uint32_t* out_depth)
{
    if (dispatch_id >= data_set_.async_ray_histories.size())
    {
        return kRraErrorIndexOutOfRange;
    }

    auto dispatch_size = data_set_.async_ray_histories[dispatch_id]->GetDerivedDispatchSize();

    *out_width  = dispatch_size.width;
    *out_height = dispatch_size.height;
    *out_depth  = dispatch_size.depth;
    return kRraOk;
}

RraErrorCode RraRayGetRayCount(uint32_t dispatch_id, GlobalInvocationID invocation_id, uint32_t* out_count)
{
    if (dispatch_id >= data_set_.async_ray_histories.size())
    {
        return kRraErrorIndexOutOfRange;
    }
    auto  rh            = data_set_.async_ray_histories[dispatch_id]->GetRayHistoryTrace();
    auto& dispatch_data = data_set_.async_ray_histories[dispatch_id]->GetDispatchData();

    if (!dispatch_data.CoordinateIsValid(invocation_id.x, invocation_id.y, invocation_id.z))
    {
        *out_count = 0;
        return kRraErrorMalformedData;
    }

    auto& ray_indices = dispatch_data.GetCoordinate(invocation_id.x, invocation_id.y, invocation_id.z).begin_identifiers;

    *out_count = (uint32_t)ray_indices.size();

    return kRraOk;
}

RraErrorCode RraRayGetRays(uint32_t dispatch_id, GlobalInvocationID invocation_id, Ray* out_rays)
{
    if (dispatch_id >= data_set_.async_ray_histories.size())
    {
        return kRraErrorIndexOutOfRange;
    }
    auto  rh            = data_set_.async_ray_histories[dispatch_id]->GetRayHistoryTrace();
    auto& dispatch_data = data_set_.async_ray_histories[dispatch_id]->GetDispatchData();

    if (!dispatch_data.CoordinateIsValid(invocation_id.x, invocation_id.y, invocation_id.z))
    {
        return kRraErrorMalformedData;
    }

    auto& ray_indices = dispatch_data.GetCoordinate(invocation_id.x, invocation_id.y, invocation_id.z).begin_identifiers;

    uint32_t i{0};
    for (RayDispatchBeginIdentifier begin_identifier : ray_indices)
    {
        rta::RayHistory rta_ray{rh->GetRayByIndex(begin_identifier.dispatch_coord_index)};

        if (rta_ray.GetTokenCount() > 0 && (rta_ray.GetToken(begin_identifier.begin_token_index).IsBegin()))
        {
            auto begin_data = reinterpret_cast<const rta::RayHistoryTokenBeginData*>(rta_ray.GetToken(begin_identifier.begin_token_index).GetPayload());
            out_rays[i].tlas_address      = ((uint64_t)begin_data->accelStructAddrHi << 32) | begin_data->accelStructAddrLo;
            out_rays[i].ray_flags         = begin_data->rayFlags;
            out_rays[i].cull_mask         = begin_data->instanceInclusionMask;
            out_rays[i].sbt_record_offset = begin_data->rayContributionToHitGroupIndex;
            out_rays[i].sbt_record_stride = begin_data->geometryMultiplier;
            out_rays[i].miss_index        = begin_data->missShaderIndex;
            out_rays[i].origin[0]         = begin_data->rayDesc.origin.x;
            out_rays[i].origin[1]         = begin_data->rayDesc.origin.y;
            out_rays[i].origin[2]         = begin_data->rayDesc.origin.z;
            out_rays[i].t_min             = begin_data->rayDesc.tMin;
            out_rays[i].direction[0]      = begin_data->rayDesc.direction.x;
            out_rays[i].direction[1]      = begin_data->rayDesc.direction.y;
            out_rays[i].direction[2]      = begin_data->rayDesc.direction.z;
            out_rays[i].t_max             = begin_data->rayDesc.tMax;
            out_rays[i].payload           = 0;  // This data does not seem available.
            out_rays[i].wave_id           = begin_data->hwWaveId;
            out_rays[i].id                = rta_ray.GetRayId();
        }
        else
        {
            return kRraErrorMalformedData;
        }
        ++i;
    }

    return kRraOk;
}

RraErrorCode RraRayGetIntersectionResult(uint32_t dispatch_id, GlobalInvocationID invocation_id, uint32_t ray_index, IntersectionResult* out_result)
{
    if (dispatch_id >= data_set_.async_ray_histories.size())
    {
        return kRraErrorIndexOutOfRange;
    }
    auto                                           rh            = data_set_.async_ray_histories[dispatch_id]->GetRayHistoryTrace();
    auto&                                          dispatch_data = data_set_.async_ray_histories[dispatch_id]->GetDispatchData();
    auto&                                          ray_id = dispatch_data.GetCoordinate(invocation_id.x, invocation_id.y, invocation_id.z).begin_identifiers;
    const std::vector<RayDispatchBeginIdentifier>& ray_indices{ray_id};

    rta::RayHistory rta_ray{rh->GetRayByIndex(ray_indices[ray_index].dispatch_coord_index)};

    *out_result       = {};
    out_result->hit_t = -1.0f;

    // If there are no rays just return.
    if (rta_ray.GetTokenCount() == 0)
    {
        return kRraOk;
    }

    int end_token_index = static_cast<size_t>(rta_ray.GetTokenCount()) - 1;

    // If not the last ray
    if (ray_index < ray_indices.size() - 1)
    {
        // Get the token behind the first token of the next ray.
        end_token_index = ray_id[ray_index + 1].begin_token_index - 1;
    }

    auto token = rta_ray.GetToken(end_token_index);

    if (token.GetType() == rta::RayHistoryTokenType::EndV2)
    {
        auto intersection_data                 = reinterpret_cast<const rta::RayHistoryTokenEndDataV2*>(token.GetPayload());
        out_result->hit_kind                   = intersection_data->hitKind;
        out_result->instance_index             = intersection_data->instanceIndex;
        out_result->geometry_index             = intersection_data->geometryIndex;
        out_result->primitive_index            = intersection_data->primitiveIndex;
        out_result->num_iterations             = intersection_data->numIterations;
        out_result->num_instance_intersections = intersection_data->numInstanceIntersections;

        if (!token.IsMiss())
        {
            out_result->hit_t = intersection_data->hitT;
        }
    }

    return kRraOk;
}

RraErrorCode RraRayGetDispatchStats(uint32_t dispatch_id, RraRayHistoryStats* out_stats)
{
    if (dispatch_id >= data_set_.async_ray_histories.size())
    {
        return kRraErrorIndexOutOfRange;
    }

    *out_stats = data_set_.async_ray_histories[dispatch_id]->GetStats();

    return kRraOk;
}

RraErrorCode RraRayGetDispatchCoordinateStats(uint32_t dispatch_id, GlobalInvocationID invocation_id, DispatchCoordinateStats* out_stats)
{
    if (dispatch_id >= data_set_.async_ray_histories.size())
    {
        return kRraErrorIndexOutOfRange;
    }
    auto& dispatch_data = data_set_.async_ray_histories[dispatch_id]->GetDispatchData();

    if (!dispatch_data.CoordinateIsValid(invocation_id.x, invocation_id.y, invocation_id.z))
    {
        return kRraErrorMalformedData;
    }

    *out_stats = dispatch_data.GetCoordinate(invocation_id.x, invocation_id.y, invocation_id.z).stats;

    return kRraOk;
}

RraErrorCode RraRayGetAnyHitInvocationData(uint32_t           dispatch_id,
                                           GlobalInvocationID invocation_id,
                                           uint32_t           ray_index,
                                           uint32_t*          out_count,
                                           AnyHitRayResult*   result)
{
    if (dispatch_id >= data_set_.async_ray_histories.size())
    {
        return kRraErrorIndexOutOfRange;
    }
    auto  rh            = data_set_.async_ray_histories[dispatch_id]->GetRayHistoryTrace();
    auto& dispatch_data = data_set_.async_ray_histories[dispatch_id]->GetDispatchData();
    auto& ray_indices   = dispatch_data.GetCoordinate(invocation_id.x, invocation_id.y, invocation_id.z).begin_identifiers;

    rta::RayHistory rta_coordinate_tokens{rh->GetRayByIndex(ray_indices[ray_index].dispatch_coord_index)};

    // Current index starts at -1 since the first token will be a begin token, putting it at 0.
    int current_ray_index{-1};
    *result = kAnyHitResultReject;
    for (const rta::RayHistoryToken& token : rta_coordinate_tokens)
    {
        if (token.IsBegin())
        {
            ++current_ray_index;
        }
        else if (((uint32_t)current_ray_index == ray_index) && (token.GetType() == rta::RayHistoryTokenType::AnyHitStatus))
        {
            (*out_count)++;
            rta::RayHistoryAnyHitStatus status = (rta::RayHistoryAnyHitStatus)token.GetControlTokenOptionalData();
            if (status == rta::RayHistoryAnyHitStatus::AcceptHit || status == rta::RayHistoryAnyHitStatus::AcceptHitAndEndSearch)
            {
                *result = kAnyHitResultAccept;
            }
        }
    }

    if (*out_count == 0)
    {
        *result = kAnyHitResultNoAnyHit;
    }

    return kRraOk;
}

RraErrorCode RraRayGetAnyHitInvocationCount(uint32_t dispatch_id, GlobalInvocationID invocation_id, uint32_t* out_count)
{
    if (dispatch_id >= data_set_.async_ray_histories.size())
    {
        return kRraErrorIndexOutOfRange;
    }
    auto  rh            = data_set_.async_ray_histories[dispatch_id]->GetRayHistoryTrace();
    auto& dispatch_data = data_set_.async_ray_histories[dispatch_id]->GetDispatchData();

    if (!dispatch_data.CoordinateIsValid(invocation_id.x, invocation_id.y, invocation_id.z))
    {
        *out_count = 0;
        return kRraErrorMalformedData;
    }

    *out_count = dispatch_data.GetCoordinate(invocation_id.x, invocation_id.y, invocation_id.z).stats.any_hit_count;

    return kRraOk;
}

RraErrorCode RraRayGetDispatchType(uint32_t dispatch_id, DispatchType* type)
{
    if (dispatch_id >= data_set_.async_ray_histories.size())
    {
        return kRraErrorIndexOutOfRange;
    }

    *type = static_cast<DispatchType>(data_set_.async_ray_histories[dispatch_id]->GetCounterInfo().pipelineType);

    return kRraOk;
}

RraErrorCode RraRayGetDispatchStatus(uint32_t dispatch_id, RraDispatchLoadStatus* status)
{
    auto& loader = data_set_.async_ray_histories[dispatch_id];
    *status      = loader->GetStatus();
    return kRraOk;
}

