//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the click-through license dialog box.
//=============================================================================

#include "views/license_dialog.h"

#include <QApplication>
#include <QtDebug>
#include <QtGlobal>
#include <QScrollBar>
#include <QDir>

#include "qt_common/utils/scaling_manager.h"

#include "public/rra_macro.h"

#include "settings/settings.h"
#include "views/widget_util.h"

#include "version.h"

LicenseDialog::LicenseDialog()
    : QDialog(nullptr)
    , ui_(new Ui::LicenseDialog)
{
    ui_->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    connect(ui_->decline_button_, &QPushButton::pressed, this, &LicenseDialog::Exit);
    connect(ui_->agree_button_, &QPushButton::pressed, this, &LicenseDialog::Agree);

    setModal(true);
    setFixedSize(size());
    ui_->text_edit_->setReadOnly(true);
    ui_->text_edit_->setTextInteractionFlags(Qt::NoTextInteraction);

    ui_->agree_button_->setEnabled(!ui_->text_edit_->verticalScrollBar()->isVisible());

    rra::widget_util::SetWidgetBackgroundColor(this, Qt::white);

    QFile license_file(QCoreApplication::applicationDirPath() + rra::text::kLicenseFile);

    if (license_file.open(QIODevice::ReadOnly) == true)
    {
        QString html_license = "";

        QTextStream in(&license_file);

        while (in.atEnd() == false)
        {
            html_license.append(in.readLine());
            // The end of each line of the license file does not get read in as a space,
            // so re-insert the space here to make sure that words do not get joined.
            html_license.append(' ');
        }

        license_file.close();

        ui_->text_edit_->setHtml(html_license);

        connect(ui_->text_edit_->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(ScrollbarChanged()));
    }
}

LicenseDialog::~LicenseDialog()
{
    delete ui_;
}

bool LicenseDialog::AgreedToLicense()
{
#ifdef BETA_LICENSE
    const QString& previous_version_string = rra::Settings::Get().GetLicenseAgreementVersion();
    if (previous_version_string.isEmpty() == false)
    {
        const QString current_version_string = PRODUCT_VERSION_STRING;

        QStringList current_split  = current_version_string.split(".");
        QStringList previous_split = previous_version_string.split(".");

        if (current_split.empty() == false)
        {
            // Remove the build number.
            current_split.removeLast();
        }

        if ((previous_split.empty() == false) && (current_split.empty() == false))
        {
            const int32_t check_count = RRA_MINIMUM(previous_split.size(), current_split.size());

            for (int32_t i = 0; i < check_count; i++)
            {
                int previous_version = previous_split[i].toInt();
                int current_version  = current_split[i].toInt();
                if (previous_version < current_version)
                {
                    return false;
                }
                else if (previous_version > current_version)
                {
                    return true;
                }
            }
        }
        // Version numbers are equal so license has been read for this version.
        return true;
    }
    return false;
#else
    return true;
#endif  // BETA_LICENSE
}

void LicenseDialog::Agree()
{
    hide();
    emit AgreeToLicense();
}

void LicenseDialog::Exit()
{
    exit(0);
}

void LicenseDialog::closeEvent(QCloseEvent* event)
{
    Q_UNUSED(event);
    Exit();
}

void LicenseDialog::ScrollbarChanged()
{
    QScrollBar* scroll_bar = ui_->text_edit_->verticalScrollBar();

    if (scroll_bar != nullptr)
    {
        if (scroll_bar->value() == scroll_bar->maximum())
        {
            ui_->agree_button_->setEnabled(true);
        }
    }
}
