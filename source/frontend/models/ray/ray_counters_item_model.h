//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the ray counters item model.
///
/// Used for the ray counters table shown in the RAY tab.
///
//=============================================================================

#ifndef RRA_MODELS_RAY_COUNTERS_ITEM_MODEL_H_
#define RRA_MODELS_RAY_COUNTERS_ITEM_MODEL_H_

#include <QAbstractItemModel>

#include "qt_common/custom_widgets/scaled_table_view.h"

#include "public/rra_ray_history.h"

namespace rra
{
    /// @brief Row Id's for the fields in the ray counters.
    enum RayCountsRow
    {
        kRayCountersRowRayPerPixel,
        kRayCountersRowRayCount,
        kRayCountersRowTraversalLoopCount,
        kRayCountersRowInstanceIntersectionCount,

        kRayCountersRowCount,
    };

    enum RayCountsColumn
    {
        kRayCountersColumnLabel,
        kRayCountersColumnData,

        kRayCountersColumnCount,
    };

    /// @brief A class to handle the model data associated with ray list table.
    class RayCountersItemModel : public QAbstractItemModel
    {
    public:
        /// @brief Constructor.
        explicit RayCountersItemModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~RayCountersItemModel();

        /// @brief Set the number of rows in the table.
        ///
        /// @param [in] rows The number of rows required.
        void SetRowCount(int rows);

        /// @brief Set the number of columns in the table.
        ///
        /// @param [in] columns The number of columns required.
        void SetColumnCount(int columns);

        /// @brief Initialize the ray counters table.
        ///
        /// @param [in] table  The table to initialize.
        void Initialize(ScaledTableView* table);

        /// @brief Set the table data.
        ///
        /// @param rays_per_pixel                 The rays per pixel.
        /// @param total_ray_count                The total ray count in the dispatch.
        /// @param traversal_count_per_ray        The total traversal count divided by ray count.
        /// @param instance_intersections_per_ray The total number of instance intersections divided by ray count.
        void SetData(float rays_per_pixel, uint32_t total_ray_count, float traversal_count_per_ray, float instance_intersections_per_ray);

        // QAbstractItemModel overrides. See Qt documentation for parameter and return values
        virtual QVariant      data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;
        virtual Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual QModelIndex   index(int row, int column, const QModelIndex& parent) const Q_DECL_OVERRIDE;
        virtual QModelIndex   parent(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual int           rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
        virtual int           columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

    private:
        int      num_rows_;                        ///< The number of rows in the table.
        int      num_columns_;                     ///< The number of columns in the table.
        float    rays_per_pixel_;                  ///< The rays per pixel.
        uint32_t total_ray_count_;                 ///< The total ray count in the dispatch.
        float    traversal_count_per_ray_;         ///< The total traversal count divided by ray count.
        float    instance_intersections_per_ray_;  ///< The total number of instance intersections divided by ray count.
    };
}  // namespace rra

#endif  // RRA_MODELS_RAY_COUNTERS_ITEM_MODEL_H_
