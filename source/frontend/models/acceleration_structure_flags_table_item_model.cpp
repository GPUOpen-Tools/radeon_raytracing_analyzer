//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the BLAS list item model.
///
/// Used for the Blas list shown in the TLAS tab.
///
//=============================================================================

#include "models/acceleration_structure_flags_table_item_model.h"

#include "vulkan/include/vulkan/vulkan_core.h"

#include "qt_common/utils/qt_util.h"

#include "constants.h"
#include "util/string_util.h"

namespace rra
{
    FlagsTableItemModel::FlagsTableItemModel(QObject* parent)
        : QAbstractItemModel(parent)
        , num_rows_(0)
        , num_columns_(0)
    {
    }

    FlagsTableItemModel::~FlagsTableItemModel()
    {
    }

    void FlagsTableItemModel::SetRowCount(int rows)
    {
        num_rows_ = rows;
        row_data_.resize(rows);
    }

    void FlagsTableItemModel::SetColumnCount(int columns)
    {
        num_columns_ = columns;
    }

    void FlagsTableItemModel::Initialize(ScaledTableView* table_view)
    {
        table_view->SetColumnWidthEms(kInstanceFlagsTableColumnCheckbox, 5);
    }

    void FlagsTableItemModel::SetRowChecked(int row, bool checked)
    {
        row_data_[row].checked = checked;
    }

    void FlagsTableItemModel::SetRowFlagName(int row, QString flag_name)
    {
        row_data_[row].flag_name = flag_name;
    }

    QVariant FlagsTableItemModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        int row = index.row();

        if (role == Qt::DisplayRole)
        {
            switch (index.column())
            {
            case kInstanceFlagsTableColumnFlag:
                return row_data_[row].flag_name;
            }
        }
        else if (role == Qt::CheckStateRole)
        {
            switch (index.column())
            {
            case kInstanceFlagsTableColumnCheckbox:
                return row_data_[row].checked ? Qt::Checked : Qt::Unchecked;
            }
        }
        else if (role == Qt::ForegroundRole)
        {
            switch (index.column())
            {
            case kInstanceFlagsTableColumnFlag:
                return QVariant(row_data_[row].checked ? QColor(Qt::black) : QColor(Qt::gray));
            }
        }
        return QVariant();
    }

    Qt::ItemFlags FlagsTableItemModel::flags(const QModelIndex& index) const
    {
        return QAbstractItemModel::flags(index);
    }

    QModelIndex FlagsTableItemModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        return createIndex(row, column);
    }

    QModelIndex FlagsTableItemModel::parent(const QModelIndex& index) const
    {
        Q_UNUSED(index);
        return QModelIndex();
    }

    int FlagsTableItemModel::rowCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_rows_;
    }

    int FlagsTableItemModel::columnCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_columns_;
    }
}  // namespace rra
