//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for RTA ray history loader.
//=============================================================================

#ifndef RRA_BACKEND_RAY_HISTORY_LOADER_H_
#define RRA_BACKEND_RAY_HISTORY_LOADER_H_

#include <vector>

#include <rdf/rdf/inc/amdrdf.h>

#include "ray_history/counter.h"
#include "ray_history/raytracing_counter.h"

namespace rta
{
    struct RayTracingCounterHeaderResult
    {
        GpuRt::CounterInfo              counterInfo;
        GpuRt::RayHistoryTraversalFlags traversalFlags;
    };

    static constexpr auto RayTracingBinaryHeaderSizeRayHistory       = sizeof(GpuRt::RayTracingBinaryHeaderRayHistory);
    static constexpr auto RayTracingBinaryHeaderSizeTraversalCounter = sizeof(GpuRt::RayTracingBinaryHeaderTraversalCounter);

    static_assert(sizeof(GpuRt::CounterInfo) == 72);

    // Loader for binary ray history or ray traversal counter files.
    // Supports loading single (partial or non-partial) binary file or
    // multiple partial files from the same dispatch.
    // Loading multiple files from different dispatches is not supported.
    class RayTracingBinaryLoader
    {
    public:
        RayTracingBinaryLoader(RayTracingCounterLoadFlags flags);

        // Loads (partial) binary file stream.
        // File headers for multiple files (streams) must match,
        // i.e. files belong to the same dispatch. Throws otherwise.
        void AddRayHistoryTrace(rdf::Stream& stream);
        // Loads (partial) binary file stream.
        // File headers for multiple files (streams) must match,
        // i.e. files belong to the same dispatch. Throws otherwise.
        void AddRayHistoryTrace(rdf::Stream&& stream);

        size_t           GetBufferSize() const;
        const std::byte* GetBufferData() const;

        std::uint32_t              GetHighestRayId() const;
        RayTracingCounterLoadFlags GetFlags() const;

        const RayTracingCounterHeaderResult& GetHeader(size_t idx) const;

        static bool ContainsRayHistoryTrace(rdf::Stream& stream);
        static bool ContainsRayHistoryTrace(rdf::Stream&& stream);

    private:
        RayTracingCounterHeaderResult ReadHeader(rdf::Stream& stream);

        RayTracingCounterLoadFlags flags_;

        std::vector<RayTracingCounterHeaderResult> headers_;
        std::vector<std::byte>                     buffer_;
    };

}  // namespace rta
#endif

