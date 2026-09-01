//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a proxy filter that processes the partitions
/// table.
//=============================================================================

#include "models/partitions_proxy_model.h"

#include <QTableView>

#include "public/rra_assert.h"

#include "models/partitions_item_model.h"

namespace rra
{
    PartitionsProxyModel::PartitionsProxyModel(QObject* parent)
        : TableProxyModel(parent)
    {
    }

    PartitionsProxyModel::~PartitionsProxyModel()
    {
    }

    PartitionsItemModel* PartitionsProxyModel::InitializeAccelerationStructureTableModels(QTableView* view, int num_rows, int num_columns)
    {
        PartitionsItemModel* model = new PartitionsItemModel();
        model->SetRowCount(num_rows);
        model->SetColumnCount(num_columns);

        setSourceModel(model);
        SetFilterKeyColumns({
            kPartitionsColumnPartitionIndex,
            kPartitionsColumnInstanceCount,
            kPartitionsColumnInternalNodeCount,
            kPartitionsColumnFatLeafCount,
            kPartitionsColumnTranslationX,
            kPartitionsColumnTranslationY,
            kPartitionsColumnTranslationZ,
            kPartitionsColumnIsGlobal,
        });

        view->setModel(this);

        return model;
    }

    QVariant PartitionsProxyModel::data(const QModelIndex& index, int role) const
    {
        if (index.column() == kPartitionsColumnIndex)
        {
            return index.row();
        }
        else
        {
            return TableProxyModel::data(index, role);
        }
    }

    bool PartitionsProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
    {
        if (FilterSearchString(source_row, source_parent) == false)
        {
            return false;
        }
        return true;
    }

    bool PartitionsProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        int left_column  = left.column();
        int right_column = right.column();
        if (left_column == right_column)
        {
            switch (left_column)
            {
            case kPartitionsColumnTranslationX:
            case kPartitionsColumnTranslationY:
            case kPartitionsColumnTranslationZ:
            {
                const float left_data  = left.data(Qt::UserRole).toFloat();
                const float right_data = right.data(Qt::UserRole).toFloat();
                return left_data < right_data;
            }

            case kPartitionsColumnPartitionIndex:
            case kPartitionsColumnInstanceCount:
            case kPartitionsColumnInternalNodeCount:
            case kPartitionsColumnFatLeafCount:
            {
                const uint32_t left_data  = left.data(Qt::UserRole).toUInt();
                const uint32_t right_data = right.data(Qt::UserRole).toUInt();
                return left_data < right_data;
            }

            case kPartitionsColumnIsGlobal:
            {
                const bool left_data  = left.data(Qt::UserRole).toBool();
                const bool right_data = right.data(Qt::UserRole).toBool();
                return left_data < right_data;
            }

            default:
                break;
            }
        }

        return QSortFilterProxyModel::lessThan(left, right);
    }
}  // namespace rra

