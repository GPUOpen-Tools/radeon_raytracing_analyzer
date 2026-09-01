//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test system info header.
//=============================================================================

#ifndef RRA_TEST_SYSTEM_INFO_H_
#define RRA_TEST_SYSTEM_INFO_H_

#include <cstdint>

#include "log.h"
#include "rra_test_base.h"

namespace backend_test
{
    class RRATestSystemInfoChunk : public RRATestBase
    {
    public:
        /// @brief Constructor.
        RRATestSystemInfoChunk(const std::string& test_name);

        /// @brief Destructor.
        ~RRATestSystemInfoChunk();

        /// @brief Test the system_info data and validate that sensible numbers are returned.
        ///
        /// @param [in] log            The log to write any output to.
        ///
        /// @return true if the tests passed, false if not.
        bool RunTests(const Log& log);

    private:
        /// @brief Test an unsigned integer value.
        ///
        /// @param [in]  text   The text describing the parameter being tested.
        /// @param [in]  units  The text describing the units of the parameter being tested.
        /// @param [in]  value  The value of the parameter being tested.
        /// @param [in]  limit  The lower limit of the value being tested.
        /// @param [in]  log    The log to write any output to.
        /// @param [out] result If the test fails, contains the test failure result.
        void TextUintValue(const char* text, const char* units, uint32_t value, uint32_t limit, const Log& log, bool* result);

        /// @brief Make sure the GPU name string is valid.
        ///
        /// Check it doesn't contain the word 'unknown'.
        ///
        /// @param [in]  log          The log to write any output to.
        /// @param [out] result       If the test fails, contains the test failure result.
        void TestName(const Log& log, bool* result);

        /// @brief Make sure the memory type is valid.
        ///
        /// @param [in]  log          The log to write any output to.
        /// @param [out] result       If the test fails, contains the test failure result.
        void TestMemoryType(const Log& log, bool* result);
    };
}  // namespace backend_test

#endif  //  RMV_TEST_SYSTEM_INFO_H_

