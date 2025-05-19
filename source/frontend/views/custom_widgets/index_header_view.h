//=============================================================================
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for a QTableHeader class which prevents header selection
/// on a specified column (usually column 0, the row index).
//=============================================================================

#ifndef RRA_VIEWS_CUSTOM_WIDGETS_INDEX_HEADER_VIEW_H_
#define RRA_VIEWS_CUSTOM_WIDGETS_INDEX_HEADER_VIEW_H_

#include <unordered_set>

#include <QHeaderView>
#include <QMouseEvent>

class IndexHeaderView : public QHeaderView
{
public:
    /// @brief Constructor.
    ///
    /// @param [in] non_selectable_index  The index of the column that can't be selected.
    /// @param [in] orientation           Whether this is a vertical or horizontal header.
    /// @param [in] parent                The parent widget.
    explicit IndexHeaderView(int non_selectable_index, Qt::Orientation orientation, QWidget* parent = nullptr);

    /// @brief Constructor.
    ///
    /// @param [in] non_selectable_indices  A set of column indices that can't be selected.
    /// @param [in] orientation             Whether this is a vertical or horizontal header.
    /// @param [in] parent                  The parent widget.
    explicit IndexHeaderView(std::unordered_set<int> non_selectable_indices, Qt::Orientation orientation, QWidget* parent = nullptr);

    /// @brief Implementation of Qt's mouse press event for this widget.
    ///
    /// @param [in] event The mouse press event.
    void mousePressEvent(QMouseEvent* event) override;

    /// @brief Implementation of Qt's mouse release event for this widget.
    ///
    /// @param [in] event The mouse press event.
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    std::unordered_set<int> non_selectable_index_;
};

#endif  // RRA_VIEWS_CUSTOM_WIDGETS_INDEX_HEADER_VIEW_H_

