//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the ray inspector ray list item model.
///
//=============================================================================

#ifndef RRA_MODELS_RAY_INSPECTOR_RAY_TREE_ITEM_H_
#define RRA_MODELS_RAY_INSPECTOR_RAY_TREE_ITEM_H_

#include <optional>

#include <QAbstractItemModel>

#include "qt_common/custom_widgets/scaled_tree_view.h"

#include "public/rra_ray_history.h"

namespace rra
{
    struct RayInspectorRayTreeItemData
    {
        uint32_t row_index;
        uint32_t dynamic_id;
        uint32_t parent_id;
        uint32_t ray_event_count;
        uint32_t ray_instance_intersections;
        uint32_t ray_any_hit_invocations;
        bool     hit;

        std::vector<std::shared_ptr<RayInspectorRayTreeItemData>> child_rays;
    };

    using RayTreeItemList = std::vector<std::shared_ptr<RayInspectorRayTreeItemData>>;

    /// @brief Column Id's for the fields in the ray list.
    enum RayInspectorRayTreeColumn
    {
        kRayInspectorRayIndexColumn,
        kRayInspectorRayListColumnTraversalLoopCount,
        kRayInspectorRayListColumnInstanceIntersections,
        kRayInspectorRayListColumnAnyHitInvocations,
        kRayInspectorRayListColumnHit,

        kRayInspectorRayListColumnCount,
    };

    /// @brief A class to handle the model data associated with ray list table.
    class RayInspectorRayTreeItem
    {
    public:
        /// @brief Constructor.
        explicit RayInspectorRayTreeItem();

        /// @brief Destructor.
        virtual ~RayInspectorRayTreeItem();

        /// @brief Add new rays to the tree.
        ///
        /// @param [in] item_list  The item list to add to the tree.
        void AddNewRays(RayTreeItemList item_list);

        /// @brief Clears all the rays.
        void ClearRays();

        /// @brief Finds the ray by its index.
        ///
        /// @param ray_index.
        ///
        /// @return A model index.
        QModelIndex FindRay(int ray_index);

        // QAbstractItemModel overrides. See Qt documentation for parameter and return values
        QVariant                 Data(int role, int column);
        RayInspectorRayTreeItem* Parent();
        int                      Row() const;
        int                      ChildCount() const;
        int                      ColumnCount() const;
        RayInspectorRayTreeItem* Child(int row);

    private:
        QList<RayInspectorRayTreeItem*> child_trees_;       ///< List of child trees.
        RayInspectorRayTreeItem*        parent_ = nullptr;  ///< Parent model.

        std::shared_ptr<RayInspectorRayTreeItemData> self_ray_ = nullptr;  ///< Stats of the ray this item contains if there is one.
    };
}  // namespace rra

#endif  // RRA_MODELS_RAY_INSPECTOR_RAY_TREE_ITEM_H_

