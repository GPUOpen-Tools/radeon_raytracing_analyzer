//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test BLAS header.
//=============================================================================

#ifndef RRA_BACKEND_TEST_RRA_TEST_BLAS_H_
#define RRA_BACKEND_TEST_RRA_TEST_BLAS_H_

#include <stdint.h>

#include "log.h"
#include "rra_test_base.h"
#include "rra_test_config.h"

namespace backend_test
{
    class RRATestBlas : public RRATestBase
    {
    public:
        /// @brief Constructor.
        explicit RRATestBlas(const std::string& test_name);

        /// @brief Destructor.
        ~RRATestBlas();

        /// @brief Test the BLAS and validate the data.
        ///
        /// @param [in] config  The current test configuration.
        ///
        /// @return true if the tests passed, false if not.
        bool RunTests(const RRATestConfig& config);

    private:
        /// @brief Traverse the BLAS and test the various attributes.
        ///
        /// @param [in] blas_index  The index of the BLAS to test.
        /// @param [in] log         The log to write any output to.
        ///
        /// @return true if the tests passed, false if not.
        bool TestBLASTraversal(uint64_t blas_index, const Log& log);

        /// @brief Test the geometry info structures.
        ///
        /// @param [in] blas_index  The index of the BLAS to test.
        /// @param [in] log         The log to write any output to.
        ///
        /// @return true if the tests passed, false if not.
        bool TestBLASGeometryInfo(uint64_t blas_index, const Log& log);
    };
}  // namespace backend_test

#endif  //  RRA_BACKEND_TEST_RRA_TEST_BLAS_H_

