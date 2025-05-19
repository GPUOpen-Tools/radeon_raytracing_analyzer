//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the debug window.
//=============================================================================

#include "views/debug_window.h"

#include <QApplication>
#include <QScrollBar>
#include <QtDebug>
#include <QtGlobal>

#ifndef _WIN32
#include "public/linux/safe_crt.h"
#endif
#include "public/rra_assert.h"

#include "util/log_file_writer.h"

// The one and only instance of the debug window.
static DebugWindow* debug_window = nullptr;

/// @brief Assert on a Qt message. Add the Qt message as part of the ASSERT warning.
///
/// @param [in] type The Qt message type.
/// @param [in] text The text string containing the error message from Qt.
static void AssertOnQtMessage(const char* type, const QString text)
{
    static char message[2048];

    sprintf_s(message, 2048, "Intercepted a %s message from Qt (%s). Please fix it!", type, text.toLatin1().data());
    RRA_ASSERT_MESSAGE(false, message);
}

/// @brief Detect the type of message sent into Qt and return it as a string.
///
/// @param [in] type The Qt message type (info/error/warning/etc).
/// @param [in] context The message context.
/// @param [in] msg The output string.
static void MyMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context);

    QString txt;

    switch (type)
    {
    case QtInfoMsg:
        txt = QString("qInfo(): %1").arg(msg);
        break;

    case QtDebugMsg:
        txt = QString("qDebug(): %1").arg(msg);
        break;

    case QtWarningMsg:
        txt = QString("qWarning(): %1").arg(msg);
        AssertOnQtMessage("WARNING", txt);
        break;

    case QtCriticalMsg:
        txt = QString("qCritical(): %1").arg(msg);
        AssertOnQtMessage("CRITICAL", txt);
        break;

    case QtFatalMsg:
        txt = QString("qFatal(): %1").arg(msg);
        AssertOnQtMessage("FATAL", txt);
        break;

    default:
        txt = QString("default: %1").arg(msg);
        break;
    }

    debug_window->EmitSetText(txt);
}

DebugWindow::DebugWindow()
    : QDialog(nullptr)
    , ui_(new Ui::DebugWindow)
{
    ui_->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Use monospace font style so that things align.
    QFont font("unexistent");
    font.setStyleHint(QFont::Monospace);
    ui_->plain_text_edit_->setFont(font);

    connect(this, &DebugWindow::EmitSetText, this, &DebugWindow::SetText);

    RegisterDbgWindow();
}

DebugWindow::~DebugWindow()
{
    delete ui_;
}

void DebugWindow::ScrollToBottom()
{
    QScrollBar* scroll_bar = ui_->plain_text_edit_->verticalScrollBar();
    scroll_bar->setValue(scroll_bar->maximum());
}

void DebugWindow::SetText(const QString& string)
{
    ui_->plain_text_edit_->appendPlainText(string);
    ScrollToBottom();
}

void DebugWindow::RegisterDbgWindow()
{
    debug_window = this;

    qInstallMessageHandler(MyMessageHandler);
}

void DebugWindow::DbgMsg(const char* format, ...)
{
    if (debug_window != nullptr)
    {
        char    buffer[2048];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, 2048, format, args);
        debug_window->EmitSetText(QString(buffer));
        rra::LogFileWriter::Get().WriteLog(rra::LogFileWriter::kDebug, buffer);
        va_end(args);
    }
}

