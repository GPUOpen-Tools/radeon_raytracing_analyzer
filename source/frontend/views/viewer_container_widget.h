//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the viewer container widget class.
///
/// This class contains regularly-used widgets which are shared between the
/// TLAS and BLAS viewer panes and slot under the list items at the top of
/// the UI
//=============================================================================

#ifndef RRA_VIEWS_VIEWER_CONTAINER_WIDGET_H_
#define RRA_VIEWS_VIEWER_CONTAINER_WIDGET_H_

#include "ui_viewer_container_widget.h"

#include "qt_common/custom_widgets/arrow_icon_combo_box.h"

#include "managers/pane_manager.h"
#include "models/viewer_container_model.h"

#include "public/renderer_adapter.h"

#include "settings/settings.h"

/// @brief Class declaration.
class ViewerContainerWidget : public QWidget
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit ViewerContainerWidget(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~ViewerContainerWidget();

    /// @brief Set the renderer adapter instance.
    ///
    /// @param [in] adapter The renderer adapter instance.
    /// @param [in] bvh_type The BVH type associated with the given adapters.
    void SetRendererAdapters(const rra::renderer::RendererAdapterMap& adapters, rra::renderer::BvhTypeFlags bvh_type);

    /// @brief Setup the UI components of this container.
    ///
    /// Initialize UI elements that can't be initialized in the constructor, such as the combo boxes.
    /// These need to be given the parent pane rather than the viewer container as their parent so that
    /// they can expand correctly when opened.
    ///
    /// @param [in] parent The pane containing the widgets in this container.
    void SetupUI(QWidget* parent, rra::RRAPaneId pane);

    /// @brief Show the coloring mode combo box depending on the rendering mode selected.
    ///
    /// There are 2 coloring mode combo box selections and they are dependent on which rendering mode
    /// is selected, so only show the relevant combo box.
    ///
    /// @param geometry_mode_enable If true, geometry mode is selected, otherwise show traversal counter mode.
    void ShowColoringMode(bool geometry_mode_enabled);

    /// @brief Set the current scene.
    ///
    /// @param scene The scene.
    void SetScene(rra::Scene* scene);

    /// @brief Disable the mask widget interaction.
    void DisableMaskWidgetInteraction();

    /// @brief Update the mask widget with given value.
    ///
    /// @param The new instance mask to filter by.
    void UpdateMaskWidget(int mask);

    /// @brief Set the heatmap type
    ///
    /// @param [in] heatmap_type The type of the heatmap to set.
    void SetHeatmap(HeatmapColorType heatmap_type);

    /// @brief Apply the state of the UI from the state in the settings.
    ///
    /// @param [in] pane The parent pane containing this viewer container.
    void ApplyUIStateFromSettings(rra::RRAPaneId pane);

private slots:

    /// @brief Set the checkbox state for the clicked bit.
    ///
    /// @param mask The bits to flip.
    void BinaryCheckboxClicked(uint32_t mask);

    /// @brief Set the instance mask filter using the hex input box.
    ///
    /// @param The new instance mask to filter by.
    void InstanceMaskHexChanged(int mask);

    /// @brief Set the bvh coloring mode to use on the BVH.
    void SetBVHColoringMode();

    /// @brief Set the heatmap mode.
    void SetHeatmapMode();

    /// @brief Set the geometry coloring mode to use on the geometry.
    ///
    /// Interrogates the UI control and passes the data off to the model.
    void SetGeometryColoringMode();

    /// @brief Set the traversal counter mode.
    void SetTraversalCounterMode();

    /// @brief A slot handled when the underlying model reports that available coloring modes been updated.
    ///
    /// @param [in] coloring_modes A reference to a vector of coloring mode info.
    /// @param [in] traversal_modes A reference to a vector of traversal counter mode info.
    /// @param [in] bvh_type Whether the BVH is a TLAS or BLAS.
    void UpdateRenderingModes(const std::vector<rra::renderer::GeometryColoringModeInfo>& coloring_modes,
                              const std::vector<rra::renderer::TraversalCounterModeInfo>& traversal_modes,
                              rra::renderer::BvhTypeFlags                                 bvh_type);

    /// @brief Checks if the instance mask should have a red background.
    ///
    /// @params mask The mask value.
    void EvaluateInstanceMaskWarning(int mask);

private:
    Ui::ViewerContainerWidget*                           ui_    = nullptr;           ///< Pointer to the Qt UI design.
    rra::ViewerContainerModel*                           model_ = nullptr;           ///< The model backing the view.
    std::vector<rra::renderer::GeometryColoringModeInfo> filtered_color_modes_;      ///< The coloring modes available to the viewer.
    std::vector<rra::renderer::TraversalCounterModeInfo> filtered_traversal_modes_;  ///< The traversal counter modes available to the viewer.
    std::vector<rra::renderer::HeatmapGenerator>         heatmap_generators_;        ///< The heatmap modes available at update.
    rra::renderer::BvhTypeFlags                          bvh_type_;                  ///< The type of the bvh.
    bool instance_mask_interaction_disabled_ = false;                                ///< A flag to indicate if the mask interaction is disabled.
    rra::RRAPaneId parent_pane_id_                     = rra::kPaneIdInvalid;        ///< The parent pane id.
};

#endif  // RRA_VIEWS_VIEWER_CONTAINER_WIDGET_H_
