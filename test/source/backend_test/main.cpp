//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test main program.
//=============================================================================

#include <iostream>

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include "rra_test_cases.h"

int32_t main(int32_t argc, char** argv)
{
    std::string input_file_path;  // The trace file to load.

    Catch::Session session;
    using namespace Catch::clara;

    // Add to Catch's composite command line parser.
    // User to specify the trace file name and whether the stats are to be dumped. The stats dump is more of a debug option and allows
    // the user to see the data in the trace in text format. Depending on the size of the trace, these text files can be large so the
    // option is off by default.
    auto cli = Opt(input_file_path, "input_rra_file_path")["--rra"]("The path to the scene file") | session.cli();

    // Now pass the new composite back to Catch so it uses that.
    int test_result = -1;
    session.cli(cli);
    int result = session.applyCommandLine(argc, argv);
    if (result != 0)
    {
        return test_result;
    }

    // All good so far. Initialize tests (read in RRA file and parse backend data).
    RRATestConfig config   = {};
    config.trace_file_name = input_file_path;
    if (RRATestCases::Get()->Initialize(config) == true)
    {
        test_result = session.run();
        RRATestCases::Get()->Shutdown();
    }

    // Test result will contain -1 if the tests were not run (some kind of initialization problem)
    // or an integer containing the number of tests that failed.
    return test_result;
}
