//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the viewer container model class.
//=============================================================================

#include "models/viewer_container_model.h"

namespace rra
{
    ViewerContainerModel::ViewerContainerModel()
        : ModelViewMapper(kViewerContainerNumWidgets)
    {
    }

    ViewerContainerModel::~ViewerContainerModel()
    {
    }

    void ViewerContainerModel::SetRendererAdapter(rra::renderer::RendererAdapter*                  adapter,
                                                  rra::renderer::BvhTypeFlags                      type,
                                                  std::vector<renderer::GeometryColoringModeInfo>& coloring_modes,
                                                  std::vector<renderer::TraversalCounterModeInfo>& traversal_modes)
    {
        render_state_adapter_ = static_cast<rra::renderer::RenderStateAdapter*>(adapter);
        render_state_adapter_->GetAvailableGeometryColoringModes(type, coloring_modes);
        render_state_adapter_->GetAvailableTraversalCounterModes(type, traversal_modes);
        render_state_adapter_->SetBVHColoringMode(0);
    }

    void ViewerContainerModel::SetGeometryColoringMode(renderer::GeometryColoringMode coloring_mode)
    {
        if (render_state_adapter_ != nullptr)
        {
            render_state_adapter_->SetGeometryColoringMode(coloring_mode);
        }
    }

    void ViewerContainerModel::SetBVHColoringMode(int index)
    {
        if (render_state_adapter_ != nullptr)
        {
            render_state_adapter_->SetBVHColoringMode(index);
        }
    }

    void ViewerContainerModel::SetTraversalCounterMode(int index)
    {
        if (render_state_adapter_ != nullptr)
        {
            render_state_adapter_->SetTraversalCounterMode(index);
        }
    }

    void ViewerContainerModel::SetHeatmapData(const rra::renderer::HeatmapData& heatmap_data)
    {
        if (render_state_adapter_ != nullptr)
        {
            render_state_adapter_->SetHeatmapData(heatmap_data);
        }
    }

    void ViewerContainerModel::SetInstanceMaskFilter(uint32_t filter)
    {
        instance_mask_filter_ = filter;

        if (scene_)
        {
            scene_->FilterNodesByInstanceMask(filter);
        }
    }

    uint32_t ViewerContainerModel::GetInstanceMaskFilter() const
    {
        return instance_mask_filter_;
    }

    void ViewerContainerModel::SetScene(rra::Scene* scene)
    {
        scene_ = scene;
    }

}  // namespace rra
