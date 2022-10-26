//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the TLAS instances item model.
//=============================================================================

#include "models/tlas/tlas_instances_item_model.h"

#include "qt_common/utils/qt_util.h"

#include "public/rra_assert.h"

#include "constants.h"
#include "settings/settings.h"

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

    TlasInstancesItemModel::TlasInstancesItemModel(QObject* parent)
        : QAbstractItemModel(parent)
        , num_rows_(0)
        , num_columns_(0)
    {
    }

    TlasInstancesItemModel::~TlasInstancesItemModel()
    {
    }

    void TlasInstancesItemModel::SetRowCount(int rows)
    {
        num_rows_ = rows;
        cache_.clear();
    }

    void TlasInstancesItemModel::SetColumnCount(int columns)
    {
        num_columns_ = columns;
    }

    void TlasInstancesItemModel::Initialize(ScaledTableView* acceleration_structure_table)
    {
        acceleration_structure_table->horizontalHeader()->setSectionsClickable(true);

        // Set default column widths wide enough to show table contents.
        acceleration_structure_table->SetColumnPadding(0);

        acceleration_structure_table->SetColumnWidthEms(kTlasInstancesColumnInstanceAddress, 18);
        acceleration_structure_table->SetColumnWidthEms(kTlasInstancesColumnInstanceOffset, 12);
        acceleration_structure_table->SetColumnWidthEms(kTlasInstancesColumnInstanceMask, 12);
        acceleration_structure_table->SetColumnWidthEms(kTlasInstancesColumnXPosition, 10);
        acceleration_structure_table->SetColumnWidthEms(kTlasInstancesColumnYPosition, 10);
        acceleration_structure_table->SetColumnWidthEms(kTlasInstancesColumnYPosition, 10);
        acceleration_structure_table->SetColumnWidthEms(kTlasInstancesColumnZPosition, 10);
        acceleration_structure_table->SetColumnWidthEms(kTlasInstancesColumnM11, 12);
        acceleration_structure_table->SetColumnWidthEms(kTlasInstancesColumnM12, 12);
        acceleration_structure_table->SetColumnWidthEms(kTlasInstancesColumnM13, 12);
        acceleration_structure_table->SetColumnWidthEms(kTlasInstancesColumnM21, 12);
        acceleration_structure_table->SetColumnWidthEms(kTlasInstancesColumnM22, 12);
        acceleration_structure_table->SetColumnWidthEms(kTlasInstancesColumnM23, 12);
        acceleration_structure_table->SetColumnWidthEms(kTlasInstancesColumnM31, 12);
        acceleration_structure_table->SetColumnWidthEms(kTlasInstancesColumnM32, 12);
        acceleration_structure_table->SetColumnWidthEms(kTlasInstancesColumnM33, 12);

        // Allow users to resize columns if desired.
        acceleration_structure_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Interactive);
    }

    void TlasInstancesItemModel::AddAccelerationStructure(const TlasInstancesStatistics& stats)
    {
        cache_.push_back(stats);
    }

    QVariant TlasInstancesItemModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        int row = index.row();

        const TlasInstancesStatistics& cache = cache_[row];

        if (role == Qt::DisplayRole)
        {
            int decimal_precision = rra::Settings::Get().GetDecimalPrecision();
            switch (index.column())
            {
            case kTlasInstancesColumnInstanceAddress:
                return QString("0x%1").arg(cache.instance_address, 0, 16);
            case kTlasInstancesColumnInstanceOffset:
                return QString("0x%1").arg(cache.instance_offset, 0, 16);
            case kTlasInstancesColumnInstanceMask:
                return QString("0x%1%2").arg((cache.instance_mask & 0xF0) >> 4, 0, 16).arg(cache.instance_mask & 0x0F, 0, 16);
            case kTlasInstancesColumnXPosition:
                return QString::number(cache.transform[kTransformIndexPosX], kQtFloatFormat, decimal_precision);
            case kTlasInstancesColumnYPosition:
                return QString::number(cache.transform[kTransformIndexPosY], kQtFloatFormat, decimal_precision);
            case kTlasInstancesColumnZPosition:
                return QString::number(cache.transform[kTransformIndexPosZ], kQtFloatFormat, decimal_precision);
            case kTlasInstancesColumnM11:
                return QString::number(cache.transform[kTransformIndexM11], kQtFloatFormat, decimal_precision);
            case kTlasInstancesColumnM12:
                return QString::number(cache.transform[kTransformIndexM12], kQtFloatFormat, decimal_precision);
            case kTlasInstancesColumnM13:
                return QString::number(cache.transform[kTransformIndexM13], kQtFloatFormat, decimal_precision);
            case kTlasInstancesColumnM21:
                return QString::number(cache.transform[kTransformIndexM21], kQtFloatFormat, decimal_precision);
            case kTlasInstancesColumnM22:
                return QString::number(cache.transform[kTransformIndexM22], kQtFloatFormat, decimal_precision);
            case kTlasInstancesColumnM23:
                return QString::number(cache.transform[kTransformIndexM23], kQtFloatFormat, decimal_precision);
            case kTlasInstancesColumnM31:
                return QString::number(cache.transform[kTransformIndexM31], kQtFloatFormat, decimal_precision);
            case kTlasInstancesColumnM32:
                return QString::number(cache.transform[kTransformIndexM32], kQtFloatFormat, decimal_precision);
            case kTlasInstancesColumnM33:
                return QString::number(cache.transform[kTransformIndexM33], kQtFloatFormat, decimal_precision);
            case kTlasInstancesColumnInstanceIndex:
                return QString::number(cache.instance_index);
            default:
                break;
            }
        }
        else if (role == Qt::ToolTipRole)
        {
            switch (index.column())
            {
            case kTlasInstancesColumnXPosition:
                return QString::number(cache.transform[kTransformIndexPosX], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kTlasInstancesColumnYPosition:
                return QString::number(cache.transform[kTransformIndexPosY], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kTlasInstancesColumnZPosition:
                return QString::number(cache.transform[kTransformIndexPosZ], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kTlasInstancesColumnM11:
                return QString::number(cache.transform[kTransformIndexM11], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kTlasInstancesColumnM12:
                return QString::number(cache.transform[kTransformIndexM12], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kTlasInstancesColumnM13:
                return QString::number(cache.transform[kTransformIndexM13], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kTlasInstancesColumnM21:
                return QString::number(cache.transform[kTransformIndexM21], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kTlasInstancesColumnM22:
                return QString::number(cache.transform[kTransformIndexM22], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kTlasInstancesColumnM23:
                return QString::number(cache.transform[kTransformIndexM23], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kTlasInstancesColumnM31:
                return QString::number(cache.transform[kTransformIndexM31], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kTlasInstancesColumnM32:
                return QString::number(cache.transform[kTransformIndexM32], kQtFloatFormat, kQtTooltipFloatPrecision);
            case kTlasInstancesColumnM33:
                return QString::number(cache.transform[kTransformIndexM33], kQtFloatFormat, kQtTooltipFloatPrecision);
            default:
                break;
            }
        }
        else if (role == Qt::UserRole)
        {
            switch (index.column())
            {
            case kTlasInstancesColumnInstanceAddress:
                return QVariant::fromValue<qulonglong>((qulonglong)cache.instance_address);
            case kTlasInstancesColumnInstanceOffset:
                return QVariant::fromValue<qulonglong>((qulonglong)cache.instance_offset);
            case kTlasInstancesColumnInstanceMask:
                return QVariant::fromValue<quint32>(cache.instance_mask);
            case kTlasInstancesColumnXPosition:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexPosX]);
            case kTlasInstancesColumnYPosition:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexPosY]);
            case kTlasInstancesColumnZPosition:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexPosZ]);
            case kTlasInstancesColumnM11:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM11]);
            case kTlasInstancesColumnM12:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM12]);
            case kTlasInstancesColumnM13:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM13]);
            case kTlasInstancesColumnM21:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM21]);
            case kTlasInstancesColumnM22:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM22]);
            case kTlasInstancesColumnM23:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM23]);
            case kTlasInstancesColumnM31:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM31]);
            case kTlasInstancesColumnM32:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM32]);
            case kTlasInstancesColumnM33:
                return QVariant::fromValue<float>(cache.transform[kTransformIndexM33]);
            case kTlasInstancesColumnInstanceIndex:
                return QVariant::fromValue<uint32_t>(cache.instance_index);
            default:
                break;
            }
        }
        return QVariant();
    }

    Qt::ItemFlags TlasInstancesItemModel::flags(const QModelIndex& index) const
    {
        return QAbstractItemModel::flags(index);
    }

    QVariant TlasInstancesItemModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal)
        {
            if (role == Qt::DisplayRole)
            {
                switch (section)
                {
                case kTlasInstancesColumnInstanceAddress:
                    return "Instance address";
                case kTlasInstancesColumnInstanceOffset:
                    return "Instance offset";
                case kTlasInstancesColumnInstanceMask:
                    return "Instance mask";
                case kTlasInstancesColumnXPosition:
                    return "X Position";
                case kTlasInstancesColumnYPosition:
                    return "Y Position";
                case kTlasInstancesColumnZPosition:
                    return "Z Position";
                case kTlasInstancesColumnM11:
                    return "Transform [0][0]";
                case kTlasInstancesColumnM12:
                    return "Transform [0][1]";
                case kTlasInstancesColumnM13:
                    return "Transform [0][2]";
                case kTlasInstancesColumnM21:
                    return "Transform [1][0]";
                case kTlasInstancesColumnM22:
                    return "Transform [1][1]";
                case kTlasInstancesColumnM23:
                    return "Transform [1][2]";
                case kTlasInstancesColumnM31:
                    return "Transform [2][0]";
                case kTlasInstancesColumnM32:
                    return "Transform [2][1]";
                case kTlasInstancesColumnM33:
                    return "Transform [2][2]";
                default:
                    break;
                }
            }
            else if (role == Qt::ToolTipRole)
            {
                switch (section)
                {
                case kTlasInstancesColumnInstanceAddress:
                    return "The address of this instance in the TLAS";
                case kTlasInstancesColumnInstanceOffset:
                    return "The offset of this instance in the TLAS";
                case kTlasInstancesColumnXPosition:
                    return "The X position of this instance";
                case kTlasInstancesColumnYPosition:
                    return "The Y position of this instance";
                case kTlasInstancesColumnZPosition:
                    return "The Z position of this instance";
                case kTlasInstancesColumnM11:
                    return "The top left transform component of this instance";
                case kTlasInstancesColumnM12:
                    return "The top center transform component of this instance";
                case kTlasInstancesColumnM13:
                    return "The top right transform component of this instance";
                case kTlasInstancesColumnM21:
                    return "The middle left transform component of this instance";
                case kTlasInstancesColumnM22:
                    return "The center transform component of this instance";
                case kTlasInstancesColumnM23:
                    return "The middle right transform component of this instance";
                case kTlasInstancesColumnM31:
                    return "The bottom left transform component of this instance";
                case kTlasInstancesColumnM32:
                    return "The bottom center transform component of this instance";
                case kTlasInstancesColumnM33:
                    return "The bottom right transform component of this instance";
                default:
                    break;
                }
            }
        }

        return QAbstractItemModel::headerData(section, orientation, role);
    }

    QModelIndex TlasInstancesItemModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        return createIndex(row, column);
    }

    QModelIndex TlasInstancesItemModel::parent(const QModelIndex& index) const
    {
        Q_UNUSED(index);
        return QModelIndex();
    }

    int TlasInstancesItemModel::rowCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_rows_;
    }

    int TlasInstancesItemModel::columnCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return num_columns_;
    }
}  // namespace rra
