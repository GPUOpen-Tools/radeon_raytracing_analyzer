//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Main entry point.
//=============================================================================

#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QStyleFactory>
#include <stdarg.h>

#include "qt_common/custom_widgets/driver_overrides_model.h"
#include "qt_common/utils/qt_util.h"
#include "qt_common/utils/scaling_manager.h"

#include "public/rra_print.h"
#include "public/graphics_context.h"

#include "constants.h"
#include "managers/message_manager.h"
#include "managers/trace_manager.h"
#include "util/rra_util.h"
#include "views/main_window.h"
#include "models/acceleration_structure_viewer_model.h"

/// @brief Handle printing from RRA backend.
///
/// @param [in] message Incoming message.
void PrintCallback(const char* message)
{
    DebugWindow::DbgMsg(message);
}

/// @brief Detect RRA trace if any was specified as command line param.
///
/// @return Empty string if no trace, and full path if valid RRA file.
static QString GetTracePath()
{
    QString out = "";

    if (QCoreApplication::arguments().count() > 1)
    {
        const QString potential_trace_path = QDir::toNativeSeparators(QCoreApplication::arguments().at(1));
        if (rra_util::TraceValidToLoad(potential_trace_path) == true)
        {
            out = potential_trace_path;
        }
    }

    return out;
}

/// @brief Main entry point.
///
/// @param [in] argc The number of arguments.
/// @param [in] argv An array containing arguments.
int main(int argc, char* argv[])
{
#ifdef _DEBUG
    RraSetPrintingCallback(PrintCallback, true);
#endif

#ifdef _LINUX
    qputenv("QT_QPA_PLATFORM", "xcb");
#endif

    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("fusion"));

    MainWindow* window = new (std::nothrow) MainWindow();
    int         result = -1;
    if (window != nullptr)
    {
        window->show();

        // Initialize scaling manager and call ScaleFactorChanged at least once, so that
        // any existing Scaled classes run their initialization as well.
        ScalingManager::Get().Initialize(window);

        rra::TraceManager::Get().Initialize(window);

        // Once the trace has been loaded initialize graphics context and upload data to the device via this callback.
        rra::TraceManager::Get().SetLoadingFinishedCallback([window]() {
            rra::renderer::CreateGraphicsContext(window);

            // Attempt to initialize the graphics context.
            if (!rra::renderer::InitializeGraphicsContext(rra::GetGraphicsContextSceneInfo()))
            {
                // Emit a signal to close the loaded trace and display a failure notification.
                QString failure_message = QString::fromStdString(rra::renderer::GetGraphicsContextInitializationError());
                emit    rra::MessageManager::Get().GraphicsContextFailedToInitialize(failure_message);
            }
        });

        // Once the trace has been closed cleanup the graphics context.
        rra::TraceManager::Get().SetClearTraceCallback([]() { rra::renderer::CleanupGraphicsContext(); });

        if (!GetTracePath().isEmpty())
        {
            rra::TraceManager::Get().LoadTrace(GetTracePath());
        }

        result = a.exec();

        driver_overrides::DriverOverridesModel::DestroyInstance();
        delete window;
    }

    return result;
}
