//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the ray list item model.
///
/// Used for the ray list shown in the RAY tab.
///
//=============================================================================

#ifndef RRA_MODELS_RAY_LIST_ITEM_MODEL_H_
#define RRA_MODELS_RAY_LIST_ITEM_MODEL_H_

#include <QAbstractItemModel>

#include "vulkan/include/vulkan/vulkan_core.h"

#include "qt_common/custom_widgets/scaled_table_view.h"

#include "public/rra_ray_history.h"

namespace rra
{
    struct RayListStatistics
    {
        GlobalInvocationID invocation_id;
        uint32_t           ray_count;
        uint32_t           ray_event_count;
        uint32_t           ray_instance_intersections;
        uint32_t           ray_any_hit_invocations;
    };

    /// @brief Column Id's for the fields in the ray list.
    enum RayListColumn
    {
        kRayListColumnInvocationX,
        kRayListColumnInvocationY,
        kRayListColumnInvocationZ,
        kRayListColumnRayCount,
        kRayListColumnTraversalCount,
        kRayListColumnInstanceIntersections,
        kRayListColumnAnyHitInvocations,
        kRayListColumnPadding,

        kRayListColumnCount,
    };

    /// @brief A class to handle the model data associated with ray list table.
    class RayListItemModel : public QAbstractItemModel
    {
    public:
        /// @brief Constructor.
        explicit RayListItemModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~RayListItemModel();

        /// @brief Set the number of columns in the table.
        ///
        /// @param [in] columns The number of columns required.
        void SetColumnCount(int columns);

        /// @brief Initialize the ray list table.
        ///
        /// An instance of this table is present in the TLAS and BLAS list panes.
        ///
        /// @param [in] acceleration_structure_table  The table to initialize.
        void Initialize(ScaledTableView* acceleration_structure_table);

        /// @brief Set the dimensions of the dispatch shown in the table to allocate memory for them.
        ///
        /// @param width  The dispatch width;
        /// @param height The dispatch height;
        /// @param depth  The dispatch depth;
        void SetDispatchDimensions(uint32_t width, uint32_t height, uint32_t depth);

        /// @brief Clear the row cache.
        void ClearCache();

        /// @brief Add a new row's statistics.
        ///
        /// @param stats The new row's statistics.
        void AddRow(RayListStatistics&& stats);

        /// @brief Get the summed statistics of all rays in the user selected box in the heatmap image.
        ///
        /// @param filter_min                      Minimum extents of the box.
        /// @param filter_max                      Maximum extents of the box.
        /// @param out_ray_count                   The summed ray count.
        /// @param out_traversal_count             The summed traversal count.
        /// @param out_instance_intersection_count The summed instance intersection count.
        void GetFilteredAggregateStatistics(const GlobalInvocationID& filter_min,
                                            const GlobalInvocationID& filter_max,
                                            uint32_t*                 out_ray_count,
                                            uint32_t*                 out_traversal_count,
                                            uint32_t*                 out_instance_intersection_count);

        // QAbstractItemModel overrides. See Qt documentation for parameter and return values
        virtual QVariant      data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;
        virtual Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual QVariant      headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
        virtual QModelIndex   index(int row, int column, const QModelIndex& parent) const Q_DECL_OVERRIDE;
        virtual QModelIndex   parent(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual int           rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
        virtual int           columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

    private:
        int                            num_rows_;     ///< The number of rows in the table.
        int                            num_columns_;  ///< The number of columns in the table.
        std::vector<RayListStatistics> cache_;        ///< Cached data from the backend.
    };
}  // namespace rra

#endif  // RRA_MODELS_RAY_LIST_ITEM_MODEL_H_
