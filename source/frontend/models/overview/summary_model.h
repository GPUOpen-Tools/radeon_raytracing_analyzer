//=============================================================================
// Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the Summary model.
//=============================================================================

#ifndef RRA_MODELS_OVERVIEW_SUMMARY_MODEL_H_
#define RRA_MODELS_OVERVIEW_SUMMARY_MODEL_H_

#include <vector>
#include <QStandardItemModel>
#include "vulkan/include/vulkan/vulkan_core.h"

#include "qt_common/custom_widgets/scaled_table_view.h"
#include "qt_common/utils/model_view_mapper.h"

#include "public/rra_error.h"
#include "public/rra_ray_history.h"

namespace rra
{
    /// @brief Structure describing the statistics needed for the TLAS list table in the overview pane.
    struct TlasListStatistics
    {
        uint64_t                                address;                  ///< The base address of the TLAS.
        VkBuildAccelerationStructureFlagBitsKHR build_flags;              ///< Build flags used when generating this TLAS.
        uint64_t                                instance_count;           ///< The instance count.
        uint64_t                                inactive_instance_count;  ///< The number of inactive instances.
        uint64_t                                blas_count;               ///< The BLAS count.
        uint32_t                                memory;                   ///< Memory used by the TLAS.
        uint64_t                                effective_memory;         ///< Memory used by the TLAS and any BLASes it references.
        uint32_t                                tlas_index;               ///< TLAS index.
        uint64_t                                total_triangle_count;     ///< The sum of the triangles in each instance referenced by this TLAS.
        uint64_t                                unique_triangle_count;    ///< The sum of the triangles in each BLAS referenced by this TLAS.
        uint64_t                                procedural_node_count;    ///< The total number of procedural nodes in BLASes referenced by this TLAS.
        uint64_t                                node_count;               ///< The number of nodes in this TLAS.
        uint64_t                                box_node_count;           ///< The number of box nodes in this TLAS.
        uint32_t                                box16_node_count;         ///< The number of box16 nodes in this TLAS.
        uint32_t                                box32_node_count;         ///< The number of box32 nodes in this TLAS.
    };

    enum GlobalStatsTableRows
    {
        kGlobalStatsTableRowTlasCount,
        kGlobalStatsTableRowBlasCount,
        kGlobalStatsTableRowBlasEmpty,
        kGlobalStatsTableRowBlasMissing,
        kGlobalStatsTableRowInstanceInactive,

        kGlobalStatsTableNumRows
    };

    /// @brief An enum of widgets used by the UI and model.
    ///
    /// Used to map UI widgets to their corresponding model data.
    enum SummaryWidgets
    {
        kSummaryNumWidgets
    };

    /// @brief Container class that holds model data for the Summary pane.
    class SummaryModel : public ModelViewMapper
    {
    public:
        /// @brief Constructor.
        explicit SummaryModel();

        /// @brief Destructor.
        virtual ~SummaryModel();

        /// @brief Initialize blank data for the model.
        void ResetModelValues();

        /// @brief Update the stats table model.
        ///
        /// This is the table at the top of the UI showing the global acceleration structure information
        /// such as the total number of TLAS and BLAS objects in the trace.
        ///
        /// @param [in] table_view   The UI table class.
        void InitializeStatsTableModel(ScaledTableView* table_view);

        /// @brief Update the model with data from the back end.
        void Update();

        /// @brief Get the data to be displayed in the TLAS pane.
        ///
        /// @return The TLAS statistics.
        const std::vector<TlasListStatistics>& GetTlasStatistics() const;

        /// @brief Get the memory used by all the TLASes and BLASes in the trace.
        ///
        /// @return The memory in bytes.
        uint64_t GetTotalTraceMemory() const;

        /// @brief Whether or not any TLASes in the trace use rebraiding.
        bool RebraidingEnabled();

        /// @brief Whether or not any TLASes in the trace use fused instances.
        bool FusedInstancesEnabled();

        /// @brief Is the specified TLAS empty.
        ///
        /// @param [in] tlas_index The index of the TLAS to query.
        ///
        /// @return true if the TLAS is empty, false otherwise.
        bool IsTlasEmpty(uint64_t tlas_index) const;

        /// @brief Get the number of dispatches in the trace file.
        ///
        /// @return The number of dispatches.
        uint32_t GetDispatchCount() const;

        /// @brief Get the dispatch dimensions.
        ///
        /// @param [in]  dispatch_id  The dispatch Id.
        /// @param [out] out_width    The x dimension.
        /// @param [out] out_height   The y dimension.
        /// @param [out] out_depth    The z dimension.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        RraErrorCode GetDispatchDimensions(uint32_t dispatch_id, uint32_t* out_width, uint32_t* out_height, uint32_t* out_depth) const;

    public slots:
        /// @brief Handle what happens when the search filter changes.
        ///
        /// @param [in] filter The search text filter.
        void SearchTextChanged(const QString& filter);

    private:
        /// @brief Update the stats table.
        void UpdateStatsTable();

        /// @brief Update the TLAS list.
        void UpdateTlasList();

        QStandardItemModel*             global_stats_table_model_ = nullptr;  ///< Model associated with the global stats table.
        std::vector<TlasListStatistics> tlas_stats_;                          ///< The TLAS data to be displayed in the TLAS pane.
        uint64_t                        total_trace_memory_{};                ///< The total memory used by all TLASes and BLASes.
    };

}  // namespace rra

#endif  // RRA_MODELS_OVERVIEW_SUMMARY_MODEL_H_
