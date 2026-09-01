//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test TLAS header.
//=============================================================================

#ifndef RRA_BACKEND_TEST_RRA_TEST_TLAS_H_
#define RRA_BACKEND_TEST_RRA_TEST_TLAS_H_

#include <stdint.h>

#include "log.h"
#include "rra_test_base.h"

namespace backend_test
{
    class RRATestTlas : public RRATestBase
    {
    public:
        /// @brief Constructor.
        explicit RRATestTlas(const std::string& test_name);

        /// @brief Destructor.
        ~RRATestTlas();

        /// @brief Test the TLAS and validate the Data.
        ///
        /// @param [in] log             The log to write any output to.
        ///
        /// @return true if the tests passed, false if not.
        bool RunTests(const Log& log);

    private:
    };
}  // namespace backend_test

#endif  //  RRA_BACKEND_TEST_RRA_TEST_TLAS_H_

