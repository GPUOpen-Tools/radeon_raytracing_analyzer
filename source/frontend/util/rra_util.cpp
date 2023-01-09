//=============================================================================
// Copyright (c) 2018-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of rra_util which holds useful utility functions.
//=============================================================================

#include "util/rra_util.h"

#include <QDir>
#include <cmath>

#include "public/rra_assert.h"

#include "settings/settings.h"
#include "version.h"

/// @brief Get the brightness of a given color.
///
/// Adds weighting values to the color components to compute a color brightness. HSV won't work that well here
/// as the brightness for 2 given hues may be different for identical saturation and value.
/// Uses a standard luminance formula.
///
/// @param [in] background_color The color to calculate the brightness of.
///
/// @return The brightness (0 is black, 255 is white).
static int GetColorBrightness(const QColor& background_color)
{
    double r          = background_color.red();
    double g          = background_color.green();
    double b          = background_color.blue();
    double brightness = (0.3 * r) + (0.59 * g) + 0.11 * b;

    return static_cast<int>(brightness);
}

QColor rra_util::GetTextColorForBackground(const QColor& background_color, bool has_white_background)
{
    int brightness = GetColorBrightness(background_color);
    if (brightness > 128)
    {
        return Qt::black;
    }
    else
    {
        if (has_white_background)
        {
            return Qt::lightGray;
        }
        return Qt::white;
    }
}

bool rra_util::TraceValidToLoad(const QString& trace_path)
{
    bool may_load = false;

    QFileInfo trace_file(trace_path);
    if (trace_file.exists() && trace_file.isFile())
    {
        const QString extension = trace_path.mid(trace_path.lastIndexOf("."), trace_path.length());
        if (extension.compare(rra::text::kRRATraceFileExtension, Qt::CaseInsensitive) == 0)
        {
            may_load = true;
        }
    }

    return may_load;
}
