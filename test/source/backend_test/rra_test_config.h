//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test config header.
//=============================================================================

#ifndef RRA_BACKEND_TEST_RRA_TEST_CONFIG_H_
#define RRA_BACKEND_TEST_RRA_TEST_CONFIG_H_

#include <string>

#include "log.h"

struct RRATestConfig
{
    std::string       trace_file_name;  ///< The name of the current file being tested.
    backend_test::Log log;              ///< The log file.
};

#endif  //  RRA_BACKEND_TEST_RRA_TEST_CONFIG_H_
