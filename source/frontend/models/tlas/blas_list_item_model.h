//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the BLAS list item model.
///
/// Used for the Blas list shown in the TLAS tab.
///
//=============================================================================

#ifndef RRA_MODELS_TLAS_BLAS_LIST_ITEM_MODEL_H_
#define RRA_MODELS_TLAS_BLAS_LIST_ITEM_MODEL_H_

#include <QAbstractItemModel>

#include "vulkan/include/vulkan/vulkan_core.h"

#include "qt_common/custom_widgets/scaled_table_view.h"

#include "public/rra_bvh.h"

namespace rra
{
    /// @brief Structure describing the statistics needed for the BLAS list pane.
    struct BlasListStatistics
    {
        uint64_t                                address;                ///< The base address.
        VkBuildAccelerationStructureFlagBitsKHR build_flags;            ///< The build flags used to build the BVH.
        uint64_t                                instance_count;         ///< The instance count.
        uint64_t                                node_count;             ///< The total number of nodes.
        uint64_t                                box_count;              ///< The number of box (internal) nodes.
        uint32_t                                box32_count;            ///< The number of box-32 nodes.
        uint32_t                                box16_count;            ///< The number of box-16 nodes.
        uint32_t                                triangle_node_count;    ///< The number of triangle nodes.
        uint32_t                                procedural_node_count;  ///< The number of procedural nodes.
        uint32_t                                memory_usage;           ///< The amount of memory this BLAS uses.
        float                                   root_sah;               ///< The root node surface area heuristic value.
        float                                   max_sah;                ///< The maximum surface area heuristic value in the BVH.
        float                                   mean_sah;               ///< The average surface area heuristic value in the BVH.
        uint32_t                                max_depth;              ///< The maximum tree depth.
        uint32_t                                avg_depth;              ///< The average tree depth.
        uint64_t                                blas_index;             ///< The BLAS index.
    };

    /// @brief Column Id's for the fields in the acceleration structure list.
    enum BlasListColumn
    {
        kBlasListColumnAddress,
        kBlasListColumnAllowUpdate,
        kBlasListColumnAllowCompaction,
        kBlasListColumnLowMemory,
        kBlasListColumnBuildType,
        kBlasListColumnInstanceCount,
        kBlasListColumnNodeCount,
        kBlasListColumnBoxCount,
        kBlasListColumnBox32Count,
        kBlasListColumnBox16Count,
        kBlasListColumnTriangleNodeCount,
        kBlasListColumnProceduralNodeCount,
        kBlasListColumnMemoryUsage,
        kBlasListColumnRootSAH,
        kBlasListColumnMinSAH,
        kBlasListColumnMeanSAH,
        kBlasListColumnMaxDepth,
        kBlasListColumnAvgDepth,
        kBlasListColumnBlasIndex,
        kBlasListColumnPadding,

        kBlasListColumnCount,
    };

    /// @brief A class to handle the model data associated with BLAS list table.
    class BlasListItemModel : public QAbstractItemModel
    {
    public:
        /// @brief Constructor.
        explicit BlasListItemModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~BlasListItemModel();

        /// @brief Set the number of rows in the table.
        ///
        /// @param [in] rows The number of rows required.
        void SetRowCount(int rows);

        /// @brief Set the number of columns in the table.
        ///
        /// @param [in] columns The number of columns required.
        void SetColumnCount(int columns);

        /// @brief Initialize the acceleration structure list table.
        ///
        /// An instance of this table is present in the TLAS and BLAS list panes.
        ///
        /// @param [in] acceleration_structure_table  The table to initialize.
        void Initialize(ScaledTableView* acceleration_structure_table);

        /// @brief Add an acceleration structure to the table.
        ///
        /// @param [in] stats  The statistics to add to the table.
        void AddAccelerationStructure(const BlasListStatistics& stats);

        // QAbstractItemModel overrides. See Qt documentation for parameter and return values
        virtual QVariant      data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;
        virtual Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual QVariant      headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
        virtual QModelIndex   index(int row, int column, const QModelIndex& parent) const Q_DECL_OVERRIDE;
        virtual QModelIndex   parent(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual int           rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
        virtual int           columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

    private:
        int                             num_rows_;     ///< The number of rows in the table.
        int                             num_columns_;  ///< The number of columns in the table.
        std::vector<BlasListStatistics> cache_;        ///< Cached data from the backend.
    };
}  // namespace rra

#endif  // RRA_MODELS_TLAS_BLAS_LIST_ITEM_MODEL_H_
