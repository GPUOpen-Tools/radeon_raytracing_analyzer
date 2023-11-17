//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of Recent traces pane.
//=============================================================================

#include "views/start/recent_traces_pane.h"

#include <QScrollArea>
#include <QScrollBar>

#include "qt_common/utils/qt_util.h"
#include "qt_common/utils/scaling_manager.h"

#include "managers/message_manager.h"
#include "managers/trace_manager.h"
#include "settings/settings.h"
#include "views/widget_util.h"

const static int     kRecentTraceSpacing            = 20;
const static int     kRecentTracePaneMargin         = 10;
const static int     kRecentTracesTextPixelFontSize = 14;
const static QString kRecentTracesNoTracesString    = "There are no recently opened BVH scenes";

RecentTracesPane::RecentTracesPane(QWidget* parent)
    : BasePane(parent)
    , ui_(new Ui::RecentTracesPane)
    , vbox_layout_(nullptr)
    , no_traces_label_(nullptr)
{
    ui_->setupUi(this);

    // Set white background for this pane.
    rra::widget_util::SetWidgetBackgroundColor(this, Qt::white);

    // Set the background color.
    QPalette palette;
    palette.setColor(QPalette::Window, Qt::GlobalColor::transparent);
    ui_->main_scroll_area_->setPalette(palette);

    scroll_area_widget_contents_ = new QWidget();
    scroll_area_widget_contents_->setPalette(palette);
    scroll_area_widget_contents_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // When deleting a trace, the setupFileList signal is fired to force an update of the file list. This needs to be done
    // on a queuedConnection so that any signals/slots/signal mappers for the trace file widgets have been cleaned up since
    // they get recreated when setting up a new file list.
    connect(&rra::MessageManager::Get(), &rra::MessageManager::RecentFileListChanged, this, &RecentTracesPane::SetupFileList, Qt::QueuedConnection);

    SetupFileList();
}

RecentTracesPane::~RecentTracesPane()
{
    for (RecentTraceWidget* widget : trace_widgets_)
    {
        delete widget;
    }
    delete ui_;
}

void RecentTracesPane::SetupFileList()
{
    const QVector<RecentFileData>& files = rra::Settings::Get().RecentFiles();

    // Clear any previous recent trace widgets.
    for (RecentTraceWidget* widget : trace_widgets_)
    {
        delete widget;
    }
    trace_widgets_.clear();

    if (no_traces_label_ != nullptr)
    {
        if (vbox_layout_ != nullptr)
        {
            vbox_layout_->removeWidget(no_traces_label_);
        }

        no_traces_label_->hide();
    }

    if (vbox_layout_ != nullptr)
    {
        delete vbox_layout_;
    }

    vbox_layout_ = new QVBoxLayout(scroll_area_widget_contents_);
    vbox_layout_->setSpacing(kRecentTraceSpacing);
    vbox_layout_->setContentsMargins(kRecentTracePaneMargin, kRecentTracePaneMargin, kRecentTracePaneMargin, kRecentTracePaneMargin);

    // If there are no recent traces to show, add a label stating so.
    if (files.size() == 0)
    {
        no_traces_label_ = new QLabel(kRecentTracesNoTracesString);

        // Set the fonts.
        QFont font;
        font.setPixelSize(ScalingManager::Get().Scaled(kRecentTracesTextPixelFontSize));
        font.setBold(true);
        font.setFamily(font.defaultFamily());
        no_traces_label_->setFont(font);

        // Add this label to the vertical layout.
        vbox_layout_->addWidget(no_traces_label_);
    }

    // Create a widget for each recent file.
    for (int i = 0; i < files.size(); i++)
    {
        RecentTraceWidget* trace_widget = new RecentTraceWidget(this);
        QSizePolicy        policy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        trace_widget->setSizePolicy(policy);
        vbox_layout_->addWidget(trace_widget);
        trace_widgets_.push_back(trace_widget);

        // Set up the widget.
        trace_widget->SetRecentFileData(files[i]);
        trace_widget->show();

        // Trigger a trace open when the trace widget is clicked.
        connect(trace_widget, &RecentTraceWidget::clicked, &rra::TraceManager::Get(), &rra::TraceManager::LoadTrace);
        connect(trace_widget, &RecentTraceWidget::clickedDelete, this, &RecentTracesPane::DeleteTrace);
        connect(trace_widget, &RecentTraceWidget::OpenFileLocationFailed, this, &RecentTracesPane::HandleTraceNotFoundError);
    }
    // Add a spacer at the bottom. The spacer item is owned by the layout so will be deleted when the layout is deleted.
    vbox_layout_->addSpacerItem(new QSpacerItem(10, 40, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    vbox_layout_->setSizeConstraint(QLayout::SetMinimumSize);
    ui_->main_scroll_area_->setWidget(scroll_area_widget_contents_);

    // Set the vertical scrollbar to the top.
    ui_->main_scroll_area_->verticalScrollBar()->setMaximum(0);
}

void RecentTracesPane::DeleteTrace(const QString& path)
{
    rra::Settings::Get().RemoveRecentFile(path);
    rra::Settings::Get().SaveSettings();
    emit RecentFileDeleted();
}

void RecentTracesPane::HandleTraceNotFoundError(const QString path)
{
    const QString text = rra::text::kOpenRecentTraceStart + path + rra::text::kOpenRecentTraceEnd;
    QtCommon::QtUtils::ShowMessageBox(this, QMessageBox::Ok, QMessageBox::Critical, rra::text::kOpenRecentTraceTitle, text);
}
