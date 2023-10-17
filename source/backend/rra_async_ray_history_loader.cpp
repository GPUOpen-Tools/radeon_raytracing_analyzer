//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the asynchronous ray history loader.
//=============================================================================

#include "public/rra_async_ray_history_loader.h"
#include <rdf/rdf/inc/amdrdf.h>
#include <future>
#include <map>

struct AsyncLoaderRayHistoryData
{
    rta::RayHistoryMetadata               metadata          = {};
    std::shared_ptr<rta::RayHistoryTrace> ray_history_trace = nullptr;
};

RraAsyncRayHistoryLoader::RraAsyncRayHistoryLoader(const char* file_path, int64_t dispatch_index)
{
    dispatch_index_ = dispatch_index;
    {
        auto           file       = rdf::Stream::OpenFile(file_path);
        rdf::ChunkFile chunk_file = rdf::ChunkFile(file);
        chunk_file.ReadChunkDataToBuffer(RRA_RAY_HISTORY_TOKENS_METADATA_IDENTIFIER, (int)dispatch_index, &metadata_);
    }

    if (metadata_.counter.lostTokenBytes > 0)
    {
        error_state_                 = true;
        load_status_.incomplete_data = true;
    }

    dim_x_ = metadata_.counter.dispatchRayDimensionX;
    dim_y_ = metadata_.counter.dispatchRayDimensionY;
    dim_z_ = metadata_.counter.dispatchRayDimensionZ;

    total_dispatch_indices_ = dim_x_ * dim_y_ * dim_z_;

    process_ = std::async(std::launch::async, [=]() {
        auto           file       = rdf::Stream::OpenFile(file_path);
        rdf::ChunkFile chunk_file = rdf::ChunkFile(file);

        size_t buffer_size = chunk_file.GetChunkDataSize(RRA_RAY_HISTORY_RAW_TOKENS_IDENTIFIER, (int)dispatch_index);
        {
            std::scoped_lock<std::mutex> plock(process_mutex_);
            bytes_required_  = buffer_size;
            bytes_processed_ = 0;
        }

        std::byte* byte_buffer = static_cast<std::byte*>(malloc(buffer_size));

        chunk_file.ReadChunkDataToBuffer(RRA_RAY_HISTORY_RAW_TOKENS_IDENTIFIER, (int)dispatch_index, byte_buffer);
        file.Close();

        ReadRayHistoryTraceFromRawBuffer(buffer_size, byte_buffer, dim_x_, dim_y_, dim_z_);
        free(byte_buffer);
        {
            std::scoped_lock<std::mutex> plock(process_mutex_);
            load_status_.raw_data_parsed = true;
            load_status_.has_errors      = error_state_;
        }

        PreProcessDispatchData();
        {
            std::scoped_lock<std::mutex> plock(process_mutex_);
            load_status_.data_indexed = true;
            load_status_.has_errors   = error_state_;
        }

        ProcessInvocationCounts();
        {
            std::scoped_lock<std::mutex> plock(process_mutex_);
            process_complete_             = true;
            load_status_.loading_complete = true;
            load_status_.has_errors       = error_state_;
        }
    });
}

bool RraAsyncRayHistoryLoader::IsDone()
{
    std::scoped_lock<std::mutex> plock(process_mutex_);
    return process_complete_;
}

float RraAsyncRayHistoryLoader::GetProcessPercentage()
{
    float percentage = 0.0f;
    {
        std::scoped_lock<std::mutex> plock(process_mutex_);

        float file_progress     = float(double(bytes_processed_) / double(bytes_required_)) * 50.0f;
        float indexing_progress = float(double(processed_dispatch_indices_) / double(total_dispatch_indices_)) * 50.0f;

        percentage = file_progress + indexing_progress;
    }

    if (IsDone() && !HasErrors())
    {
        percentage = 100.0f;
    }

    return percentage;
}

bool RraAsyncRayHistoryLoader::HasErrors()
{
    std::scoped_lock<std::mutex> plock(process_mutex_);
    return error_state_;
}

void RraAsyncRayHistoryLoader::WaitProcess()
{
    bool get_the_future = false;
    {
        std::scoped_lock<std::mutex> plock(process_mutex_);
        get_the_future = !process_complete_;
    }

    if (get_the_future)
    {
        process_.get();
    }
}

void RraAsyncRayHistoryLoader::FindDispatchDimsFromTokens()
{
    if (total_dispatch_indices_ != 0)
    {
        return;
    }

    auto& rh = ray_history_trace_;
    int   dispatch_coord_count{rh->GetRayCount(rta::RayHistoryTrace::ExcludeEmptyRays)};

    // Gather indices
    for (int dispatch_coord_index{0}; dispatch_coord_index < dispatch_coord_count; ++dispatch_coord_index)
    {
        rta::RayHistory ray{rh->GetRayByIndex(dispatch_coord_index)};

        for (rta::RayHistoryToken token : ray)
        {
            if (token.IsBegin())
            {
                auto begin_data = reinterpret_cast<const rta::RayHistoryTokenBeginDataV2*>(token.GetPayload());
                dim_x_          = std::max(dim_x_, begin_data->dispatchRaysIndex[0]);
                dim_y_          = std::max(dim_y_, begin_data->dispatchRaysIndex[1]);
                dim_z_          = std::max(dim_z_, begin_data->dispatchRaysIndex[2]);
            }
        }
    }

    // Increment each dim by one since these are dimension sizes and not indices.
    dim_x_ += 1;
    dim_y_ += 1;
    dim_z_ += 1;

    total_dispatch_indices_ = dim_x_ * dim_y_ * dim_z_;
}

std::shared_ptr<rta::RayHistoryTrace> RraAsyncRayHistoryLoader::GetRayHistoryTrace()
{
    WaitProcess();

    return ray_history_trace_;
}

rta::RayHistoryMetadata RraAsyncRayHistoryLoader::GetMetadata()
{
    return metadata_;
}

RayDispatchData& RraAsyncRayHistoryLoader::GetDispatchData()
{
    WaitProcess();
    return dispatch_data_;
}

RraRayHistoryStats RraAsyncRayHistoryLoader::GetStats()
{
    std::scoped_lock<std::mutex> plock(process_mutex_);
    return invocation_counts_;
}

size_t RraAsyncRayHistoryLoader::GetTotalRayCount()
{
    std::scoped_lock<std::mutex> plock(process_mutex_);
    return total_ray_count_;
}

rta::DispatchSize RraAsyncRayHistoryLoader::GetDerivedDispatchSize() const
{
    rta::DispatchSize dispatch_size = {};

    dispatch_size.width  = dim_x_;
    dispatch_size.height = dim_y_;
    dispatch_size.depth  = dim_z_;

    return dispatch_size;
}

RraDispatchLoadStatus RraAsyncRayHistoryLoader::GetStatus()
{
    RraDispatchLoadStatus status = {};
    {
        std::scoped_lock<std::mutex> plock(process_mutex_);
        status = load_status_;
    }
    status.load_percentage = GetProcessPercentage();
    return status;
}

void RraAsyncRayHistoryLoader::ReadRayHistoryTraceFromRawBuffer(size_t buffer_size, std::byte* buffer_data, uint32_t dx, uint32_t dy, uint32_t dz)
{
    if (error_state_)
    {
        return;
    }

    using namespace rta;

    struct RayStateAsyncLoader
    {
        std::vector<std::byte> tokenData;
        // Offsets of token starts
        std::vector<RayHistoryTrace::TokenIndex> index;

        bool beginToken = false;
        bool endToken   = false;
    };

    // Using a map here improves compression ratio on Windows by 2x, as
    // consecutive indices compress _much_ better. On Linux the effect is
    // minimal as the unordered_map implementation there is well sorted
    std::map<uint32_t, RayStateAsyncLoader> rayData;
    int                                     totalTokenCount = 0;

    const auto bufferStart          = buffer_data;
    const auto bufferEnd            = buffer_data + buffer_size;
    auto       CheckOffsetIsInRange = [bufferStart, bufferEnd](const void* p, const size_t size) -> bool {
        return p >= bufferStart && (static_cast<const std::byte*>(p) + size) <= bufferEnd;
    };

    for (size_t offset = 0, end = buffer_size; offset < end;)
    {
        {
            std::scoped_lock<std::mutex> plock(process_mutex_);
            bytes_processed_ = offset;
        }

        const void* readPointer = buffer_data + offset;
        // We've reached the end of the stream, can't read anything here
        if (!CheckOffsetIsInRange(readPointer, sizeof(RayHistoryTokenId)))
        {
            break;
        }
        // Tokens
        const auto id = static_cast<const RayHistoryTokenId*>(readPointer);

        auto& rayState = rayData[id->id];

        ++totalTokenCount;

        readPointer = buffer_data + offset + sizeof(RayHistoryTokenId);

        // The read pointer is pointing at the beginning of the control
        // token, so check if we can read the whole control token. If not the
        // trace is definitely broken
        if (!CheckOffsetIsInRange(readPointer, sizeof(RayHistoryTokenControl)))
        {
            {
                std::scoped_lock<std::mutex> plock(process_mutex_);
                error_state_ = true;
            }
            break;
        }

        if (id->control)
        {
            // If it's a control word, then the payload comes right after
            // this one
            const auto control = static_cast<const RayHistoryTokenControl*>(readPointer);
            offset += sizeof(RayHistoryTokenId);

            // Check that we can actually read the whole token payload. Note
            // that the read pointer didn't move yet so we need to check for
            // both the token and the payload. At this point we know
            // that we can dereference control, because that's checked before
            // this loop
            if (!CheckOffsetIsInRange(readPointer, sizeof(RayHistoryTokenControl) + control->tokenLength))
            {
                {
                    std::scoped_lock<std::mutex> plock(process_mutex_);
                    error_state_ = true;
                }
                break;
            }

            const auto tokenStart = rayState.tokenData.size();

            //assert(tokenStart <= std::numeric_limits<std::uint32_t>::max());

            const RayHistoryTrace::TokenIndex tokenIndex = {static_cast<uint32_t>(tokenStart), 1};
            rayState.index.push_back(tokenIndex);

            switch (control->type)
            {
            case RayHistoryTokenType::Begin:
                assert(control->tokenLength * 4 == sizeof(RayHistoryTokenBeginData));
                rayState.beginToken = true;
                break;
            case RayHistoryTokenType::BeginV2:
                assert(control->tokenLength * 4 == sizeof(RayHistoryTokenBeginDataV2));
                rayState.beginToken = true;
                break;
            case RayHistoryTokenType::AnyHitStatus:
                assert(control->tokenLength == 0);
                break;
            case RayHistoryTokenType::FunctionCallV2:
                // The new function call control token has length 3
                assert(control->tokenLength == 3);
                break;
            case RayHistoryTokenType::End:
                assert(control->tokenLength * 4 == sizeof(RayHistoryTokenEndData));
                rayState.endToken = true;
                break;
            case RayHistoryTokenType::EndV2:
                assert(control->tokenLength * 4 == sizeof(RayHistoryTokenEndDataV2));
                rayState.endToken = true;
                break;
            default:
                // All other tokens have length 2
                //assert(control->tokenLength == 2);
                break;
            }

            rayState.tokenData.insert(
                rayState.tokenData.end(),
                buffer_data + offset,
                buffer_data + offset + sizeof(RayHistoryTokenControl) + static_cast<std::size_t>(control->tokenLength) * 4 /* size in DWORDS */);

            offset += sizeof(RayHistoryTokenControl);
            offset += static_cast<std::size_t>(control->tokenLength) * 4 /* size in DWORDS */;
        }
        else
        {
            // No need to check anything here because the control token is
            // fully available
            offset += sizeof(RayHistoryTokenId);

            const auto tokenStart = rayState.tokenData.size();

            // Check if TokenId and TokenControl are invalid, i.e. all zero
            if (std::all_of(buffer_data + offset - sizeof(RayHistoryTokenId), buffer_data + offset + sizeof(RayHistoryTokenControl), [](const std::byte b) {
                    return b == std::byte();
                }))
            {
                offset += sizeof(RayHistoryTokenControl);
                // Do not count invalid tokens
                --totalTokenCount;
                continue;
            }

            const RayHistoryTrace::TokenIndex tokenIndex = {static_cast<uint32_t>(tokenStart), 0};

            rayState.index.push_back(tokenIndex);

            rayState.tokenData.insert(rayState.tokenData.end(), buffer_data + offset, buffer_data + offset + sizeof(RayHistoryTokenControl));

            offset += sizeof(RayHistoryTokenControl);
        }
    }

    // At this point, we have the data in rayData. We now compact it such
    // that all tokenData entries get pasted together, and adjust the
    // indices accordingly.
    std::size_t totalCombinedTokenSize = 0;

    for (const auto& kv : rayData)
    {
        totalCombinedTokenSize += kv.second.tokenData.size();
        RRA_ASSERT(kv.second.beginToken);
    }

    std::vector<std::byte> combinedTokenData;
    combinedTokenData.reserve(totalCombinedTokenSize);

    std::vector<RayHistoryTrace::TokenIndex> combinedTokenIndices;
    combinedTokenIndices.reserve(totalTokenCount);

    std::vector<RayHistoryTrace::RayRange> combinedRayRanges;
    combinedRayRanges.reserve(rayData.size());

    for (const auto& kv : rayData)
    {
        const RayHistoryTrace::RayRange range = {
            kv.first, static_cast<std::uint32_t>(combinedTokenIndices.size()), static_cast<std::uint32_t>(kv.second.index.size()), combinedTokenData.size()};

        combinedRayRanges.push_back(range);

        combinedTokenData.insert(combinedTokenData.end(), kv.second.tokenData.begin(), kv.second.tokenData.end());

        combinedTokenIndices.insert(combinedTokenIndices.end(), kv.second.index.begin(), kv.second.index.end());
    }

    auto result =
        std::make_shared<RayHistoryTrace>(std::move(combinedTokenData), std::move(combinedTokenIndices), std::move(combinedRayRanges), (dx * dy * dz) - 1);

    const DispatchSize dispatchSize = {dx, dy, dz};

    // Sanity check: #dispatched pixels = #rays
    if (const auto rayCount = result->GetRayCount(RayHistoryTrace::IncludeEmptyRays);
        (dispatchSize.width * dispatchSize.height * dispatchSize.depth) != (uint32_t)rayCount)
    {
        std::scoped_lock<std::mutex> plock(process_mutex_);
        if (total_dispatch_indices_ > 0)
        {
            error_state_ = true;
        }
    }

    ray_history_trace_ = result;
}

void RraAsyncRayHistoryLoader::PreProcessDispatchData()
{
    if (error_state_)
    {
        return;
    }

    FindDispatchDimsFromTokens();

    auto& rh = ray_history_trace_;
    int   dispatch_coord_count{rh->GetRayCount(rta::RayHistoryTrace::ExcludeEmptyRays)};

    rta::DispatchSize dispatch_size = {};
    dispatch_size.width             = dim_x_;
    dispatch_size.height            = dim_y_;
    dispatch_size.depth             = dim_z_;

    RayDispatchData dispatch_data = {};

    // Resize indices
    dispatch_data.dispatch_width  = dispatch_size.width;
    dispatch_data.dispatch_height = dispatch_size.height;

    dispatch_data.dispatch_ray_indices.resize(dispatch_size.width * dispatch_size.height * dispatch_size.depth);

    // Gather indices
    for (int dispatch_coord_index{0}; dispatch_coord_index < dispatch_coord_count; ++dispatch_coord_index)
    {
        processed_dispatch_indices_++;

        rta::RayHistory ray{rh->GetRayByIndex(dispatch_coord_index)};

        uint32_t x = 0;
        uint32_t y = 0;
        uint32_t z = 0;

        uint32_t begin_token_index = 0;

        for (rta::RayHistoryToken token : ray)
        {
            if (token.IsBegin())
            {
                auto begin_data = reinterpret_cast<const rta::RayHistoryTokenBeginDataV2*>(token.GetPayload());
                x               = begin_data->dispatchRaysIndex[0];
                y               = begin_data->dispatchRaysIndex[1];
                z               = begin_data->dispatchRaysIndex[2];

                RayDispatchBeginIdentifier begin_identifier = {(uint32_t)dispatch_coord_index, begin_token_index};

                if (!dispatch_data.CoordinateIsValid(x, y, z))
                {
                    std::scoped_lock<std::mutex> plock(process_mutex_);
                    error_state_ = true;
                    return;
                }

                dispatch_data.GetCoordinate(x, y, z).begin_identifiers.push_back(begin_identifier);
                dispatch_data.GetCoordinate(x, y, z).stats.ray_count++;

                std::scoped_lock<std::mutex> plock(process_mutex_);
                total_ray_count_++;
            }

            if (token.GetType() == rta::RayHistoryTokenType::AnyHitStatus)
            {
                dispatch_data.GetCoordinate(x, y, z).stats.any_hit_count++;
            }

            if (token.GetType() == rta::RayHistoryTokenType::EndV2)
            {
                auto end_token = reinterpret_cast<const rta::RayHistoryTokenEndDataV2*>(token.GetPayload());
                dispatch_data.GetCoordinate(x, y, z).stats.loop_iteration_count += end_token->numIterations;
                dispatch_data.GetCoordinate(x, y, z).stats.intersection_count += end_token->numInstanceIntersections;
            }

            begin_token_index++;
        }
    }

    // Add data to the dispatch list.
    dispatch_data_ = dispatch_data;
}

void RraAsyncRayHistoryLoader::ProcessInvocationCounts()
{
    if (error_state_)
    {
        return;
    }

    invocation_counts_.raygen_count = dim_x_ * dim_y_ * dim_z_;

    // This may change some day.
    invocation_counts_.pixel_count = invocation_counts_.raygen_count;

    invocation_counts_.ray_count = total_ray_count_;

    auto& rh = ray_history_trace_;

    const int dispatch_coord_count{rh->GetRayCount(rta::RayHistoryTrace::ExcludeEmptyRays)};
    for (int dispatch_coord_index{0}; dispatch_coord_index < dispatch_coord_count; ++dispatch_coord_index)
    {
        const rta::RayHistory& rta_ray{rh->GetRayByIndex(dispatch_coord_index)};

        for (const rta::RayHistoryToken& token : rta_ray)
        {
            std::scoped_lock<std::mutex> plock(process_mutex_);

            if (token.GetType() == rta::RayHistoryTokenType::ProceduralIntersectionStatus)
            {
                invocation_counts_.intersection_count++;
            }
            else if (token.GetType() == rta::RayHistoryTokenType::AnyHitStatus)
            {
                invocation_counts_.any_hit_count++;
            }
            else if (token.GetType() == rta::RayHistoryTokenType::EndV2)
            {
                auto end_token = reinterpret_cast<const rta::RayHistoryTokenEndDataV2*>(token.GetPayload());

                if (token.IsMiss())
                {
                    invocation_counts_.miss_count++;
                }
                else
                {
                    invocation_counts_.closest_hit_count++;
                }

                invocation_counts_.loop_iteration_count += end_token->numIterations;
                invocation_counts_.instance_intersection_count += end_token->numInstanceIntersections;
            }
        }
    }
}
