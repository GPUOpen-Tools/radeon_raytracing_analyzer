//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the Side pane container class.
//=============================================================================

#ifndef RRA_VIEWS_SIDE_PANELS_SIDE_PANE_CONTAINER_H_
#define RRA_VIEWS_SIDE_PANELS_SIDE_PANE_CONTAINER_H_

#include "ui_side_pane_container.h"

#include "views/side_panels/view_pane.h"

/// @brief Class declaration.
class SidePaneContainer : public QWidget
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit SidePaneContainer(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~SidePaneContainer();

    /// @brief Connect the incoming map of RendererAdapter instances with the side panel view models.
    ///
    /// @param [in] adapters A renderer adapter map used to alter various render states.
    void SetRendererAdapters(const rra::renderer::RendererAdapterMap& adapters);

    /// @brief Handle what happens when a trace is opened.
    void OnTraceOpen();

    /// @brief Get the view pane.
    ///
    /// @returns The view pane.
    ViewPane* GetViewPane() const;

    /// @brief  Notify the side pane container that the BLAS viewer mode is active.
    void MarkAsBLAS();

    /// @brief  Mark a BLAS as containing or not containing procedural primitives.
    ///
    /// @param [in] is_procedural True if procedural, false otherwise.
    void MarkProceduralGeometry(bool is_procedural);

private:
    /// @brief Update the side pane state.
    ///
    /// If a user clicks a side pane button, ensure that the correct side pane
    /// is shown. Also handle the pane toggling if a user re-clicks the same
    /// button.
    ///
    /// @param [in] pane_index The index of now active side pane.
    void UpdateSidePane(int pane_index);

    Ui::SidePaneContainer* ui_        = nullptr;  ///< Pointer to the Qt UI design.
    ViewPane*              view_pane_ = nullptr;  ///< The camera controls side panel.

    int side_pane_index_;  ///< Index of side pane currently selected.
};

#endif  // RRA_VIEWS_BLAS_BLAS_VIEWER_PANE_H_
