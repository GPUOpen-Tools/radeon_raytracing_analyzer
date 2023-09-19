//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the ray inspector ray list item model.
///
//=============================================================================

#ifndef RRA_MODELS_RAY_INSPECTOR_RAY_LIST_ITEM_MODEL_H_
#define RRA_MODELS_RAY_INSPECTOR_RAY_LIST_ITEM_MODEL_H_

#include <QAbstractItemModel>

#include "qt_common/custom_widgets/scaled_table_view.h"

#include "public/rra_ray_history.h"

namespace rra
{
    struct RayInspectorRayListStatistics
    {
        uint32_t ray_event_count;
        uint32_t ray_instance_intersections;
        uint32_t ray_any_hit_invocations;
        bool     hit;
    };

    /// @brief Column Id's for the fields in the ray list.
    enum RayInspectorRayListColumn
    {
        kRayInspectorRayIndexColumn,
        kRayInspectorRayListColumnTraversalLoopCount,
        kRayInspectorRayListColumnInstanceIntersections,
        kRayInspectorRayListColumnAnyHitInvocations,
        kRayInspectorRayListColumnHit,

        kRayInspectorRayListColumnCount,
    };

    /// @brief A class to handle the model data associated with ray list table.
    class RayInspectorRayListItemModel : public QAbstractItemModel
    {
    public:
        /// @brief Constructor.
        explicit RayInspectorRayListItemModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~RayInspectorRayListItemModel();

        /// @brief Set the number of rows in the table.
        ///
        /// @param [in] rows The number of rows required.
        void SetRowCount(int rows);

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

        /// @brief Add a ray to the table.
        ///
        /// @param [in] stats  The statistics to add to the table.
        void AddRow(const RayInspectorRayListStatistics& stats);

        // QAbstractItemModel overrides. See Qt documentation for parameter and return values
        virtual QVariant      data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;
        virtual Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual QVariant      headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
        virtual QModelIndex   index(int row, int column, const QModelIndex& parent) const Q_DECL_OVERRIDE;
        virtual QModelIndex   parent(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual int           rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
        virtual int           columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

    private:
        int                                        num_rows_;     ///< The number of rows in the table.
        int                                        num_columns_;  ///< The number of columns in the table.
        std::vector<RayInspectorRayListStatistics> cache_;        ///< Cached data from the backend.
    };
}  // namespace rra

#endif  // RRA_MODELS_RAY_INSPECTOR_RAY_LIST_ITEM_MODEL_H_
