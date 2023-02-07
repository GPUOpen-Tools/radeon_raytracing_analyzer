//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header of a widget that provides additional styles for QSlider.
//=============================================================================

#ifndef RRA_VIEWS_CUSTOM_WIDGETS_SLIDER_STYLE_H_
#define RRA_VIEWS_CUSTOM_WIDGETS_SLIDER_STYLE_H_

#include <QProxyStyle>

class AbsoluteSliderPositionStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    int styleHint(QStyle::StyleHint hint, const QStyleOption* option = 0, const QWidget* widget = 0, QStyleHintReturn* returnData = 0) const
    {
        if (hint == QStyle::SH_Slider_AbsoluteSetButtons)
            return (Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

#endif  // RRA_VIEWS_CUSTOM_WIDGETS_SLIDER_STYLE_H_
