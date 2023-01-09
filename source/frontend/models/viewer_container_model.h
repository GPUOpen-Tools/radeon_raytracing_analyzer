//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of the viewer container model class.
//=============================================================================

#ifndef RRA_MODELS_VIEWER_CONTAINER_MODEL_H_
#define RRA_MODELS_VIEWER_CONTAINER_MODEL_H_

#include "qt_common/custom_widgets/arrow_icon_combo_box.h"
#include "qt_common/utils/model_view_mapper.h"

#include "public/renderer_interface.h"
#include "public/render_state_adapter.h"

namespace rra
{
    /// @brief Enum containing indices for the widgets shared between the model and UI.
    enum ViewerContainerWidgets
    {
        kViewerContainerNumWidgets,
    };

    class ViewerContainerModel : public ModelViewMapper
    {
        Q_OBJECT

    public:
        /// @brief Explicit constructor.
        explicit ViewerContainerModel();

        /// @brief Destructor.
        virtual ~ViewerContainerModel();

        /// @brief Set the renderer adapter instance.
        ///
        /// @param [in] adapter The renderer adapter instance.
        /// @param [in] type A flag indicating the type of BVH viewer being controlled.
        /// @param [out] coloring_modes An array of coloring modes.
        /// @param [out] traversal_modes An array of traveral counter modes.
        void SetRendererAdapter(rra::renderer::RendererAdapter*                  adapter,
                                rra::renderer::BvhTypeFlags                      type,
                                std::vector<renderer::GeometryColoringModeInfo>& coloring_modes,
                                std::vector<renderer::TraversalCounterModeInfo>& traversal_modes);

        /// @brief Set the renderer geometry coloring mode.
        ///
        /// @param [in] coloring_mode The enum value corresponding to the coloring mode.
        void SetGeometryColoringMode(renderer::GeometryColoringMode coloring_mode);

        /// @brief Set the renderer bvh coloring mode.
        ///
        /// @param [in] index An index selected in the combo box corresponding to the coloring mode.
        void SetBVHColoringMode(int index);

        /// @brief Set the traversal rendering counter mode.
        ///
        /// @param [in] index An index selected in the combo box corresponding to the counter mode.
        void SetTraversalCounterMode(int index);

        /// @brief Set the heatmap data.
        ///
        /// @param [in] heatmap_data The raw data of the heatmap.
        void SetHeatmapData(rra::renderer::HeatmapData heatmap_data);

    private:
        rra::renderer::RenderStateAdapter* render_state_adapter_ = nullptr;  ///< The adapter used to toggle mesh render states.
    };
}  // namespace rra

#endif  // RRA_MODELS_VIEWER_CONTAINER_MODEL_H_
