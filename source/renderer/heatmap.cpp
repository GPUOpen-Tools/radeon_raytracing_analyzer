//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definitions for the Heatmap.
//=============================================================================

#include "public/heatmap.h"

namespace rra
{
    namespace renderer
    {
        const size_t kDefaultHeatmapSize = 1000;

        Heatmap::Heatmap()
        {
            heatmap_data_.push_back({0.0f, 0.0f, 1.0f, 1.0f});
            heatmap_data_.push_back({0.0f, 1.0f, 1.0f, 1.0f});
            heatmap_data_.push_back({0.0f, 1.0f, 0.0f, 1.0f});
            heatmap_data_.push_back({1.0f, 1.0f, 0.0f, 1.0f});
            heatmap_data_.push_back({1.0f, 0.0f, 0.0f, 1.0f});
        }

        Heatmap::Heatmap(HeatmapData heatmap_data)
        {
            heatmap_data_ = heatmap_data;
            if (heatmap_data_.size() < 2)
            {
                heatmap_data_.clear();
                heatmap_data_.push_back({0.0f, 0.0f, 0.0f, 1.0f});
                heatmap_data_.push_back({1.0f, 1.0f, 1.0f, 1.0f});
            }
        }

        const HeatmapData& Heatmap::GetData() const
        {
            return heatmap_data_;
        }

        HeatmapData Heatmap::GetClassicHeatmapData()
        {
            HeatmapData heatmap_data;
            heatmap_data.resize(kDefaultHeatmapSize);
            for (size_t i = 0; i < heatmap_data.size(); i++)
            {
                const float kPi   = 3.14159265358979323846f;
                float       level = float(i) / float(heatmap_data.size() - 1) * (kPi * 0.5f);
                float       r     = glm::clamp(float(sin(level)), 0.0f, 1.0f);
                float       g     = glm::clamp(float(sin(level * 2.0f)), 0.0f, 1.0f);
                float       b     = glm::clamp(float(cos(level)), 0.0f, 1.0f);

                heatmap_data[i] = glm::vec4(r, g, b, 1.0f);
            }

            return heatmap_data;
        }

        /// @brief Viridis heatmap.
        ///
        /// @param t The sample value from 0 to 1.
        ///
        /// @return The sampled color.
        glm::vec4 Viridis(float t)
        {
            const glm::vec3 c0 = glm::vec3(0.2777273272234177, 0.005407344544966578, 0.3340998053353061);
            const glm::vec3 c1 = glm::vec3(0.1050930431085774, 1.404613529898575, 1.384590162594685);
            const glm::vec3 c2 = glm::vec3(-0.3308618287255563, 0.214847559468213, 0.09509516302823659);
            const glm::vec3 c3 = glm::vec3(-4.634230498983486, -5.799100973351585, -19.33244095627987);
            const glm::vec3 c4 = glm::vec3(6.228269936347081, 14.17993336680509, 56.69055260068105);
            const glm::vec3 c5 = glm::vec3(4.776384997670288, -13.74514537774601, -65.35303263337234);
            const glm::vec3 c6 = glm::vec3(-5.435455855934631, 4.645852612178535, 26.3124352495832);

            return glm::vec4(c0 + t * (c1 + t * (c2 + t * (c3 + t * (c4 + t * (c5 + t * c6))))), 1.0);
        }

        /// @brief Plasma heatmap.
        ///
        /// @param t The sample value from 0 to 1.
        ///
        /// @return The sampled color.
        glm::vec4 Plasma(float t)
        {
            const glm::vec3 c0 = glm::vec3(0.05873234392399702, 0.02333670892565664, 0.5433401826748754);
            const glm::vec3 c1 = glm::vec3(2.176514634195958, 0.2383834171260182, 0.7539604599784036);
            const glm::vec3 c2 = glm::vec3(-2.689460476458034, -7.455851135738909, 3.110799939717086);
            const glm::vec3 c3 = glm::vec3(6.130348345893603, 42.3461881477227, -28.51885465332158);
            const glm::vec3 c4 = glm::vec3(-11.10743619062271, -82.66631109428045, 60.13984767418263);
            const glm::vec3 c5 = glm::vec3(10.02306557647065, 71.41361770095349, -54.07218655560067);
            const glm::vec3 c6 = glm::vec3(-3.658713842777788, -22.93153465461149, 18.19190778539828);

            return glm::vec4(c0 + t * (c1 + t * (c2 + t * (c3 + t * (c4 + t * (c5 + t * c6))))), 1.0);
        }

        std::vector<HeatmapGenerator> Heatmap::GetHeatmapGenerators()
        {
            std::vector<HeatmapGenerator> generators;

            generators.push_back({Heatmap::GetClassicHeatmapData, "Heatmap as Temperature", "Heatmap as a range of blue to red."});

            generators.push_back({[]() -> HeatmapData {
                                      HeatmapData data;
                                      data.push_back({0.0f, 1.0f, 1.0f, 1.0f});
                                      data.push_back({0.0f, 1.0f, 0.0f, 1.0f});
                                      data.push_back({1.0f, 1.0f, 0.0f, 1.0f});
                                      data.push_back({1.0f, 0.0f, 0.0f, 1.0f});
                                      data.push_back({0.8f, 0.0f, 0.0f, 1.0f});

                                      return data;
                                  },
                                  "Heatmap as Spectrum",
                                  "Heatmap as a full spectrum gradient, blue to green to red."});

            generators.push_back({[]() -> HeatmapData {
                                      HeatmapData data;
                                      data.push_back({0.1f, 0.1f, 0.1f, 1.0f});
                                      data.push_back({0.5f, 0.5, 0.5, 1.0f});
                                      data.push_back({0.9f, 0.9f, 0.9f, 1.0f});

                                      return data;
                                  },
                                  "Heatmap as Grayscale",
                                  "Heatmap as a grayscale, dark to light."});

            generators.push_back({[]() -> HeatmapData {
                                      HeatmapData data;
                                      for (size_t i = 0; i < kDefaultHeatmapSize; i++)
                                      {
                                          data.push_back(Viridis(i / static_cast<float>(kDefaultHeatmapSize - 1)));
                                      }
                                      return data;
                                  },
                                  "Heatmap as Viridis",
                                  "Heatmap as Viridis, a Perceptually Uniform Sequential heatmap."});

            generators.push_back({[]() -> HeatmapData {
                                      HeatmapData data;
                                      for (size_t i = 0; i < kDefaultHeatmapSize; i++)
                                      {
                                          data.push_back(Plasma(i / static_cast<float>(kDefaultHeatmapSize - 1)));
                                      }
                                      return data;
                                  },
                                  "Heatmap as Plasma",
                                  "Heatmap as Plasma, a Perceptually Uniform Sequential heatmap."});

            return generators;
        }

        glm::vec4 HeatmapData::Evaluate(float value)
        {
            auto sz = size();
            if (sz < 2)
            {
                return {};
            }
            value = glm::clamp(value, 0.0f, 1.0f);
            value = value * (sz - 1);

            float  lower = glm::floor(value);
            size_t index = static_cast<size_t>(lower);

            if (index + 1 >= sz)
            {
                return back();
            }

            return glm::mix(operator[](index), operator[](index + 1), value - lower);
        }

    }  // namespace renderer

}  // namespace rra

