//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the MessageManager.
///
/// The message manager is used to send messages between panes and allow
/// for broadcasting of UI events. For example, if a resource is selected
/// in one pane, any interested panes can set up a connection for the
/// ResourceSelected signal and respond to it.
/// NOTE: The message manager should be used sparingly; if there is a direct
/// connection possible between a signal and slot, that should be used.
///
//=============================================================================

#ifndef RRA_MANAGERS_MESSAGE_MANAGER_H_
#define RRA_MANAGERS_MESSAGE_MANAGER_H_

#include <QObject>

#include "managers/pane_manager.h"
#include "glm/glm/glm.hpp"

namespace rra
{
    /// @brief Class that allows communication between any custom QObjects.
    class MessageManager : public QObject
    {
        Q_OBJECT

    public:
        /// @brief Accessor for singleton instance.
        ///
        /// @return A reference to the message manager.
        static MessageManager& Get();

    signals:
        /// @brief Signal to open a trace via a file menu.
        void OpenTraceFileMenuClicked();

        /// @brief Something changed the file list (either a delete or a new file added).
        void RecentFileListChanged();

        /// @brief Signal to navigate to a specific pane.
        ///
        /// @param [in] pane The pane to navigate to.
        void PaneSwitchRequested(rra::RRAPaneId pane);

        /// @brief Signal to indicate the user has clicked on a BLAS (in the blas list or in the tree).
        ///
        /// @param [in] blas_index The index of the BLAS clicked on.
        void BlasSelected(uint64_t blas_index);

        /// @brief Signal to indicate the user has clicked on an instance (in the tree).
        ///
        /// @param [in] instance_index The index of the instance clicked on.
        void InstanceSelected(uint32_t instance_index);

        /// @brief Signal to indicate the user has clicked on an instance in the inspector pane.
        ///
        /// @param [in] instance_index The index of the instance clicked on.
        void InspectorInstanceSelected(uint32_t instance_index);

        /// @brief Signal to indicate the user has double-clicked on an instance on the instances pane.
        ///
        /// @param [in] tlas_index The index of the selected TLAS.
        /// @param [in] blas_index The index of the selected BLAS.
        /// @param [in] instance_index The index of the selected BLAS instance.
        void InstancesTableDoubleClicked(uint64_t tlas_index, uint64_t blas_index, uint64_t instance_index);

        /// @brief Signal to indicate that the user has selected a TLAS.
        ///
        /// Selection can be done from the combo box in the TLAS viewer pane or the TLAS table in the Summary pane.
        ///
        /// @param [in] index The index of the TLAS selected.
        void TlasSelected(uint64_t index);

        /// @brief Signal to hint TLAS viewer to assume new camera parameters.
        ///
        /// @param [in] origin The origin to place the camera.
        /// @param [in] forward The direction of the camera
        /// @param [in] up The up axis of the camera.
        void TlasAssumeCamera(glm::vec3 origin, glm::vec3 forward, glm::vec3 up);

        /// @brief Signal to indicate that the user has selected a triangle in the BLAS viewer.
        ///
        /// @param [in] triangle_node_id The triangle node id selected.
        void TriangleViewerSelected(uint32_t triangle_node_id);

        /// @brief Signal to indicate that the user has selected a triangle in the Triangles table.
        ///
        /// @param [in] triangle_node_id The triangle node id selected.
        void TriangleTableSelected(uint32_t triangle_node_id);

        /// @brief Signal to indicate the render state has changed.
        ///
        /// Emitted if a hotkey was pressed to toggle something in the renderer. Any UI widgets that set
        /// up render state can look for this signal and update themselves with the new render state.
        void RenderStateChanged();

        /// @brief Signal to indicate that the traversal counter slider needs updating.
        ///
        /// Emitted if a hotkey was pressed to update the traversal counter range.
        void TraversalSliderChanged(uint32_t min, uint32_t max);

        /// @brief Signal to indicate the graphics context failed to initialize.
        ///
        /// @param [in] failure_message The failure message.
        void GraphicsContextFailedToInitialize(const QString& failure_message);

        /// @brief Signal to indicate that a dispatch has been selected in the overview pane.
        ///
        /// @param [in] dispatch_id The trace rays dispatch ID.
        void DispatchSelected(uint32_t dispatch_id);

        /// @brief Signal to indicate that a ray has been selected in the ray history pane.
        ///
        /// @param [in] dispatch_id The dispatch ID.
        /// @param [in] x           The x-component of the dispatch coordinate.
        /// @param [in] y           The y-component of the dispatch coordinate.
        /// @param [in] z           The z-component of the dispatch coordinate.
        void RayCoordinateSelected(uint32_t dispatch_id, uint32_t x, uint32_t y, uint32_t z);

        /// @brief Signal to reset the UI to its default state.
        ///
        /// @param [in] pane The pane whose UI state is to be reset.
        void ResetUIState(rra::RRAPaneId pane);
    };
}  // namespace rra

#endif  // RRA_MANAGERS_MESSAGE_MANAGER_H_
