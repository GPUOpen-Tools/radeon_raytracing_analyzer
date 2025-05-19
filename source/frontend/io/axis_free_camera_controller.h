//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the axis-free style camera controller.
//=============================================================================

#ifndef RRA_IO_AXIS_FREE_CAMERA_CONTROLLER_H_
#define RRA_IO_AXIS_FREE_CAMERA_CONTROLLER_H_

#include <map>

#include "io/viewer_io.h"

namespace rra
{
    class AxisFreeController : public ViewerIO
    {
    public:
        /// @brief Constructor.
        AxisFreeController();

        /// @brief Destructor.
        ~AxisFreeController();

        /// @brief Get the name of this controller.
        ///
        /// @returns The name of this controller.
        const std::string& GetName() const override;

        /// @brief Reset the controller. Wipe internal state.
        void Reset() override;

        /// @brief Key pressed callback for Qt based camera controller.
        ///
        /// @param [in] key The key that the user pressed on.
        void KeyPressed(Qt::Key key) override;

        /// @brief Key released callback for Qt based camera controller.
        ///
        /// @param [in] key The key that the user released.
        void KeyReleased(Qt::Key key) override;

        /// @brief This is called when the mouse is moved.
        ///
        /// @param [in] pos The cursor new position.
        void MouseMoved(QPoint pos) override;

        /// @brief This is called when the mouse wheel is moved.
        ///
        /// @param [in] wheel_event The mouse wheel move event.
        void MouseWheelMoved(QWheelEvent* wheel_event) override;

        /// @brief Reset the camera position.
        virtual void ResetCameraPosition() override;

        /// @brief Reset the camera's arc radius without changing the camera's position.
        virtual void ResetArcRadius() override;

        /// @brief Focus camera on volume.
        ///
        /// @param [in] camera The camera to focus.
        /// @param [in] volume The volume to focus on.
        ///
        /// @returns The radius of the volume to be used for various camera attributes.
        virtual float FocusCameraOnVolume(renderer::Camera* camera, BoundingVolumeExtents volume) override;

        /// @brief Generate a readable string containing internal state of camera controller.
        ///
        /// Intended for copy/paste functionality of camera's transform.
        /// @returns A readable string describing state of camera controller.
        virtual std::string GetReadableString() const override;

        /// @brief Process the user inputs to manipulate the camera.
        void ProcessUserInputs() override;

        /// @brief Update the control hotkeys on a widget.
        ///
        /// @param [in] widget The widget to update.
        virtual void UpdateControlHotkeys(QWidget* widget) override;

        /// @brief Rotate the camera such that its new forward vector matches the forward parameter.
        ///
        /// @param [in] forward The camera's new forward vector.
        virtual void SetRotationFromForward(glm::vec3 forward) override;

        /// @brief Whether or not this controller has support for orthographic projection.
        ///
        /// @returns True if orthographic projection is supported, false otherwise.
        virtual bool SupportsOrthographicProjection() const override;

        /// @brief Whether or not this controller has support for setting an up axis.
        ///
        /// @returns True if an up axis is supported, false otherwise.
        virtual bool SupportsUpAxis() const override;

        /// @brief Fit to the given camera params.
        ///
        /// @param [in] position The absolute camera position.
        /// @param [in] forward The camera forward direction.
        /// @param [in] up The camera up direction.
        /// @param [in] fov The field of view of the camera.
        /// @param [in] speed The speed of the camera.
        virtual void FitCameraParams(glm::vec3            position,
                                     glm::vec3            forward,
                                     glm::vec3            up,
                                     std::optional<float> opt_fov   = std::nullopt,
                                     std::optional<float> opt_speed = std::nullopt) override;

        /// @brief Pitch the camera by the given angle.
        ///
        /// @param [in] angle The angle to pitch by.
        void Pitch(float angle);

        /// @brief Yaw the camera by the given angle.
        ///
        /// @param [in] angle The angle to yaw by.
        void Yaw(float angle);

        /// @brief Roll the camera by the given angle.
        ///
        /// @param [in] angle The angle to roll by.
        void Roll(float angle);

        /// @brief Get the reflection matrix accounting for flipping vertical/horizontal.
        ///
        /// There is a separate implementation of this in ViewerIOOrientation but the axis-free
        /// controller requires its own implementation.
        ///
        /// @returns Matrix which reflects about axes depending on flip horizontal/vertical settings.
        glm::mat3 GetReflectionMatrix() const;

        /// @brief Get the index determining the order this control style will appear in the combo box.
        ///
        /// @returns Index into the combo box.
        virtual uint32_t GetComboBoxIndex() const override;

        /// @brief Called when the user changes control styles.
        virtual void ControlStyleChanged() override;

    protected:
        /// @brief After parsing the readable string, this function will update the state of the ViewerIO.
        ///
        /// @param state The parsed readable string.
        virtual void UpdateFromReadableStringState(const ReadableStringState& state) override;

    private:
        float     movement_speed_scroll_multiplier_ = 1.0f;  ///< The movement multiplier for the camera speed.
        glm::vec3 pitch_yaw_roll_{};                         ///< The pitch, yaw, and roll to rotate the camera for this frame.

        // We use a separate rotation matrix than the Camera's since yaw/pitch/roll change rotation relative to last frame
        // instead of representing absolute orientation like euler angles. This makes it easier to apply transforms like flip horizontal
        // by retaining this untransformed rotation matrix.
        glm::mat4 rotation_{1.0f};  ///< Rotation that the camera gets updated with each frame.
    };
}  // namespace rra

#endif

