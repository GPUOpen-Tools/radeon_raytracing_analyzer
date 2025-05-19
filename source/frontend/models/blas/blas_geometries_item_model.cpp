//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the BLAS geometry item model.
//=============================================================================

#include "models/blas/blas_geometries_item_model.h"

#include "qt_common/utils/qt_util.h"

#include "public/rra_api_info.h"
#include "public/rra_assert.h"

#include "constants.h"
#include "settings/settings.h"
#include "util/rra_util.h"
#include "views/custom_widgets/index_header_view.h"

namespace rra
{
    BlasGeometriesItemModel::BlasGeometriesItemModel(QObject* parent)
        : QAbstractItemModel(parent)
        , num_rows_(0)
        , num_columns_(0)
    {
    }

    BlasGeometriesItemModel::~BlasGeometriesItemModel()
    {
    }

    void BlasGeometriesItemModel::SetRowCount(int rows)
    {
        num_rows_ = rows;
        cache_.clear();
    }

    void BlasGeometriesItemModel::SetColumnCount(int columns)
    {
        num_columns_ = columns;
    }

    void BlasGeometriesItemModel::Initialize(QTableView* acceleration_structure_table)
    {
        acceleration_structure_table->setHorizontalHeader(new IndexHeaderView(kBlasGeometriesColumnIndex, Qt::Horizontal, acceleration_structure_table));
        rra_util::InitializeTableView(acceleration_structure_table);
        acceleration_structure_table->sortByColumn(kBlasGeometriesColumnGeometryIndex, Qt::AscendingOrder);

        acceleration_structure_table->setColumnWidth(kBlasGeometriesColumnGeometryIndex, 130);
        acceleration_structure_table->setColumnWidth(kBlasGeometriesColumnGeometryFlagOpaque, 100);
        acceleration_structure_table->setColumnWidth(kBlasGeometriesColumnGeometryFlagNoDuplicateAnyHit, 200);
        acceleration_structure_table->setColumnWidth(kBlasGeometriesColumnPrimitiveCount, 130);
    }

    void BlasGeometriesItemModel::AddGeometryStructure(const BlasGeometriesStatistics& stats)
    {
        cache_.push_back(stats);
    }

    QVariant BlasGeometriesItemModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        int row = index.row();

        const BlasGeometriesStatistics& cache = cache_[row];

        if (role == Qt::DisplayRole)
        {
            switch (index.column())
            {
            case kBlasGeometriesColumnGeometryIndex:
                return QString("%1").arg(cache.geometry_index);
            case kBlasGeometriesColumnPrimitiveCount:
                return QString("%1").arg(cache.primitive_count);
            case kBlasGeometriesColumnPadding:
                return QString{""};
            default:
                break;
            }
        }
        else if (role == Qt::ToolTipRole)
        {
        }
        else if (role == Qt::CheckStateRole)
        {
            switch (index.column())
            {
            case kBlasGeometriesColumnGeometryFlagOpaque:
                return cache.geometry_flag_opaque ? Qt::Checked : Qt::Unchecked;
            case kBlasGeometriesColumnGeometryFlagNoDuplicateAnyHit:
                return cache.geometry_flag_no_duplicate_any_hit ? Qt::Checked : Qt::Unchecked;
            }
        }
        else if (role == Qt::UserRole)
        {
            switch (index.column())
            {
            case kBlasGeometriesColumnGeometryIndex:
                return QVariant::fromValue<quint32>(cache.geometry_index);
            case kBlasGeometriesColumnPrimitiveCount:
                return QVariant::fromValue<quint32>(cache.primitive_count);
            case kBlasGeometriesColumnPadding:
                return QVariant();
            default:
                break;
            }
        }
        return QVariant();
    }

    Qt::ItemFlags BlasGeometriesItemModel::flags(const QModelIndex& index) const
    {
        return QAbstractItemModel::flags(index);
    }

    QVariant BlasGeometriesItemModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal)
        {
            if (role == Qt::DisplayRole)
            {
                switch (section)
                {
                case kBlasGeometriesColumnIndex:
                    return "Row Id";
                case kBlasGeometriesColumnGeometryIndex:
                    return "Geometry index";
                case kBlasGeometriesColumnGeometryFlagOpaque:
                    return "Opaque";
                case kBlasGeometriesColumnGeometryFlagNoDuplicateAnyHit:
                    return "No duplicate\nany hit invocation";
                case kBlasGeometriesColumnPrimitiveCount:
                    return "Primitive count";
                case kBlasGeometriesColumnPadding:
                    return "";
                default:
                    break;
                }
            }
            else if (role == Qt::ToolTipRole)
            {
                switch (section)
                {
                case kBlasGeometriesColumnIndex:
                    return "The index of the row in the table";
                case kBlasGeometriesColumnGeometryIndex:
                    return "The API index of this geometry";
                case kBlasGeometriesColumnGeometryFlagOpaque:
                    if (RraApiInfoIsVulkan())
                    {
                        return "Presence of the VK_GEOMETRY_OPAQUE_BIT_KHR geometry flag";
                    }
                    else
                    {
                        return "Presence of the D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE geometry flag";
                    }
                case kBlasGeometriesColumnGeometryFlagNoDuplicateAnyHit:
                    if (RraApiInfoIsVulkan())
                    {
                        return "Presence of the VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR geometry flag";
                    }
                    else
                    {
                        return "Presence of the D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION geometry flag";
                    }
                case kBlasGeometriesColumnPrimitiveCount:
                    return "The number of primitives in this geometry";
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

    QModelIndex BlasGeometriesItemModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        return createIndex(row, column);
    }

    QModelIndex BlasGeometriesItemModel::parent(const QModelIndex& index) const
    {
        Q_UNUSED(index);
        return QModelIndex();
    }

    int BlasGeometriesItemModel::rowCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_rows_;
    }

    int BlasGeometriesItemModel::columnCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_columns_;
    }
}  // namespace rra

