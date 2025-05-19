//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the about pane.
//=============================================================================

#ifndef RRA_VIEWS_START_ABOUT_PANE_H_
#define RRA_VIEWS_START_ABOUT_PANE_H_

#include <QDialog>
#include <QLabel>

#include "qt_common/custom_widgets/scaled_push_button.h"
#include "update_check_api/source/update_check_results_dialog.h"

#include "ui_about_pane.h"

#include "views/base_pane.h"

/// @brief Support for the about pane.
class AboutPane : public BasePane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit AboutPane(QWidget* parent);

    /// @brief Destructor.
    virtual ~AboutPane();

    /// @brief Overridden window show event.
    ///
    /// @param [in] event The show event object.
    virtual void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;

private slots:
    /// @brief Open RRA help file.
    ///
    /// Present the user with help.
    void OpenHelp();

    /// @brief Open trace help file.
    ///
    /// Present the user with help about how to capture a trace with the panel.
    void OpenTraceHelp();

    /// @brief Open RRA help file.
    ///
    /// Present the user with the license.
    void OpenLicense();

    /// @brief Perform a check for updates.
    ///
    /// Runs a background thread that goes online to look for updates.
    void CheckForUpdates();

    /// @brief Callback after a check for updates has returned.
    ///
    /// Displays a modal dialog box with the update information or error message.
    ///
    /// @param [in] thread               The background thread that was checking for updates.
    /// @param [in] update_check_results The results of the check for updates.
    void CheckForUpdatesCompleted(UpdateCheck::ThreadController* thread, const UpdateCheck::Results& update_check_results);

    /// @brief Callback for when check for updates has returned due to being cancelled.
    ///
    /// Restores the UI to allow checking for updates again.
    ///
    /// @param [in] thread The background thread that was checking for updates.
    void CheckForUpdatesCancelled(UpdateCheck::ThreadController* thread);

private:
    /// @brief Open an HTML file.
    ///
    /// @param [in] html_file the file to open.
    void OpenHtmlFile(const QString& html_file);

    /// @brief Initialize the button.
    ///
    /// @param [in] button Pointer to the push button.
    void InitButton(ScaledPushButton* button);

    Ui::AboutPane* ui_;  ///< Pointer to the Qt UI design.

    /// @brief A dialog that is displayed while the check for updates is in-progress.
    ///
    /// Closing this dialog will signal the check for updates to be cancelled.
    /// It will close automatically after the check for updates completes.
    QDialog* check_for_updates_pending_dialog_;

    /// @brief The label on the check for updates pending dialog.
    QLabel* check_for_updates_dialog_label_;

    /// @brief This class creates and interacts with the background thread that performs the check for updates.
    ///
    /// We need to store a member variable so that we can cancel the thread if needed. The thread will emit a
    /// signal when the check for updates has either been cancelled or after it has completed.
    UpdateCheck::ThreadController* check_for_updates_thread_;
};

#endif  // RRA_VIEWS_START_ABOUT_PANE_H_

