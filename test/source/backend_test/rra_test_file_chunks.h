//=============================================================================
// Copyright (c) 2021-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test file chunks header.
//=============================================================================

#ifndef RRA_BACKEND_TEST_RRA_TEST_FILE_CHUNKS_H_
#define RRA_BACKEND_TEST_RRA_TEST_FILE_CHUNKS_H_

#include <string>

#include "log.h"
#include "rra_test_base.h"
#include "rra_test_config.h"

namespace backend_test
{
    class RRATestFileChunks : public RRATestBase
    {
    public:
        /// @brief Constructor.
        explicit RRATestFileChunks(const std::string& test_name);

        /// @brief Destructor.
        ~RRATestFileChunks();

        /// @brief Test the file chunks and validate that sensible numbers are returned.
        ///
        /// @param [in] config  The current test configuration.
        ///
        /// @return true if the tests passed, false if not.
        bool RunTests(const RRATestConfig& config);
    };
}  // namespace backend_test

#endif  //  RRA_BACKEND_TEST_RRA_TEST_FILE_CHUNKS_H_

