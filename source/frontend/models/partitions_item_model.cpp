//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the partitions item model. Used by the TLAS
/// partitions table for partitioned TLASes (PTLAS).
//=============================================================================

#include "models/partitions_item_model.h"

#include "qt_common/utils/qt_util.h"

#include "public/rra_assert.h"

#include "constants.h"
#include "settings/settings.h"
#include "util/rra_util.h"
#include "views/custom_widgets/index_header_view.h"

namespace rra
{
    PartitionsItemModel::PartitionsItemModel(QObject* parent)
        : QAbstractItemModel(parent)
        , num_rows_(0)
        , num_columns_(0)
    {
    }

    PartitionsItemModel::~PartitionsItemModel()
    {
    }

    void PartitionsItemModel::SetRowCount(int rows)
    {
        num_rows_ = rows;
        cache_.clear();
    }

    void PartitionsItemModel::SetColumnCount(int columns)
    {
        num_columns_ = columns;
    }

    void PartitionsItemModel::Initialize(QTableView* partitions_table)
    {
        partitions_table->setHorizontalHeader(new IndexHeaderView(kPartitionsColumnIndex, Qt::Horizontal, partitions_table));
        rra_util::InitializeTableView(partitions_table);
        partitions_table->sortByColumn(kPartitionsColumnPartitionIndex, Qt::AscendingOrder);

        partitions_table->setColumnWidth(kPartitionsColumnPartitionIndex, 120);
        partitions_table->setColumnWidth(kPartitionsColumnInstanceCount, 120);
        partitions_table->setColumnWidth(kPartitionsColumnInternalNodeCount, 140);
        partitions_table->setColumnWidth(kPartitionsColumnFatLeafCount, 120);
        partitions_table->setColumnWidth(kPartitionsColumnTranslationX, 100);
        partitions_table->setColumnWidth(kPartitionsColumnTranslationY, 100);
        partitions_table->setColumnWidth(kPartitionsColumnTranslationZ, 100);
        partitions_table->setColumnWidth(kPartitionsColumnIsGlobal, 100);
    }

    void PartitionsItemModel::AddPartition(const PartitionsTableStatistics& stats)
    {
        cache_.push_back(stats);
    }

    QVariant PartitionsItemModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        const int row = index.row();
        if (row < 0 || row >= static_cast<int>(cache_.size()))
        {
            return QVariant();
        }

        const PartitionsTableStatistics& cache = cache_[row];
        if (role == Qt::DisplayRole)
        {
            int decimal_precision = rra::Settings::Get().GetDecimalPrecision();
            switch (index.column())
            {
            case kPartitionsColumnPartitionIndex:
                return QString::number(cache.partition_index);
            case kPartitionsColumnInstanceCount:
                return QString::number(cache.instance_count);
            case kPartitionsColumnInternalNodeCount:
                return QString::number(cache.internal_node_count);
            case kPartitionsColumnFatLeafCount:
                return QString::number(cache.fat_leaf_count);
            case kPartitionsColumnTranslationX:
                return QString::number(cache.translation[0], kQtFloatFormat, decimal_precision);
            case kPartitionsColumnTranslationY:
                return QString::number(cache.translation[1], kQtFloatFormat, decimal_precision);
            case kPartitionsColumnTranslationZ:
                return QString::number(cache.translation[2], kQtFloatFormat, decimal_precision);
            case kPartitionsColumnIsGlobal:
                return cache.is_global ? QString("Yes") : QString("No");
            case kPartitionsColumnPadding:
                return QString{""};
            default:
                break;
            }
        }
        else if (role == Qt::ToolTipRole)
        {
            switch (index.column())
            {
            case kPartitionsColumnTranslationX:
                return QString::number(cache.translation[0], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kPartitionsColumnTranslationY:
                return QString::number(cache.translation[1], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kPartitionsColumnTranslationZ:
                return QString::number(cache.translation[2], kQtFloatFormat, kQtTooltipFloatPrecision);
            default:
                break;
            }
        }
        else if (role == Qt::UserRole)
        {
            switch (index.column())
            {
            case kPartitionsColumnPartitionIndex:
                return QVariant::fromValue<quint32>(cache.partition_index);
            case kPartitionsColumnInstanceCount:
                return QVariant::fromValue<quint32>(cache.instance_count);
            case kPartitionsColumnInternalNodeCount:
                return QVariant::fromValue<quint32>(cache.internal_node_count);
            case kPartitionsColumnFatLeafCount:
                return QVariant::fromValue<quint32>(cache.fat_leaf_count);
            case kPartitionsColumnTranslationX:
                return QVariant::fromValue<float>(cache.translation[0]);
            case kPartitionsColumnTranslationY:
                return QVariant::fromValue<float>(cache.translation[1]);
            case kPartitionsColumnTranslationZ:
                return QVariant::fromValue<float>(cache.translation[2]);
            case kPartitionsColumnIsGlobal:
                return QVariant::fromValue<bool>(cache.is_global);
            case kPartitionsColumnPadding:
                return QVariant();
            default:
                break;
            }
        }
        else if (role == Qt::CheckStateRole)
        {
            switch (index.column())
            {
            case kPartitionsColumnIsGlobal:
                return cache.is_global ? Qt::Checked : Qt::Unchecked;
            default:
                break;
            }
        }
        return QVariant();
    }

    Qt::ItemFlags PartitionsItemModel::flags(const QModelIndex& index) const
    {
        return QAbstractItemModel::flags(index);
    }

    QVariant PartitionsItemModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal)
        {
            if (role == Qt::DisplayRole)
            {
                switch (section)
                {
                case kPartitionsColumnIndex:
                    return "Row Id";
                case kPartitionsColumnPartitionIndex:
                    return "Partition index";
                case kPartitionsColumnInstanceCount:
                    return "Instance count";
                case kPartitionsColumnInternalNodeCount:
                    return "Internal node count";
                case kPartitionsColumnFatLeafCount:
                    return "Fat leaf count";
                case kPartitionsColumnTranslationX:
                    return "Translation X";
                case kPartitionsColumnTranslationY:
                    return "Translation Y";
                case kPartitionsColumnTranslationZ:
                    return "Translation Z";
                case kPartitionsColumnIsGlobal:
                    return "Global";
                case kPartitionsColumnPadding:
                    return "";
                default:
                    break;
                }
            }
            else if (role == Qt::ToolTipRole)
            {
                switch (section)
                {
                case kPartitionsColumnIndex:
                    return "The index of the row in the table";
                case kPartitionsColumnPartitionIndex:
                    return "The index of this partition";
                case kPartitionsColumnInstanceCount:
                    return "The number of instances assigned to this partition";
                case kPartitionsColumnInternalNodeCount:
                    return "The number of internal nodes in this partition";
                case kPartitionsColumnFatLeafCount:
                    return "The number of fat leaf nodes in this partition";
                case kPartitionsColumnTranslationX:
                    return "The X component of the partition translation";
                case kPartitionsColumnTranslationY:
                    return "The Y component of the partition translation";
                case kPartitionsColumnTranslationZ:
                    return "The Z component of the partition translation";
                case kPartitionsColumnIsGlobal:
                    return "Whether this is the global partition holding unassigned instances";
                default:
                    break;
                }
            }
            else if (role == Qt::TextAlignmentRole)
            {
                return Qt::AlignRight;
            }
        }

        return QAbstractItemModel::headerData(section, orientation, role);
    }

    QModelIndex PartitionsItemModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        return createIndex(row, column);
    }

    QModelIndex PartitionsItemModel::parent(const QModelIndex& index) const
    {
        Q_UNUSED(index);
        return QModelIndex();
    }

    int PartitionsItemModel::rowCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_rows_;
    }

    int PartitionsItemModel::columnCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_columns_;
    }
}  // namespace rra

