//=============================================================================
/// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief Widget for a colored legend.
//=============================================================================

#ifndef RRA_VIEWS_CUSTOM_WIDGETS_DISPATCH_LEGEND_H_
#define RRA_VIEWS_CUSTOM_WIDGETS_DISPATCH_LEGEND_H_

#include <QWidget>

class DispatchLegend : public QWidget
{
    Q_OBJECT

public:
    /// Constructor
    ///
    /// @param [in]  parent The parent widget
    DispatchLegend(QWidget* parent);

    /// Virtual destructor
    virtual ~DispatchLegend();

    /// @brief Set the color of this widget.
    void SetColor(const QColor& color);

    /// @brief Custom sizeHint implementation that supports DPI scaling.
    ///
    /// @return A default sizeHint since the size of this widget can grow to fit
    /// the space allowed by the layout.
    virtual QSize sizeHint() const Q_DECL_OVERRIDE;

protected:
    /// @brief Implementation of Qt's paint for this widget.
    ///
    /// @param [in] event The paint event.
    virtual void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;

private slots:
    /// Callback for when the DPI scale factor changes
    void OnScaleFactorChanged();

private:
    QColor color_;   ///< The color of the legend.
};

#endif  // RRA_VIEWS_CUSTOM_WIDGETS_DISPATCH_LEGEND_H_
