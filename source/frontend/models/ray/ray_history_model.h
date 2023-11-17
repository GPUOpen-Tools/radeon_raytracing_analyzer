//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the ray history model.
//=============================================================================

#ifndef RRA_MODELS_RAY_HISTORY_MODEL_H_
#define RRA_MODELS_RAY_HISTORY_MODEL_H_

#include "qt_common/custom_widgets/scaled_table_view.h"
#include "qt_common/utils/model_view_mapper.h"

#include "public/renderer_interface.h"
#include "public/renderer_types.h"
#include "models/ray/ray_list_item_model.h"
#include "models/ray/ray_list_proxy_model.h"
#include "models/ray/ray_counters_item_model.h"

namespace rra
{
    namespace renderer
    {
        struct DispatchIdData;
    }

    enum DispatchStatsTableRows
    {
        kDispatchStatsTableRowRaysPerPixel,
        kDispatchStatsTableRowRayCount,
        kDispatchStatsTableRowTraversalCount,
        kDispatchStatsTableRowInstanceIntersections,

        kDispatchStatsTableNumRows
    };

    enum ShaderBindingTableRows
    {
        kShaderBindingTableRowRaygen,
        kShaderBindingTableRowMiss,
        kShaderBindingTableRowHit,
        kShaderBindingTableRowCallable,

        kShaderBindingTableNumRows
    };

    /// @brief Enum containing indices for the widgets shared between the model and UI.
    enum RayListWidgets
    {
        kRayListNumWidgets,
    };

    /// @brief Container class that holds model data for the ray history pane.
    class RayHistoryModel : public ModelViewMapper
    {
    public:
        /// @brief Constructor.
        ///
        /// param [in] num_model_widgets the number of widgets that require updating by the model.
        explicit RayHistoryModel(int32_t num_model_widgets);

        /// @brief Destructor.
        virtual ~RayHistoryModel();

        /// @brief Initialize blank data for the model.
        void ResetModelValues();

        /// @brief Initialize the stats table model.
        ///
        /// @param [in] table_view   The UI table class.
        void InitializeStatsTableModel(ScaledTableView* table_view);

        /// @brief Initialize the shader binding table model.
        ///
        /// @param [in] table_view   The UI table class.
        void InitializeShaderBindingTableModel(ScaledTableView* table_view);

        /// @brief Initialize the table model.
        ///
        /// @param [in] table_view  The view to the table.
        /// @param [in] num_rows    Total rows of the table.
        /// @param [in] num_columns Total columns of the table.
        void InitializeTableModel(ScaledTableView* table_view, uint num_rows, uint num_columns);

        /// @brief Update the ray list table.
        ///
        /// param [in] dispatch_id  The index of the vkCmdTraceRaysKHR() call.
        void UpdateRayList(uint64_t dispatch_id);

        /// @brief Get the proxy model. Used to set up a connection between the table being sorted and the UI update.
        ///
        /// @return the proxy model.
        RayListProxyModel* GetProxyModel() const;

        /// @brief Repopulate ray list and create ray history image statistics buffer.
        ///
        /// @param dispatch_id         The dispatch ID.
        /// @param [out] out_max_count Maximum count of each dispatch ID statistic.
        void CreateRayHistoryStatsBuffer(uint32_t dispatch_id, renderer::DispatchIdData* out_max_count);

        /// @brief Render the ray history image from the previously generated statistics buffer.
        ///
        /// @param heatmap_min The minimum heatmap slider value.
        /// @param heatmap_max The maximum heatmap slider value.
        /// @param ray_index   The index of the current ray.
        /// @param color_mode  The color mode used for rendering.
        /// @param reshaped_x  The dispatch width, after reshaping for 1D dispatches.
        /// @param reshaped_y  The dispatch height, after reshaping for 1D dispatches.
        /// @param reshaped_z  The dispatch depth, after reshaping for 1D dispatches.
        ///
        /// @return The ray history image.
        QImage RenderRayHistoryImage(uint32_t heatmap_min,
                                     uint32_t heatmap_max,
                                     uint32_t ray_index,
                                     uint32_t reshaped_x,
                                     uint32_t reshaped_y,
                                     uint32_t reshaped_z);

        /// @brief Set the heatmap data.
        ///
        /// @param [in] heatmap_data The raw data of the heatmap.
        void SetHeatmapData(renderer::HeatmapData heatmap_data);

        /// @brief Set heatmap update callback.
        ///
        /// @param [in] heatmap_update_callback The callback.
        void SetHeatmapUpdateCallback(std::function<void(rra::renderer::HeatmapData)> heatmap_update_callback);

        /// @brief Set the color mode to render the heatmap with.
        void SetColorMode(renderer::RayHistoryColorMode color_mode);

        /// @brief Get the currently set color mode.
        ///
        /// @return The color mode.
        renderer::RayHistoryColorMode GetColorMode() const;

        /// @brief Get the ray dispatch coordinate from a model index from the ray list.
        ///
        /// @param [in] index                       The model index.
        /// @param [out] x                          The x-component of the ray dispatch coordinate.
        /// @param [out] y                          The y-component of the ray dispatch coordinate.
        /// @param [out] z                          The z-component of the ray dispatch coordinate.
        /// @param [out] total_rays_at_coordinate   The number of total rays at the ray dispatch coordinate.
        void GetRayCoordinate(const QModelIndex& index, uint32_t* x, uint32_t* y, uint32_t* z, uint32_t* total_rays_at_coordinate);

        /// @brief Get the model index from a ray's dispatch coordinate.
        ///
        /// @param coordinate The ray coordinate.
        QModelIndex GetDispatchCoordinateIndex(const GlobalInvocationID& coordinate);

        /// @brief Set the slice to be viewed of the 3D dispatch.
        ///
        /// @param index The index.
        void SetSliceIndex(uint32_t index);

        /// @brief Get the slice being viewed of the 3D dispatch.
        ///
        /// @return The slice index.
        uint32_t GetSliceIndex() const;

        /// @brief Set the plane of the 3D dispatch that will be rendered in the heatmap.
        ///
        /// @param slice_plane The plane of the 3D dispatch to be rendered.
        void SetSlicePlane(renderer::SlicePlane slice_plane);

        /// @brief Get the currently set slice plane.
        ///
        /// @return The slice plane.
        renderer::SlicePlane GetSlicePlane() const;

    public slots:
        /// @brief Handle what happens when the search filter changes.
        ///
        /// @param [in] filter The search text filter.
        void SearchTextChanged(const QString& filter);

        /// @brief Update the stats table.
        void UpdateStatsTable();

        /// @brief Update the shader binding table.
        void UpdateShaderBindingTable();

    private:
        RayListItemModel*                          table_model_;                           ///< Holds the shader binding table data.
        RayListProxyModel*                         proxy_model_;                           ///< Proxy model for the BLAS list table.
        RayCountersItemModel*                      stats_table_model_          = nullptr;  ///< Model associated with the stats table.
        QStandardItemModel*                        shader_binding_table_model_ = nullptr;  ///< Model associated with the shader binding table.
        std::function<void(renderer::HeatmapData)> heatmap_update_callback_    = nullptr;  ///< The heatmap update callback.
        uint32_t                                   current_dispatch_id_        = 0;        ///< The currently selected dispatch id.
        renderer::RayHistoryColorMode              color_mode_;                            ///< The color mode to render the heatmap with.
        uint32_t                                   slice_index_ = 0;                       ///< The slice to be viewed of the 3D dispatch.
        renderer::SlicePlane slice_plane_ = renderer::kSlicePlaneXY;  ///< Determines which plane of a 3D dispatch will be shown in the heatmap image.
    };
}  // namespace rra

#endif  // RRA_MODELS_RAY_HISTORY_MODEL_H_
