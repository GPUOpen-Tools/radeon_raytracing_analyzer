//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the click-through license dialog box.
//=============================================================================

#ifndef RRA_VIEWS_LICENSE_DIALOG_H_
#define RRA_VIEWS_LICENSE_DIALOG_H_

#include "ui_license_dialog.h"

#include <QDialog>

/// @brief Support for the agreement window.
class LicenseDialog : public QDialog
{
    Q_OBJECT

public:
    /// @brief Constructor.
    explicit LicenseDialog();

    /// @brief Destructor.
    virtual ~LicenseDialog();

    /// @brief Has the user agreed to the license.
    ///
    /// @return true if so, false if not.
    bool AgreedToLicense();

signals:
    /// @brief Signal that the user agrees to the license.
    void AgreeToLicense();

private slots:
    /// @brief Make sure the user actually read the license.
    ///
    /// Only enable the agree button if scrolled to the bottom.
    void ScrollbarChanged();

    /// @brief Kill the app.
    void Exit();

    /// @brief Accept license agreement.
    void Agree();

private:
    /// @brief Kill the app.
    ///
    /// @param [in] event The CloseEvent.
    void closeEvent(QCloseEvent* event);

    Ui::LicenseDialog* ui_;  ///< Pointer to the Qt UI design.
};

#endif  // RRA_VIEWS_LICENSE_DIALOG_H_
