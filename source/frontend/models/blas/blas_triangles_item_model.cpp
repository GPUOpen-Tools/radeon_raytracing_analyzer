//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the BLAS triangles item model.
//=============================================================================

#include "models/blas/blas_triangles_item_model.h"

#include "qt_common/utils/qt_util.h"

#include "public/rra_assert.h"

#include "constants.h"
#include "settings/settings.h"

namespace rra
{
    BlasTrianglesItemModel::BlasTrianglesItemModel(QObject* parent)
        : QAbstractItemModel(parent)
        , num_rows_(0)
        , num_columns_(0)
    {
    }

    BlasTrianglesItemModel::~BlasTrianglesItemModel()
    {
    }

    void BlasTrianglesItemModel::SetRowCount(int rows)
    {
        num_rows_ = rows;
        cache_.clear();
    }

    void BlasTrianglesItemModel::SetColumnCount(int columns)
    {
        num_columns_ = columns;
    }

    void BlasTrianglesItemModel::Initialize(ScaledTableView* acceleration_structure_table)
    {
        acceleration_structure_table->horizontalHeader()->setSectionsClickable(true);

        // Set default column widths wide enough to show table contents.
        acceleration_structure_table->SetColumnPadding(0);

        acceleration_structure_table->SetColumnWidthEms(kBlasTrianglesColumnTriangleAddress, 18);
        acceleration_structure_table->SetColumnWidthEms(kBlasTrianglesColumnTriangleOffset, 15);
        acceleration_structure_table->SetColumnWidthEms(kBlasTrianglesColumnTriangleCount, 13);
        acceleration_structure_table->SetColumnWidthEms(kBlasTrianglesColumnGeometryIndex, 13);
        acceleration_structure_table->SetColumnWidthEms(kBlasTrianglesColumnIsInactive, 10);
        acceleration_structure_table->SetColumnWidthEms(kBlasTrianglesColumnTriangleSurfaceArea, 15);
        acceleration_structure_table->SetColumnWidthEms(kBlasTrianglesColumnSAH, 10);
        acceleration_structure_table->SetColumnWidthEms(kBlasTrianglesColumnVertex0, 20);
        acceleration_structure_table->SetColumnWidthEms(kBlasTrianglesColumnVertex1, 20);
        acceleration_structure_table->SetColumnWidthEms(kBlasTrianglesColumnVertex2, 20);
        acceleration_structure_table->SetColumnWidthEms(kBlasTrianglesColumnVertex3, 20);

        // Allow users to resize columns if desired.
        acceleration_structure_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Interactive);
    }

    void BlasTrianglesItemModel::AddTriangleStructure(const BlasTrianglesStatistics& stats)
    {
        cache_.push_back(stats);
    }

    QVariant BlasTrianglesItemModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        int row = index.row();

        const BlasTrianglesStatistics& cache = cache_[row];

        if (role == Qt::DisplayRole)
        {
            int decimal_precision = rra::Settings::Get().GetDecimalPrecision();
            switch (index.column())
            {
            case kBlasTrianglesColumnTriangleAddress:
                return QString("0x") + QString("%1").arg(cache.triangle_address, 0, 16);
            case kBlasTrianglesColumnTriangleOffset:
                return QString("0x") + QString("%1").arg(cache.triangle_offset, 0, 16);
            case kBlasTrianglesColumnTriangleCount:
                return QString("%1").arg(cache.triangle_count);
            case kBlasTrianglesColumnGeometryIndex:
                return QString("%1").arg(cache.geometry_index);
            case kBlasTrianglesColumnIsInactive:
                return (cache.is_inactive == true ? QString("Yes") : QString("No"));
            case kBlasTrianglesColumnTriangleSurfaceArea:
                return QString::number(cache.triangle_surface_area, kQtFloatFormat, decimal_precision);
            case kBlasTrianglesColumnSAH:
                return QString::number(cache.sah, kQtFloatFormat, decimal_precision);
            case kBlasTrianglesColumnVertex0:
                return QString::number(cache.vertex_0.x, kQtFloatFormat, decimal_precision) + "," +
                       QString::number(cache.vertex_0.y, kQtFloatFormat, decimal_precision) + "," +
                       QString::number(cache.vertex_0.z, kQtFloatFormat, decimal_precision);
            case kBlasTrianglesColumnVertex1:
                return QString::number(cache.vertex_1.x, kQtFloatFormat, decimal_precision) + "," +
                       QString::number(cache.vertex_1.y, kQtFloatFormat, decimal_precision) + "," +
                       QString::number(cache.vertex_1.z, kQtFloatFormat, decimal_precision);
            case kBlasTrianglesColumnVertex2:
                return QString::number(cache.vertex_2.x, kQtFloatFormat, decimal_precision) + "," +
                       QString::number(cache.vertex_2.y, kQtFloatFormat, decimal_precision) + "," +
                       QString::number(cache.vertex_2.z, kQtFloatFormat, decimal_precision);
            case kBlasTrianglesColumnVertex3:
                if (cache.triangle_count > 1)
                {
                    return QString::number(cache.vertex_3.x, kQtFloatFormat, decimal_precision) + "," +
                           QString::number(cache.vertex_3.y, kQtFloatFormat, decimal_precision) + "," +
                           QString::number(cache.vertex_3.z, kQtFloatFormat, decimal_precision);
                }
                else
                {
                    return "-";
                }

            default:
                break;
            }
        }
        else if (role == Qt::ToolTipRole)
        {
            switch (index.column())
            {
            case kBlasTrianglesColumnTriangleSurfaceArea:
                return QString::number(cache.triangle_surface_area, kQtFloatFormat, kQtTooltipFloatPrecision);
            case kBlasTrianglesColumnSAH:
                return QString::number(cache.sah, kQtFloatFormat, kQtTooltipFloatPrecision);
            case kBlasTrianglesColumnVertex0:
                return QString::number(cache.vertex_0.x, kQtFloatFormat, kQtTooltipFloatPrecision) + ", " +
                       QString::number(cache.vertex_0.y, kQtFloatFormat, kQtTooltipFloatPrecision) + ", " +
                       QString::number(cache.vertex_0.z, kQtFloatFormat, kQtTooltipFloatPrecision);
            case kBlasTrianglesColumnVertex1:
                return QString::number(cache.vertex_1.x, kQtFloatFormat, kQtTooltipFloatPrecision) + ", " +
                       QString::number(cache.vertex_1.y, kQtFloatFormat, kQtTooltipFloatPrecision) + ", " +
                       QString::number(cache.vertex_1.z, kQtFloatFormat, kQtTooltipFloatPrecision);
            case kBlasTrianglesColumnVertex2:
                return QString::number(cache.vertex_2.x, kQtFloatFormat, kQtTooltipFloatPrecision) + ", " +
                       QString::number(cache.vertex_2.y, kQtFloatFormat, kQtTooltipFloatPrecision) + ", " +
                       QString::number(cache.vertex_2.z, kQtFloatFormat, kQtTooltipFloatPrecision);
            case kBlasTrianglesColumnVertex3:
                if (cache.triangle_count > 1)
                {
                    return QString::number(cache.vertex_3.x, kQtFloatFormat, kQtTooltipFloatPrecision) + ", " +
                           QString::number(cache.vertex_3.y, kQtFloatFormat, kQtTooltipFloatPrecision) + ", " +
                           QString::number(cache.vertex_3.z, kQtFloatFormat, kQtTooltipFloatPrecision);
                }
                else
                {
                    break;
                }

            default:
                break;
            }
        }
        else if (role == Qt::UserRole)
        {
            switch (index.column())
            {
            case kBlasTrianglesColumnTriangleAddress:
                return QVariant::fromValue<qulonglong>(cache.triangle_address);
            case kBlasTrianglesColumnTriangleOffset:
                return QVariant::fromValue<qulonglong>(cache.triangle_offset);
            case kBlasTrianglesColumnTriangleCount:
                return QVariant::fromValue<quint32>(cache.triangle_count);
            case kBlasTrianglesColumnGeometryIndex:
                return QVariant::fromValue<quint32>(cache.geometry_index);
            case kBlasTrianglesColumnIsInactive:
                return QVariant::fromValue<bool>(cache.is_inactive);
            case kBlasTrianglesColumnTriangleSurfaceArea:
                return QVariant::fromValue<float>(cache.triangle_surface_area);
            case kBlasTrianglesColumnSAH:
                return QVariant::fromValue<float>(cache.sah);
            // Userdata in the vertex columns isn't going to be used so use them to return the node id.
            case kBlasTrianglesColumnVertex0:
            case kBlasTrianglesColumnVertex1:
            case kBlasTrianglesColumnVertex2:
            case kBlasTrianglesColumnVertex3:
                return QVariant::fromValue<uint32_t>(cache.node_id);

            default:
                break;
            }
        }
        return QVariant();
    }

    Qt::ItemFlags BlasTrianglesItemModel::flags(const QModelIndex& index) const
    {
        return QAbstractItemModel::flags(index);
    }

    QVariant BlasTrianglesItemModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal)
        {
            if (role == Qt::DisplayRole)
            {
                switch (section)
                {
                case kBlasTrianglesColumnTriangleAddress:
                    return "Triangle address";
                case kBlasTrianglesColumnTriangleOffset:
                    return "Triangle offset";
                case kBlasTrianglesColumnTriangleCount:
                    return "Triangle count";
                case kBlasTrianglesColumnGeometryIndex:
                    return "Geometry index";
                case kBlasTrianglesColumnIsInactive:
                    return "Is inactive";
                case kBlasTrianglesColumnTriangleSurfaceArea:
                    return "Triangle surface area";
                case kBlasTrianglesColumnSAH:
                    return "SAH";
                case kBlasTrianglesColumnVertex0:
                    return "Vertex0";
                case kBlasTrianglesColumnVertex1:
                    return "Vertex1";
                case kBlasTrianglesColumnVertex2:
                    return "Vertex2";
                case kBlasTrianglesColumnVertex3:
                    return "Vertex3";
                default:
                    break;
                }
            }
            else if (role == Qt::ToolTipRole)
            {
                switch (section)
                {
                case kBlasTrianglesColumnTriangleAddress:
                    return "The address of this triangle in the BLAS";
                case kBlasTrianglesColumnTriangleOffset:
                    return "The offset of this triangle in the BLAS";
                case kBlasTrianglesColumnTriangleCount:
                    return "The number of triangles in this triangle node";
                case kBlasTrianglesColumnGeometryIndex:
                    return "The geometry index";
                case kBlasTrianglesColumnIsInactive:
                    return "Whether this triangle is inactive or not";
                case kBlasTrianglesColumnTriangleSurfaceArea:
                    return "The surface area of the triangle";
                case kBlasTrianglesColumnSAH:
                    return "The triangle surface area heuristic value";
                default:
                    break;
                }
            }
        }

        return QAbstractItemModel::headerData(section, orientation, role);
    }

    QModelIndex BlasTrianglesItemModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        return createIndex(row, column);
    }

    QModelIndex BlasTrianglesItemModel::parent(const QModelIndex& index) const
    {
        Q_UNUSED(index);
        return QModelIndex();
    }

    int BlasTrianglesItemModel::rowCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_rows_;
    }

    int BlasTrianglesItemModel::columnCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_columns_;
    }
}  // namespace rra
