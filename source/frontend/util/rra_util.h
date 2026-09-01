//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for rra_util which holds useful utility functions.
//=============================================================================

#ifndef RRA_UTIL_RRA_UTIL_H_
#define RRA_UTIL_RRA_UTIL_H_

#include <QColor>
#include <QTableView>

#include "managers/pane_manager.h"

namespace rra_util
{
    /// @brief Get the text color that works best displayed on top of a given background color.
    ///
    /// Make the light color off-white so it can be seen against the white background.
    ///
    /// @param [in] background_color     The color that the text is to be displayed on top of.
    /// @param [in] has_white_background Is the text drawn to a mainly white background?
    ///  This will be the case for small objects that need coloring where the text
    ///  extends onto the background. If this is the case and the text color needs to
    ///  be light to contrast against a darker color, use a light gray rather than white.
    ///
    /// @return The text color, either black or white/light gray.
    QColor GetTextColorForBackground(const QColor& background_color, bool has_white_background = false);

    /// @brief Return whether a trace may be loaded.
    ///
    /// @param [in] trace_path The path to the trace.
    ///
    /// @return true if we may attempt an actual trace load, false otherwise.
    bool TraceValidToLoad(const QString& trace_path);

    /// @brief Initialize a tableview with parameters so that all tables have the same look and feel.
    ///
    /// @param [in] table  The table to be initialized.
    void InitializeTableView(QTableView* table);

    /// @brief Update the tooltip in the renderer widget.
    ///
    /// @param [in] pane_id          The pane ID of the renderer widget.
    /// @param [in] renderer_widget  Pointer to the renderer widget.
    /// @param [in] tool_tip_string  The tooltip text to be displayed.
    void UpdateRendererTooltip(rra::RRAPaneId pane_id, QWidget* renderer_widget, const QString& tool_tip_string);
};  // namespace rra_util

#endif  // RRA_UTIL_RRA_UTIL_H_

