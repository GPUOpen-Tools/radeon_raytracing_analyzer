//=============================================================================
/// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Header file for a ray structure tree view.
///
/// Based on a scaled treeview, it adds a right-click context menu.
//=============================================================================

#ifndef RRA_VIEWS_RAY_INSPECTOR_TREE_VIEW_H_
#define RRA_VIEWS_RAY_INSPECTOR_TREE_VIEW_H_

#include "qt_common/custom_widgets/scaled_tree_view.h"

#include <QTreeView>

#include "models/ray/ray_inspector_model.h"

/// This class implements a Tree View specific to an acceleration structure.
class RayInspectorTreeView : public ScaledTreeView
{
    Q_OBJECT

public:
    /// Explicit constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit RayInspectorTreeView(QWidget* parent = nullptr);

    /// @brief Set the viewer model.
    ///
    /// @param [in] model The viewer model to set.
    /// @param [in] current_bvh_index The current bvh index.
    void SetViewerModel(rra::RayInspectorModel* model, uint64_t current_bvh_index);

    /// @brief Set focus on selected ray function.
    ///
    /// @param [in] focus_on_selected_ray The lambda function
    void SetFocusOnSelectedRayCallback(std::function<void()> focus_on_selected_ray);

    /// @brief Set focus on selected ray function.
    ///
    /// @param [in] focus_on_selected_ray The lambda function
    void SetResetSceneCallback(std::function<void()> reset_scene);

    /// Destructor.
    virtual ~RayInspectorTreeView();

protected:
    /// @brief Override the widget event handler.
    ///
    /// @param [in] event The event data.
    ///
    /// \returns True if the event was recognized and handled, and false otherwise.
    bool event(QEvent* event) Q_DECL_OVERRIDE;

private:
    rra::RayInspectorModel* model_                 = nullptr;  ///< The viewer model.
    uint64_t                current_bvh_index_     = 0;        ///< The current bvh index.
    std::function<void()>   focus_on_selected_ray_ = nullptr;  ///< A lamdda callback to focus on selected ray.
    std::function<void()>   reset_scene_           = nullptr;  ///< A lamdda callback to reset scene.
};

#endif  // RRA_VIEWS_ACCELERATION_STRUCTURE_TREE_VIEW_H_
