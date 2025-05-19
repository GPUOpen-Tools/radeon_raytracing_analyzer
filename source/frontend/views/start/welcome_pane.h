//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the welcome pane.
//=============================================================================

#ifndef RRA_VIEWS_START_WELCOME_PANE_H_
#define RRA_VIEWS_START_WELCOME_PANE_H_

#include <QScrollArea>
#include <QVBoxLayout>
#include <QVector>

#include "qt_common/custom_widgets/quick_link_button_widget.h"
#include "qt_common/custom_widgets/recent_trace_mini_widget.h"
#include "qt_common/custom_widgets/scaled_push_button.h"
#include "update_check_api/source/update_check_results_dialog.h"

#include "ui_welcome_pane.h"

#include "views/base_pane.h"

/// @brief Class declaration.
class WelcomePane : public BasePane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit WelcomePane(QWidget* parent);

    /// @brief Destructor.
    virtual ~WelcomePane();

    /// @brief Overridden window show event.
    ///
    /// @param [in] event The show event object.
    virtual void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;

private slots:
    /// @brief Setup the list of recent files.
    ///
    /// Called whenever the number of recent files changes or whenever the list needs updating.
    void SetupFileList();

    /// @brief Open help file.
    ///
    /// Present the user with help regarding RRA.
    void OpenHelp();

    /// @brief Open trace help file.
    ///
    /// Present the user with help about how to capture a trace with the panel.
    void OpenTraceHelp();

    /// @brief Open a URL to GPUOpen.
    void OpenGPUOpenURL();

    /// @brief Open a URL to GitHub.
    void OpenGitHubURL();

    /// @brief Open a URL to RGP.
    void OpenRGPURL();

    /// @brief Open a URL to RGA.
    void OpenRGAURL();

    /// @brief Open a URL to RGD.
    void OpenRGDURL();

    /// @brief Open a URL to RMV.
    void OpenRMVURL();

    /// @brief Open sample trace.
    void OpenSampleTrace();

    /// @brief Open performance guide on GPUOpen.
    void OpenRDNAPerformanceURL();

    /// @brief Notify the user that a new version is available.
    ///
    /// @param [in] thread               The background thread that checked for updates.
    /// @param [in] update_check_results The results of the check for updates.
    void NotifyOfNewVersion(UpdateCheck::ThreadController* thread, const UpdateCheck::Results& update_check_results);

private:
    /// @brief Open an HTML file.
    ///
    /// @param [in] html_file the file to open.
    void OpenHtmlFile(const QString& html_file);

    /// @brief Initialize the button.
    ///
    /// @param button [in] Pointer to the push button.
    void InitButton(ScaledPushButton* button);

    Ui::WelcomePane*                ui_;             ///< Pointer to the Qt UI design.
    QVector<RecentTraceMiniWidget*> trace_widgets_;  ///< Array of trace widgets.

    /// @brief This class creates and interacts with the background thread that performs the check for updates.
    ///
    /// We need to store a member variable so that we can cancel the thread if needed. The thread will emit a
    /// signal when the check for updates has either been cancelled or after it has completed.
    std::unique_ptr<UpdateCheck::ThreadController> check_for_updates_thread_;
};

#endif  // RRA_VIEWS_START_WELCOME_PANE_H_

