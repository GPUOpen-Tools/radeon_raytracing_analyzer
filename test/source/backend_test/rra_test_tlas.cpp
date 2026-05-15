//=============================================================================
// Copyright (c) 2023-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test TLAS implementation.
//=============================================================================

#include "rra_test_tlas.h"

#include <deque>

#include "public/rra_bvh.h"
#include "public/rra_macro.h"
#include "public/rra_tlas.h"

namespace backend_test
{
    RRATestTlas::RRATestTlas(const std::string& test_name)
        : RRATestBase(test_name)
    {
    }

    RRATestTlas::~RRATestTlas()
    {
    }

    bool RRATestTlas::RunTests(const Log& log)
    {
        AddTestStartMessage(log);

        bool result = true;

        // Iterate through all BLASes.
        uint64_t blas_count = 0;
        if (RraBvhGetTlasCount(&blas_count) != kRraOk)
        {
            log.Write(" ERROR: Unable to get TLAS count");
            return false;
        }

        AddTestResultMessage(result, log);
        return result;
    }

}  // namespace backend_test

