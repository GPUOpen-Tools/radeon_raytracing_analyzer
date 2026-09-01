//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test cases header.
//=============================================================================

#ifndef RRA_BACKEND_TEST_RRA_TEST_CASES_H_
#define RRA_BACKEND_TEST_RRA_TEST_CASES_H_

#include <string>

#include "log.h"
#include "rra_test_config.h"

class RRATestCases
{
public:
    /// @brief Accessor for singleton instance.
    ///
    /// @return The singleton instance.
    static RRATestCases* Get();

    /// @brief Initialize anything needed for testing.
    ///
    /// Load in the trace and parse it.
    ///
    /// @param [in] config  The current test configuration.
    ///
    /// @return true if trace file loaded and parsed correctly, false otherwise.
    bool Initialize(const RRATestConfig& config);

    /// @brief Add a message to the log.
    ///
    /// @param [in] log_message The text to add to the log.
    void Log(const char* log_message, ...);

    /// @brief Clean up after testing.
    void Shutdown();

    /// @brief Test the file chunks in the trace.
    ///
    /// @return true if the tests passed, false if not.
    bool TestFileChunks(const char* test_name) const;

    /// @brief Test the mapping between TLAS and BLAS objects.
    ///
    /// @return true if the tests passed, false if not.
    bool TestTlasBlasMapping(const char* test_name) const;

    /// @brief Test the shared HLSL shader on the CPU side.
    ///
    /// @return true if the tests passed, false if not.
    bool TestHlsl(const char* test_name) const;

    /// @brief Test the ASIC chunk.
    ///
    /// @return true if the tests passed, false if not.
    bool TestAsicChunk(const char* test_name) const;

    /// @brief Test the System Info chunk.
    ///
    /// @return true if the tests passed, false if not.
    bool TestSystemInfoChunk(const char* test_name) const;

    /// @brief Test the BLAS data.
    ///
    /// @return true if the tests passed, false if not.
    bool TestBlas(const char* test_name) const;

    /// @brief Test the TLAS data.
    ///
    /// @return true if the tests passed, false if not.
    bool TestTlas(const char* test_name) const;

    /// @brief Test the ray history data.
    ///
    /// @return true if the tests passed, false if not.
    bool TestRayHistory(const char* test_name) const;

private:
    /// @brief Constructor.
    RRATestCases();

    /// @brief Destructor.
    ~RRATestCases();

    static RRATestCases* instance_;  ///< The singleton instance.
    RRATestConfig        config_;    ///< The current test configuration
};

#endif  //  RRA_BACKEND_TEST_RRA_TEST_CASES_H_

