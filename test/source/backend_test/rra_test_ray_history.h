//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test ray history header.
//=============================================================================

#ifndef RRA_BACKEND_TEST_RRA_TEST_RAY_HISTORY_H_
#define RRA_BACKEND_TEST_RRA_TEST_RAY_HISTORY_H_

#include <stdint.h>

#include "log.h"
#include "rra_test_base.h"

namespace backend_test
{
    class RRATestRayHistory : public RRATestBase
    {
    public:
        /// @brief Constructor.
        explicit RRATestRayHistory(const std::string& test_name);

        /// @brief Destructor.
        ~RRATestRayHistory();

        /// @brief Test the ray history and validate the data.
        ///
        /// @param [in] log             The log to write any output to.
        ///
        /// @return true if the tests passed, false if not.
        bool RunTests(const Log& log);

        /// @brief Test a given dispatch to see if it was loaded correctly.
        ///
        /// @param [in] dispatch_id     The dispatch id to test.
        /// @param [in] log             The log to write any output to.
        ///
        /// @return true if the tests passed, false if not.
        bool TestDispatch(uint32_t dispatch_id, const Log& log);

    private:
    };
}  // namespace backend_test

#endif  //  RRA_BACKEND_TEST_RRA_TEST_RAY_HISTORY_H_
