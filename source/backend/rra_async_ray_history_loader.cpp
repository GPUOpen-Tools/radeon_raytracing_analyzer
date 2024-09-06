//=============================================================================
// Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the asynchronous ray history loader.
//=============================================================================

#include "public/rra_async_ray_history_loader.h"
#include <rdf/rdf/inc/amdrdf.h>
#include <algorithm>
#include <execution>
#include <future>

// How frequently (every n loops) to update the loading progress bar.
// Higher values reduce amount of locking, and increase performance.
constexpr uint32_t kLoadUpdateFrequency{200};

struct AsyncLoaderRayHistoryData
{
    std::shared_ptr<rta::RayHistoryTrace> ray_history_trace = nullptr;
};

// Is the chunk version less than the version passed in.
//
// @param chunk_version  The chunk version.
// @param major_version  The major version requested.
// @param minor_version  The minor version requested.
//
// @return true if chunk_version < requested version, false otherwise.
static bool ChunkVersionLessThan(uint32_t chunk_version, uint32_t major_version, uint32_t minor_version)
{
    uint32_t requested_version = (major_version << 16) | minor_version;
    if (chunk_version < requested_version)
    {
        return true;
    }
    return false;
}

RraAsyncRayHistoryLoader::RraAsyncRayHistoryLoader(const char* file_path, int64_t dispatch_index)
{
    dispatch_index_ = dispatch_index;
    {
        auto           file       = rdf::Stream::OpenFile(file_path);
        rdf::ChunkFile chunk_file = rdf::ChunkFile(file);

        // Get chunk version number.
        uint32_t version = chunk_file.GetChunkVersion(RRA_RAY_HISTORY_TOKENS_METADATA_IDENTIFIER);

        // Read chunk into a buffer.
        auto                   metadata_chunk_size = chunk_file.GetChunkDataSize(RRA_RAY_HISTORY_TOKENS_METADATA_IDENTIFIER);
        std::vector<std::byte> metadata_buffer(metadata_chunk_size);
        chunk_file.ReadChunkDataToBuffer(RRA_RAY_HISTORY_TOKENS_METADATA_IDENTIFIER, (int)dispatch_index, metadata_buffer.data());

        // Offset to keep track of where we are.
        int64_t offset = 0;

        // Iterate through each metadata field and load them if possible.
        while (offset <= metadata_chunk_size)
        {
            rta::RayHistoryMetadataInfo current_info = {};
            std::memcpy(&current_info, metadata_buffer.data() + offset, sizeof(rta::RayHistoryMetadataInfo));
            offset += sizeof(rta::RayHistoryMetadataInfo);

            if (current_info.kind == rta::RayHistoryMetadataKind::DXC_RayTracingCounterInfo)
            {
                // Copy CounterInfo struct.
                if (ChunkVersionLessThan(version, 1, 1))
                {
                    // Back compat. file versions < 1.1
                    struct CounterInfo_V0
                    {
                        GpuRt::uint32 dispatchRayDimensionX;      // DispatchRayDimension X
                        GpuRt::uint32 dispatchRayDimensionY;      // DispatchRayDimension Y
                        GpuRt::uint32 dispatchRayDimensionZ;      // DispatchRayDimension Z
                        GpuRt::uint32 hitGroupShaderRecordCount;  // Hit-group shader record count
                        GpuRt::uint32 missShaderRecordCount;      // Miss shader record count
                        GpuRt::uint32 pipelineShaderCount;        // Pipeline per-shader count
                        GpuRt::uint64 stateObjectHash;            // State object hash
                        GpuRt::uint32 counterMode;                // Counter mode
                        GpuRt::uint32 counterMask;                // Traversal counter mask
                        GpuRt::uint32 counterStride;              // Ray tracing counter stride
                        GpuRt::uint32 rayCounterDataSize;         // Per-ray counter data
                        GpuRt::uint32 lostTokenBytes;             // Total lost token bytes
                        GpuRt::uint32 counterRayIdRangeBegin;     // Partial rayID range begin
                        GpuRt::uint32 counterRayIdRangeEnd;       // Partial rayID range end
                        GpuRt::uint32 pipelineType;               // Pipeline type (native RT or RayQuery). RayTracing=0, Compute=1, Graphics=2
                    };
                    std::memcpy(&counter_info_, metadata_buffer.data() + offset, sizeof(CounterInfo_V0));
                    counter_info_.isIndirect = 0;
                }
                else
                {
                    std::memcpy(&counter_info_, metadata_buffer.data() + offset, sizeof(GpuRt::CounterInfo));
                }
            }
            else if (current_info.kind == rta::RayHistoryMetadataKind::UserMarkerInfo)
            {
                // Copy user marker context.
                std::memcpy(&user_marker_context_, metadata_buffer.data() + offset, sizeof(uint64_t));  
                break;
            }

            offset += current_info.sizeInByte;
        }
    }

    if (counter_info_.lostTokenBytes > 0)
    {
        error_state_                 = true;
        load_status_.incomplete_data = true;
    }

    dim_x_ = counter_info_.dispatchRayDimensionX;
    dim_y_ = counter_info_.dispatchRayDimensionY;
    dim_z_ = counter_info_.dispatchRayDimensionZ;

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
        {
            std::scoped_lock<std::mutex> plock(process_mutex_);
            load_status_.data_decompressed = true;
            load_status_.has_errors        = error_state_;
        }

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
    if (process_complete_local_)
    {
        return true;
    }

    std::scoped_lock<std::mutex> plock(process_mutex_);
    process_complete_local_ = process_complete_;
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
    if (process_complete_local_)
    {
        return;
    }

    bool get_the_future = false;
    {
        std::scoped_lock<std::mutex> plock(process_mutex_);
        get_the_future          = !process_complete_;
        process_complete_local_ = process_complete_;
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

rta::RayHistoryTrace* RraAsyncRayHistoryLoader::GetRayHistoryTrace()
{
    WaitProcess();
    return ray_history_trace_.get();
}

GpuRt::CounterInfo RraAsyncRayHistoryLoader::GetCounterInfo()
{
    return counter_info_;
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
        size_t   token_byte_start;
        size_t   token_byte_current_idx;
        uint32_t current_index;

        bool beginToken = false;
        bool endToken   = false;
    };

    std::vector<RayStateAsyncLoader> rayData(dx * dy * dz);
    int                              totalTokenCount = 0;

    const auto bufferStart          = buffer_data;
    const auto bufferEnd            = buffer_data + buffer_size;
    auto       CheckOffsetIsInRange = [bufferStart, bufferEnd](const void* p, const size_t size) -> bool {
        return p >= bufferStart && (static_cast<const std::byte*>(p) + size) <= bufferEnd;
    };

    // First pass over buffer_size counts token size and index count.
    size_t   offset   = 0;
    uint32_t loop_idx = 0;
    while (offset < buffer_size)
    {
        if (loop_idx++ % kLoadUpdateFrequency == 0)
        {
            std::scoped_lock<std::mutex> plock(process_mutex_);
            bytes_processed_ = offset / 2;
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

            ++rayState.current_index;
            size_t token_data_size{sizeof(RayHistoryTokenControl) + static_cast<std::size_t>(control->tokenLength) * 4};  // Size in DWORDS.
            rayState.token_byte_start += token_data_size;

            offset += token_data_size;
        }
        else
        {
            // No need to check anything here because the control token is
            // fully available
            offset += sizeof(RayHistoryTokenId);

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

            ++rayState.current_index;
            rayState.token_byte_start += sizeof(RayHistoryTokenControl);

            offset += sizeof(RayHistoryTokenControl);
        }
    }

    // Compute partial sums which will act as begin indices into combined vectors. And make combined ray ranges.
    std::vector<RayHistoryTrace::RayRange> combinedRayRanges;
    combinedRayRanges.reserve(rayData.size());
    size_t   token_partial_sum = 0;
    uint32_t index_partial_sum = 0;
    uint32_t rs_id{0};
    for (RayStateAsyncLoader& rs : rayData)
    {
        const RayHistoryTrace::RayRange range = {rs_id++, static_cast<std::uint32_t>(index_partial_sum), rs.current_index, token_partial_sum};
        combinedRayRanges.push_back(range);

        size_t token_start_offset{token_partial_sum};
        token_partial_sum += rs.token_byte_start;
        rs.token_byte_start       = token_start_offset;
        rs.token_byte_current_idx = token_start_offset;

        uint32_t index_start_offset{index_partial_sum};
        index_partial_sum += rs.current_index;
        rs.current_index = index_start_offset;
    }

    std::vector<std::byte> combinedTokenData;
    combinedTokenData.resize(token_partial_sum);

    std::vector<RayHistoryTrace::TokenIndex> combinedTokenIndices;
    combinedTokenIndices.resize(totalTokenCount);

    // Second pass over buffer_data populating all the combined vectors.
    offset = 0;
    while (offset < buffer_size)
    {
        if (loop_idx++ % kLoadUpdateFrequency == 0)
        {
            std::scoped_lock<std::mutex> plock(process_mutex_);
            bytes_processed_ = (buffer_size + offset) / 2;
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

        readPointer = buffer_data + offset + sizeof(RayHistoryTokenId);

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

            const auto                        tokenStart   = rayState.token_byte_current_idx - rayState.token_byte_start;
            const RayHistoryTrace::TokenIndex tokenIndex   = {static_cast<uint32_t>(tokenStart), 1};
            combinedTokenIndices[rayState.current_index++] = tokenIndex;

            size_t token_data_size{sizeof(RayHistoryTokenControl) + static_cast<std::size_t>(control->tokenLength) * 4};  // Size in DWORDS.
            std::memcpy(&combinedTokenData[rayState.token_byte_current_idx], buffer_data + offset, token_data_size);
            rayState.token_byte_current_idx += token_data_size;

            offset += token_data_size;
        }
        else
        {
            // No need to check anything here because the control token is
            // fully available
            offset += sizeof(RayHistoryTokenId);

            // Check if TokenId and TokenControl are invalid, i.e. all zero
            if (std::all_of(buffer_data + offset - sizeof(RayHistoryTokenId), buffer_data + offset + sizeof(RayHistoryTokenControl), [](const std::byte b) {
                    return b == std::byte();
                }))
            {
                offset += sizeof(RayHistoryTokenControl);
                continue;
            }

            const auto                        tokenStart   = rayState.token_byte_current_idx - rayState.token_byte_start;
            const RayHistoryTrace::TokenIndex tokenIndex   = {static_cast<uint32_t>(tokenStart), 0};
            combinedTokenIndices[rayState.current_index++] = tokenIndex;

            std::memcpy(&combinedTokenData[rayState.token_byte_current_idx], buffer_data + offset, sizeof(RayHistoryTokenControl));
            rayState.token_byte_current_idx += sizeof(RayHistoryTokenControl);

            offset += sizeof(RayHistoryTokenControl);
        }
    }

    auto result =
        std::make_shared<RayHistoryTrace>(std::move(combinedTokenData), std::move(combinedTokenIndices), std::move(combinedRayRanges), (dx * dy * dz) - 1);

    const DispatchSize dispatchSize = {dx, dy, dz};

    // Sanity check: #dispatched pixels = #rays
    const auto rayCount = result->GetRayCount(RayHistoryTrace::IncludeEmptyRays);
    if ((dispatchSize.width * dispatchSize.height * dispatchSize.depth) != (uint32_t)rayCount)
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
    std::vector<uint32_t> dispatch_coord_indices(dispatch_coord_count);
    std::iota(dispatch_coord_indices.begin(), dispatch_coord_indices.end(), 0);
    std::for_each(std::execution::par, dispatch_coord_indices.begin(), dispatch_coord_indices.end(), [&](int dispatch_coord_index) {
        // Count number of processed dispatch indices per thread and only periodically update that value to main thread to avoid unnecessary locking.
        thread_local uint32_t local_processed_dispatch_indices{};

        if (local_processed_dispatch_indices++ == kLoadUpdateFrequency)
        {
            {
                std::scoped_lock<std::mutex> plock(process_mutex_);
                processed_dispatch_indices_ += local_processed_dispatch_indices;
            }
            local_processed_dispatch_indices = 0;
        }

        // First pass. Just count rays for next pass.
        rta::RayHistory          ray{rh->GetRayByIndex(dispatch_coord_index)};
        DispatchCoordinateStats* coordinate_stats{nullptr};
        for (rta::RayHistoryToken token : ray)
        {
            if (token.IsBegin())
            {
                auto     begin_data = reinterpret_cast<const rta::RayHistoryTokenBeginDataV2*>(token.GetPayload());
                uint32_t x          = begin_data->dispatchRaysIndex[0];
                uint32_t y          = begin_data->dispatchRaysIndex[1];
                uint32_t z          = begin_data->dispatchRaysIndex[2];

                if (!dispatch_data.CoordinateIsValid(x, y, z))
                {
                    std::scoped_lock<std::mutex> plock(process_mutex_);
                    error_state_ = true;
                    return;
                }

                coordinate_stats = &dispatch_data.GetCoordinate(x, y, z).stats;

                // Initially just count rays to use in next pass to record data about them.
                ++coordinate_stats->ray_count;
            }

            if (coordinate_stats)
            {
                if (token.GetType() == rta::RayHistoryTokenType::AnyHitStatus)
                {
                    ++coordinate_stats->any_hit_count;
                }

                if (token.GetType() == rta::RayHistoryTokenType::EndV2)
                {
                    auto end_token = reinterpret_cast<const rta::RayHistoryTokenEndDataV2*>(token.GetPayload());
                    coordinate_stats->loop_iteration_count += end_token->numIterations;
                    coordinate_stats->intersection_count += end_token->numInstanceIntersections;
                }
            }
        }

        // Second pass after we've counted number of rays in dispatch to pre-allocate begin identifiers.
        uint32_t begin_token_index = 0;
        for (rta::RayHistoryToken token : ray)
        {
            if (token.IsBegin())
            {
                auto                    begin_data      = reinterpret_cast<const rta::RayHistoryTokenBeginDataV2*>(token.GetPayload());
                uint32_t                x               = begin_data->dispatchRaysIndex[0];
                uint32_t                y               = begin_data->dispatchRaysIndex[1];
                uint32_t                z               = begin_data->dispatchRaysIndex[2];
                DispatchCoordinateData& coordinate_data = dispatch_data.GetCoordinate(x, y, z);

                // For the first ray we encounter of a dispatch coordinate, we do one allocation for all the rays in this coordinate.
                if (coordinate_data.begin_identifiers.empty())
                {
                    coordinate_data.begin_identifiers.reserve(coordinate_data.stats.ray_count);
                    std::scoped_lock<std::mutex> plock(process_mutex_);
                    total_ray_count_ += coordinate_data.stats.ray_count;
                }

                coordinate_data.begin_identifiers.emplace_back((uint32_t)dispatch_coord_index, begin_token_index);
            }

            ++begin_token_index;
        }
    });

    // Add data to the dispatch list.
    dispatch_data_ = std::move(dispatch_data);
}

void RraAsyncRayHistoryLoader::ProcessInvocationCounts()
{
    if (error_state_)
    {
        return;
    }

    {
        std::scoped_lock<std::mutex> plock(process_mutex_);
        invocation_counts_.raygen_count = dim_x_ * dim_y_ * dim_z_;

        // This may change some day.
        invocation_counts_.pixel_count = invocation_counts_.raygen_count;

        invocation_counts_.ray_count = total_ray_count_;
    }

    auto&              rh = ray_history_trace_;
    RraRayHistoryStats local_stats{};  // Stats local to this thread, to be periodically synched with main thread.

    const int dispatch_coord_count{rh->GetRayCount(rta::RayHistoryTrace::ExcludeEmptyRays)};
    uint32_t  loop_idx{0};
    for (int dispatch_coord_index{0}; dispatch_coord_index < dispatch_coord_count; ++dispatch_coord_index)
    {
        const rta::RayHistory& rta_ray{rh->GetRayByIndex(dispatch_coord_index)};

        for (const rta::RayHistoryToken& token : rta_ray)
        {
            if (token.GetType() == rta::RayHistoryTokenType::ProceduralIntersectionStatus)
            {
                local_stats.intersection_count++;
            }
            else if (token.GetType() == rta::RayHistoryTokenType::AnyHitStatus)
            {
                local_stats.any_hit_count++;
            }
            else if (token.GetType() == rta::RayHistoryTokenType::EndV2)
            {
                auto end_token = reinterpret_cast<const rta::RayHistoryTokenEndDataV2*>(token.GetPayload());

                if (token.IsMiss())
                {
                    local_stats.miss_count++;
                }
                else
                {
                    local_stats.closest_hit_count++;
                }

                local_stats.loop_iteration_count += end_token->numIterations;
                local_stats.instance_intersection_count += end_token->numInstanceIntersections;
            }

            // Periodically synch values with the main thread.
            if (loop_idx++ % kLoadUpdateFrequency == 0)
            {
                std::scoped_lock<std::mutex> plock(process_mutex_);
                invocation_counts_.intersection_count          = local_stats.intersection_count;
                invocation_counts_.any_hit_count               = local_stats.any_hit_count;
                invocation_counts_.miss_count                  = local_stats.miss_count;
                invocation_counts_.closest_hit_count           = local_stats.closest_hit_count;
                invocation_counts_.loop_iteration_count        = local_stats.loop_iteration_count;
                invocation_counts_.instance_intersection_count = local_stats.instance_intersection_count;
            }
        }
    }
}
