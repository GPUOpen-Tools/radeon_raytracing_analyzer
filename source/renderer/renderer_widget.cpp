//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the Renderer Widget.
//=============================================================================

#include <QDebug>
#include <QEvent>
#include <QWheelEvent>

#include "public/renderer_widget.h"
#include "public/renderer_types.h"
#include "public/orientation_gizmo.h"

constexpr int kTargetFps            = 512.0f;
constexpr int kMillisecondsPerFrame = static_cast<int>((1.0f / kTargetFps) * 1000.0f);
const char*   kFocusInBorderStyle   = "border-style: solid; border-width: 3px; border-color: rgb(0, 122, 217);";
const char*   kFocusOutBorderStyle  = "border-style: solid; border-width: 3px; border-color: rgb(200,200,200);";

RendererWidget::RendererWidget(QWidget* parent)
    : QWidget(parent)
    , device_initialized_(false)
    , render_active_(false)
    , started_(false)
    , renderer_interface_(nullptr)
{
    QPalette widget_palette = palette();
    widget_palette.setColor(QPalette::Window, Qt::black);
    setAutoFillBackground(true);
    setPalette(widget_palette);

    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_NativeWindow);

    // Setting these attributes to our widget and returning null on paintEngine event
    // tells Qt that we'll handle all drawing and updating the widget ourselves.
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);

    parent->setStyleSheet(kFocusOutBorderStyle);
}

void RendererWidget::Run()
{
    timer_.start(kMillisecondsPerFrame);
    render_active_ = started_ = true;
}

void RendererWidget::PauseFrames()
{
    if (!timer_.isActive() || !started_)
    {
        return;
    }

    disconnect(&timer_, &QTimer::timeout, this, &RendererWidget::RenderFrame);
    timer_.stop();
    render_active_ = false;

    renderer_interface_->WaitForGpu();
}

void RendererWidget::ContinueFrames()
{
    if (timer_.isActive() || !started_)
    {
        return;
    }

    connect(&timer_, &QTimer::timeout, this, &RendererWidget::RenderFrame);
    timer_.start(kMillisecondsPerFrame);
    render_active_ = true;
}

void RendererWidget::Release()
{
    device_initialized_ = false;
    disconnect(&timer_, &QTimer::timeout, this, &RendererWidget::RenderFrame);
    timer_.stop();

    assert(renderer_interface_ != nullptr);
    if (renderer_interface_ != nullptr)
    {
        renderer_interface_->WaitForGpu();
        renderer_interface_->Shutdown();
    }
}

void RendererWidget::SetRendererInterface(rra::renderer::RendererInterface* renderer_interface)
{
    renderer_interface_ = renderer_interface;
    if (renderer_interface_ != nullptr)
    {
        // Send the widget's window info to the renderer interface.
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        window_info_.window_handle = reinterpret_cast<HWND>(winId());
#elif defined(VK_USE_PLATFORM_XCB_KHR)
        QPlatformNativeInterface* native_interface = QGuiApplication::platformNativeInterface();
        Q_ASSERT(native_interface);
        xcb_connection_t* connection = static_cast<xcb_connection_t*>(native_interface->nativeResourceForWindow("connection", this->windowHandle()));
        Q_ASSERT(connection);
        xcb_window_t xcb_window = static_cast<xcb_window_t>(winId());
        Q_ASSERT(xcb_window);
        window_info_.window     = xcb_window;
        window_info_.connection = connection;
#endif
        renderer_interface_->SetWindowInfo(&window_info_);
    }
    else
    {
        device_initialized_ = false;
    }
}

bool RendererWidget::GetRendererIsFocused()
{
    return renderer_is_focused_;
}

void RendererWidget::leaveEvent(QEvent* event)
{
    RRA_UNUSED(event);
    rra::renderer::SetOrientationGizmoSelected(rra::renderer::OrientationGizmoHitType::kNone);
    if (renderer_interface_ != nullptr)
    {
        renderer_interface_->MarkAsDirty();
    }
}

void RendererWidget::showEvent(QShowEvent* event)
{
    if (renderer_interface_ != nullptr)
    {
        if (!device_initialized_)
        {
            device_initialized_ = Initialize();
            emit DeviceInitialized(device_initialized_);
        }
    }

    QWidget::showEvent(event);
}

bool RendererWidget::Initialize()
{
    bool result = CreateDevice();
    if (result)
    {
        connect(&timer_, &QTimer::timeout, this, &RendererWidget::RenderFrame);
    }

    // Tracks mouse even if no buttons are being pressed. Needed when hovering over orientation gizmo.
    setMouseTracking(true);

    return result;
}

bool RendererWidget::CreateDevice()
{
    bool result = false;

    assert(renderer_interface_ != nullptr);
    if (renderer_interface_ != nullptr)
    {
        result = renderer_interface_->InitializeDevice();

        if (result)
        {
            ResizeSwapChain(width(), height());
        }
    }

    return result;
}

void RendererWidget::RenderFrame()
{
    assert(renderer_interface_ != nullptr);
    if (renderer_interface_ != nullptr)
    {
        if (render_active_)
        {
            renderer_interface_->MoveToNextFrame();

            renderer_interface_->DrawFrame();
        }
    }
}

void RendererWidget::ResizeSwapChain(int width, int height)
{
    assert(renderer_interface_ != nullptr);
    if (renderer_interface_ != nullptr)
    {
        renderer_interface_->WaitForGpu();

        renderer_interface_->SetDimensions(width, height);
    }
}

void RendererWidget::wheelEvent(QWheelEvent* event)
{
    QWidget::wheelEvent(event);
}

QPaintEngine* RendererWidget::paintEngine() const
{
    // The renderer implementation draws all the widget contents, so this must always return null.
    return Q_NULLPTR;
}

void RendererWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
}

void RendererWidget::resizeEvent(QResizeEvent* event)
{
    UpdateSwapchainSize();
    QWidget::resizeEvent(event);
}

void RendererWidget::UpdateSwapchainSize()
{
    if (device_initialized_)
    {
        ResizeSwapChain(width(), height());
    }
}

bool RendererWidget::IsFocused(QWidget* widget)
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    return ::GetFocus() == reinterpret_cast<HWND>(widget->winId());
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    (void)widget;
    return false;
#else
    (void)widget;
    return false;
#endif
}

void RendererWidget::SetFocus()
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    ::SetFocus(window_info_.window_handle);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    xcb_map_window(window_info_.connection, window_info_.window);
#endif
}

bool RendererWidget::event(QEvent* event)
{
    switch (event->type())
    {
    // Workaround for https://bugreports.qt.io/browse/QTBUG-42183 to get key strokes.
    // To make sure that we always have focus on the widget when we enter the rect area.
    case QEvent::Enter:
        break;
    case QEvent::FocusIn:
        parentWidget()->setStyleSheet(kFocusInBorderStyle);
        renderer_is_focused_ = true;
        break;
    case QEvent::FocusAboutToChange:
        break;
    case QEvent::FocusOut:
        parentWidget()->setStyleSheet(kFocusOutBorderStyle);
        renderer_is_focused_ = false;
        emit FocusOut();
        break;
    case QEvent::KeyPress:
        emit KeyPressed(static_cast<QKeyEvent*>(event));
        break;
    case QEvent::KeyRelease:
        emit KeyReleased(static_cast<QKeyEvent*>(event));
        break;
    case QEvent::MouseMove:
        emit MouseMoved(static_cast<QMouseEvent*>(event));
        break;
    case QEvent::MouseButtonPress:
        emit MousePressed(static_cast<QMouseEvent*>(event));
        break;
    case QEvent::MouseButtonRelease:
        emit MouseReleased(static_cast<QMouseEvent*>(event));
        break;
    case QEvent::MouseButtonDblClick:
        emit MouseDoubleClicked(static_cast<QMouseEvent*>(event));
        break;
    case QEvent::Wheel:
        emit MouseWheelMoved(static_cast<QWheelEvent*>(event));
        break;
    default:
        break;
    }

    return QWidget::event(event);
}

#ifdef WIN32
/// Move this platform-specific code.
/// The Windows message pump procedure.
///
/// @param [in] message The message details.
///
/// @returns The procedure return value.
LRESULT WndProc(MSG* message)
{
    // Process wheel events using Qt's event-system.
    if (message->message == WM_MOUSEWHEEL || message->message == WM_MOUSEHWHEEL)
    {
        return false;
    }

    return false;
}
#endif

bool RendererWidget::nativeEvent(const QByteArray& event_type, void* message, long* result)
{
    Q_UNUSED(event_type);
    Q_UNUSED(result);

#ifdef WIN32
    MSG* msg = reinterpret_cast<MSG*>(message);
    return static_cast<bool>(WndProc(msg));
#else
    return QWidget::nativeEvent(event_type, message, result);
#endif
}
