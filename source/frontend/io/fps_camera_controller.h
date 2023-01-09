//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the fps style camera controller.
//=============================================================================

#ifndef RRA_IO_FPS_CAMERA_CONTROLLER_H_
#define RRA_IO_FPS_CAMERA_CONTROLLER_H_

#include "viewer_io.h"
#include <map>

namespace rra
{
    class FPSController : public ViewerIO
    {
    public:
        /// @brief Constructor.
        FPSController();

        /// @brief Destructor.
        ~FPSController();

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

        /// @brief Get the index determining the order this control style will appear in the combo box.
        ///
        /// @returns Index into the combo box.
        virtual uint32_t GetComboBoxIndex() const override;

    private:
        float movement_speed_scroll_multiplier_ = 1.0f;  ///< The movement multiplier for the camera speed.
    };
}  // namespace rra

#endif
