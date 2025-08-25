//=============================================================================
// Copyright (c) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for a tlas pane.
//=============================================================================

#ifndef RRA_VIEWS_OVERVIEW_TLAS_PANE_H_
#define RRA_VIEWS_OVERVIEW_TLAS_PANE_H_

#include "ui_tlas_pane.h"

#include "views/overview/summary_pane.h"

/// @brief Class declaration.
class TlasPane : public QWidget
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent Pointer to the parent widget.
    explicit TlasPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~TlasPane();

    /// @brief Overridden paintEvent method.
    ///
    /// @param event The paint event object
    void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;

    /// @brief Set the statistics for this TLAS pane.
    ///
    /// @param [in] summary_pane        Pointer to the (parent) summary_pane.
    /// @param [in] tlas                Reference to the TLAS statistics.
    /// @param [in] empty               Is the TLAS empty.
    /// @param [in] rebraiding_enabled  Is rebraiding enabled for this TLAS.
    void SetTlasStats(SummaryPane* summary_pane, const rra::TlasListStatistics& tlas, bool empty, bool rebraiding_enabled);

private:
    Ui_TlasPane* ui_;  ///< The Qt Instance of this object.
};

#endif  // RRA_VIEWS_OVERVIEW_TLAS_PANE_H_
