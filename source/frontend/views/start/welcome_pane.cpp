//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the welcome pane.
//=============================================================================

#include "views/start/welcome_pane.h"

#include <QPalette>
#include <QUrl>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileInfo>

#include "qt_common/utils/qt_util.h"

#include "constants.h"
#include "version.h"
#include "managers/message_manager.h"
#include "managers/trace_manager.h"
#include "settings/settings.h"
#include "views/widget_util.h"

static const int kMaxRecentFilesToShow = 8;

WelcomePane::WelcomePane(QWidget* parent)
    : BasePane(parent)
    , ui_(new Ui::WelcomePane)
{
    ui_->setupUi(this);

    constexpr int id = qRegisterMetaType<RraErrorCode>();
    Q_UNUSED(id);

    SetupFileList();

    // Set up the buttons.
    InitButton(ui_->open_trace_button_);
    InitButton(ui_->see_more_recent_files_button_);
    InitButton(ui_->open_getting_started_button_);
    InitButton(ui_->open_help_button_);

    ui_->quick_link_gpu_open_->SetTitle("GPUOpen website");
    ui_->quick_link_gpu_open_->SetDescLineOne("Check out the latest development blogs, performance tips & tricks ");
    ui_->quick_link_gpu_open_->SetDescLineTwo("and open source releases.");

    ui_->quick_link_github_->SetTitle("Encounter a problem or have an idea?");
    ui_->quick_link_github_->SetDescLineOne("To provide feedback or suggestions, or to file a bug, visit our");
    ui_->quick_link_github_->SetDescLineTwo("GitHub page.");

    ui_->quick_link_rgp_->SetTitle("Explore Radeon GPU Profiler");
    ui_->quick_link_rgp_->SetDescLineOne("Find performance bottlenecks and fine tune your application");
    ui_->quick_link_rgp_->SetDescLineTwo("using Radeon GPU Profiler. Available right now at GPUOpen.");

    ui_->quick_link_rga_->SetTitle("Explore Radeon GPU Analyzer");
    ui_->quick_link_rga_->SetDescLineOne("Dig into the disassembly, resource utilization and register liveness of");
    ui_->quick_link_rga_->SetDescLineTwo("your shaders using RGA. Available right now at GPUOpen.");

    ui_->quick_link_rgd_->SetTitle("Explore Radeon GPU Detective");
    ui_->quick_link_rgd_->SetDescLineOne("Investigate GPU crashes, gather your evidence, and probe any page");
    ui_->quick_link_rgd_->SetDescLineTwo("faults! Learn more on GPUOpen.");

    ui_->quick_link_rmv_->SetTitle("Explore Radeon Memory Visualizer");
    ui_->quick_link_rmv_->SetDescLineOne("Dig into your GPU memory usage and open the door to new");
    ui_->quick_link_rmv_->SetDescLineTwo("optimization opportunities! Available right now at GPUOpen.");

    ui_->quick_link_sample_trace_->SetTitle("Sample scene");
    ui_->quick_link_sample_trace_->SetDescLineOne("Still got your training wheels on? Check out a sample scene to see");
    ui_->quick_link_sample_trace_->SetDescLineTwo("what we can do!");

    ui_->quick_link_rdna_performance_->SetTitle("RDNA performance guide");
    ui_->quick_link_rdna_performance_->SetDescLineOne("Learn valuable optimization techniques from this in-depth performance");
    ui_->quick_link_rdna_performance_->SetDescLineTwo("guide full of tidbits, tips and tricks.");

    // Connect buttons to slots.
    connect(ui_->open_trace_button_, &QPushButton::clicked, [=]() { emit rra::MessageManager::Get().OpenTraceFileMenuClicked(); });
    connect(ui_->see_more_recent_files_button_, &QPushButton::clicked, [=]() {
        emit rra::MessageManager::Get().PaneSwitchRequested(rra::kPaneIdStartRecentTraces);
    });
    connect(ui_->open_getting_started_button_, &QPushButton::clicked, this, &WelcomePane::OpenTraceHelp);
    connect(ui_->open_help_button_, &QPushButton::clicked, this, &WelcomePane::OpenHelp);
    connect(ui_->quick_link_gpu_open_, &QPushButton::clicked, this, &WelcomePane::OpenGPUOpenURL);
    connect(ui_->quick_link_github_, &QPushButton::clicked, this, &WelcomePane::OpenGitHubURL);
    connect(ui_->quick_link_rgp_, &QPushButton::clicked, this, &WelcomePane::OpenRGPURL);
    connect(ui_->quick_link_rga_, &QPushButton::clicked, this, &WelcomePane::OpenRGAURL);
    connect(ui_->quick_link_rgd_, &QPushButton::clicked, this, &WelcomePane::OpenRGDURL);
    connect(ui_->quick_link_rmv_, &QPushButton::clicked, this, &WelcomePane::OpenRMVURL);
    connect(ui_->quick_link_sample_trace_, &QPushButton::clicked, this, &WelcomePane::OpenSampleTrace);
    connect(ui_->quick_link_rdna_performance_, &QPushButton::clicked, this, &WelcomePane::OpenRDNAPerformanceURL);
    connect(&rra::MessageManager::Get(), &rra::MessageManager::RecentFileListChanged, this, &WelcomePane::SetupFileList, Qt::QueuedConnection);

    // Notifications are always hidden by default, and will be displayed if new notifications become available.
    ui_->notifications_label_->setVisible(false);
    ui_->notify_update_available_button_->setVisible(false);

    if (rra::Settings::Get().GetCheckForUpdatesOnStartup())
    {
        UpdateCheck::ThreadController* background_thread =
            new UpdateCheck::ThreadController(this, PRODUCT_MAJOR_VERSION, PRODUCT_MINOR_VERSION, PRODUCT_BUILD_NUMBER, PRODUCT_BUGFIX_NUMBER);

        // Get notified when the check for updates has completed.
        // There is not a way in the UI to cancel this thread, so no reason to connect to its CheckForUpdatesCancelled callback.
        connect(background_thread, &UpdateCheck::ThreadController::CheckForUpdatesComplete, this, &WelcomePane::NotifyOfNewVersion);

        background_thread->StartCheckForUpdates(rra::kUpdateCheckUrl, rra::kUpdateCheckAssetName);
    }
}

WelcomePane::~WelcomePane()
{
}

void WelcomePane::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);

    // If the RDP documentation isn't present, hide the button to show it.
    QFileInfo file_info(QCoreApplication::applicationDirPath() + rra::text::kTraceHelpFile);

    // Check to see if the file is not a directory and that it exists.
    if (file_info.isFile() && file_info.exists())
    {
        ui_->open_getting_started_button_->show();
    }
    else
    {
        ui_->open_getting_started_button_->hide();
    }
}

void WelcomePane::SetupFileList()
{
    const QVector<RecentFileData>& files = rra::Settings::Get().RecentFiles();

    // Clear any previous recent trace widgets.
    for (RecentTraceMiniWidget* widget : trace_widgets_)
    {
        delete widget;
    }

    trace_widgets_.clear();

    // Create a widget for each recent file.
    int max_files_to_show = 0;
    files.size() > kMaxRecentFilesToShow ? max_files_to_show = kMaxRecentFilesToShow : max_files_to_show = files.size();
    for (int i = 0; i < max_files_to_show; i++)
    {
        RecentTraceMiniWidget* trace_widget = new RecentTraceMiniWidget(ui_->recent_traces_wrapper_);
        trace_widgets_.push_back(trace_widget);

        // Set up the widget.
        trace_widget->SetFile(files[i]);
        trace_widget->show();

        // Trigger a trace open when the trace widget is clicked.
        connect(trace_widget, &RecentTraceMiniWidget::clicked, &rra::TraceManager::Get(), &rra::TraceManager::LoadTrace);

        ui_->recent_traces_wrapper_->layout()->addWidget(trace_widget);
    }

    const size_t recent_file_count = files.size();

    if (recent_file_count > kMaxRecentFilesToShow)
    {
        ui_->see_more_recent_files_button_->show();
    }
    else
    {
        ui_->see_more_recent_files_button_->hide();
    }

    if (recent_file_count == 0)
    {
        ui_->empty_recent_files_label_->show();
    }
    else
    {
        ui_->empty_recent_files_label_->hide();
    }
}

void WelcomePane::InitButton(ScaledPushButton* button)
{
    // Init the button.
    button->setCursor(Qt::PointingHandCursor);
    button->SetLinkStyleSheet();
}

void WelcomePane::OpenHtmlFile(const QString& html_file)
{
    // Get the file info.
    QFileInfo file_info(QCoreApplication::applicationDirPath() + html_file);

    // Check to see if the file is not a directory and that it exists.
    if (file_info.isFile() && file_info.exists())
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + html_file));
    }
    else
    {
        // The selected html file is missing on the disk so display a message box stating so.
        const QString text = rra::text::kMissingHelpFile + html_file;
        QtCommon::QtUtils::ShowMessageBox(this, QMessageBox::Ok, QMessageBox::Critical, rra::text::kMissingHelpFile, text);
    }
}

void WelcomePane::OpenHelp()
{
    OpenHtmlFile(rra::text::kHelpFile);
}

void WelcomePane::OpenTraceHelp()
{
    OpenHtmlFile(rra::text::kTraceHelpFile);
}

void WelcomePane::OpenGPUOpenURL()
{
    QDesktopServices::openUrl(rra::text::kGpuOpenUrl);
}

void WelcomePane::OpenGitHubURL()
{
    QDesktopServices::openUrl(rra::text::kRraGithubUrl);
}

void WelcomePane::OpenRGPURL()
{
    QDesktopServices::openUrl(rra::text::kRgpGpuOpenUrl);
}

void WelcomePane::OpenRGAURL()
{
    QDesktopServices::openUrl(rra::text::kRgaGpuOpenUrl);
}

void WelcomePane::OpenRGDURL()
{
    QDesktopServices::openUrl(rra::text::kRgdGpuOpenUrl);
}

void WelcomePane::OpenRMVURL()
{
    QDesktopServices::openUrl(rra::text::kRmvGpuOpenUrl);
}

void WelcomePane::OpenSampleTrace()
{
    rra::TraceManager::Get().LoadTrace(QCoreApplication::applicationDirPath() + rra::text::kSampleTraceLocation);
}

void WelcomePane::OpenRDNAPerformanceURL()
{
    QDesktopServices::openUrl(rra::text::kRdnaPerformanceGpuOpenUrl);
}

void WelcomePane::NotifyOfNewVersion(UpdateCheck::ThreadController* thread, const UpdateCheck::Results& update_check_results)
{
    if (update_check_results.was_check_successful && update_check_results.update_info.is_update_available && !update_check_results.update_info.releases.empty())
    {
        ui_->notifications_label_->setVisible(true);
        ui_->notify_update_available_button_->setVisible(true);
        ui_->notify_update_available_button_->SetTitle("New Version Available!");
        ui_->notify_update_available_button_->SetDescLineOne(update_check_results.update_info.releases[0].title.c_str());
        ui_->notify_update_available_button_->SetDescLineTwo("Click here for more information.");

        // This dialog will get deleted when the WelcomePane is deleted.
        UpdateCheckResultsDialog* results_dialog = new UpdateCheckResultsDialog(this);
        results_dialog->setWindowFlags((results_dialog->windowFlags() & ~Qt::WindowContextHelpButtonHint) | Qt::MSWindowsFixedSizeDialogHint);
        results_dialog->setFixedSize(rra::kUpdatesResultsDialogWidth, rra::kUpdatesResultsDialogHeight);
        results_dialog->SetShowTags(false);
        results_dialog->SetResults(update_check_results);

        QDialogButtonBox* button_box = results_dialog->findChild<QDialogButtonBox*>("button_box_");
        if (button_box != nullptr)
        {
            QPushButton* close_button = button_box->button(QDialogButtonBox::Close);
            if (close_button != nullptr)
            {
                close_button->setCursor(Qt::PointingHandCursor);
            }
        }

        // Connect the button so that the when it is clicked, the dialog is shown.
        // This is why the dialog should not be deleted earlier - it could get opened any time.
        connect(ui_->notify_update_available_button_, &QPushButton::clicked, results_dialog, &QDialog::show);
    }

    // Delete the thread so that it no longer exists in the background.
    if (thread != nullptr)
    {
        delete thread;
    }
}
