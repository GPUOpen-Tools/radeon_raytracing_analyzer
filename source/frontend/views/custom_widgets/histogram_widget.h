//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Header for the histogram graphics view.
//=============================================================================

#ifndef RRA_VIEWS_CUSTOM_WIDGETS_HISTOGRAM_WIDGET_H_
#define RRA_VIEWS_CUSTOM_WIDGETS_HISTOGRAM_WIDGET_H_

#include <vector>

#include <QGraphicsItem>
#include <QGraphicsView>

/// @brief Implements a histogram with range selection indicators.
class HistogramWidget : public QGraphicsView
{
    Q_OBJECT

public:
    /// @brief Storage struct that represents a value range and the number of entries contained within.
    struct HistogramData
    {
        uint32_t min_value;  ///< The minimum of the value range.
        uint32_t max_value;  ///< The maximum of the value range.
        uint32_t count;      ///< The number of entries contained in the range.
    };

    /// @brief The modifiers for the selected range.
    enum HistogramSelectionMode
    {
        kHistogramSelectionModeRange,  ///< Indicates that the whole of the selected range is selected.
        kHistogramSelectionModeLeft,   ///< Indicates that the start of the selected range is selected.
        kHistogramSelectionModeRight   ///< Indicates that the end of the selected range is selected.
    };

    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit HistogramWidget(QWidget* parent = nullptr);

    /// @brief Virtual destructor.
    virtual ~HistogramWidget();

    /// @brief Clears all data from the histogram.
    void Clear();

    /// @brief Sets the histogram data.
    ///
    /// @param [in] data     Pointer to an array of data elements to build the histogram of.
    /// @param [in] num_data Number of elements the data consists of.
    void SetData(const HistogramData* data, uint32_t num_data);

    /// @brief Returns the bounding data for the histogram.
    ///
    /// @return The min/max range and max count for the histogram.
    HistogramData GetBoundingData() const
    {
        return bounding_data_;
    }

    /// @brief Sets the histogram wavefront range selection mode.
    ///
    /// @param [in] mode The wavefront range selection mode.
    void SetSelectionMode(HistogramSelectionMode mode);

public slots:

    /// @brief Sets the bounds and attributes of the selected range.
    ///
    /// @param [in] min_value       The lower bound of the selection range.
    /// @param [in] max_value       The upper bound of the selection range.
    void SetSelectionAttributes(int32_t min_value, int32_t max_value);

protected:
    /// @brief Handles window resize events
    ///
    /// @param [in] event The resize event.
    void resizeEvent(QResizeEvent* event) Q_DECL_OVERRIDE;

private:
    HistogramData bounding_data_;  ///< Stores the min/max range and the max count found in the provided data.

    int32_t selection_min_value_;  ///< The minimum of the selected range.
    int32_t selection_max_value_;  ///< The maximum of the selected range.

    QGraphicsLineItem* selection_range_indicator_left_;   ///< The range minimum selection indicator.
    QGraphicsLineItem* selection_range_indicator_right_;  ///< The range maximum selection indicator.
    QGraphicsRectItem* selection_range_indicator_range_;  ///< The range selection indicator.
    QGraphicsScene*    graphics_scene_;                   ///< The graphics scene containing the indicators.

    HistogramSelectionMode active_selection_mode_;  ///< The active wavefront range selection mode.

    std::vector<QGraphicsRectItem*> histogram_elements_;  ///< Vector keeping track of all of the bucket rectangles in the scene.
};

#endif  // RRA_VIEWS_CUSTOM_WIDGETS_HISTOGRAM_WIDGET_H_

