//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the partitions item model. Used by the TLAS
/// partitions table for partitioned TLASes (PTLAS).
//=============================================================================

#ifndef RRA_MODELS_PARTITIONS_ITEM_MODEL_H_
#define RRA_MODELS_PARTITIONS_ITEM_MODEL_H_

#include <QAbstractItemModel>
#include <QTableView>

#include "public/rra_bvh.h"

namespace rra
{
    /// @brief Structure describing the statistics needed for the Partitions list pane.
    struct PartitionsTableStatistics
    {
        uint32_t partition_index;      ///< The partition index (equals the partition count for the global partition).
        uint32_t instance_count;       ///< The number of instances assigned to the partition.
        uint32_t internal_node_count;  ///< The number of internal nodes in the partition.
        uint32_t fat_leaf_count;       ///< The number of fat leaf nodes in the partition.
        float    translation[3];       ///< The translation applied to the partition.
        bool     is_global;            ///< True if this is the global partition slot.
    };

    /// @brief Column Id's for the fields in the partitions list.
    enum PartitionsColumn
    {
        kPartitionsColumnIndex,
        kPartitionsColumnPartitionIndex,
        kPartitionsColumnInstanceCount,
        kPartitionsColumnInternalNodeCount,
        kPartitionsColumnFatLeafCount,
        kPartitionsColumnTranslationX,
        kPartitionsColumnTranslationY,
        kPartitionsColumnTranslationZ,
        kPartitionsColumnIsGlobal,
        kPartitionsColumnPadding,

        kPartitionsColumnCount,
    };

    /// @brief A class to handle the model data associated with the partitions table.
    class PartitionsItemModel : public QAbstractItemModel
    {
    public:
        /// @brief Constructor.
        ///
        /// @param [in] parent The parent widget.
        explicit PartitionsItemModel(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~PartitionsItemModel();

        /// @brief Set the number of rows in the table.
        ///
        /// @param [in] rows The number of rows required.
        void SetRowCount(int rows);

        /// @brief Set the number of columns in the table.
        ///
        /// @param [in] columns The number of columns required.
        void SetColumnCount(int columns);

        /// @brief Initialize the partitions table.
        ///
        /// @param [in] partitions_table  The table to initialize.
        void Initialize(QTableView* partitions_table);

        /// @brief Add a partition to the table.
        ///
        /// @param [in] stats  The statistics to add to the table.
        void AddPartition(const PartitionsTableStatistics& stats);

        // QAbstractItemModel overrides. See Qt documentation for parameter and return values
        virtual QVariant      data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;
        virtual Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual QVariant      headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
        virtual QModelIndex   index(int row, int column, const QModelIndex& parent) const Q_DECL_OVERRIDE;
        virtual QModelIndex   parent(const QModelIndex& index) const Q_DECL_OVERRIDE;
        virtual int           rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
        virtual int           columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

    private:
        int                                    num_rows_;     ///< The number of rows in the table.
        int                                    num_columns_;  ///< The number of columns in the table.
        std::vector<PartitionsTableStatistics> cache_;        ///< Cached data from the backend.
    };
}  // namespace rra

#endif  // RRA_MODELS_PARTITIONS_ITEM_MODEL_H_

