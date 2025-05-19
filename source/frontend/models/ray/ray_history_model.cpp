//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the ray history model.
//=============================================================================

#include "models/ray/ray_history_model.h"

#include <algorithm>
#include <thread>

#include <QHeaderView>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>

#include "public/graphics_context.h"

#include "constants.h"
#include "models/ray/ray_list_item_model.h"
#include "settings/settings.h"
#include "util/string_util.h"
#include "views/widget_util.h"

#undef min

namespace rra
{
    RayHistoryModel::RayHistoryModel(int32_t num_model_widgets)
        : ModelViewMapper(num_model_widgets)
        , table_model_(nullptr)
        , proxy_model_(nullptr)
    {
    }

    RayHistoryModel::~RayHistoryModel()
    {
        delete table_model_;
        delete proxy_model_;
        delete shader_binding_table_model_;
        delete stats_table_model_;
    }

    void RayHistoryModel::ResetModelValues()
    {
        table_model_->removeRows(0, table_model_->rowCount());
        table_model_->ClearCache();
    }

    void RayHistoryModel::InitializeStatsTableModel(ScaledTableView* table_view)
    {
        stats_table_model_ = new RayCountersItemModel(table_view);

        table_view->setModel(stats_table_model_);

        stats_table_model_->SetRowCount(kRayCountersRowCount);
        stats_table_model_->SetColumnCount(kRayCountersColumnCount);
    }

    void RayHistoryModel::InitializeShaderBindingTableModel(ScaledTableView* table_view)
    {
        shader_binding_table_model_ = new QStandardItemModel(kShaderBindingTableNumRows, 4);

        shader_binding_table_model_->setHorizontalHeaderItem(0, new QStandardItem("Group type"));
        QStandardItem* stride = new QStandardItem("Stride");
        stride->setTextAlignment(Qt::AlignRight);
        shader_binding_table_model_->setHorizontalHeaderItem(1, stride);
        QStandardItem* size = new QStandardItem("Size");
        size->setTextAlignment(Qt::AlignRight);
        shader_binding_table_model_->setHorizontalHeaderItem(2, size);
        QStandardItem* base_address = new QStandardItem("Base address");
        base_address->setTextAlignment(Qt::AlignRight);
        shader_binding_table_model_->setHorizontalHeaderItem(3, base_address);

        table_view->setModel(shader_binding_table_model_);

        table_view->SetColumnWidthEms(0, 10);
        table_view->SetColumnWidthEms(1, 8);
        table_view->SetColumnWidthEms(2, 8);
        table_view->SetColumnWidthEms(3, 12);

        UpdateShaderBindingTable();
    }

    /// @brief                 Update only a segment of the ray list (for parallelization).
    ///
    /// @param begin_x         The first x-coordinate (inclusive) to update.
    /// @param end_x           The last x-coordinate (exclusive) to update.
    /// @param dispatch_id     The dispatch ID.
    /// @param dispatch_width  The dispatch width.
    /// @param dispatch_height The dispatch height.
    /// @param dispatch_depth  The dispatch depth.
    /// @param table_model     The table model to update the values of.
    void UpdateRayListSegment(uint32_t          begin_x,
                              uint32_t          end_x,
                              uint32_t          dispatch_id,
                              uint32_t          dispatch_width,
                              uint32_t          dispatch_height,
                              uint32_t          dispatch_depth,
                              RayListItemModel* table_model)
    {
        RRA_UNUSED(dispatch_width);

        for (uint32_t x{begin_x}; x < end_x; ++x)
        {
            for (uint32_t y{0}; y < dispatch_height; ++y)
            {
                for (uint32_t z{0}; z < dispatch_depth; ++z)
                {
                    RayListStatistics stats = {};
                    stats.invocation_id     = {x, y, z};

                    DispatchCoordinateStats dispatch_coord_stats;

                    RraRayGetDispatchCoordinateStats(dispatch_id, stats.invocation_id, &dispatch_coord_stats);

                    if (dispatch_coord_stats.ray_count == 0)
                    {
                        continue;
                    }

                    stats.ray_count                  = dispatch_coord_stats.ray_count;
                    stats.ray_instance_intersections = dispatch_coord_stats.intersection_count;
                    stats.ray_event_count            = dispatch_coord_stats.loop_iteration_count;
                    stats.ray_any_hit_invocations    = dispatch_coord_stats.any_hit_count;

                    table_model->AddRow(std::move(stats));
                }
            }
        }
    }

    void RayHistoryModel::UpdateRayList(uint64_t dispatch_id)
    {
        current_dispatch_id_ = dispatch_id;
        ResetModelValues();

        uint32_t dispatch_width{};
        uint32_t dispatch_height{};
        uint32_t dispatch_depth{};
        if (RraRayGetDispatchDimensions(dispatch_id, &dispatch_width, &dispatch_height, &dispatch_depth) != kRraOk)
        {
            return;
        }

        table_model_->SetDispatchDimensions(dispatch_width, dispatch_height, dispatch_depth);

        // Only use 1 thread at the moment since skipping empty rays makes the index we're writing to cache_ unpredictable.
        const uint32_t           thread_count{1};
        std::vector<std::thread> threads{};
        threads.reserve(thread_count);

        const uint32_t segment_size{dispatch_width / thread_count};
        uint32_t       begin{0};

        for (uint32_t i{0}; i < thread_count; ++i)
        {
            // For the last segment, we make sure that it covers elements up to the end of the list in the event that dispatch_width isn't divided evenly by processor_count.
            const uint32_t end{(i == thread_count - 1) ? dispatch_width : begin + segment_size};
            threads.push_back(std::thread{UpdateRayListSegment, begin, end, dispatch_id, dispatch_width, dispatch_height, dispatch_depth, table_model_});
            begin += segment_size;
        }

        for (std::thread& t : threads)
        {
            t.join();
        }

        UpdateStatsTable();
        proxy_model_->SetFilterAcceptsAll(true);
        proxy_model_->invalidate();
        proxy_model_->SetFilterAcceptsAll(false);
    }

    void RayHistoryModel::InitializeTableModel(ScaledTableView* table_view, uint num_rows, uint num_columns)
    {
        if (proxy_model_ != nullptr)
        {
            delete proxy_model_;
            proxy_model_ = nullptr;
        }

        proxy_model_ = new RayListProxyModel();
        table_model_ = proxy_model_->InitializeRayTableModels(table_view, num_rows, num_columns);
        table_model_->Initialize(table_view);
    }

    void RayHistoryModel::SearchTextChanged(const QString& filter)
    {
        proxy_model_->SetSearchFilter(filter);
        proxy_model_->invalidate();
    }

    RayListProxyModel* RayHistoryModel::GetProxyModel() const
    {
        return proxy_model_;
    }

    void RayHistoryModel::CreateRayHistoryStatsBuffer(uint32_t dispatch_id, rra::renderer::DispatchIdData* out_max_count)
    {
        GraphicsContextCreateRayHistoryStatsBuffer(dispatch_id, out_max_count);
    }

    QImage RayHistoryModel::RenderRayHistoryImage(uint32_t heatmap_min,
                                                  uint32_t heatmap_max,
                                                  uint32_t ray_index,
                                                  uint32_t reshaped_x,
                                                  uint32_t reshaped_y,
                                                  uint32_t reshaped_z) const
    {
        return GraphicsContextRenderRayHistoryImage(
            heatmap_min, heatmap_max, ray_index, reshaped_x, reshaped_y, reshaped_z, color_mode_, slice_index_, slice_plane_);
    }

    void RayHistoryModel::SetHeatmapData(rra::renderer::HeatmapData heatmap_data)
    {
        GraphicsContextSetRayHistoryHeatmapData(heatmap_data);
        heatmap_update_callback_(heatmap_data);
    }

    void RayHistoryModel::SetHeatmapUpdateCallback(std::function<void(renderer::HeatmapData)> heatmap_update_callback)
    {
        heatmap_update_callback_ = heatmap_update_callback;
    }

    void RayHistoryModel::SetColorMode(renderer::RayHistoryColorMode color_mode)
    {
        color_mode_ = color_mode;
    }

    renderer::RayHistoryColorMode RayHistoryModel::GetColorMode() const
    {
        return color_mode_;
    }

    void RayHistoryModel::GetRayCoordinate(const QModelIndex& index, uint32_t* x, uint32_t* y, uint32_t* z, uint32_t* total_rays_at_coordinate)
    {
        const QModelIndex x_column_index = proxy_model_->index(index.row(), kRayListColumnInvocationX, QModelIndex());
        const QModelIndex y_column_index = proxy_model_->index(index.row(), kRayListColumnInvocationY, QModelIndex());
        const QModelIndex z_column_index = proxy_model_->index(index.row(), kRayListColumnInvocationZ, QModelIndex());

        if (x_column_index.isValid() && y_column_index.isValid() && z_column_index.isValid())
        {
            const QModelIndex x_proxy_model_index = proxy_model_->mapToSource(x_column_index);
            const QModelIndex y_proxy_model_index = proxy_model_->mapToSource(y_column_index);
            const QModelIndex z_proxy_model_index = proxy_model_->mapToSource(z_column_index);

            if (x_proxy_model_index.isValid() && y_proxy_model_index.isValid() && z_proxy_model_index.isValid())
            {
                *x = x_column_index.data(Qt::UserRole).toULongLong();
                *y = y_column_index.data(Qt::UserRole).toULongLong();
                *z = z_column_index.data(Qt::UserRole).toULongLong();
                RraRayGetRayCount(current_dispatch_id_, {*x, *y, *z}, total_rays_at_coordinate);
            }
        }
    }

    QModelIndex RayHistoryModel::GetDispatchCoordinateIndex(const GlobalInvocationID& coordinate)
    {
        return proxy_model_->FindModelIndex(
            coordinate.x, coordinate.y, coordinate.z, kRayListColumnInvocationX, kRayListColumnInvocationY, kRayListColumnInvocationZ);
    }

    void RayHistoryModel::SetSliceIndex(uint32_t index)
    {
        slice_index_ = index;
    }

    uint32_t RayHistoryModel::GetSliceIndex() const
    {
        return slice_index_;
    }

    void RayHistoryModel::SetSlicePlane(renderer::SlicePlane slice_plane)
    {
        slice_plane_ = slice_plane;
    }

    renderer::SlicePlane RayHistoryModel::GetSlicePlane() const
    {
        return slice_plane_;
    }

    void RayHistoryModel::UpdateStatsTable()
    {
        GlobalInvocationID filter_min{};
        GlobalInvocationID filter_max{};
        proxy_model_->GetFilterMinAndMax(&filter_min, &filter_max);

        uint32_t ray_count{};
        uint32_t traversal_count{};
        uint32_t instance_intersection_count{};
        table_model_->GetFilteredAggregateStatistics(filter_min, filter_max, &ray_count, &traversal_count, &instance_intersection_count);

        uint32_t pixel_count = (filter_max.x - filter_min.x + 1) * (filter_max.y - filter_min.y + 1);
        if (!pixel_count)
        {
            uint32_t width{};
            uint32_t height{};
            uint32_t depth{};
            RraRayGetDispatchDimensions(current_dispatch_id_, &width, &height, &depth);
            pixel_count = width * height;
        }

        float rays_per_pixel                      = (float)ray_count / pixel_count;
        float traversal_count_per_ray             = ray_count ? traversal_count / (float)ray_count : 0.0f;
        float instance_intersection_count_per_ray = ray_count ? instance_intersection_count / (float)ray_count : 0.0f;

        stats_table_model_->SetData(rays_per_pixel, ray_count, traversal_count_per_ray, instance_intersection_count_per_ray);
    }

    void RayHistoryModel::UpdateShaderBindingTable()
    {
        uint32_t raygen_stride{64};
        uint32_t miss_stride{64};
        uint32_t hit_stride{128};
        uint32_t callable_stride{0};

        uint32_t raygen_size{64};
        uint32_t miss_size{128};
        uint32_t hit_size{1664};
        uint32_t callable_size{0};

        uint32_t raygen_count{raygen_stride == 0 ? 0 : raygen_size / raygen_stride};
        uint32_t miss_count{miss_stride == 0 ? 0 : miss_size / miss_stride};
        uint32_t hit_count{hit_stride == 0 ? 0 : hit_size / hit_stride};
        uint32_t callable_count{callable_stride == 0 ? 0 : callable_size / callable_stride};

        QString raygen_count_str{raygen_count < 2 ? "" : " (" + QString::number(raygen_count) + ")"};
        QString miss_count_str{miss_count < 2 ? "" : " (" + QString::number(miss_count) + ")"};
        QString hit_count_str{hit_count < 2 ? "" : " (" + QString::number(hit_count) + ")"};
        QString callable_count_str{callable_count < 2 ? "" : " (" + QString::number(callable_count) + ")"};

        widget_util::SetTableModelData(shader_binding_table_model_, "Raygen" + raygen_count_str, kShaderBindingTableRowRaygen, 0);
        widget_util::SetTableModelData(shader_binding_table_model_, "Miss" + miss_count_str, kShaderBindingTableRowMiss, 0);
        widget_util::SetTableModelData(shader_binding_table_model_, "Hit" + hit_count_str, kShaderBindingTableRowHit, 0);
        widget_util::SetTableModelData(shader_binding_table_model_, "Callable" + callable_count_str, kShaderBindingTableRowCallable, 0);

        widget_util::SetTableModelData(
            shader_binding_table_model_, rra::string_util::LocalizedValue(raygen_stride), kShaderBindingTableRowRaygen, 1, Qt::AlignRight);
        widget_util::SetTableModelData(
            shader_binding_table_model_, rra::string_util::LocalizedValue(miss_stride), kShaderBindingTableRowMiss, 1, Qt::AlignRight);
        widget_util::SetTableModelData(shader_binding_table_model_, rra::string_util::LocalizedValue(hit_stride), kShaderBindingTableRowHit, 1, Qt::AlignRight);
        widget_util::SetTableModelData(
            shader_binding_table_model_, rra::string_util::LocalizedValue(callable_stride), kShaderBindingTableRowCallable, 1, Qt::AlignRight);

        widget_util::SetTableModelData(
            shader_binding_table_model_, rra::string_util::LocalizedValue(raygen_size), kShaderBindingTableRowRaygen, 2, Qt::AlignRight);
        widget_util::SetTableModelData(shader_binding_table_model_, rra::string_util::LocalizedValue(miss_size), kShaderBindingTableRowMiss, 2, Qt::AlignRight);
        widget_util::SetTableModelData(shader_binding_table_model_, rra::string_util::LocalizedValue(hit_size), kShaderBindingTableRowHit, 2, Qt::AlignRight);
        widget_util::SetTableModelData(
            shader_binding_table_model_, rra::string_util::LocalizedValue(callable_size), kShaderBindingTableRowCallable, 2, Qt::AlignRight);

        widget_util::SetTableModelData(shader_binding_table_model_, "0x" + QString("%1").arg(4574537, 0, 16), kShaderBindingTableRowRaygen, 3, Qt::AlignRight);
        widget_util::SetTableModelData(shader_binding_table_model_, "0x" + QString("%1").arg(4574585, 0, 16), kShaderBindingTableRowMiss, 3, Qt::AlignRight);
        widget_util::SetTableModelData(shader_binding_table_model_, "0x" + QString("%1").arg(4574599, 0, 16), kShaderBindingTableRowHit, 3, Qt::AlignRight);
        widget_util::SetTableModelData(shader_binding_table_model_, "0x" + QString("%1").arg(0, 0, 16), kShaderBindingTableRowCallable, 3, Qt::AlignRight);
    }

}  // namespace rra

