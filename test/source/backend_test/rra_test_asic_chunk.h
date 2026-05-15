//=============================================================================
// Copyright (c) 2023-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test ASIC chunk header.
//=============================================================================

#ifndef RRA_BACKEND_TEST_RRA_TEST_ASIC_CHUNK_H_
#define RRA_BACKEND_TEST_RRA_TEST_ASIC_CHUNK_H_

#include "log.h"
#include "rra_test_base.h"
#include "rra_test_config.h"

namespace backend_test
{
    class RRATestAsicChunk : public RRATestBase
    {
    public:
        /// @brief Constructor.
        explicit RRATestAsicChunk(const std::string& test_name);

        /// @brief Destructor.
        ~RRATestAsicChunk();

        /// @brief Test the asic file chunk and validate the data.
        ///
        /// @param [in] config  The current test configuration.
        ///
        /// @return true if the tests passed, false if not.
        bool RunTests(const RRATestConfig& config);

    private:
        /// @brief Test the GPU device name.
        ///
        /// @param [in] log         The log to write any output to.
        ///
        /// @return true if the tests passed, false if not.
        bool TestDeviceName(const Log& log);

        /// @brief Test the GPU clock frequencies.
        ///
        /// @param [in] log         The log to write any output to.
        ///
        /// @return true if the tests passed, false if not.
        bool TestClocks(const Log& log);

        /// @brief Test the GPU memory bandwidth.
        ///
        /// @param [in] log         The log to write any output to.
        ///
        /// @return true if the tests passed, false if not.
        bool TestMemoryBandwidth(const Log& log);
    };
}  // namespace backend_test

#endif  //  RRA_BACKEND_TEST_RRA_TEST_ASIC_CHUNK_H_

