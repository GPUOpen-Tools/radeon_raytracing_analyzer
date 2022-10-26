//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the Heatmap.
//=============================================================================

#ifndef RRA_HEATMAP_H_
#define RRA_HEATMAP_H_

#include "glm/glm/glm.hpp"
#include <vector>
#include <functional>
#include <map>
#include <string>

namespace rra
{
    namespace renderer
    {
        // Heatmap data storage.
        class HeatmapData : public std::vector<glm::vec4>
        {
        public:
            /// @brief Evaluate the heatmap data.
            ///
            /// @param value The value from 0 to 1 to evaluate with.
            ///
            /// @return The evaluated color.
            glm::vec4 Evaluate(float value);
        };

        /// @brief The heatmap generation data.
        struct HeatmapGenerator
        {
            std::function<HeatmapData()> generator_function;  ///< The generator function for the heatmap.
            std::string                  name;                ///< The name of the heatmap.
            std::string                  tooltip;             ///< The tooltip to show for this heatmap.
        };

        /// @brief The heatmap for the renderer.
        class Heatmap
        {
        public:
            /// @brief Default constructor.
            Heatmap();

            /// @brief Constructor with raw data input.
            Heatmap(HeatmapData data);

            /// @brief Get raw heatmap data.
            ///
            /// @returns The heatmap raw data.
            const HeatmapData& GetData() const;

            /// @brief Get classic rra heatmap.
            ///
            /// @returns The heatmap raw data of a classic rra heatmap.
            static HeatmapData GetClassicHeatmapData();

            /// @brief Get the heatmap generators.
            ///
            /// @returns The heatmap generators.
            static std::vector<HeatmapGenerator> GetHeatmapGenerators();

        private:
            HeatmapData heatmap_data_;
        };
    }  // namespace renderer

}  // namespace rra

#endif
