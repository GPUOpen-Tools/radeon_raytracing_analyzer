//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the instances item model. Shared between TLAS
/// and BLAS instances tables.
//=============================================================================

#include "models/instances_item_model.h"

#include "qt_common/utils/qt_util.h"

#include "public/rra_api_info.h"
#include "public/rra_assert.h"

#include "constants.h"
#include "settings/settings.h"
#include "util/rra_util.h"
#include "views/custom_widgets/index_header_view.h"

namespace rra
{
    // Indices in the instance transform.
    static const int kTransformIndexPosX = 3;
    static const int kTransformIndexPosY = 7;
    static const int kTransformIndexPosZ = 11;
    static const int kTransformIndexM11  = 0;
    static const int kTransformIndexM12  = 4;
    static const int kTransformIndexM13  = 8;
    static const int kTransformIndexM21  = 1;
    static const int kTransformIndexM22  = 5;
    static const int kTransformIndexM23  = 9;
    static const int kTransformIndexM31  = 2;
    static const int kTransformIndexM32  = 6;
    static const int kTransformIndexM33  = 10;

    InstancesItemModel::InstancesItemModel(QObject* parent)
        : QAbstractItemModel(parent)
        , num_rows_(0)
        , num_columns_(0)
    {
    }

    InstancesItemModel::~InstancesItemModel()
    {
    }

    void InstancesItemModel::SetRowCount(int rows)
    {
        num_rows_ = rows;
        cache_.clear();
    }

    void InstancesItemModel::SetColumnCount(int columns)
    {
        num_columns_ = columns;
    }

    void InstancesItemModel::Initialize(QTableView* acceleration_structure_table)
    {
        acceleration_structure_table->setHorizontalHeader(new IndexHeaderView(kInstancesColumnIndex, Qt::Horizontal, acceleration_structure_table));
        rra_util::InitializeTableView(acceleration_structure_table);
        acceleration_structure_table->sortByColumn(kInstancesColumnInstanceIndex, Qt::AscendingOrder);

        acceleration_structure_table->setColumnWidth(kInstancesColumnInstanceIndex, 120);
        acceleration_structure_table->setColumnWidth(kInstancesColumnInstanceAddress, 140);
        acceleration_structure_table->setColumnWidth(kInstancesColumnInstanceOffset, 120);
        acceleration_structure_table->setColumnWidth(kInstancesColumnInstanceMask, 120);
        acceleration_structure_table->setColumnWidth(kInstancesColumnCullDisableFlag, 100);
        acceleration_structure_table->setColumnWidth(kInstancesColumnFlipFacingFlag, 100);
        acceleration_structure_table->setColumnWidth(kInstancesColumnForceOpaqueFlag, 100);
        acceleration_structure_table->setColumnWidth(kInstancesColumnForceNoOpaqueFlag, 120);
        acceleration_structure_table->setColumnWidth(kInstancesColumnRebraidSiblingCount, 140);
        acceleration_structure_table->setColumnWidth(kInstancesColumnXPosition, 100);
        acceleration_structure_table->setColumnWidth(kInstancesColumnYPosition, 100);
        acceleration_structure_table->setColumnWidth(kInstancesColumnYPosition, 100);
        acceleration_structure_table->setColumnWidth(kInstancesColumnZPosition, 100);
        acceleration_structure_table->setColumnWidth(kInstancesColumnM11, 120);
        acceleration_structure_table->setColumnWidth(kInstancesColumnM12, 120);
        acceleration_structure_table->setColumnWidth(kInstancesColumnM13, 120);
        acceleration_structure_table->setColumnWidth(kInstancesColumnM21, 120);
        acceleration_structure_table->setColumnWidth(kInstancesColumnM22, 120);
        acceleration_structure_table->setColumnWidth(kInstancesColumnM23, 120);
        acceleration_structure_table->setColumnWidth(kInstancesColumnM31, 120);
        acceleration_structure_table->setColumnWidth(kInstancesColumnM32, 120);
        acceleration_structure_table->setColumnWidth(kInstancesColumnM33, 120);
    }

    void InstancesItemModel::AddAccelerationStructure(const InstancesTableStatistics& stats)
    {
        cache_.push_back(stats);
    }

    QVariant InstancesItemModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        int row = index.row();

        const InstancesTableStatistics& cache = cache_[row];

        if (role == Qt::DisplayRole)
        {
            int decimal_precision = rra::Settings::Get().GetDecimalPrecision();
            switch (index.column())
            {
            case kInstancesColumnInstanceIndex:
                return QString::number(cache.instance_index);
            case kInstancesColumnInstanceAddress:
                return QString("0x%1").arg(cache.instance_address, 0, 16);
            case kInstancesColumnInstanceOffset:
                return QString("0x%1").arg(cache.instance_offset, 0, 16);
            case kInstancesColumnInstanceMask:
                return QString("0x%1%2").arg((cache.instance_mask & 0xF0) >> 4, 0, 16).arg(cache.instance_mask & 0x0F, 0, 16);
            case kInstancesColumnRebraidSiblingCount:
                return QString::number(cache.rebraid_sibling_count);
            case kInstancesColumnXPosition:
                return QString::number(cache.transform[kTransformIndexPosX], kQtFloatFormat, decimal_precision);
            case kInstancesColumnYPosition:
                return QString::number(cache.transform[kTransformIndexPosY], kQtFloatFormat, decimal_precision);
            case kInstancesColumnZPosition:
                return QString::number(cache.transform[kTransformIndexPosZ], kQtFloatFormat, decimal_precision);
            case kInstancesColumnM11:
                return QString::number(cache.transform[kTransformIndexM11], kQtFloatFormat, decimal_precision);
            case kInstancesColumnM12:
                return QString::number(cache.transform[kTransformIndexM12], kQtFloatFormat, decimal_precision);
            case kInstancesColumnM13:
                return QString::number(cache.transform[kTransformIndexM13], kQtFloatFormat, decimal_precision);
            case kInstancesColumnM21:
                return QString::number(cache.transform[kTransformIndexM21], kQtFloatFormat, decimal_precision);
            case kInstancesColumnM22:
                return QString::number(cache.transform[kTransformIndexM22], kQtFloatFormat, decimal_precision);
            case kInstancesColumnM23:
                return QString::number(cache.transform[kTransformIndexM23], kQtFloatFormat, decimal_precision);
            case kInstancesColumnM31:
                return QString::number(cache.transform[kTransformIndexM31], kQtFloatFormat, decimal_precision);
            case kInstancesColumnM32:
                return QString::number(cache.transform[kTransformIndexM32], kQtFloatFormat, decimal_precision);
            case kInstancesColumnM33:
                return QString::number(cache.transform[kTransformIndexM33], kQtFloatFormat, decimal_precision);
            case kInstancesColumnUniqueInstanceIndex:
                return QString::number(cache.unique_instance_index);
            case kInstancesColumnPadding:
                return QString{""};
            default:
                break;
            }
        }
        else if (role == Qt::ToolTipRole)
        {
            switch (index.column())
            {
            case kInstancesColumnXPosition:
                return QString::number(cache.transform[kTransformIndexPosX], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kInstancesColumnYPosition:
                return QString::number(cache.transform[kTransformIndexPosY], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kInstancesColumnZPosition:
                return QString::number(cache.transform[kTransformIndexPosZ], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kInstancesColumnM11:
                return QString::number(cache.transform[kTransformIndexM11], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kInstancesColumnM12:
                return QString::number(cache.transform[kTransformIndexM12], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kInstancesColumnM13:
                return QString::number(cache.transform[kTransformIndexM13], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kInstancesColumnM21:
                return QString::number(cache.transform[kTransformIndexM21], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kInstancesColumnM22:
                return QString::number(cache.transform[kTransformIndexM22], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kInstancesColumnM23:
                return QString::number(cache.transform[kTransformIndexM23], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kInstancesColumnM31:
                return QString::number(cache.transform[kTransformIndexM31], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kInstancesColumnM32:
                return QString::number(cache.transform[kTransformIndexM32], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kInstancesColumnM33:
                return QString::number(cache.transform[kTransformIndexM33], kQtFloatFormat, kQtTooltipFloatPrecision);
            default:
                break;
            }
        }
        else if (role == Qt::UserRole)
        {
            switch (index.column())
            {
            case kInstancesColumnInstanceIndex:
                return QVariant::fromValue<quint32>(cache.instance_index);
            case kInstancesColumnInstanceAddress:
                return QVariant::fromValue<qulonglong>((qulonglong)cache.instance_address);
            case kInstancesColumnInstanceOffset:
                return QVariant::fromValue<qulonglong>((qulonglong)cache.instance_offset);
            case kInstancesColumnInstanceMask:
                return QVariant::fromValue<quint32>(cache.instance_mask);
            case kInstancesColumnCullDisableFlag:
                return QVariant::fromValue<bool>(cache.cull_disable_flag);
            case kInstancesColumnFlipFacingFlag:
                return QVariant::fromValue<bool>(cache.flip_facing_flag);
            case kInstancesColumnForceOpaqueFlag:
                return QVariant::fromValue<bool>(cache.force_opaque);
            case kInstancesColumnForceNoOpaqueFlag:
                return QVariant::fromValue<bool>(cache.force_no_opaque);
            case kInstancesColumnRebraidSiblingCount:
                return QVariant::fromValue<quint32>(cache.rebraid_sibling_count);
            case kInstancesColumnXPosition:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexPosX]);
            case kInstancesColumnYPosition:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexPosY]);
            case kInstancesColumnZPosition:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexPosZ]);
            case kInstancesColumnM11:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM11]);
            case kInstancesColumnM12:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM12]);
            case kInstancesColumnM13:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM13]);
            case kInstancesColumnM21:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM21]);
            case kInstancesColumnM22:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM22]);
            case kInstancesColumnM23:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM23]);
            case kInstancesColumnM31:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM31]);
            case kInstancesColumnM32:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM32]);
            case kInstancesColumnM33:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM33]);
            case kInstancesColumnUniqueInstanceIndex:
                return QVariant::fromValue<uint32_t>(cache.unique_instance_index);
            case kInstancesColumnPadding:
                return QVariant();
            default:
                break;
            }
        }
        else if (role == Qt::CheckStateRole)
        {
            switch (index.column())
            {
            case kInstancesColumnCullDisableFlag:
                return cache.cull_disable_flag ? Qt::Checked : Qt::Unchecked;
            case kInstancesColumnFlipFacingFlag:
                return cache.flip_facing_flag ? Qt::Checked : Qt::Unchecked;
            case kInstancesColumnForceOpaqueFlag:
                return cache.force_opaque ? Qt::Checked : Qt::Unchecked;
            case kInstancesColumnForceNoOpaqueFlag:
                return cache.force_no_opaque ? Qt::Checked : Qt::Unchecked;
            }
        }
        return QVariant();
    }

    Qt::ItemFlags InstancesItemModel::flags(const QModelIndex& index) const
    {
        return QAbstractItemModel::flags(index);
    }

    QVariant InstancesItemModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal)
        {
            if (role == Qt::DisplayRole)
            {
                switch (section)
                {
                case kInstancesColumnIndex:
                    return "Row Id";
                case kInstancesColumnInstanceIndex:
                    return "Instance index";
                case kInstancesColumnInstanceAddress:
                    return "Instance address";
                case kInstancesColumnInstanceOffset:
                    return "Instance offset";
                case kInstancesColumnInstanceMask:
                    return "Instance mask";
                case kInstancesColumnCullDisableFlag:
                    return "Cull disable";
                case kInstancesColumnFlipFacingFlag:
                    return "Flip facing";
                case kInstancesColumnForceOpaqueFlag:
                    return "Force opaque";
                case kInstancesColumnForceNoOpaqueFlag:
                    return "Force no opaque";
                case kInstancesColumnRebraidSiblingCount:
                    return "Rebraid sibling count";
                case kInstancesColumnXPosition:
                    return "X Position";
                case kInstancesColumnYPosition:
                    return "Y Position";
                case kInstancesColumnZPosition:
                    return "Z Position";
                case kInstancesColumnM11:
                    return "Transform [0][0]";
                case kInstancesColumnM12:
                    return "Transform [0][1]";
                case kInstancesColumnM13:
                    return "Transform [0][2]";
                case kInstancesColumnM21:
                    return "Transform [1][0]";
                case kInstancesColumnM22:
                    return "Transform [1][1]";
                case kInstancesColumnM23:
                    return "Transform [1][2]";
                case kInstancesColumnM31:
                    return "Transform [2][0]";
                case kInstancesColumnM32:
                    return "Transform [2][1]";
                case kInstancesColumnM33:
                    return "Transform [2][2]";
                case kInstancesColumnPadding:
                    return "";
                default:
                    break;
                }
            }
            else if (role == Qt::ToolTipRole)
            {
                switch (section)
                {
                case kInstancesColumnIndex:
                    return "The index of the row in the table";
                case kInstancesColumnInstanceIndex:
                    return "The index of this instance";
                case kInstancesColumnInstanceMask:
                    if (RraApiInfoIsVulkan())
                    {
                        return "VkAccelerationStructureInstanceKHR::mask";
                    }
                    else
                    {
                        return "D3D12_RAYTRACING_INSTANCE_DESC::InstanceMask";
                    }
                case kInstancesColumnCullDisableFlag:
                    if (RraApiInfoIsVulkan())
                    {
                        return "Presence of the VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR instance flag";
                    }
                    else
                    {
                        return "Presence of the D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE instance flag";
                    }
                case kInstancesColumnFlipFacingFlag:
                    if (RraApiInfoIsVulkan())
                    {
                        return "Presence of the VK_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT_KHR instance flag";
                    }
                    else
                    {
                        return "Presence of the D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE instance flag";
                    }
                case kInstancesColumnForceOpaqueFlag:
                    if (RraApiInfoIsVulkan())
                    {
                        return "Presence of the VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR instance flag";
                    }
                    else
                    {
                        return "Presence of the D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE instance flag";
                    }
                case kInstancesColumnForceNoOpaqueFlag:
                    if (RraApiInfoIsVulkan())
                    {
                        return "Presence of the VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR instance flag";
                    }
                    else
                    {
                        return "Presence of the D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE instance flag";
                    }
                case kInstancesColumnRebraidSiblingCount:
                    return "The driver may split an instance into multiple instances. This is the number of siblings in such cases.";
                case kInstancesColumnInstanceAddress:
                    return "The virtual address of this instance in GPU memory";
                case kInstancesColumnInstanceOffset:
                    return "The address of this instance relative to the TLAS address";
                case kInstancesColumnM11:
                    return "The top left transform component of this instance";
                case kInstancesColumnM12:
                    return "The top center transform component of this instance";
                case kInstancesColumnM13:
                    return "The top right transform component of this instance";
                case kInstancesColumnM21:
                    return "The middle left transform component of this instance";
                case kInstancesColumnM22:
                    return "The center transform component of this instance";
                case kInstancesColumnM23:
                    return "The middle right transform component of this instance";
                case kInstancesColumnM31:
                    return "The bottom left transform component of this instance";
                case kInstancesColumnM32:
                    return "The bottom center transform component of this instance";
                case kInstancesColumnM33:
                    return "The bottom right transform component of this instance";
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

    QModelIndex InstancesItemModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        return createIndex(row, column);
    }

    QModelIndex InstancesItemModel::parent(const QModelIndex& index) const
    {
        Q_UNUSED(index);
        return QModelIndex();
    }

    int InstancesItemModel::rowCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_rows_;
    }

    int InstancesItemModel::columnCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_columns_;
    }
}  // namespace rra

