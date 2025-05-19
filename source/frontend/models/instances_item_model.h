//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the instances item model. Shared between TLAS and
/// BLAS instances tables.
//=============================================================================

#ifndef RRA_MODELS_INSTANCES_ITEM_MODEL_H_
#define RRA_MODELS_INSTANCES_ITEM_MODEL_H_

#include <QAbstractItemModel>
#include <QTableView>

#include "public/rra_bvh.h"

namespace rra
{
    /// @brief Structure describing the statistics needed for the Instances list pane.
    struct InstancesTableStatistics
    {
        uint64_t instance_address;       ///< The base address of the instance.
        uint64_t instance_offset;        ///< The offset of the instance in the TLAS.
        uint32_t instance_index;         ///< The instance index.
        uint32_t unique_instance_index;  ///< The unique instance index.
        float    transform[12];          ///< The instance transform.
        uint32_t instance_mask;          ///< The 8 bit instance mask.
        bool     cull_disable_flag;      ///< Instance flag disabling triangle frontface/backface culling.
        bool     flip_facing_flag;       ///< Instance flag flipping the front face of each triangle.
        bool     force_opaque;           ///< Instance flag forcing instance to be opaque.
        bool     force_no_opaque;        ///< Instance flag forcing instance to not be opaque.
        uint32_t rebraid_sibling_count;  ///< The count of rebraid siblings for this instance.
    };

    /// @brief Column Id's for the fields in the acceleration structure list.
    enum InstancesColumn
    {
        kInstancesColumnIndex,
        kInstancesColumnInstanceIndex,
        kInstancesColumnInstanceAddress,
        kInstancesColumnInstanceOffset,
        kInstancesColumnInstanceMask,
        kInstancesColumnCullDisableFlag,
        kInstancesColumnFlipFacingFlag,
        kInstancesColumnForceOpaqueFlag,
        kInstancesColumnForceNoOpaqueFlag,
        kInstancesColumnRebraidSiblingCount,
        kInstancesColumnXPosition,
        kInstancesColumnYPosition,
        kInstancesColumnZPosition,
        kInstancesColumnM11,
        kInstancesColumnM12,
        kInstancesColumnM13,
        kInstancesColumnM21,
        kInstancesColumnM22,
        kInstancesColumnM23,
        kInstancesColumnM31,
        kInstancesColumnM32,
        kInstancesColumnM33,
        kInstancesColumnUniqueInstanceIndex,
        kInstancesColumnPadding,

        kInstancesColumnCount,
    };

    /// @brief A class to handle the model data associated with BLAS list table.
    class InstancesItemModel : public QAbstractItemModel
    {
    public:
        /// @brief Constructor.
        ///
        /// @param [in] parent The parent widget.
        explicit InstancesItemModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~InstancesItemModel();

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
        void Initialize(QTableView* acceleration_structure_table);

        /// @brief Add an acceleration structure to the table.
        ///
        /// @param [in] stats  The statistics to add to the table.
        void AddAccelerationStructure(const InstancesTableStatistics& stats);

        // QAbstractItemModel overrides. See Qt documentation for parameter and return values
        virtual QVariant      data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;
        virtual Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual QVariant      headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
        virtual QModelIndex   index(int row, int column, const QModelIndex& parent) const Q_DECL_OVERRIDE;
        virtual QModelIndex   parent(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual int           rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
        virtual int           columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

    private:
        int                                   num_rows_;     ///< The number of rows in the table.
        int                                   num_columns_;  ///< The number of columns in the table.
        std::vector<InstancesTableStatistics> cache_;        ///< Cached data from the backend.
    };
}  // namespace rra

#endif  // RRA_MODELS_INSTANCES_ITEM_MODEL_H_

