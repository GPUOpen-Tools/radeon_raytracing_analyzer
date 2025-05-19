//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the BLAS triangles item model.
//=============================================================================

#include "models/blas/blas_triangles_item_model.h"

#include <unordered_set>

#include "qt_common/utils/qt_util.h"

#include "public/rra_api_info.h"
#include "public/rra_assert.h"

#include "constants.h"
#include "settings/settings.h"
#include "util/rra_util.h"
#include "views/custom_widgets/index_header_view.h"

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

    void BlasTrianglesItemModel::Initialize(QTableView* acceleration_structure_table)
    {
        acceleration_structure_table->setHorizontalHeader(
            new IndexHeaderView({kBlasTrianglesColumnIndex, kBlasTrianglesColumnVertex0, kBlasTrianglesColumnVertex1, kBlasTrianglesColumnVertex2},
                                Qt::Horizontal,
                                acceleration_structure_table));
        rra_util::InitializeTableView(acceleration_structure_table);
        acceleration_structure_table->sortByColumn(kBlasTrianglesColumnGeometryIndex, Qt::AscendingOrder);

        acceleration_structure_table->setColumnWidth(kBlasTrianglesColumnGeometryIndex, 130);
        acceleration_structure_table->setColumnWidth(kBlasTrianglesColumnGeometryFlagOpaque, 100);
        acceleration_structure_table->setColumnWidth(kBlasTrianglesColumnGeometryFlagNoDuplicateAnyHit, 150);
        acceleration_structure_table->setColumnWidth(kBlasTrianglesColumnPrimitiveIndex, 130);
        acceleration_structure_table->setColumnWidth(kBlasTrianglesColumnNodeAddress, 180);
        acceleration_structure_table->setColumnWidth(kBlasTrianglesColumnNodeOffset, 150);
        acceleration_structure_table->setColumnWidth(kBlasTrianglesColumnTriangleCount, 150);
        acceleration_structure_table->setColumnWidth(kBlasTrianglesColumnActive, 100);
        acceleration_structure_table->setColumnWidth(kBlasTrianglesColumnTriangleSurfaceArea, 150);
        acceleration_structure_table->setColumnWidth(kBlasTrianglesColumnSAH, 100);
        acceleration_structure_table->setColumnWidth(kBlasTrianglesColumnVertex0, 210);
        acceleration_structure_table->setColumnWidth(kBlasTrianglesColumnVertex1, 210);
        acceleration_structure_table->setColumnWidth(kBlasTrianglesColumnVertex2, 210);
        acceleration_structure_table->setColumnWidth(kBlasTrianglesColumnPadding, 50);
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
            case kBlasTrianglesColumnGeometryIndex:
                return QString("%1").arg(cache.geometry_index);
            case kBlasTrianglesColumnPrimitiveIndex:
                return QString("%1").arg(cache.primitive_index);
            case kBlasTrianglesColumnNodeAddress:
                return QString("0x") + QString("%1").arg(cache.triangle_address, 0, 16);
            case kBlasTrianglesColumnNodeOffset:
                return QString("0x") + QString("%1").arg(cache.triangle_offset, 0, 16);
            case kBlasTrianglesColumnTriangleCount:
                return QString("%1").arg(cache.triangle_count);
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
            case kBlasTrianglesColumnPadding:
                return QString{""};

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

            default:
                break;
            }
        }
        else if (role == Qt::CheckStateRole)
        {
            switch (index.column())
            {
            case kBlasTrianglesColumnGeometryFlagOpaque:
                return cache.geometry_flag_opaque ? Qt::Checked : Qt::Unchecked;
            case kBlasTrianglesColumnGeometryFlagNoDuplicateAnyHit:
                return cache.geometry_flag_no_duplicate_any_hit ? Qt::Checked : Qt::Unchecked;
            case kBlasTrianglesColumnActive:
                return cache.is_inactive ? Qt::Unchecked : Qt::Checked;
            }
        }
        else if (role == Qt::UserRole)
        {
            switch (index.column())
            {
            case kBlasTrianglesColumnGeometryIndex:
                return QVariant::fromValue<quint32>(cache.geometry_index);
            case kBlasTrianglesColumnPrimitiveIndex:
                return QVariant::fromValue<quint32>(cache.primitive_index);
            case kBlasTrianglesColumnNodeAddress:
                return QVariant::fromValue<qulonglong>(cache.triangle_address);
            case kBlasTrianglesColumnNodeOffset:
                return QVariant::fromValue<qulonglong>(cache.triangle_offset);
            case kBlasTrianglesColumnTriangleCount:
                return QVariant::fromValue<qulonglong>(cache.triangle_count);
            case kBlasTrianglesColumnActive:
                return QVariant::fromValue<bool>(cache.is_inactive);
            case kBlasTrianglesColumnTriangleSurfaceArea:
                return QVariant::fromValue<float>(cache.triangle_surface_area);
            case kBlasTrianglesColumnSAH:
                return QVariant::fromValue<float>(cache.sah);
            case kBlasTrianglesColumnVertex0:
            case kBlasTrianglesColumnVertex1:
            case kBlasTrianglesColumnVertex2:
                return QVariant::fromValue<uint32_t>(cache.node_id);
            // Userdata in the padding column isn't going to be used so use it to return the node id.
            case kBlasTrianglesColumnPadding:
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
                case kBlasTrianglesColumnIndex:
                    return "Row Id";
                case kBlasTrianglesColumnGeometryIndex:
                    return "Geometry index";
                case kBlasTrianglesColumnGeometryFlagOpaque:
                    return "Opaque";
                case kBlasTrianglesColumnGeometryFlagNoDuplicateAnyHit:
                    return "No duplicate\nany hit invocation";
                case kBlasTrianglesColumnPrimitiveIndex:
                    return "Primitive index";
                case kBlasTrianglesColumnNodeAddress:
                    return "Node address";
                case kBlasTrianglesColumnTriangleCount:
                    return "Triangle count";
                case kBlasTrianglesColumnNodeOffset:
                    return "Node offset";
                case kBlasTrianglesColumnActive:
                    return "Active";
                case kBlasTrianglesColumnTriangleSurfaceArea:
                    return "Triangle\nsurface area";
                case kBlasTrianglesColumnSAH:
                    return "SAH";
                case kBlasTrianglesColumnVertex0:
                    return "Vertex0";
                case kBlasTrianglesColumnVertex1:
                    return "Vertex1";
                case kBlasTrianglesColumnVertex2:
                    return "Vertex2";
                case kBlasTrianglesColumnPadding:
                    return "";
                default:
                    break;
                }
            }
            else if (role == Qt::ToolTipRole)
            {
                switch (section)
                {
                case kBlasTrianglesColumnIndex:
                    return "The index of the row in the table";
                case kBlasTrianglesColumnGeometryIndex:
                    return "The index of the geometry that this triangle belongs to";
                case kBlasTrianglesColumnGeometryFlagOpaque:
                    if (RraApiInfoIsVulkan())
                    {
                        return "Presence of the VK_GEOMETRY_OPAQUE_BIT_KHR geometry flag";
                    }
                    else
                    {
                        return "Presence of the D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE geometry flag";
                    }
                case kBlasTrianglesColumnGeometryFlagNoDuplicateAnyHit:
                    if (RraApiInfoIsVulkan())
                    {
                        return "Presence of the VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR geometry flag";
                    }
                    else
                    {
                        return "Presence of the D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION geometry flag";
                    }
                case kBlasTrianglesColumnPrimitiveIndex:
                    return "The index of the triangle accessible in shaders";
                case kBlasTrianglesColumnNodeAddress:
                    return "The virtual address of this triangle in GPU memory";
                case kBlasTrianglesColumnNodeOffset:
                    return "The address of this triangle relative to the BLAS address";
                case kBlasTrianglesColumnActive:
                    if (RraApiInfoIsVulkan())
                    {
                        return "The active status of the triangle according to the definition in the Vulkan specification";
                    }
                    else
                    {
                        return "The active status of the triangle according to the definition in the DirectX 12 specification";
                    }
                case kBlasTrianglesColumnSAH:
                    return "The triangle surface area heuristic value";
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

