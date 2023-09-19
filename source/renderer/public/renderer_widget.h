//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration of the Renderer Widget.
//=============================================================================

#ifndef RRA_RENDERER_RENDERER_WIDGET_H_
#define RRA_RENDERER_RENDERER_WIDGET_H_

#include <QWidget>
#include <QTimer>

#include "renderer_interface.h"

/// @brief The Renderer Widget class declaration.
///
/// The Renderer Widget is a custom widget used to display a rendered scene.
class RendererWidget : public QWidget
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget instance.
    RendererWidget(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~RendererWidget() = default;

    /// @brief Start the rendering loop within the widget.
    void Run();

    /// @brief Pause rendering new frames.
    void PauseFrames();

    /// @brief Resume rendering frames.
    void ContinueFrames();

    /// @brief Shut down and release all internal renderer resources.
    void Release();

    /// @brief Set the renderer interface used to draw new frames within the widget.
    ///
    /// @param [in] renderer_interface The renderer instance used to draw new frames within the widget.
    void SetRendererInterface(rra::renderer::RendererInterface* renderer_interface);

    /// @brief Query whether the 3D renderer is currently in focus.
    /// 
    /// @return True if the 3D renderer is in focus.
    bool GetRendererIsFocused();

    /// @brief Update swapchain size based on widget size.
    void UpdateSwapchainSize();

    /// @brief Overriden mouse leaving widget event.
    ///
    /// @param [in] event The leave event object.
    virtual void leaveEvent(QEvent* event) Q_DECL_OVERRIDE;

protected:
    /// @brief Override the widget event handler.
    ///
    /// @param [in] event The event data.
    ///
    /// \returns True if the event was recognized and handled, and false otherwise.
    bool event(QEvent* event) Q_DECL_OVERRIDE;

    /// @brief Override the widget show event handler.
    ///
    /// @param [in] event The show event data.
    void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;

    /// @brief Override the paint engine to use.
    ///
    /// \returns This function always returns nullptr because the widget is custom painted.
    QPaintEngine* paintEngine() const Q_DECL_OVERRIDE;

    /// @brief Override the paint event, making it do nothing. The RendererInterface will present frames instead.
    ///
    /// @param [in] event The paint event data.
    void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;

    /// @brief Override the resize event handler.
    ///
    /// @param [in] event The resize event data.
    void resizeEvent(QResizeEvent* event) Q_DECL_OVERRIDE;

    /// @brief Override the wheel event handler.
    ///
    /// @param [in] event The wheel event data.
    void wheelEvent(QWheelEvent* event) Q_DECL_OVERRIDE;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    /// @brief Override the native platform event handler implementation.
    ///
    /// @param [in] event_type The event type.
    /// @param [in] message The event message data.
    /// @param [out] result The result code.
    ///
    /// \returns True if the event was recognized and handled, and false otherwise.
    bool nativeEvent(const QByteArray& event_type, void* message, long* result) Q_DECL_OVERRIDE;

#elif QT_VERSION >= 0x050000

#else
    /// @brief Override the Windows event handler implementation.
    ///
    /// @param [in] message The event message data.
    /// @param [out] result The result code.
    ///
    /// \returns True if the event was recognized and handled, and false otherwise.
    bool winEvent(MSG* message, long* result) Q_DECL_OVERRIDE;
#endif

signals:
    /// @brief This signal is emitted when graphics device initialization is complete.
    ///
    /// @param [in] success True if the device was initialized successfully.
    void DeviceInitialized(bool success);

    /// @brief This signal is emitted when a key is pressed while the widget has focus.
    ///
    /// @param [in] key_event The pressed key event data.
    void KeyPressed(QKeyEvent* key_event);

    /// @brief This signal is emitted when a key is released while the widget has focus.
    ///
    /// @param [in] key_event The released key event data.
    void KeyReleased(QKeyEvent* key_event);

    /// @brief This signal is emitted when the mouse is moved within the widget.
    ///
    /// @param [in] mouse_event The mouse move event data.
    void MouseMoved(QMouseEvent* mouse_event);

    /// @brief This signal is emitted when a mouse button is pressed within the widget.
    ///
    /// @param [in] mouse_event The mouse button press event data.
    void MousePressed(QMouseEvent* mouse_event);

    /// @brief This signal is emitted when a mouse button is double clicked within the widget.
    ///
    /// @param [in] mouse_event The mouse button double click event data.
    void MouseDoubleClicked(QMouseEvent* mouse_event);

    /// @brief This signal is emitted when a mouse button is released within the widget.
    ///
    /// @param [in] mouse_event The mouse button release event data.
    void MouseReleased(QMouseEvent* mouse_event);

    /// @brief This signal is emitted when a mouse wheel is moved within the widget.
    ///
    /// @param [in] wheel_event The mouse wheel move event.
    void MouseWheelMoved(QWheelEvent* wheel_event);

    /// @brief This signal is emitted when the focus is about to go out.
    void FocusOut();

    /// @brief This signal is emitted when the focus is about to go in.
    void FocusIn();

private slots:
    /// @brief Render a new frame of the scene.
    void RenderFrame();

private:
    /// @brief Initialize the widget instance's underlying renderer.
    ///
    /// @returns True if initialization was successful.
    bool Initialize();

    /// @brief Create the render device used to draw new frames.
    ///
    /// @returns True if the device was created successfully and false in case of failure.
    bool CreateDevice();

    /// @brief Resize the renderer swapchain dimensions.
    ///
    /// @param [in] width The new window width.
    /// @param [out] height The new window height.
    void ResizeSwapChain(int width, int height);

    /// @brief Set the focus to this widget's window.
    void SetFocus();

    /// @brief Does the given widget have the application focus?
    ///
    /// @param [in] widget The widget to check for focus.
    ///
    /// @returns True if the given widget has application focus, false if not.
    bool IsFocused(QWidget* widget);

    QTimer                            timer_;                          ///< A timer used to redraw the scene at a specific rate.
    bool                              device_initialized_;             ///< A flag indicating if the device has been initialized.
    bool                              render_active_;                  ///< True when rendering is currently active.
    bool                              started_;                        ///< True if rendering has started.
    bool                              renderer_is_focused_ = false;    ///< True if renderer is focus of application.
    rra::renderer::RendererInterface* renderer_interface_  = nullptr;  ///< The renderer instance used to draw the frame.
    rra::renderer::WindowInfo         window_info_         = {};       ///< The widget's platform window handle info.
};

#endif  // RRA_RENDERER_RENDERER_WIDGET_H_
