//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the side pane model base class.
//=============================================================================

#ifndef RRA_MODELS_SIDE_PANELS_SIDE_PANEL_MODEL_H_
#define RRA_MODELS_SIDE_PANELS_SIDE_PANEL_MODEL_H_

#include "qt_common/utils/model_view_mapper.h"

#include "public/renderer_adapter.h"

namespace rra
{
    /// @brief The side panel model base class declaration.
    class SidePanelModel : public ModelViewMapper
    {
    public:
        /// @brief Constructor.
        explicit SidePanelModel(uint32_t model_count)
            : ModelViewMapper(model_count)
        {
        }

        /// @brief Destructor.
        virtual ~SidePanelModel() = default;

        /// @brief Set the renderer adapter instance.
        ///
        /// @param [in] adapter The renderer adapter instance.
        virtual void SetRendererAdapters(const rra::renderer::RendererAdapterMap& adapters) = 0;

        /// @brief Update the model with the current BVH state.
        ///
        /// Propagate the state to the UI.
        virtual void Update() = 0;
    };
}  // namespace rra

#endif  // #define RRA_MODELS_SIDE_PANELS_SIDE_PANEL_MODEL_H_
