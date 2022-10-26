//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for orientation gizmo.
//=============================================================================

#ifndef RRA_ORIENTATION_GIZMO_H_
#define RRA_ORIENTATION_GIZMO_H_

#include <vector>
#include "glm/glm/glm.hpp"

namespace rra
{
    namespace renderer
    {
        /// @brief Describes which, if any, orientation gizmo element was clicked.
        enum class OrientationGizmoHitType
        {
            kX,
            kY,
            kZ,
            kMinusX,
            kMinusY,
            kMinusZ,
            kBackground,
            kNone,
        };

        /// @brief Holds transform info of each orientation element, and their draw order.
        struct OrientationGizmoTransformInfo
        {
            float circle_radius;      ///< Radius of labels.
            float background_radius;  ///< Radius of transparent background circle.

            glm::mat4 x_axis_transform;  ///< Transform of x-axis.
            glm::mat4 y_axis_transform;  ///< Transform of y-axis.
            glm::mat4 z_axis_transform;  ///< Transform of z-axis.

            glm::mat4 x_circle_transform;        ///< Transform of x label.
            glm::mat4 y_circle_transform;        ///< Transform of y label.
            glm::mat4 z_circle_transform;        ///< Transform of z label.
            glm::mat4 minus_x_circle_transform;  ///< Transform of negative x label.
            glm::mat4 minus_y_circle_transform;  ///< Transform of negative y label.
            glm::mat4 minus_z_circle_transform;  ///<  Transform of negative z label.

            glm::mat4 background_transform;  ///< Transform of background circle.
            glm::mat4 screen_transform;      ///< Transform of all elements to move to top right and correct aspect.

            std::vector<int> order;  ///< Draw order of elements.
        };

        /// @brief Return orientation of each gizmo element and draw order.
        /// 
        /// @param [in] rotation Camera rotation.
        /// @param [in] window_ratio The viewport's width / height.
        /// @returns Transform info.
        OrientationGizmoTransformInfo GetOrientationGizmoTransformInfo(glm::mat4 rotation, float window_ratio);

        /// @brief Get the closest orientation gizmo element that the mouse is over.
        /// 
        /// @param [in] rotation Camera rotation.
        /// @param [in] window_ratio The viewport's width / height.
        /// @param [in] hit_coords Normalized mouse coordinates.
        /// @returns Enum describing what element, if any, was hit.
        OrientationGizmoHitType CheckOrientationGizmoHit(glm::mat4 rotation, float window_ratio, glm::vec2 hit_coords);

        /// @brief Convert gizmo hit into a forward vector to assign to the camera.
        /// 
        /// @param [in] gizmo_hit The element that was clicked on the orientation gizmo.
        /// @returns Camera's new forward vector.
        glm::vec3 GetForwardFromGizmoHit(OrientationGizmoHitType gizmo_hit);

        /// @brief Set the element of the gizmo that the mouse is currently hovering over.
        /// 
        /// @param [in] gizmo_hit The orientation gizmo element that should be highlighted.
        void SetOrientationGizmoSelected(OrientationGizmoHitType gizmo_hit);

    }  // namespace renderer

}  // namespace rra

#endif
