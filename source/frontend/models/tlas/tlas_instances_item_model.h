//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the TLAS instances item model.
//=============================================================================

#ifndef RRA_MODELS_TLAS_TLAS_INSTANCES_ITEM_MODEL_H_
#define RRA_MODELS_TLAS_TLAS_INSTANCES_ITEM_MODEL_H_

#include <QAbstractItemModel>

#include "qt_common/custom_widgets/scaled_table_view.h"

#include "public/rra_bvh.h"

namespace rra
{
    /// @brief Structure describing the statistics needed for the Instances list pane.
    struct TlasInstancesStatistics
    {
        uint64_t instance_address;  ///< The base address of the instance.
        uint64_t instance_offset;   ///< The offset of the instance in the TLAS.
        uint32_t instance_index;    ///< The instance index.
        float    transform[12];     ///< The instance transform.
        uint32_t instance_mask;     ///< The 8 bit instance mask.
    };

    /// @brief Column Id's for the fields in the acceleration structure list.
    enum TlasInstancesColumn
    {
        kTlasInstancesColumnInstanceAddress,
        kTlasInstancesColumnInstanceOffset,
        kTlasInstancesColumnInstanceMask,
        kTlasInstancesColumnXPosition,
        kTlasInstancesColumnYPosition,
        kTlasInstancesColumnZPosition,
        kTlasInstancesColumnM11,
        kTlasInstancesColumnM12,
        kTlasInstancesColumnM13,
        kTlasInstancesColumnM21,
        kTlasInstancesColumnM22,
        kTlasInstancesColumnM23,
        kTlasInstancesColumnM31,
        kTlasInstancesColumnM32,
        kTlasInstancesColumnM33,
        kTlasInstancesColumnInstanceIndex,

        kTlasInstancesColumnCount,
    };

    /// @brief A class to handle the model data associated with BLAS list table.
    class TlasInstancesItemModel : public QAbstractItemModel
    {
    public:
        /// @brief Constructor.
        ///
        /// @param [in] parent The parent widget.
        explicit TlasInstancesItemModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~TlasInstancesItemModel();

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
        void AddAccelerationStructure(const TlasInstancesStatistics& stats);

        // QAbstractItemModel overrides. See Qt documentation for parameter and return values
        virtual QVariant      data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;
        virtual Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual QVariant      headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
        virtual QModelIndex   index(int row, int column, const QModelIndex& parent) const Q_DECL_OVERRIDE;
        virtual QModelIndex   parent(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual int           rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
        virtual int           columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

    private:
        int                                  num_rows_;     ///< The number of rows in the table.
        int                                  num_columns_;  ///< The number of columns in the table.
        std::vector<TlasInstancesStatistics> cache_;        ///< Cached data from the backend.
    };
}  // namespace rra

#endif  // RRA_MODELS_TLAS_TLAS_INSTANCES_ITEM_MODEL_H_
