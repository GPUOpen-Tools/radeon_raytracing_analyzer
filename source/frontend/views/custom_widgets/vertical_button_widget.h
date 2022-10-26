//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Header file for a vertical QPushButton.
//=============================================================================

#ifndef RRA_VIEWS_CUSTOM_WIDGETS_VERTICAL_BUTTON_WIDGET_H_
#define RRA_VIEWS_CUSTOM_WIDGETS_VERTICAL_BUTTON_WIDGET_H_

#include <QPushButton>

/// This class implements a rotated QPushButton.
class VerticalButtonWidget : public QPushButton
{
    Q_OBJECT

public:
    /// @brief Explicit constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit VerticalButtonWidget(QWidget* parent = nullptr);

    /// @brief Virtual destructor.
    virtual ~VerticalButtonWidget();

protected:
    /// @brief Overridden paint event for this QPushButton.
    ///
    /// @param [in] event The paint event.
    virtual void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;

    /// @brief Overridden minimum size hint.
    ///
    /// @return The minimum size hint.
    virtual QSize minimumSizeHint() const Q_DECL_OVERRIDE;

    /// @brief Overridden size hint.
    ///
    /// @return The size hint.
    virtual QSize sizeHint() const Q_DECL_OVERRIDE;
};

#endif  // RRA_VIEWS_CUSTOM_WIDGETS_VERTICAL_BUTTON_WIDGET_H_
