//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the BLAS list item model.
///
/// Used for the Blas list shown in the TLAS tab.
///
//=============================================================================

#include "models/tlas/blas_list_item_model.h"

#include <math.h>

#include "qt_common/utils/qt_util.h"

#include "public/rra_api_info.h"
#include "public/rra_rtip_info.h"

#include "constants.h"
#include "settings/settings.h"
#include "util/rra_util.h"
#include "util/string_util.h"
#include "views/custom_widgets/index_header_view.h"

namespace rra
{
    BlasListItemModel::BlasListItemModel(QObject* parent)
        : QAbstractItemModel(parent)
        , num_rows_(0)
        , num_columns_(0)
    {
    }

    BlasListItemModel::~BlasListItemModel()
    {
    }

    void BlasListItemModel::SetRowCount(int rows)
    {
        num_rows_ = rows;
        cache_.clear();
    }

    void BlasListItemModel::SetColumnCount(int columns)
    {
        num_columns_ = columns;
    }

    void BlasListItemModel::Initialize(QTableView* acceleration_structure_table)
    {
        acceleration_structure_table->setHorizontalHeader(new IndexHeaderView(kBlasListColumnIndex, Qt::Horizontal, acceleration_structure_table));
        rra_util::InitializeTableView(acceleration_structure_table);
        acceleration_structure_table->sortByColumn(kBlasListColumnAddress, Qt::AscendingOrder);

        acceleration_structure_table->setColumnWidth(kBlasListColumnAddress, 120);
        acceleration_structure_table->setColumnWidth(kBlasListColumnAllowUpdate, 100);
        acceleration_structure_table->setColumnWidth(kBlasListColumnAllowCompaction, 120);
        acceleration_structure_table->setColumnWidth(kBlasListColumnLowMemory, 100);
        acceleration_structure_table->setColumnWidth(kBlasListColumnBuildType, 80);
        acceleration_structure_table->setColumnWidth(kBlasListColumnInstanceCount, 80);
        acceleration_structure_table->setColumnWidth(kBlasListColumnNodeCount, 60);
        acceleration_structure_table->setColumnWidth(kBlasListColumnBoxCount, 60);
        acceleration_structure_table->setColumnWidth(kBlasListColumnBox32Count, 90);
        acceleration_structure_table->setColumnWidth(kBlasListColumnBox16Count, 90);
        acceleration_structure_table->setColumnWidth(kBlasListColumnTriangleNodeCount, 100);
        acceleration_structure_table->setColumnWidth(kBlasListColumnProceduralNodeCount, 120);
        acceleration_structure_table->setColumnWidth(kBlasListColumnMemoryUsage, 100);
        acceleration_structure_table->setColumnWidth(kBlasListColumnRootSAH, 90);
        acceleration_structure_table->setColumnWidth(kBlasListColumnMinSAH, 90);
        acceleration_structure_table->setColumnWidth(kBlasListColumnMeanSAH, 90);
        acceleration_structure_table->setColumnWidth(kBlasListColumnMaxDepth, 90);
        acceleration_structure_table->setColumnWidth(kBlasListColumnAvgDepth, 90);
        acceleration_structure_table->setColumnWidth(kBlasListColumnBlasIndex, 70);
    }

    void BlasListItemModel::AddAccelerationStructure(const BlasListStatistics& stats)
    {
        cache_.push_back(stats);
    }

    /// @brief Get the color of the build type text.
    ///
    /// @param [in] build_flags The flags used to build the BLAS.
    ///
    /// @return The color the text should be displayed as.
    static QColor GetBuildTypeColor(VkBuildAccelerationStructureFlagBitsKHR build_flags)
    {
        bool fast_trace = build_flags & VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        bool fast_build = build_flags & VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;

        if (fast_trace && fast_build)
        {
            return QColor(Qt::red);
        }

        return QColor();
    }

    /// @brief Get the color of the nan field.
    ///
    /// @param [in] value The floating value to check for.
    ///
    /// @return The color the text should be displayed as.
    static QColor GetNanTypeColor(float value)
    {
        if (isnan(value))
        {
            return QColor(Qt::red);
        }

        return QColor();
    }

    QVariant BlasListItemModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        int row = index.row();

        const BlasListStatistics& cache = cache_[row];

        if (role == Qt::DisplayRole)
        {
            int decimal_precision = rra::Settings::Get().GetDecimalPrecision();
            switch (index.column())
            {
            case kBlasListColumnAddress:
                return QString("0x") + QString("%1").arg(cache.address, 0, 16);
            case kBlasListColumnBuildType:
                return rra::string_util::GetBuildTypeString(cache.build_flags);
            case kBlasListColumnInstanceCount:
                return QString::number(cache.instance_count);
            case kBlasListColumnNodeCount:
                return QString::number(cache.node_count);
            case kBlasListColumnBoxCount:
                return QString::number(cache.box_count);
            case kBlasListColumnBox32Count:
                return QString::number(cache.box32_count);
            case kBlasListColumnBox16Count:
                return QString::number(cache.box16_count);
            case kBlasListColumnTriangleNodeCount:
                return QString::number(cache.triangle_node_count);
            case kBlasListColumnProceduralNodeCount:
                return QString::number(cache.procedural_node_count);
            case kBlasListColumnMemoryUsage:
                return rra::string_util::LocalizedValueMemory(static_cast<double>(cache.memory_usage), false, true);
            case kBlasListColumnRootSAH:
                return QString::number(cache.root_sah, kQtFloatFormat, decimal_precision);
            case kBlasListColumnMinSAH:
                return QString::number(cache.max_sah, kQtFloatFormat, decimal_precision);
            case kBlasListColumnMeanSAH:
                return QString::number(cache.mean_sah, kQtFloatFormat, decimal_precision);
            case kBlasListColumnMaxDepth:
                return QString::number(cache.max_depth);
            case kBlasListColumnAvgDepth:
                return QString::number(cache.avg_depth);
            case kBlasListColumnBlasIndex:
                return QString::number(cache.blas_index);
            case kBlasListColumnPadding:
                return QString{""};
            default:
                break;
            }
        }

        else if (role == Qt::ToolTipRole)
        {
            switch (index.column())
            {
            case kBlasListColumnRootSAH:
                return QString::number(cache.root_sah, kQtFloatFormat, kQtTooltipFloatPrecision);
            case kBlasListColumnMinSAH:
                return QString::number(cache.max_sah, kQtFloatFormat, kQtTooltipFloatPrecision);
            case kBlasListColumnMeanSAH:
                return QString::number(cache.mean_sah, kQtFloatFormat, kQtTooltipFloatPrecision);
            default:
                break;
            }
        }
        else if (role == Qt::BackgroundRole)
        {
            switch (index.column())
            {
            case kBlasListColumnBuildType:
                return QVariant(GetBuildTypeColor(cache.build_flags));
            case kBlasListColumnRootSAH:
                return QVariant(GetNanTypeColor(cache.root_sah));
            case kBlasListColumnMeanSAH:
                return QVariant(GetNanTypeColor(cache.mean_sah));
            }
            return QVariant();
        }
        else if (role == Qt::CheckStateRole)
        {
            switch (index.column())
            {
            case kBlasListColumnAllowUpdate:
                return cache.build_flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR ? Qt::Checked : Qt::Unchecked;
            case kBlasListColumnAllowCompaction:
                return cache.build_flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR ? Qt::Checked : Qt::Unchecked;
            case kBlasListColumnLowMemory:
                return cache.build_flags & VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR ? Qt::Checked : Qt::Unchecked;
            }
        }
        else if (role == Qt::UserRole)
        {
            switch (index.column())
            {
            case kBlasListColumnAddress:
                return QVariant::fromValue<qulonglong>(cache.address);
            case kBlasListColumnAllowUpdate:
                return QVariant::fromValue<uint32_t>(cache.build_flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR);
            case kBlasListColumnAllowCompaction:
                return QVariant::fromValue<uint32_t>(cache.build_flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
            case kBlasListColumnLowMemory:
                return QVariant::fromValue<uint32_t>(cache.build_flags & VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR);
            case kBlasListColumnBuildType:
                return QVariant::fromValue<uint32_t>(cache.build_flags);
            case kBlasListColumnInstanceCount:
                return QVariant::fromValue<qulonglong>(cache.instance_count);
            case kBlasListColumnNodeCount:
                return QVariant::fromValue<qulonglong>(cache.node_count);
            case kBlasListColumnBoxCount:
                return QVariant::fromValue<qulonglong>(cache.box_count);
            case kBlasListColumnBox32Count:
                return QVariant::fromValue<qulonglong>(cache.box32_count);
            case kBlasListColumnBox16Count:
                return QVariant::fromValue<qulonglong>(cache.box16_count);
            case kBlasListColumnTriangleNodeCount:
                return QVariant::fromValue<qulonglong>(cache.triangle_node_count);
            case kBlasListColumnProceduralNodeCount:
                return QVariant::fromValue<qulonglong>(cache.procedural_node_count);
            case kBlasListColumnMemoryUsage:
                return QVariant::fromValue<qulonglong>(cache.memory_usage);
            case kBlasListColumnRootSAH:
                return QVariant::fromValue<float>(cache.root_sah);
            case kBlasListColumnMinSAH:
                return QVariant::fromValue<float>(cache.max_sah);
            case kBlasListColumnMeanSAH:
                return QVariant::fromValue<float>(cache.mean_sah);
            case kBlasListColumnMaxDepth:
                return QVariant::fromValue<uint32_t>(cache.max_depth);
            case kBlasListColumnAvgDepth:
                return QVariant::fromValue<uint32_t>(cache.avg_depth);
            case kBlasListColumnBlasIndex:
                return QVariant::fromValue<uint64_t>(cache.blas_index);
            case kBlasListColumnPadding:
                return QVariant();
            default:
                break;
            }
        }
        return QVariant();
    }

    Qt::ItemFlags BlasListItemModel::flags(const QModelIndex& index) const
    {
        return QAbstractItemModel::flags(index);
    }

    QVariant BlasListItemModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal)
        {
            if (role == Qt::DisplayRole)
            {
                switch (section)
                {
                case kBlasListColumnIndex:
                    return "Row Id";
                case kBlasListColumnAddress:
                    return "Address";
                case kBlasListColumnAllowUpdate:
                    return "Allow update";
                case kBlasListColumnAllowCompaction:
                    return "Allow compaction";
                case kBlasListColumnLowMemory:
                    return "Low memory";
                case kBlasListColumnBuildType:
                    return "Build type";
                case kBlasListColumnInstanceCount:
                    return "Instances";
                case kBlasListColumnNodeCount:
                    return "Nodes";
                case kBlasListColumnBoxCount:
                    return "Boxes";
                case kBlasListColumnBox32Count:
                    if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
                    {
                        return "128-bit boxes";
                    }
                    else
                    {
                        return "32-bit boxes";
                    }
                case kBlasListColumnBox16Count:
                    return "16-bit boxes";
                case kBlasListColumnTriangleNodeCount:
                    return "Triangle nodes";
                case kBlasListColumnProceduralNodeCount:
                    return "Procedural nodes";
                case kBlasListColumnMemoryUsage:
                    return "Memory usage";
                case kBlasListColumnRootSAH:
                    return "Root SAH";
                case kBlasListColumnMinSAH:
                    return "Min SAH";
                case kBlasListColumnMeanSAH:
                    return "Mean SAH";
                case kBlasListColumnMaxDepth:
                    return "Max. depth";
                case kBlasListColumnAvgDepth:
                    return "Avg. depth";
                case kBlasListColumnBlasIndex:
                    return "BLAS index";
                case kBlasListColumnPadding:
                    return "";
                default:
                    break;
                }
            }
            else if (role == Qt::ToolTipRole)
            {
                switch (section)
                {
                case kBlasListColumnIndex:
                    return "The index of the row in the table";
                case kBlasListColumnAddress:
                    return "The base address of this BLAS";
                case kBlasListColumnAllowUpdate:
                    if (RraApiInfoIsVulkan())
                    {
                        return "Presence of the VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR acceleration structure flag";
                    }
                    else
                    {
                        return "Presence of the D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE acceleration structure flag";
                    }
                case kBlasListColumnAllowCompaction:
                    if (RraApiInfoIsVulkan())
                    {
                        return "Presence of the VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR acceleration structure flag";
                    }
                    else
                    {
                        return "Presence of the D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION acceleration structure flag";
                    }
                case kBlasListColumnLowMemory:
                    if (RraApiInfoIsVulkan())
                    {
                        return "Presence of the VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR acceleration structure flag";
                    }
                    else
                    {
                        return "Presence of the D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY acceleration structure flag";
                    }
                case kBlasListColumnBuildType:
                    if (RraApiInfoIsVulkan())
                    {
                        return "Presence of either the VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR or "
                               "VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR flag";
                    }
                    else
                    {
                        return "Presence of either the D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE or "
                               "D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD flag";
                    }
                case kBlasListColumnInstanceCount:
                    return "The number of instances of this BLAS used in the scene";
                case kBlasListColumnNodeCount:
                    return "The total (sum of box and triangle nodes) number of nodes in this BLAS.";
                case kBlasListColumnBoxCount:
                    return "The total number of box (internal) nodes in this BLAS";
                case kBlasListColumnBox32Count:
                    return "The number of box nodes with 32-bit floating point precision bounding boxes in this BLAS";
                case kBlasListColumnBox16Count:
                    return "The number of box nodes with 16-bit floating point precision bounding boxes in this BLAS";
                case kBlasListColumnTriangleNodeCount:
                    return "The total number of triangle nodes in this BLAS";
                case kBlasListColumnProceduralNodeCount:
                    return "The total number of procedural nodes in this BLAS";
                case kBlasListColumnMemoryUsage:
                    return "The amount of memory that this BLAS uses";
                case kBlasListColumnRootSAH:
                    return "The surface area heuristic of the BLAS root node";
                case kBlasListColumnMinSAH:
                    return "The minimum surface area heuristic of all triangle nodes in the BLAS";
                case kBlasListColumnMeanSAH:
                    return "The average surface area heuristic of all triangle nodes in the BLAS";
                case kBlasListColumnMaxDepth:
                    return "The maximum depth of all triangle nodes in this BLAS";
                case kBlasListColumnAvgDepth:
                    return "The average depth of all triangle nodes in this BLAS";
                case kBlasListColumnBlasIndex:
                    return "The internal BLAS index";
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

    QModelIndex BlasListItemModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        return createIndex(row, column);
    }

    QModelIndex BlasListItemModel::parent(const QModelIndex& index) const
    {
        Q_UNUSED(index);
        return QModelIndex();
    }

    int BlasListItemModel::rowCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_rows_;
    }

    int BlasListItemModel::columnCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_columns_;
    }
}  // namespace rra

