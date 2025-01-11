//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header of the dispatch loading bar.
//=============================================================================

#ifndef RRA_VIEWS_CUSTOM_WIDGETS_DISPATCH_LOADING_BAR_H_
#define RRA_VIEWS_CUSTOM_WIDGETS_DISPATCH_LOADING_BAR_H_

#include <QColor>
#include <QWidget>

#include "qt_common/utils/common_definitions.h"

/// @brief Support for the dispatch loading bar.
class DispatchLoadingBar : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(qreal percentage READ FillPercentage WRITE SetFillPercentage NOTIFY FillPercentageChanged);

public:
    /// @brief Explicit constructor
    /// @param parent The parent of this widget
    explicit DispatchLoadingBar(QWidget* parent = nullptr);

    /// Destructor
    virtual ~DispatchLoadingBar();

    /// Provide a sizeHint for this widget.
    /// This particular widget does not have any specific dimensions that it needs,
    /// so it is really up to the layout to determine the size of this widget.
    /// Since this is the case, just return a simple default size here.
    /// @return The preferred size of the widget based on the current DPI scalefactor.
    virtual QSize sizeHint() const Q_DECL_OVERRIDE;

    /// The current fill percentage of the completion bar.
    /// \return The current filled percentage.
    qreal FillPercentage() const
    {
        return fill_percentage_;
    }

    /// @brief Set how far to fill the bar, should be 1-100.
    /// @param percentage The percentage of the widget width to fill in.
    void SetFillPercentage(qreal percentage);

    /// @brief Sets the text to render as we load.
    ///
    /// @param loader_text The text to display during loading.
    void SetText(QString loader_text);

    /// @brief  Mark the loading bar for errors.
    void MarkError();

signals:
    /// @brief Emitted when the fill percentage has changed.
    /// @param percentage The new fill percentage.
    void FillPercentageChanged(qreal percentage);

protected:
    /// @brief Paint the completion bar.
    /// @param paint_event The painter event.
    virtual void paintEvent(QPaintEvent* paint_event) Q_DECL_OVERRIDE;

private:
    qreal fill_percentage_;  ///< Percentage of the bar that was filled in

    const QColor kEmptyColor_[ColorThemeType::kColorThemeTypeCount] = {
        QColor(204, 204, 204),
        QColor(50, 50, 50)};                          ///< The default color for the empty portion of the widget for each color theme.
    const QColor kFillColor_  = QColor(0, 118, 215);  ///< The default color for the filled portion of the widget.
    const QColor kErrorColor_ = QColor(255, 0, 0);    ///< The default color for error.

    const int kDefaultWidth_  = 350;  ///< Default width of the widget.
    const int kDefaultHeight_ = 20;   ///< Default height of the widget.

    QString loader_text_ = "";  ///< Default text to display as we load.

    bool in_error_state_ = false;  ///< A flag to indicate that the dispatch has errors.
};

#endif  // QTCOMMON_CUSTOM_WIDGETS_COMPLETION_BAR_WIDGET_H_
