//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the Ray history pane.
//=============================================================================

#ifndef RRA_VIEWS_RAY_RAY_HISTORY_PANE_H_
#define RRA_VIEWS_RAY_RAY_HISTORY_PANE_H_

#include "ui_ray_history_pane.h"

#include "qt_common/utils/zoom_icon_group_manager.h"

#include "models/ray/ray_history_model.h"
#include "models/ray/ray_list_table_item_delegate.h"
#include "views/base_pane.h"
#include "ui_ray_history_viewer_widget.h"

/// @brief Class declaration.
class RayHistoryPane : public BasePane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit RayHistoryPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~RayHistoryPane();

    /// @brief Set the dispatch ID.
    ///
    /// @param dispatch_id The dispatch ID.
    void SetDispatchId(uint64_t dispatch_id);

    /// @brief Called when dispatch combo box selection changes.
    void UpdateSelectedDispatch();

    /// @brief Called when a ray coordinate is double clicked from the ray table.
    ///
    /// @param index The model index.
    void SelectRayAndSwitchPane(const QModelIndex& index);

    /// @brief Overridden window resize event.
    ///
    /// @param [in] event The resize event object.
    virtual void resizeEvent(QResizeEvent* event) Q_DECL_OVERRIDE;

    /// @brief Overridden window show event.
    ///
    /// @param [in] event The show event object.
    virtual void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;

    /// @brief Call OnTraceOpen() for all panes.
    virtual void OnTraceOpen() Q_DECL_OVERRIDE;

    /// @brief Trace closed.
    virtual void OnTraceClose() Q_DECL_OVERRIDE;

private slots:
    /// @brief Set the heatmap mode.
    void SetHeatmapSpectrum();

    /// @brief Set the color mode.
    void SetColorMode();

    /// @brief Set the slice of the 3D dispatch to be displayed in the heatmap.
    void SetSlicePlane();

    /// @brief Set the max traversal counter range.
    ///
    /// @param [in] min_value The minimum counter value.
    /// @param [in] max_value The maximum counter value.
    ///
    /// Interrogates the UI control and passes the data off to the model.
    void SetTraversalCounterRange(int min_value, int max_value);

    /// @brief Set the slice of a 3D dispatch to inspect.
    ///
    /// @param slice_index The index of the dimension not shown in the heatmap image.
    void DispatchSliceChanged(int slice_index);

    /// @brief Update the reshaped dimension of the heatmap.
    ///
    /// @param value The new value for the reshape dimension.
    void ReshapeDimensionChanged(int);

    /// @brief Set the state of the zoom buttons.
    ///
    /// @param [in] zoom_in         Is any further zoom in possible? True if so, false otherwise.
    /// @param [in] zoom_out        Is any further zoom out possible? True if so, false otherwise.
    /// @param [in] zoom_selection  Is a zoom-to-selection possible? True if so, false otherwise.
    /// @param [in] zoom_reset      Is a reset possible? True if so, false otherwise.
    void UpdateZoomButtons(bool zoom_in, bool zoom_out, bool zoom_selection, bool reset);

    /// @brief Called when a ray coordinate is selected from the ray table.
    void SelectRayCoordinate();

private:
    /// @brief Render the ray history heatmap image.
    ///
    /// @param color_mode The color mode to be used for rendering.
    ///
    /// @return The rendered image.
    QImage RenderRayHistoryImage();

    /// @brief Get the maximum heatmap value of the current coloring mode.
    ///
    /// @return The maximum value.
    uint32_t GetCurrentColorModeMaxStatistic();

    /// @brief Create and render the heatmap image.
    void CreateAndRenderImage();

    /// @brief Update the heatmap slider range.
    void UpdateSliderRange();

    /// @brief Update the dispatch ranges for the currently selected dispatch.
    void UpdateDispatchSpinBoxRanges();

    /// @brief Initialize the vector of user-defined reshaped dimensions.
    void InitializeReshapedDimensions();

    /// @brief Get the 1D coordinate of the heatmap x, y coordinate (for 1D dispatches only).
    ///
    /// @param x The x heatmap coordinate.
    /// @param y The y heatmap coordinate.
    ///
    /// @return The 1D coordinate.
    uint32_t Get1DCoordinate(uint32_t x, uint32_t y);

    /// @brief Clear the currently selected ray.
    void ClearRaySelection();

private:
    Ui::RayHistoryPane*                          ui_{};                          ///< Pointer to the Qt UI design.
    rra::RayHistoryModel*                        model_{};                       ///< The ray history model.
    uint32_t                                     dispatch_id_{0xFFFFFFFF};       ///< The current dispatch ID.
    TableItemDelegate*                           table_delegate_{};              ///< The delegate to draw the table rows.
    Ui_RayHistoryViewerWidget                    ray_history_viewer_{};          ///< The viewer widget for the ray history image.
    bool                                         show_event_occured_{};          ///< True if showEvent() has been called.
    std::vector<rra::renderer::HeatmapGenerator> heatmap_generators_{};          ///< The heatmap modes available at update.
    rra::renderer::DispatchIdData                max_statistics_{};              ///< The maximum of each statistic type in the current heatmap image.
    ZoomIconGroupManager*                        zoom_icon_manager_;             ///< The object responsible for the zoom icon status.
    std::vector<GlobalInvocationID>              dispatch_reshaped_dimensions_;  ///< The reshaped dimensions of 1D dispatches.

    // Choose the color mode names and the order they're displayed in.
    const std::vector<std::pair<rra::renderer::RayHistoryColorMode, std::string>> color_modes_and_names_{
        {rra::renderer::kRayHistoryColorModeTraversalCount, "Color heatmap by traversal loop count"},
        {rra::renderer::kRayHistoryColorModeInstanceIntersectionCount, "Color heatmap by instance intersection count"},
        {rra::renderer::kRayHistoryColorModeRayCount, "Color heatmap by ray count"},
        {rra::renderer::kRayHistoryColorModeAnyHitInvocationCount, "Color heatmap by any hit invocation count"},
    };
};

#endif  // RRA_VIEWS_RAY_RAY_HISTORY_PANE_H_
