//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header of a widget that implements a custom radio button.
//=============================================================================

#ifndef RRA_VIEWS_CUSTOM_WIDGETS_COLORED_RADIO_BUTTON_H_
#define RRA_VIEWS_CUSTOM_WIDGETS_COLORED_RADIO_BUTTON_H_

#include <QRadioButton>

/// @brief Modern-style colored checkbox that is aware of resize and mouse events.
class ColoredRadioButton : public QRadioButton
{
    Q_OBJECT
    Q_PROPERTY(qreal button_text_ratio READ GetButtonTextRatio WRITE SetButtonTextRatio)

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit ColoredRadioButton(QWidget* parent);

    /// @brief Destructor.
    virtual ~ColoredRadioButton();

    /// @brief Initialize a ColoredCheckbox.
    ///
    /// @param [in] checked         Whether the checkbox starts in the checked state or not.
    /// @param [in] primary_color   Primary color theme.
    void Initialize(bool checked, const QColor& primary_color);

    /// @brief Update the text to be displayed next to the checkbox.
    ///
    /// @param [in] text The text to be displayed.
    void UpdateText(const QString& text);

    /// @brief Update the primary checkbox color. This is the color used in single-colored checkboxes.
    ///
    /// @param [in] color The primary color to use.
    void UpdatePrimaryColor(const QColor& color);

    /// @brief Set the button-to-text ratio.
    ///
    /// @param [in] button_text_ratio The button to text ratio.
    void SetButtonTextRatio(qreal button_text_ratio);

    /// @brief Get the button-to-text ratio.
    ///
    /// @return The button to text ratio.
    qreal GetButtonTextRatio() const;

signals:
    /// @brief Signal to indicate that the checkbox was clicked on.
    void Clicked();

protected:
    /// @brief Provides a desired sizeHint that allows the text and bar to be visible.
    ///
    /// @return The size hint.
    virtual QSize sizeHint() const Q_DECL_OVERRIDE;

    /// @brief Implementation of Qt's paint for this widget.
    ///
    /// @param [in] event The paint event.
    virtual void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;

    /// @brief Overridden window resize event.
    ///
    /// @param [in] event The resize event object.
    virtual void resizeEvent(QResizeEvent* event) Q_DECL_OVERRIDE;

    /// @brief Implementation of Qt's mouse press event for this widget.
    ///
    /// @param [in] event The mouse press event.
    virtual void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;

private:
    /// @brief Get the switch height based on the font size.
    ///
    /// @param [in] font_metrics The font metrics for the font used by this widget.
    ///
    /// @return The switch height.
    qreal GetSwitchHeight(const QFontMetricsF& font_metrics) const;

    QColor color_;              ///< Color for radio button.
    qreal  button_text_ratio_;  ///< The button-to-text ratio ie how much bigger the button is relative to the button text.
};

#endif  // RRA_VIEWS_CUSTOM_WIDGETS_COLORED_RADIO_BUTTON_H_

