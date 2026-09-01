//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test hlsl shader implementation.
//=============================================================================

#include "rra_test_hlsl.h"

#include <glm/glm/glm.hpp>

#include "public/rra_macro.h"
#include "public/shared.h"

#include "rra_test_config.h"

namespace backend_test
{
    /// @brief Function to compare a floating point test value against an expected value.
    ///
    /// Make sure the test value is within an accepted range of an expected value.
    ///
    /// @param test_value     The value to test.
    /// @param expected_value The expected value of the test value.
    ///
    /// @return 0 if the test value is acceptable, 1 if not.
    static uint32_t TestValue(float test_value, float expected_value)
    {
        static const float kEpsilon  = 0.00001f;
        float              min_value = expected_value - kEpsilon;
        float              max_value = expected_value + kEpsilon;
        if (test_value > min_value && test_value < max_value)
        {
            return 0;
        }
        return 1;
    }

    /// @brief Get a heatmap color.
    ///
    /// @param index A color index between 0 and 255 inclusive.
    ///
    /// @return A color value, as RGBA.
    static rra::renderer::float4 GetHeatmapColor(uint32_t index)
    {
        float level = static_cast<float>(index) / 255.0f;
        return rra::renderer::heatmap(level);
    }

    /// @brief Get a random color based on an index.
    ///
    /// @param index A color index, from 0 - MAX_UINT
    ///
    /// @return A color value, as RGBA.
    static rra::renderer::float4 GetRandomColor(uint32_t index)
    {
        rra::renderer::float4 color = {};
        uint32_t*             temp;

        color.r = rra::renderer::rand(index);
        temp    = reinterpret_cast<uint32_t*>(&color.r);
        color.g = rra::renderer::rand(*temp);
        temp    = reinterpret_cast<uint32_t*>(&color.g);
        color.b = rra::renderer::rand(*temp);
        color.a = 1.0f;

        return color;
    }

    RRATestHlsl::RRATestHlsl(const std::string& test_name)
        : RRATestBase(test_name)
    {
    }

    RRATestHlsl::~RRATestHlsl()
    {
    }

    bool RRATestHlsl::RunTests(const RRATestConfig& config)
    {
        AddTestStartMessage(config.log);

        // A structure describing the test data. Consists of an index
        // and the corresponding expected color values for that index.
        struct TestData
        {
            uint32_t index;
            float    r;
            float    g;
            float    b;
        };

        // The heatmap test data (expected results for the given index).
        static const TestData kHeatmapData[] = {
            {0, 0.000000f, 0.000000f, 1.000000f},
            {64, 0.384106f, 0.709281f, 0.923289f},
            {128, 0.709281f, 0.999981f, 0.704926f},
            {192, 0.925638f, 0.700543f, 0.378411f},
            {255, 1.000000f, 0.000000f, 0.000000f},
        };

        // The random color generator data (expected results for the given index).
        static const TestData kRandomData[] = {
            {0, 0.405067f, 0.032015f, 0.804665f},
            {64, 0.648958f, 0.438790f, 0.207869f},
            {128, 0.986906f, 0.581543f, 0.991805f},
            {192, 0.165663f, 0.701782f, 0.006648f},
            {255, 0.335747f, 0.239935f, 0.725728f},
        };

        uint32_t fail_count = 0;

        size_t heatmap_size = sizeof(kHeatmapData) / sizeof(TestData);
        for (size_t i = 0; i < heatmap_size; i++)
        {
            rra::renderer::float4 color = GetHeatmapColor(kHeatmapData[i].index);
            fail_count += TestValue(color.r, kHeatmapData[i].r);
            fail_count += TestValue(color.g, kHeatmapData[i].g);
            fail_count += TestValue(color.b, kHeatmapData[i].b);
        }

        size_t random_size = sizeof(kRandomData) / sizeof(TestData);
        for (size_t i = 0; i < random_size; i++)
        {
            rra::renderer::float4 color = GetRandomColor(kRandomData[i].index);
            fail_count += TestValue(color.r, kRandomData[i].r);
            fail_count += TestValue(color.g, kRandomData[i].g);
            fail_count += TestValue(color.b, kRandomData[i].b);
        }

        AddTestResultMessage((fail_count == 0), config.log);
        return (fail_count == 0);
    }

}  // namespace backend_test

