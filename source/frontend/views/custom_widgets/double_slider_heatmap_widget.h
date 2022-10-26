//=============================================================================
/// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief Header for the double slider with heatmap.
//=============================================================================
#ifndef RRA_VIEWS_CUSTOM_WIDGETS_DOUBLE_SLIDER_HEATMAP_WIDGET_H_
#define RRA_VIEWS_CUSTOM_WIDGETS_DOUBLE_SLIDER_HEATMAP_WIDGET_H_

#include <QSlider>
#include <QStyle>
#include <QStylePainter>
#include <QObject>

class DoubleSliderHeatmapWidget : public QSlider
{
    Q_OBJECT

    Q_PROPERTY(int lower_value READ LowerValue WRITE SetLowerValue)
    Q_PROPERTY(int upper_value READ UpperValue WRITE SetUpperValue)
    Q_PROPERTY(int lower_position READ LowerPosition WRITE SetLowerPosition)
    Q_PROPERTY(int upper_position READ UpperPosition WRITE SetUpperPosition)
    Q_PROPERTY(HandleMovementModeType movement_mode READ HandleMovementMode WRITE SetHandleMovementMode)
    Q_ENUMS(HandleMovementModeType)

public:
    /// Constructor
    /// \param parent The parent of double slider.
    explicit DoubleSliderHeatmapWidget(QWidget* parent = nullptr);

    /// Constructor
    /// \param orientation The type of orientation of slider.
    /// \param parent The parent of double slider.
    explicit DoubleSliderHeatmapWidget(Qt::Orientation orientation, QWidget* parent = nullptr);

    /// Destructor
    virtual ~DoubleSliderHeatmapWidget();

    /// The DoubleSliderWidget can shrink / expand to fill whatever amount of space
    /// is allocated by the layout, so simply return a default sizeHint that is
    /// scaled according to the ScalingManager.
    /// \return A default size hint.
    virtual QSize sizeHint() const Q_DECL_OVERRIDE;

    enum HandleMovementModeType
    {
        kFreeMovement,
        kNoCrossing,
        kNoOverlapping
    };

    enum SpanHandle
    {
        kNoHandle,
        kLowerHandle,
        kUpperHandle
    };

    /// Intializes member variables to default values.
    void Init();

    /// Initializes style options of double slider.
    /// \param option The style option of double slider.
    void InitStyleOption(QStyleOptionSlider* option, DoubleSliderHeatmapWidget::SpanHandle span_handle = DoubleSliderHeatmapWidget::kUpperHandle) const;

    /// Gets the handle movement mode for this widget.
    /// \return The handle movement mode.
    HandleMovementModeType HandleMovementMode() const;

    /// Set handle movement mode for this widget.
    /// \param movement_mode The mode to be set.
    void SetHandleMovementMode(HandleMovementModeType movement_mode);

    /// Get the lower value for this widget.
    /// \return The lower value of slider.
    int LowerValue() const;

    /// Gets the upper value for this widget.
    /// \return The upper value of slider
    int UpperValue() const;

    /// Get the lower position for this widget.
    /// \return The lower position of slider.
    int LowerPosition() const;

    /// Get the upper position for this widget.
    /// \return The upper position of slider.
    int UpperPosition() const;

    /// Gets the value associated with the supplied point.
    /// \param handle_point A point along the slider.
    /// \return The x or y coordinate depending on the slider orientation.
    int Pick(const QPoint& handle_point) const;

    /// Converts pixel positions to range value for this item.
    /// \param pixel_position The position of slider.
    /// \return The range value from the pixel value.
    int PixelPosToRangeValue(int pixel_position) const;

    /// Handle the mouse press event.
    /// \param mouse_position The position of mouse press.
    /// \param control The control of Double Slider.
    /// \param value The value of the position of the slider.
    /// \param span_handle The slider handle which generated the mouse press event.
    void HandleMousePress(const QPoint& mouse_position, QStyle::SubControl& control, int value, DoubleSliderHeatmapWidget::SpanHandle span_handle);

    /// This helper function is for mouse move, but is also called
    /// from mouse down events in a few cases.
    /// \param mouse_position The position of mouse.
    void HandleMouseMove(const QPoint& mouse_position);

    /// Sets trigger actions for this item.
    /// \param slider_action The slider action for this item.
    /// \param main_action This variable states wheter slider action is main action.
    void TriggerAction(QAbstractSlider::SliderAction slider_action, bool main_action);

    /// Swaps the left and right handles.
    void SwapControls();

    /// @brief Set the heatmap.
    /// @param heatmap The heatmap to set.
    void SetHeatmap(QPixmap heatmap);

public slots:
    /// Set lower value for the double slider.
    /// \param lower_value The lower value of double slider.
    void SetLowerValue(int lower_value);

    /// Implementation of set upper value for the double slider.
    /// \param upper_value The upper value of double slider.
    void SetUpperValue(int upper_value);

    /// Implementation of set Span for the double slider.
    /// \param lower_value The lower value of double slider.
    /// \param upper_value The upper value of double slider.
    void SetSpan(int lower_value, int upper_value);

    /// Set the lower position for the double slider.
    /// \param lower_position The lower position of double slider.
    void SetLowerPosition(int lower_position);

    /// Set the upper position for the double slider.
    /// \param upper_position The upper position of double slider.
    void SetUpperPosition(int upper_position);

    /// Update the range for this item.
    /// \param min_value The min value of double slider.
    /// \param max_value The max value of double slider.
    void UpdateRange(int min_value, int max_value);

    /// Trigger actions when a handle is pressed and moved.
    void MovePressedHandle();

signals:
    void SpanChanged(int lower_value, int upper_value);

    void LowerValueChanged(int lower_value);

    void UpperValueChanged(int upper_value);

    void LowerPositionChanged(int lower_position);

    void UpperPositionChanged(int upper_position);

    void SliderPressed(SpanHandle slider_handle);

    void SliderReleased();

protected:
    /// Override Qt's key press event for the double slider.
    /// \param event The key event of double slider.
    virtual void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;

    /// Override Qt's mouse press event for the double slider.
    /// \param event The mouse event of double slider.
    virtual void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;

    /// Override Qt's mouse move event for the double slider.
    /// \param event The mouse move event of double slider.
    virtual void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;

    /// Override Qt's mouse release event for the double slider.
    /// \param event The mouse event of double slider.
    virtual void mouseReleaseEvent(QMouseEvent* event) Q_DECL_OVERRIDE;

    /// Override Qt's paint event for the double slider.
    /// \param event The mouse event of double slider.
    virtual void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;

private:
    /// Draw a handle for this item.
    /// \param painter The painter used to draw span for this item.
    /// \param span_handle The handle of double slider.
    void DrawHandle(QStylePainter* painter, DoubleSliderHeatmapWidget::SpanHandle span_handle) const;

    /// Draw the heatmap for this item.
    /// \param painter The painter used to draw heatmap for this item.
    /// \param span_area The area of heatmap.
    void DrawHeatmap(QStylePainter* painter, const QRect& span_area) const;

    /// Setup painter for this item.
    /// \param painter The painter used to draw this item.
    /// \param orientation The type of orientation applied on this item.
    /// \param start_x x co-ordinate of point1 in linear gradient.
    /// \param start_y y co-ordinate of point1 in linear gradient.
    /// \param end_x x co-ordinate of point2 in linear gradient.
    /// \param end_y y co-ordinate of point2 in linear gradient.
    void SetupPainter(QPainter* painter, Qt::Orientation orientation, qreal start_x, qreal start_y, qreal end_x, qreal end_y) const;

    int                                               lower_value_;            ///< lower value of the slider
    int                                               upper_value_;            ///< upper value of the slider
    int                                               lower_pos_;              ///< lower position of the slider
    int                                               upper_pos_;              ///< upper position of the slider
    int                                               offset_pos_;             ///< offset to the position
    int                                               position_;               ///< position of slider
    DoubleSliderHeatmapWidget::SpanHandle             last_pressed_span_;      ///< last pressed state of span handle
    DoubleSliderHeatmapWidget::SpanHandle             main_span_control_;      ///< main control of span handle
    QStyle::SubControl                                lower_pressed_control_;  ///< style of lower pressed
    QStyle::SubControl                                upper_pressed_control_;  ///< style of upper pressed
    DoubleSliderHeatmapWidget::HandleMovementModeType handle_movement_;        ///< movement of handle
    bool                                              is_first_movement_;      ///< states first movement
    bool                                              block_tracking_;         ///< states tracking of the movement
    QPixmap                                           heatmap_;                ///< The heatmap to paint in between handles.
};

#endif  // QTCOMMON_CUSTOM_WIDGETS_DOUBLE_SLIDER_WIDGET_H_
