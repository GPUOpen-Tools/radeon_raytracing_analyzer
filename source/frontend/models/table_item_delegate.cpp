//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Implementation of RRA's table item delegate.
//=============================================================================

#include "models/table_item_delegate.h"

#include <algorithm>
#include <QMap>
#include <QPainter>
#include <QScreen>
#include <QWindow>

#include "qt_common/utils/scaling_manager.h"

#include "constants.h"
#include "views/widget_util.h"
#include "util/string_util.h"
#include "qt_common/custom_widgets/scaled_table_view.h"
#include "settings/settings.h"

TableItemDelegate::TableItemDelegate(QObject* parent)
    : QAbstractItemDelegate(parent)
{
}

TableItemDelegate::TableItemDelegate(ScaledTableView* table_view, QObject* parent)
    : QAbstractItemDelegate(parent)
    , table_view_{table_view}
{
}

TableItemDelegate::~TableItemDelegate()
{
}

bool TableItemDelegate::CheckboxAt(int row, int column) const
{
    Q_UNUSED(row);
    Q_UNUSED(column);

    return false;
}

bool TableItemDelegate::Vec3At(int row, int column) const
{
    Q_UNUSED(row);
    Q_UNUSED(column);

    return false;
}

QSize TableItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    return QSize(0, 0);
}

void TableItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->setPen(QColor(Qt::black));

    QColor background_color = index.data(Qt::BackgroundRole).value<QColor>();

    // Draw cell background color.
    if (background_color.isValid())
    {
        // Draw background if valid.
        painter->fillRect(option.rect, QBrush(background_color));

        painter->setPen(QColor(Qt::white));
    }
    else if ((option.state & QStyle::State_Selected) != 0)
    {
        // Draw selection highlight (and set text color to white).
        painter->fillRect(option.rect, QBrush(rra::kTableRowSelectedColor));
    }

    if (CheckboxAt(index.row(), index.column()))
    {
        // Draw data as a checkbox.
        rra::widget_util::DrawCheckboxCell(painter, option.rect, index.data(Qt::CheckStateRole).toBool(), true);
    }
    else if (Vec3At(index.row(), index.column()))
    {
        std::vector<std::string> vec3_string = rra::string_util::Split(index.data().toString().toStdString(), ",");

        if (vec3_string.size() == 3 && table_view_)
        {
            auto rect = option.rect;

            // We use the DPI to make the spacing look nice on both 4k and 1080p monitors.
            QScreen* srn               = table_view_->window()->windowHandle()->screen();
            qreal    dpi               = (qreal)srn->logicalDotsPerInch();
            int      decimal_precision = rra::Settings::Get().GetDecimalPrecision();

            // Decimal spacing ensures the vec3 columns don't overlap each other.
            float decimal_spacing = 0.07f * decimal_precision + 0.3f;
            int   min_spacing     = (int)(dpi * decimal_spacing);
            int   max_spacing     = min_spacing * 3;

            int spacing = std::max(rect.width() / 3, min_spacing);
            spacing     = std::min(spacing, max_spacing);

            painter->drawText(rect, Qt::AlignRight | Qt::AlignVCenter, QString::fromStdString(vec3_string[2]));
            rect.setRight(rect.right() - spacing);
            painter->drawText(rect, Qt::AlignRight | Qt::AlignVCenter, QString::fromStdString(vec3_string[1]));
            rect.setRight(rect.right() - spacing);
            painter->drawText(rect, Qt::AlignRight | Qt::AlignVCenter, QString::fromStdString(vec3_string[0]));
        }
    }
    else
    {
        painter->drawText(option.rect, Qt::AlignRight | Qt::AlignVCenter, index.data().toString());
    }
}
