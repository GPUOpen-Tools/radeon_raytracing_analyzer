//=============================================================================
/// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Header file for an acceleration structure tree view.
///
/// Based on a scaled treeview, it adds a right-click context menu.
//=============================================================================

#ifndef RRA_VIEWS_ACCELERATION_STRUCTURE_TREE_VIEW_H_
#define RRA_VIEWS_ACCELERATION_STRUCTURE_TREE_VIEW_H_

#include "qt_common/custom_widgets/scaled_tree_view.h"

#include <QTreeView>

#include "models/acceleration_structure_viewer_model.h"

/// This class implements a Tree View specific to an acceleration structure.
class AccelerationStructureTreeView : public ScaledTreeView
{
    Q_OBJECT

public:
    /// Explicit constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit AccelerationStructureTreeView(QWidget* parent = nullptr);

    /// @brief Set the viewer model.
    ///
    /// @param [in] model The viewer model to set.
    /// @param [in] current_bvh_index The current bvh index.
    void SetViewerModel(rra::AccelerationStructureViewerModel* model, uint64_t current_bvh_index);

    /// Destructor.
    virtual ~AccelerationStructureTreeView();

    /// @brief Create a context menu to allow the user control over what they want to visualize in the scene.
    ///
    /// @param [in] event The event.
    virtual void contextMenuEvent(QContextMenuEvent* event) Q_DECL_OVERRIDE;

protected:
    /// @brief Override the widget event handler.
    ///
    /// @param [in] event The event data.
    ///
    /// \returns True if the event was recognized and handled, and false otherwise.
    bool event(QEvent* event) Q_DECL_OVERRIDE;

private:
    rra::AccelerationStructureViewerModel* model_             = nullptr;  ///< The viewer model.
    uint64_t                               current_bvh_index_ = 0;        ///< The current bvh index.
};

#endif  // RRA_VIEWS_ACCELERATION_STRUCTURE_TREE_VIEW_H_
