//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the Summary model.
//=============================================================================

#include "models/overview/summary_model.h"

#include <QVariant>

#include "public/rra_blas.h"
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

        global_stats_table_model_->setHorizontalHeaderItem(0, new QStandardItem("Type"));
        global_stats_table_model_->setHorizontalHeaderItem(1, new QStandardItem("Count"));

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
        RraBvhGetTlasCount(&tlas_count);

        bool rebraiding_enabled{false};
        for (uint64_t tlas_index = 0; tlas_index < tlas_count; ++tlas_index)
        {
            bool tlas_rebraiding_enabled{};
            RraTlasGetRebraidingEnabled(tlas_index, &tlas_rebraiding_enabled);
            rebraiding_enabled |= tlas_rebraiding_enabled;
        }

        return rebraiding_enabled;
    }

    bool SummaryModel::FusedInstancesEnabled()
    {
        uint64_t tlas_count = 0;
        RraBvhGetTlasCount(&tlas_count);

        bool fused_instances_enabled{false};
        for (uint64_t tlas_index = 0; tlas_index < tlas_count; ++tlas_index)
        {
            bool tlas_fused_instance_enabled{};
            RraTlasGetFusedInstancesEnabled(tlas_index, &tlas_fused_instance_enabled);
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
            widget_util::SetTableModelData(global_stats_table_model_, rra::string_util::LocalizedValue(tlas_count), kGlobalStatsTableRowTlasCount, 1);
        }

        uint64_t blas_count = 0;
        if (RraBvhGetBlasCount(&blas_count) == kRraOk)
        {
            widget_util::SetTableModelData(global_stats_table_model_, rra::string_util::LocalizedValue(blas_count), kGlobalStatsTableRowBlasCount, 1);
        }

        uint64_t empty_blas_count = 0;
        if (RraBvhGetEmptyBlasCount(&empty_blas_count) == kRraOk)
        {
            widget_util::SetTableModelData(global_stats_table_model_, rra::string_util::LocalizedValue(empty_blas_count), kGlobalStatsTableRowBlasEmpty, 1);
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
                widget_util::SetTableModelData(global_stats_table_model_, rra::string_util::LocalizedValue(missing_blas_count), last_row, 1);
            }
            last_row++;
        }

        if (!rebraid_enabled)
        {
            widget_util::SetTableModelData(global_stats_table_model_, "Inactive instances", last_row, 0);

            uint64_t inactive_instance_count = 0;
            if (RraBvhGetInactiveInstancesCount(&inactive_instance_count) == kRraOk)
            {
                widget_util::SetTableModelData(global_stats_table_model_, rra::string_util::LocalizedValue(inactive_instance_count), last_row, 1);
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
        RraBvhGetTlasCount(&tlas_count);

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

}  // namespace rra
