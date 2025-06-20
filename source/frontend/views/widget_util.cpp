//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a number of widget utilities.
///
/// These functions apply a common look and feel to various widget types.
///
//=============================================================================

#include <QHeaderView>
#include <QStandardItemModel>

#include "qt_common/utils/qt_util.h"

#include "constants.h"
#include "settings/settings.h"
#include "views/widget_util.h"

namespace rra
{
    void widget_util::ApplyStandardPaneStyle(QScrollArea* scroll_area)
    {
        scroll_area->setFrameStyle(QFrame::NoFrame);
    }

    void widget_util::InitSingleSelectComboBox(QWidget*           parent,
                                               ArrowIconComboBox* combo_box,
                                               const QString&     default_text,
                                               bool               retain_default_text,
                                               const QString      prefix_text)
    {
        if (combo_box != nullptr)
        {
            combo_box->InitSingleSelect(parent, default_text, retain_default_text, prefix_text);
            combo_box->setCursor(Qt::PointingHandCursor);
        }
    }

    void widget_util::InitializeComboBox(QWidget* parent, ArrowIconComboBox* combo_box, const std::vector<std::string>& string_list)
    {
        InitSingleSelectComboBox(parent, combo_box, "", false);
        combo_box->ClearItems();
        for (auto it = string_list.begin(); it != string_list.end(); ++it)
        {
            combo_box->AddItem((*it).c_str());
        }
        combo_box->SetSelectedRow(0);
        combo_box->SetMaximumHeight(400);
    }

    void widget_util::RePopulateComboBox(ArrowIconComboBox* combo_box, const std::vector<std::string>& string_list)
    {
        combo_box->ClearItems();
        for (auto it = string_list.begin(); it != string_list.end(); ++it)
        {
            combo_box->AddItem((*it).c_str());
        }
    }

    void widget_util::DrawCheckboxCell(QPainter* painter, const QRectF& cell_rect, bool checked, bool center)
    {
        const static int    kCheckBoxBoxSize = 14;
        const static QColor kCheckBoxCheckedColor(42, 42, 42);
        const static QColor kCheckBoxUncheckedColor(224, 224, 224);
        const static QColor kCheckBoxCheckColor(255, 255, 255);

        const int scaled_check_box_size = kCheckBoxBoxSize;
        const int half_size             = scaled_check_box_size / 2;

        // Center the checkbox by finding out the width of the column, then subtract half the checkbox width
        // from the center of the column to allow for the checkbox width.
        const int   column_center = center ? (cell_rect.width() / 2) - half_size : cell_rect.width() - scaled_check_box_size;
        const QRect rect(cell_rect.x() + column_center, cell_rect.center().y() - half_size, scaled_check_box_size, scaled_check_box_size);

        // Turn on Antialiasing.
        painter->setRenderHint(QPainter::Antialiasing, true);

        // Draw box.
        if (checked)
        {
            painter->fillRect(rect, kCheckBoxCheckedColor);

            // Check points.
            QPointF p1(rect.left() + rect.width() * 0.15, rect.top() + rect.height() * 0.55);
            QPointF p2(rect.left() + rect.width() * 0.35, rect.top() + rect.height() * 0.75);
            QPointF p3(rect.left() + rect.width() * 0.85, rect.top() + rect.height() * 0.25);

            // Draw check.
            QPen temp = painter->pen();
            QPen check_pen(kCheckBoxCheckColor);
            check_pen.setWidthF(1.2);
            painter->setPen(check_pen);
            painter->drawLine(p1, p2);
            painter->drawLine(p2, p3);
            painter->setPen(temp);
        }
        else
        {
            painter->fillRect(rect, kCheckBoxUncheckedColor);
        }

        // Turn off Antialiasing.
        painter->setRenderHint(QPainter::Antialiasing, false);
    }

    void widget_util::SetTableModelData(QStandardItemModel* model, const QString& data, uint row, uint column, enum Qt::AlignmentFlag alignment)
    {
        model->setData(model->index(row, column), QString(data));
        model->setData(model->index(row, column), (int)alignment | Qt::AlignVCenter, Qt::TextAlignmentRole);
    }

    void widget_util::SetTableModelDecimalData(QStandardItemModel* model, float data, uint row, uint column, enum Qt::AlignmentFlag alignment)
    {
        int decimal_precision = rra::Settings::Get().GetDecimalPrecision();

        model->setData(model->index(row, column), QString::number(data, kQtFloatFormat, decimal_precision));
        model->setData(model->index(row, column), (int)alignment | Qt::AlignVCenter, Qt::TextAlignmentRole);
        model->setData(model->index(row, column), QString::number(data, kQtFloatFormat, kQtTooltipFloatPrecision), Qt::ToolTipRole);
    }

}  // namespace rra

