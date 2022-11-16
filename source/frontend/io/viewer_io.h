//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration to qt based user input controller interface.
//=============================================================================

#ifndef RRA_IO_VIEWER_IO_H_
#define RRA_IO_VIEWER_IO_H_

#include <QWidget>
#include "public/camera.h"
#include "models/acceleration_structure_viewer_model.h"

namespace rra
{
    const float  kViewerIOFarPlaneMultiplier = 100.0f;          // The far plane radius is multiplied by this value.
    const QPoint kInvalidPosition            = QPoint(-1, -1);  ///< A point representing an invalid position.

    class ViewModel;  ///< Forward declaration to manipulate camera orientation from the controller.

    /// @brief Enumeration for up axis.
    enum class ViewerIOUpAxis
    {
        kUpAxisX = 0,
        kUpAxisY = 1,
        kUpAxisZ = 2,
    };

    /// @brief Enumeration for result code when pasting camera.
    enum class ViewerIOCameraPasteResult
    {
        kSuccess,
        kFailure,
        kOrthographicNotSupported,
    };

    /// @brief A structure to contain camera orientation data.
    struct ViewerIOOrientation
    {
        ViewerIOUpAxis up_axis         = ViewerIOUpAxis::kUpAxisY;
        bool           flip_vertical   = false;
        bool           flip_horizontal = false;

        /// @brief Confines the axes to intuitive planes depending on the orientation.
        ///
        /// @param [out] euler The euler angle in xyz.
        ///
        /// @returns The confined euler angles.
        glm::vec3 Confine(glm::vec3 euler) const;

        /// @brief Get unmapped euler values by the given forward vector.
        ///
        /// @param [in] forward The forward direction.
        ///
        /// @returns The unmapped euler angles.
        glm::vec3 GetEulerByForward(glm::vec3 forward) const;

        /// @brief Maps the given euler angles from xyz to this orientation.
        ///
        /// @param [inout] euler The euler angle in xyz.
        /// Depending on the up axis, the X axis of the input is dampened to prevent a vertical flip.
        ///
        /// @returns The converted euler angles.
        glm::vec3 MapEuler(glm::vec3 euler) const;

        /// @brief Returns the reflection matrix depending on inversion params.
        ///
        /// @returns The reflection matrix.
        glm::mat3 GetReflectionMatrix() const;

        /// @brief Get default euler rotation for the current orientation.
        ///
        /// @returns euler angles for the current orientation.
        glm::vec3 GetDefaultEuler() const;

        /// @brief Get control mapping.
        ///
        /// @returns The control mapping.
        glm::vec3 GetControlMapping() const;
    };

    /// @brief Camera controller handling IO.
    ///
    /// We make this a subclass of CameraController to maintain separation of renderer
    /// and Qt code, since ViewerIO uses some Qt classes.
    class ViewerIO : public renderer::CameraController
    {
    public:
        /// @brief Get the name of this controller.
        ///
        /// @returns The name of this controller.
        virtual const std::string& GetName() const = 0;

        /// @brief Reset the controller. Wipe internal state.
        virtual void Reset() = 0;

        /// @brief Key pressed callback for Qt based camera controller.
        ///
        /// @param [in] key The key that the user pressed on.
        virtual void KeyPressed(Qt::Key key) = 0;

        /// @brief Key released callback for Qt based camera controller.
        ///
        /// @param [in] key The key that the user released.
        virtual void KeyReleased(Qt::Key key) = 0;

        /// @brief This is called when a mouse button is pressed.
        ///
        /// @param [in] mouse_event The mouse button press event data.
        void MousePressed(QMouseEvent* mouse_event);

        /// @brief This signal is emitted when a mouse button is double clicked within the widget.
        ///
        /// @param [in] mouse_event The mouse button release event data.
        /// @param [in] window_size The window size to calculate normalized coords if needed.
        /// @param [in] camera The camera for matrix calculations.
        /// @param [in] cast_ray Whether a ray should be cast into the scene.
        ///
        /// @return A SceneCollectionModelClosestHit object describing the object under the mouse.
        SceneCollectionModelClosestHit MouseDoubleClicked(QMouseEvent* mouse_event, glm::vec2 window_size, const renderer::Camera* camera, bool cast_ray);

        /// @brief This is called when a mouse button is released.
        ///
        /// @param [in] mouse_event The mouse button release event data.
        /// @param [in] window_size The window size to calculate normalized coords if needed.
        /// @param [in] camera The camera for matrix calculations.
        /// @param [in] cast_ray Whether a ray should be cast into the scene.
        ///
        /// @return A SceneCollectionModelClosestHit object describing the object under the mouse.
        SceneCollectionModelClosestHit MouseReleased(QMouseEvent* mouse_event, glm::vec2 window_size, const renderer::Camera* camera, bool cast_ray);

        /// @brief This is called when the mouse is moved.
        ///
        /// @param [in] pos The cursor new position.
        virtual void MouseMoved(QPoint pos) = 0;

        /// @brief This is called when the mouse wheel is moved.
        ///
        /// @param [in] wheel_event The mouse wheel move event.
        virtual void MouseWheelMoved(QWheelEvent* wheel_event) = 0;

        /// @brief Reset the camera position.
        virtual void ResetCameraPosition() = 0;

        /// @brief Reset the camera's arc radius without changing the camera's position.
        virtual void ResetArcRadius() = 0;

        /// @brief Focus camera on volume.
        ///
        /// @param [in] camera The camera to focus.
        /// @param [in] volume The volume to focus on.
        ///
        /// @returns The radius of the volume to be used for various camera attributes.
        virtual float FocusCameraOnVolume(renderer::Camera* camera, BoundingVolumeExtents volume) = 0;

        /// @brief Generate a readable string containing internal state of camera controller.
        ///
        /// Intended for copy/paste functionality of camera's transform.
        /// @returns A readable string describing state of camera controller.
        virtual std::string GetReadableString() const;

        /// @brief Set the camera controller to the state it was in when ToReadableString() was called.
        ///
        /// Intended for copy/paste functionality of camera's transform.
        /// @param [in] readable_string A string returned from ToReadableString().
        /// @returns Result code indicating success or failure.
        virtual ViewerIOCameraPasteResult SetStateFromReadableString(const std::string& readable_string);

        /// @brief Set the viewer model.
        ///
        /// @param [in] viewer_model The viewer model to set.
        /// @param [in] bvh_index The bvh index to use alongside the model.
        void SetViewerModel(AccelerationStructureViewerModel* viewer_model, uint64_t bvh_index);

        /// @brief Set the view model.
        ///
        /// @param [in] view_model The view model to set.
        void SetViewModel(ViewModel* view_model);

        /// @brief Set the up axis by max cardinal dimension.
        ///
        /// @param [in] up The up axis to use.
        void SetViewModelUpByCardinalMax(glm::vec3 up);

        /// @brief Update the view model.
        void UpdateViewModel();

        /// @brief Set the camera orientation for this controller.
        ///
        /// @param [in] camera_orientation The camera orientation.
        void SetCameraOrientation(ViewerIOOrientation camera_orientation);

        /// @brief Get the camera orientation.
        ///
        /// @return The camera orientation.
        ViewerIOOrientation GetCameraOrientation() const;

        /// @brief Update the control hotkeys on a widget.
        ///
        /// @param [in] widget The widget to update.
        virtual void UpdateControlHotkeys(QWidget* widget) = 0;

        /// @brief Handles the context menu.
        ///
        /// @param [in] mouse_event The mouse event to arrange the menu with.
        /// @param [in] window_size The window size to translate mouse event.
        /// @param [in] camera The camera to shoot rays from.
        void HandleContextMenu(QMouseEvent* mouse_event, glm::vec2 window_size, const renderer::Camera* camera);

        /// @brief Fit to the given camera params.
        ///
        /// @param [in] position The absolute camera position.
        /// @param [in] forward The camera forward direction.
        /// @param [in] up The camera up direction.
        virtual void FitCameraParams(glm::vec3 position, glm::vec3 forward, glm::vec3 up);

        /// @brief Rotate the camera such that its new forward vector matches the forward parameter.
        ///
        /// @param [in] forward The camera's new forward vector.
        virtual void SetRotationFromForward(glm::vec3 forward) = 0;

        /// @brief Whether or not this controller has support for orthographic projection.
        ///
        /// @returns True if orthographic projection is supported, false otherwise.
        virtual bool SupportsOrthographicProjection() const = 0;

        /// @brief Whether or not this controller has support for setting an up axis.
        ///
        /// @returns True if an up axis is supported, false otherwise.
        virtual bool SupportsUpAxis() const = 0;

        /// @brief Get the index of the the current scene.
        ///
        /// @return Index of scene.
        uint64_t GetSceneIndex() const;

        /// @brief Set the last mouse position as invalid.
        void InvalidateLastMousePosition();

        /// @brief Get the threshold of considering a mouse movement dragging.
        ///
        /// @returns Pixel distance considered dragging.
        float GetMouseMoveDelta() const;

        /// @brief Move the camera controller to (0, 0, 0).
        void MoveToOrigin();

        /// @brief Focus on the selected objects.
        void FocusOnSelection();

        /// @brief Get the index determining the order this control style will appear in the combo box.
        ///
        /// @returns Index into the combo box.
        virtual uint32_t GetComboBoxIndex() const = 0;

        /// @brief Called when the user changes control styles.
        virtual void ControlStyleChanged();

    private:
        /// @brief Whether the mouse moved a small enough distance between press and release to cast a ray.
        ///
        /// @returns True if mouse moved less than delta, false otherwise.
        bool MouseMovedWithinDelta() const;

        /// @brief Calculate the closest hit scene object.
        ///
        /// @param [in] mouse_event The mouse button release event data.
        /// @param [in] window_size The window size to calculate normalized coords if needed.
        /// @param [in] camera The camera for matrix calculations.
        /// @param [out] SceneCollectionModelClosestHit object to receive the object under the mouse.
        void CalculateClosestHit(QMouseEvent*                    mouse_event,
                                 glm::vec2                       window_size,
                                 const renderer::Camera*         camera,
                                 SceneCollectionModelClosestHit& closest_hit) const;

    protected:
        /// @brief Create the widget layout for the control hotkeys.
        ///
        /// @param [in] widget The widget to update.
        /// @param [in] control_strings The list of control strings used to populate the layout.
        void CreateControlHotkeysLayout(QWidget* widget, const std::vector<std::pair<std::string, std::string>>& control_strings);

        std::map<Qt::Key, bool>               key_states_;                       ///< A map to contain key states.
        std::vector<Qt::Key>                  key_releases_;                     ///< A list to contain key releases.
        std::chrono::steady_clock::time_point elapsed_time_start_;               ///< Used to track elapsed time for processing user inputs.
        Qt::MouseButton last_mouse_button_pressed_ = Qt::MouseButton::NoButton;  ///< The last button press state to make decisions on mouse callbacks.

        glm::vec3                         pan_distance_              = {};       ///< The pan distance to track camera position.
        AccelerationStructureViewerModel* viewer_model_              = nullptr;  ///< The viewer model to manipulate.
        ViewModel*                        view_model_                = nullptr;  ///< The view model to update.
        uint64_t                          viewer_bvh_index_          = 0;        ///< The bvh index to manipulate scene model.
        ViewerIOOrientation               camera_orientation_        = {};       ///< The camera orientation.
        glm::vec3                         euler_angles_              = {};       ///< The euler angles to track camera rotation.
        float                             arc_radius_                = 0.0f;     ///< The arc radius to track camera anchor.
        bool                              updated_                   = false;    ///< A flag to check if the contoller was updated.
        bool                              should_focus_on_selection_ = false;    ///< A flag to keep determine if the controller should focus on selection.
        QPoint                            last_mouse_position_       = {};       ///< The last mouse position to keep track of displacement.
        QPoint                            mouse_press_position_      = {};       ///< Position the last time mouse was pressed down.
        float                             mouse_move_delta_          = 3.0f;  ///< Maximum distance mouse can move between press and release for ray to be cast.
    };
}  // namespace rra

#endif
