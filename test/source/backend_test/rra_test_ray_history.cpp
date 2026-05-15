//=============================================================================
// Copyright (c) 2023-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test ray history implementation.
//=============================================================================

#include "rra_test_ray_history.h"

#include <chrono>
#include <thread>

#include "public/rra_ray_history.h"

namespace backend_test
{
    RRATestRayHistory::RRATestRayHistory(const std::string& test_name)
        : RRATestBase(test_name)
    {
    }

    RRATestRayHistory::~RRATestRayHistory()
    {
    }

    bool RRATestRayHistory::RunTests(const Log& log)
    {
        AddTestStartMessage(log);

        bool result = true;

        uint32_t dispatch_count = 0;
        RraRayGetDispatchCount(&dispatch_count);

        if (dispatch_count == 0)
        {
            log.Write(" WARNING: There are no dispatches in the trace.");
        }

        for (uint32_t i = 0; i < dispatch_count; i++)
        {
            if (!TestDispatch(i, log))
            {
                result = false;
            }
        }

        AddTestResultMessage(result, log);
        return result;
    }

    bool RRATestRayHistory::TestDispatch(uint32_t dispatch_id, const Log& log)
    {
        auto start = std::chrono::high_resolution_clock::now();

        RraDispatchLoadStatus load_status;

        // Wait until dispatch loading is complete.
        while (true)
        {
            RraRayGetDispatchStatus(dispatch_id, &load_status);

            if (load_status.loading_complete)
            {
                // Loading complete, proceed with the checks.
                break;
            }

            auto end      = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);

            // We've been trying to load for 5 mins, something is not right if it takes this long.
            if (duration.count() > 300)
            {
                log.Write(" ERROR: Loading timeout on dispatch #%u", dispatch_id);
                return false;
            }

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(100ms);
        }

        if (load_status.has_errors)
        {
            if (load_status.incomplete_data)
            {
                log.Write(" ERROR: Not enough space to capture dispatch #%u, please increase the buffer size", dispatch_id);
                return false;
            }

            if (!load_status.raw_data_parsed)
            {
                log.Write(" ERROR: Raw data could not be parsed for dispatch #%u", dispatch_id);
                return false;
            }

            if (!load_status.data_indexed)
            {
                log.Write(" ERROR: Some ray indexes are out of range for dispatch #%u", dispatch_id);
                return false;
            }

            log.Write(" ERROR: Unknown error for dispatch #%u", dispatch_id);
            return false;
        }

        uint32_t width  = 0;
        uint32_t height = 0;
        uint32_t depth  = 0;
        RraRayGetDispatchDimensions(dispatch_id, &width, &height, &depth);

        if (width * height * depth == 0)
        {
            log.Write(" ERROR: Dispatch dimensions are 0", dispatch_id);
            return false;
        }

        // This dispatch is ok.
        return true;
    }

}  // namespace backend_test

