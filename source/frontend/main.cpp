//=============================================================================
// Copyright (c) 2021-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Main entry point.
//=============================================================================

#include <stdarg.h>
#include <vector>

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStyleFactory>

#include "qt_common/custom_widgets/driver_overrides_model.h"
#include "qt_common/utils/qt_util.h"
#include "qt_common/utils/scaling_manager.h"

#include "public/graphics_context.h"
#include "public/renderer_interface.h"
#include "public/rra_print.h"

#include "constants.h"
#include "managers/message_manager.h"
#include "managers/trace_manager.h"
#include "models/acceleration_structure_viewer_model.h"
#include "util/rra_util.h"
#include "views/main_window.h"

namespace rra
{
    class SceneNode;
}

/// @brief Handle printing from RRA backend.
///
/// @param [in] message Incoming message.
void PrintCallback(LogLevel log_level, const char* message)
{
    DebugWindow::DbgMsg(log_level, message);
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
    RraSetPrintingCallback(PrintCallback, true);

#ifdef _LINUX
    qputenv("QT_QPA_PLATFORM", "xcb");
#endif

    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("fusion"));

    // The blas_root_nodes[i] node is the root node of BLAS index i.
    std::vector<rra::SceneNode*> blas_root_nodes{};
    MainWindow* window = new (std::nothrow) MainWindow(&blas_root_nodes);  // Save reference to BLAS root nodes in frontend before they're populated.
    int         result = -1;
    if (window != nullptr)
    {
#ifdef _WIN32
        SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);
#endif
        window->show();
        rra::renderer::GraphicsContextSceneInfo* info = new rra::renderer::GraphicsContextSceneInfo{};

        // Initialize scaling manager and call ScaleFactorChanged at least once, so that
        // any existing Scaled classes run their initialization as well.
        ScalingManager::Get().Initialize(window);

        rra::TraceManager::Get().Initialize(window);

        // Once the trace has been loaded initialize graphics context and upload data to the device via this callback.
        rra::TraceManager::Get().SetLoadingFinishedCallback([window, info, &blas_root_nodes]() {
            rra::renderer::CreateGraphicsContext(window);

            // Attempt to initialize the graphics context.
            if (!rra::renderer::InitializeGraphicsContext(rra::GetGraphicsContextSceneInfo(blas_root_nodes, info)))  // Populate BLAS root nodes.
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

        for (rra::renderer::TraversalTree& traversal_tree : info->acceleration_structures)
        {
            delete[] traversal_tree.child_nodes_buffer;
        }

        delete window;
        delete info;
    }

    return result;
}

