//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend catch2 test cases.
//=============================================================================

#include <catch.hpp>

#include "rra_test_cases.h"

TEST_CASE("TestFileChunks", "RRATest")
{
    bool result = RRATestCases::Get()->TestFileChunks("File chunk");
    REQUIRE(result == true);
}

TEST_CASE("TestAsic", "RRATest")
{
    bool result = RRATestCases::Get()->TestAsicChunk("Asic chunk");
    REQUIRE(result == true);
}

TEST_CASE("TestSystemInfo", "RRATest")
{
    bool result = RRATestCases::Get()->TestSystemInfoChunk("System info chunk");
    REQUIRE(result == true);
}

TEST_CASE("TestTlas", "RRATest")
{
    bool result = RRATestCases::Get()->TestTlas("TLAS");
    REQUIRE(result == true);
}

TEST_CASE("TestBlas", "RRATest")
{
    bool result = RRATestCases::Get()->TestBlas("BLAS");
    REQUIRE(result == true);
}

TEST_CASE("TestTlasBlasMapping", "RRATest")
{
    bool result = RRATestCases::Get()->TestTlasBlasMapping("TLAS->BLAS mapping");
    REQUIRE(result == true);
}

TEST_CASE("TestHlsl", "RRATest")
{
    bool result = RRATestCases::Get()->TestHlsl("HLSL");
    REQUIRE(result == true);
}

TEST_CASE("TestRayHistory", "RRATest")
{
    bool result = RRATestCases::Get()->TestRayHistory("Ray history");
    REQUIRE(result == true);
}

