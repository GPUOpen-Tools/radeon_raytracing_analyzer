//=============================================================================
// Copyright (c) 2023-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test base class implementation.
//=============================================================================

#include "rra_test_base.h"

namespace backend_test
{
    RRATestBase::RRATestBase(const std::string& test_name)
        : test_name_(test_name)
    {
    }

    RRATestBase::~RRATestBase()
    {
    }

    void RRATestBase::AddTestStartMessage(const Log& log) const
    {
        std::string str = test_name_ + " test started";
        log.Write(str.c_str());
    }

    void RRATestBase::AddTestResultMessage(bool test_result, const Log& log) const
    {
        if (test_result == true)
        {
            std::string str = test_name_ + " test OK";
            log.Write(str.c_str());
        }
        else
        {
            std::string str = test_name_ + " test FAILED";
            log.Write(str.c_str());
        }
    }

}  // namespace backend_test

