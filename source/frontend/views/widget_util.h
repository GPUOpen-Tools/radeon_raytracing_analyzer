//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of a number of widget utilities.
///
/// These functions apply a common look and feel to various widget types.
///
//=============================================================================

#ifndef RRA_UTIL_WIDGET_UTIL_H_
#define RRA_UTIL_WIDGET_UTIL_H_

#include <QWidget>
#include <QString>
#include <QGraphicsView>
#include <QTableView>
#include <QStandardItemModel>
#include <stdint.h>

#include "qt_common/custom_widgets/text_search_widget.h"
#include "qt_common/custom_widgets/double_slider_widget.h"
#include "qt_common/custom_widgets/arrow_icon_combo_box.h"
#include "qt_common/custom_widgets/colored_legend_scene.h"

namespace rra
{
    namespace widget_util
    {
        /// @brief Apply standard styling for a given top level pane's scroll area.
        ///
        /// @param [in] scroll_area  The scroll area.
        void ApplyStandardPaneStyle(QScrollArea* scroll_area);

        /// @brief Initialize an ArrowIconComboBox for single selection.
        ///
        /// @param [in]     parent              The combo box parent.
        /// @param [in,out] combo_box           The combo box to initialize.
        /// @param [in]     default_text        The default text string.
        /// @param [in]     retain_default_text If true, the combo box text doesn't change to the selected
        ///  item text (in the case of combo boxes requiring check boxes).
        /// @param [in]     prefix_text         The text used to prefix the default text.
        void InitSingleSelectComboBox(QWidget*           parent,
                                      ArrowIconComboBox* combo_box,
                                      const QString&     default_text,
                                      bool               retain_default_text,
                                      const QString      prefix_text = "");

        /// @brief Initialize an ArrowIconComboBox with some text.
        ///
        /// @param [in] parent      The combo box parent.
        /// @param [in] combo_box   The combo box to initialize.
        /// @param [in] string_list The list of strings to add to the combo box.
        void InitializeComboBox(QWidget* parent, ArrowIconComboBox* combo_box, const std::vector<std::string>& string_list);

        /// @brief Repopulate an ArrowIconComboBox with new text.
        ///
        /// @param [in] combo_box   The combo box to repopulate.
        /// @param [in] string_list The list of strings to add to the combo box.
        void RePopulateComboBox(ArrowIconComboBox* combo_box, const std::vector<std::string>& string_list);

        /// @brief Checkbox cell paint function.
        ///
        /// @param [in] painter     The painter object to use.
        /// @param [in] cell_rect   Bounding box of this cell (use this for alignment within the cell).
        /// @param [in] checked     Whether the checkbox is checked.
        /// @param [in] center      Should the checkbox be center-aligned. If false, use left-alignment.
        void DrawCheckboxCell(QPainter* painter, const QRectF& cell_rect, bool checked, bool center);

        /// @brief Helper function to set a cell in the specified table model.
        ///
        /// @param [in] model     Pointer to the model.
        /// @param [in] data      The view to the table.
        /// @param [in] row       The destination row.
        /// @param [in] column    The destination column.
        /// @param [in] alignment The data alignment.
        void SetTableModelData(QStandardItemModel* model, const QString& data, uint row, uint column, enum Qt::AlignmentFlag alignment = Qt::AlignLeft);

        /// @brief Helper function to set a decimal cell in the specified table model.
        ///
        /// @param [in] model     Pointer to the model.
        /// @param [in] data      The decimal value.
        /// @param [in] row       The destination row.
        /// @param [in] column    The destination column.
        /// @param [in] alignment The data alignment.
        void SetTableModelDecimalData(QStandardItemModel* model, float data, uint row, uint column, enum Qt::AlignmentFlag alignment);

    }  // namespace widget_util
}  // namespace rra

#endif  // RRA_UTIL_WIDGET_UTIL_H_
