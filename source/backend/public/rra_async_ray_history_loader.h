//=============================================================================
// Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the asynchronous ray history loader.
//=============================================================================

#ifndef RRA_BACKEND_PUBLIC_ASYNC_RAY_HISTORY_LOADER_H_
#define RRA_BACKEND_PUBLIC_ASYNC_RAY_HISTORY_LOADER_H_

#include <future>
#include <memory>
#include <mutex>

#include "rra_ray_history.h"
#include <ray_history/ray_history.h>

/// @brief A class to load the ray history data.
class RraAsyncRayHistoryLoader
{
public:
    RraAsyncRayHistoryLoader(const char* file_path, int64_t dispatch_index);

    /// @brief Check if the loader has finished it's work.
    /// @return True if the loader is done.
    bool IsDone();

    /// @brief Check if the loader has encountered errors.
    /// @return True if there was an error.
    bool HasErrors();

    /// @brief Get the raw trace.
    /// @return The trace.
    rta::RayHistoryTrace* GetRayHistoryTrace();

    /// @brief Get the counter info of the dispatch.
    /// @return The counter info.
    GpuRt::CounterInfo GetCounterInfo();

    /// @brief Get the indexed dispatch data.
    /// @return The dispatch data.
    RayDispatchData& GetDispatchData();

    /// @brief Get stats of the dispatch.
    /// @return The stats.
    RraRayHistoryStats GetStats();

    /// @brief Get the status of the loader.
    /// @return The status.
    RraDispatchLoadStatus GetStatus();

    /// @brief Get the total ray count of the dispatch.
    /// @return The number of rays.
    size_t GetTotalRayCount();

    /// @brief Get derived dispatch size.
    /// @return The derived dispatch size.
    rta::DispatchSize GetDerivedDispatchSize() const;

private:
    /// @brief Get the percentage of the loader's progress.
    /// @return The loaded percentage.
    float GetProcessPercentage();

    /// @brief Read all the data from the raw buffer.
    /// @param buffer_size The buffer size in bytes.
    /// @param buffer_data The buffer pointer.
    /// @param dx X dimension size.
    /// @param dy Y dimension size.
    /// @param dz Z dimension size.
    void ReadRayHistoryTraceFromRawBuffer(size_t buffer_size, std::byte* buffer_data, uint32_t dx, uint32_t dy, uint32_t dz);

    /// @brief Process dispatch data.
    void PreProcessDispatchData();

    /// @brief Process the invocation counts.
    void ProcessInvocationCounts();

    /// @brief Waits for everthing to finish.
    void WaitProcess();

    /// @brief Find dispatch dims from token data.
    void FindDispatchDimsFromTokens();

    int64_t           dispatch_index_ = 0;              ///< The dispatch index to load.
    std::future<void> process_;                         ///< The future of the loading process.
    std::mutex        process_mutex_;                   ///< A mutex to manage loading steps and feedback.
    bool              process_complete_       = false;  ///< A flag to indicate if the loading is complete.
    bool              process_complete_local_ = false;  ///< A thread-local flag to check if the process is complete, so it can be accessed without a lock.

    size_t bytes_required_  = 0;  ///< Number of bytes required to finish loading.
    size_t bytes_processed_ = 0;  ///< Number of bytes processed.

    size_t total_dispatch_indices_     = 0;  ///< Number of dispatch indices. (or pixels)
    size_t processed_dispatch_indices_ = 0;  ///< Number of processed indices.

    bool error_state_ = false;  ///< If there was an error this becomes true.

    size_t total_ray_count_ = 0;  ///< Number of rays. (not pixels)

    GpuRt::CounterInfo counter_info_;  ///< The counter info from the metadata chunk.

    std::shared_ptr<rta::RayHistoryTrace> ray_history_trace_ = nullptr;  ///< The actual trace data.

    RayDispatchData       dispatch_data_     = {};  ///< All of the indexing and individual stats that we've gathered.
    RraRayHistoryStats    invocation_counts_ = {};  ///< The general stats of the dispatch.
    RraDispatchLoadStatus load_status_       = {};  ///< The status of the loader for outside use.

    uint32_t dim_x_ = 0;  ///< The dispatch dimension x.
    uint32_t dim_y_ = 0;  ///< The dispatch dimension y.
    uint32_t dim_z_ = 0;  ///< The dispatch dimension z.
};

#endif
