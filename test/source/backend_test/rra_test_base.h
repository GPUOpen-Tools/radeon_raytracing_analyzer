//=============================================================================
// Copyright (c) 2023-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test base class header.
//=============================================================================

#ifndef RRA_BACKEND_TEST_RRA_TEST_BASE_H_
#define RRA_BACKEND_TEST_RRA_TEST_BASE_H_

#include <string>

#include "log.h"

namespace backend_test
{
    class RRATestBase
    {
    public:
        /// @brief Constructor.
        explicit RRATestBase(const std::string& test_name);

        /// @brief Destructor.
        virtual ~RRATestBase();

        /// @brief Add a message to the log indicate the start of a test.
        ///
        /// @param [in] log         The log to write the message to.
        void AddTestStartMessage(const Log& log) const;

        /// @brief Add a message to the log indicate the end of a test.
        ///
        /// @param [in] log         The log to write the message to.
        void AddTestResultMessage(bool test_result, const Log& log) const;

    private:
        std::string test_name_;  ///< The name of this test.
    };
}  // namespace backend_test

#endif  //  RRA_BACKEND_TEST_RRA_TEST_BASE_H_

