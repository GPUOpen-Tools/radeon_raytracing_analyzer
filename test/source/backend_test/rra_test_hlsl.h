//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test hlsl shader header.
//=============================================================================

#ifndef RRA_BACKEND_TEST_RRA_TEST_HLSL_H_
#define RRA_BACKEND_TEST_RRA_TEST_HLSL_H_

#include <string>

#include "log.h"
#include "rra_test_base.h"
#include "rra_test_config.h"

namespace backend_test
{
    class RRATestHlsl : public RRATestBase
    {
    public:
        /// @brief Constructor.
        explicit RRATestHlsl(const std::string& test_name);

        /// @brief Destructor.
        ~RRATestHlsl();

        /// @brief Test the hlsl shared shader and validate that sensible numbers are returned.
        ///
        /// @param [in] config  The current test configuration.
        ///
        /// @return true if the tests passed, false if not.
        bool RunTests(const RRATestConfig& config);
    };
}  // namespace backend_test

#endif  //  RRA_BACKEND_TEST_RRA_TEST_HLSL_H_

