//=============================================================================
/// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Header for a depth slider widget.
//=============================================================================
#ifndef QTCOMMON_CUSTOM_WIDGETS_DEPTH_SLIDER_WIDGET_H_
#define QTCOMMON_CUSTOM_WIDGETS_DEPTH_SLIDER_WIDGET_H_

#include <QWidget>
#include <QObject>
#include <QPainter>

/// Class definition for the depth slider widget, which allows the user to select
/// a range of values. This is used for the tree-depth slider in the TLAS pane.
///
/// There's currently no support for orientation; a horizontal slider is assumed.
class DepthSliderWidget : public QWidget
{
    Q_OBJECT

public:
    /// Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit DepthSliderWidget(QWidget* parent = nullptr);

    /// Destructor.
    virtual ~DepthSliderWidget();

    /// Set lower value for the depth slider.
    ///
    /// @param [in] lower_value The lower value of the depth slider.
    void SetLowerValue(int lower_value);

    /// Set upper value for the depth slider.
    ///
    /// @param [in] upper_value The upper value of the depth slider.
    void SetUpperValue(int upper_value);

    /// Set upper limit for the depth slider.
    ///
    /// @param [in] upper_bound The upper bound of the depth slider.
    void SetUpperBound(int upper_bound);

signals:
    /// @brief Signal to indicate that the slider values have changed.
    ///
    /// @param [in] lower_value The lower slider value.
    /// @param [in] upper_value The upper slider value.
    void SpanChanged(int lower_value, int upper_value);

protected:
    /// Override Qt's mouse press event for the depth slider.
    ///
    /// @param [in] event The mouse event of the depth slider.
    virtual void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;

    /// Override Qt's mouse move event for the depth slider.
    ///
    /// @param [in] event The mouse move event of the depth slider.
    virtual void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;

    /// Override Qt's paint event for the depth slider.
    ///
    /// @param [in] event The paint event of the depth slider.
    virtual void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;

private:
    /// Get the offset of the segment that the mouse is currently over.
    ///
    /// @param [in] event The mouse move event of the depth slider.
    ///
    /// @return The segment offset.
    float GetSegmentOffset(QMouseEvent* event);

    int clicked_value_;  ///< The value of the slider clicked on.
    int lower_value_;    ///< Lower value of the slider.
    int upper_value_;    ///< Upper value of the slider.
    int upper_bound_;    ///< The upper bound of the slider.
};

#endif  // QTCOMMON_CUSTOM_WIDGETS_DEPTH_SLIDER_WIDGET_H_
