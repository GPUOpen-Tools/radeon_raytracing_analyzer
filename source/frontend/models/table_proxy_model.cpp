//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a proxy filter that processes multiple columns.
//=============================================================================

#include "models/table_proxy_model.h"

namespace rra
{
    TableProxyModel::TableProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
        , min_size_(0)
        , max_size_(UINT64_MAX)
    {
    }

    TableProxyModel::~TableProxyModel()
    {
    }

    void TableProxyModel::SetFilterKeyColumns(const QList<qint32>& columns)
    {
        column_filters_.clear();

        search_filter_ = QString();
        for (int i = 0; i < columns.size(); i++)
        {
            column_filters_.insert(columns[i]);
        }
    }

    void TableProxyModel::SetSearchFilter(const QString& filter)
    {
        search_filter_ = filter;
    }

    void TableProxyModel::SetSizeFilter(uint64_t min, uint64_t max)
    {
        min_size_ = min;
        max_size_ = max;
    }

    uint64_t TableProxyModel::GetIndexValue(const QModelIndex& index) const
    {
        const QString data = sourceModel()->data(index).toString();
        bool          ok   = false;
        return data.toULongLong(&ok, 0);
    }

    qulonglong TableProxyModel::GetData(int row, int column)
    {
        qulonglong out = 0;

        const QModelIndex model_index = index(row, column, QModelIndex());

        if (model_index.isValid() == true)
        {
            out = data(model_index, Qt::UserRole).toULongLong();
        }
        return out;
    }

    QModelIndex TableProxyModel::FindModelIndex(qulonglong lookup, int column) const
    {
        QModelIndex out_model_index;

        const int num_rows = rowCount();

        for (int row = 0; row < num_rows; row++)
        {
            const QModelIndex model_index = index(row, column, QModelIndex());

            if (model_index.isValid() == true)
            {
                qulonglong value = data(model_index, Qt::UserRole).toULongLong();
                if (value == lookup)
                {
                    out_model_index = model_index;
                    break;
                }
            }
        }

        return out_model_index;
    }

    bool TableProxyModel::FilterSearchString(int row, const QModelIndex& source_parent) const
    {
        if (column_filters_.empty() == false && search_filter_.isEmpty() == false)
        {
            for (auto iter = column_filters_.begin(); iter != column_filters_.end(); ++iter)
            {
                const QModelIndex& index = sourceModel()->index(row, *iter, source_parent);

                QString indexData = index.data().toString();

                if (indexData.contains(search_filter_, Qt::CaseInsensitive) == true)
                {
                    // Found a match so don't need to check for further matches.
                    return true;
                }
            }
            return false;
        }
        return true;
    }
}  // namespace rra
