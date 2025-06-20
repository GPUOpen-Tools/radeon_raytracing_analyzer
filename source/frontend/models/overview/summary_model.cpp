//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the Summary model.
//=============================================================================

#include "models/overview/summary_model.h"

#include <QVariant>

#ifdef _LINUX
#include "public/linux/safe_crt.h"
#endif
#include "public/rra_api_info.h"
#include "public/rra_ray_history.h"
#include "public/rra_tlas.h"

#include "managers/trace_manager.h"
#include "settings/settings.h"
#include "util/string_util.h"
#include "views/widget_util.h"

namespace rra
{
    SummaryModel::SummaryModel()
        : ModelViewMapper(kSummaryNumWidgets)
    {
    }

    SummaryModel::~SummaryModel()
    {
        delete global_stats_table_model_;
    }

    void SummaryModel::ResetModelValues()
    {
        for (int i = 0; i < kGlobalStatsTableNumRows; i++)
        {
            widget_util::SetTableModelData(global_stats_table_model_, "", i, 0);
            widget_util::SetTableModelData(global_stats_table_model_, "", i, 1);
        }

        tlas_stats_.clear();
    }

    void SummaryModel::InitializeStatsTableModel(ScaledTableView* table_view)
    {
        global_stats_table_model_ = new QStandardItemModel(kGlobalStatsTableNumRows, 2);

        QStandardItem* count_item = new QStandardItem("Count");
        count_item->setTextAlignment(Qt::AlignRight);
        global_stats_table_model_->setHorizontalHeaderItem(0, new QStandardItem("Type"));
        global_stats_table_model_->setHorizontalHeaderItem(1, count_item);

        table_view->setModel(global_stats_table_model_);

        table_view->SetColumnWidthEms(0, 15);
        table_view->SetColumnWidthEms(1, 10);
    }

    void SummaryModel::Update()
    {
        UpdateStatsTable();
        UpdateTlasList();
    }

    const std::vector<TlasListStatistics>& SummaryModel::GetTlasStatistics() const
    {
        return tlas_stats_;
    }

    uint64_t SummaryModel::GetTotalTraceMemory() const
    {
        return total_trace_memory_;
    }

    bool SummaryModel::RebraidingEnabled()
    {
        uint64_t tlas_count = 0;
        RraErrorCode error_code = RraBvhGetTlasCount(&tlas_count);
        RRA_ASSERT(error_code);

        bool rebraiding_enabled{false};
        for (uint64_t tlas_index = 0; tlas_index < tlas_count; ++tlas_index)
        {
            bool tlas_rebraiding_enabled{};
            error_code = RraTlasGetRebraidingEnabled(tlas_index, &tlas_rebraiding_enabled);
            RRA_ASSERT(error_code == kRraOk);
            rebraiding_enabled |= tlas_rebraiding_enabled;
        }

        return rebraiding_enabled;
    }

    bool SummaryModel::FusedInstancesEnabled()
    {
        uint64_t tlas_count = 0;
        RraErrorCode error_code = RraBvhGetTlasCount(&tlas_count);
        RRA_ASSERT(error_code);

        bool fused_instances_enabled{false};
        for (uint64_t tlas_index = 0; tlas_index < tlas_count; ++tlas_index)
        {
            bool tlas_fused_instance_enabled{};
            error_code = RraTlasGetFusedInstancesEnabled(tlas_index, &tlas_fused_instance_enabled);
            RRA_ASSERT(error_code == kRraOk);
            fused_instances_enabled |= tlas_fused_instance_enabled;
        }

        return fused_instances_enabled;
    }

    void SummaryModel::UpdateStatsTable()
    {
        widget_util::SetTableModelData(global_stats_table_model_, "Total TLASes", kGlobalStatsTableRowTlasCount, 0);
        widget_util::SetTableModelData(global_stats_table_model_, "Total BLASes", kGlobalStatsTableRowBlasCount, 0);
        widget_util::SetTableModelData(global_stats_table_model_, "Empty BLASes", kGlobalStatsTableRowBlasEmpty, 0);

        uint64_t tlas_count = 0;
        if (RraBvhGetTlasCount(&tlas_count) == kRraOk)
        {
            widget_util::SetTableModelData(
                global_stats_table_model_, rra::string_util::LocalizedValue(tlas_count), kGlobalStatsTableRowTlasCount, 1, Qt::AlignRight);
        }

        uint64_t blas_count = 0;
        if (RraBvhGetBlasCount(&blas_count) == kRraOk)
        {
            widget_util::SetTableModelData(
                global_stats_table_model_, rra::string_util::LocalizedValue(blas_count), kGlobalStatsTableRowBlasCount, 1, Qt::AlignRight);
        }

        uint64_t empty_blas_count = 0;
        if (RraBvhGetEmptyBlasCount(&empty_blas_count) == kRraOk)
        {
            widget_util::SetTableModelData(
                global_stats_table_model_, rra::string_util::LocalizedValue(empty_blas_count), kGlobalStatsTableRowBlasEmpty, 1, Qt::AlignRight);
        }

        // Calculate how many rows are needed.
        int  last_row                = kGlobalStatsTableRowBlasMissing;
        bool rebraid_enabled         = RebraidingEnabled();
        bool fused_instances_enabled = FusedInstancesEnabled();

        int extra_rows = 0;
        if (!fused_instances_enabled)
        {
            extra_rows++;
        }
        if (!rebraid_enabled)
        {
            extra_rows++;
        }
        global_stats_table_model_->setRowCount(last_row + extra_rows);

        // Add the extra rows if necessary.
        if (!fused_instances_enabled)
        {
            widget_util::SetTableModelData(global_stats_table_model_, "Missing BLASes", last_row, 0);

            uint64_t missing_blas_count = 0;
            if (RraBvhGetMissingBlasCount(&missing_blas_count) == kRraOk)
            {
                widget_util::SetTableModelData(global_stats_table_model_, rra::string_util::LocalizedValue(missing_blas_count), last_row, 1, Qt::AlignRight);
            }
            last_row++;
        }

        if (!rebraid_enabled)
        {
            widget_util::SetTableModelData(global_stats_table_model_, "Inactive instances", last_row, 0);

            uint64_t inactive_instance_count = 0;
            if (RraBvhGetInactiveInstancesCount(&inactive_instance_count) == kRraOk)
            {
                widget_util::SetTableModelData(
                    global_stats_table_model_, rra::string_util::LocalizedValue(inactive_instance_count), last_row, 1, Qt::AlignRight);
            }
        }

        if (RraBvhGetTotalTraceSizeInBytes(&total_trace_memory_) != kRraOk)
        {
            // On failure we set to 0 so it won't go unnoticed.
            total_trace_memory_ = 0;
        }
    }

    void SummaryModel::UpdateTlasList()
    {
        uint64_t tlas_count = 0;
        RraErrorCode error_code = RraBvhGetTlasCount(&tlas_count);
        RRA_ASSERT(error_code);

        uint64_t rows_added = 0;

        for (uint64_t tlas_index = 0; tlas_index < tlas_count; tlas_index++)
        {
            TlasListStatistics stats = {};

            if (RraTlasGetBaseAddress(tlas_index, &stats.address) != kRraOk)
            {
                continue;
            }

            if (RraTlasGetBuildFlags(tlas_index, &stats.build_flags) != kRraOk)
            {
                continue;
            }

            if (RraTlasGetBlasCount(tlas_index, &stats.blas_count) != kRraOk)
            {
                continue;
            }

            if (RraTlasGetInstanceNodeCount(tlas_index, &stats.instance_count) != kRraOk)
            {
                continue;
            }

            if (RraTlasGetSizeInBytes(tlas_index, &stats.memory) != kRraOk)
            {
                continue;
            }

            if (RraTlasGetEffectiveSizeInBytes(tlas_index, &stats.effective_memory) != kRraOk)
            {
                continue;
            }

            if (RraTlasGetTotalNodeCount(tlas_index, &stats.node_count) != kRraOk)
            {
                continue;
            }

            if (RraTlasGetBoxNodeCount(tlas_index, &stats.box_node_count) != kRraOk)
            {
                continue;
            }

            if (RraTlasGetBox16NodeCount(tlas_index, &stats.box16_node_count) != kRraOk)
            {
                continue;
            }

            if (RraTlasGetBox32NodeCount(tlas_index, &stats.box32_node_count) != kRraOk)
            {
                continue;
            }

            if (RraTlasGetTotalTriangleCount(tlas_index, &stats.total_triangle_count) != kRraOk)
            {
                continue;
            }

            if (RraTlasGetUniqueTriangleCount(tlas_index, &stats.unique_triangle_count) != kRraOk)
            {
                continue;
            }

            if (RraTlasGetTotalProceduralNodeCount(tlas_index, &stats.procedural_node_count) != kRraOk)
            {
                continue;
            }

            if (RraTlasGetInactiveInstancesCount(tlas_index, &stats.inactive_instance_count) != kRraOk)
            {
                continue;
            }

            stats.tlas_index = tlas_index;
            tlas_stats_.push_back(stats);
            rows_added++;
        }

        Q_ASSERT(rows_added == tlas_count);
    }

    void SummaryModel::SearchTextChanged(const QString& filter)
    {
        RRA_UNUSED(filter);
    }

    bool SummaryModel::IsTlasEmpty(uint64_t tlas_index) const
    {
        return RraTlasIsEmpty(tlas_index);
    }

    uint32_t SummaryModel::GetDispatchCount() const
    {
        uint32_t     num_dispatches = 0;
        RraErrorCode status         = RraRayGetDispatchCount(&num_dispatches);
        if (status == kRraOk)
        {
            return num_dispatches;
        }
        return 0;
    }

    RraErrorCode SummaryModel::GetDispatchDimensions(uint32_t dispatch_id, uint32_t* out_width, uint32_t* out_height, uint32_t* out_depth) const
    {
        return RraRayGetDispatchDimensions(dispatch_id, out_width, out_height, out_depth);
    }

}  // namespace rra

