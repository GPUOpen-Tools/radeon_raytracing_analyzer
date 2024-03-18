//=============================================================================
// Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header of a widget that implements a custom graphics view.
//=============================================================================

#ifndef RRA_VIEWS_CUSTOM_WIDGETS_RAY_HISTORY_GRAPHICS_VIEW_H_
#define RRA_VIEWS_CUSTOM_WIDGETS_RAY_HISTORY_GRAPHICS_VIEW_H_

#include <functional>
#include <QGraphicsView>

/// @brief Information about currently selected pixel of heatmap image.
struct PixelSelectionInfo
{
    uint32_t              x;
    uint32_t              y;
    bool                  selected;
    QGraphicsPolygonItem* triangle_icon;
    QBrush                triangle_brush;
    QPen                  triangle_pen;
};

/// @brief Modern-style colored checkbox that is aware of resize and mouse events.
class RayHistoryGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param parent The parent widget.
    RayHistoryGraphicsView(QWidget* parent);

    /// @brief Destructor.
    ~RayHistoryGraphicsView();

    /// @brief Set the callback to be called when the user does a box select.
    ///
    /// @param callback The callback.
    void SetBoxSelectCallback(std::function<void(uint32_t min_x, uint32_t min_y, uint32_t max_x, uint32_t max_y)> callback);

    /// @brief Set the callback to be called when the user selects a single pixel.
    ///
    /// @param callback The callback.
    void SetPixelSelectCallback(std::function<void(uint32_t x, uint32_t y, bool double_click)> callback);

    /// @brief Set the callback to be called when the user hovers over a pixel in the heatmap.
    ///
    /// @param callback The callback.
    void SetPixelHoverCallback(std::function<void(uint32_t x, uint32_t y, bool in_bounds)> callback);

    /// @brief Set the heatmap image to be displayed.
    ///
    /// @param image The heatmap image.
    void SetHeatmapImage(const QImage& image);

    /// @brief Clear the box select that the user drew.
    void ClearBoxSelect();

    /// @brief Set the pixel to show the selected cursor under.
    ///
    /// @param x The x-coordinate.
    /// @param y The y-coordinate.
    void SetSelectedPixel(uint32_t x, uint32_t y);

signals:
    /// @brief Signal that the state of the zoom buttons need updating.
    ///
    /// @param [in] zoom_in         Is any further zoom in possible? True if so, false otherwise.
    /// @param [in] zoom_out        Is any further zoom out possible? True if so, false otherwise.
    /// @param [in] zoom_selection  Is a zoom-to-selection possible? True if so, false otherwise.
    /// @param [in] zoom_reset      Is a reset possible? True if so, false otherwise.
    void UpdateZoomButtons(bool zoom_in, bool zoom_out, bool zoom_selection, bool reset);

public slots:
    /// @brief Apply a single zoom in. Called when the user clicks the "Zoom in" button.
    void ZoomIn();

    /// @brief Apply a single zoom out. Called when the user clicks the "Zoom out" button.
    void ZoomOut();

    /// @brief Zoom to the selected region in the heatmap.
    void ZoomToSelection();

    /// @brief Reset the zoom level.
    void ResetZoom();

    /// @brief Hide the selected cursor icon.
    void HideSelectedPixelIcon();

private:
    /// @brief Override behavior for the QT mouse move event.
    ///
    /// @param event The QT mouse event.
    virtual void mouseMoveEvent(QMouseEvent* event);

    /// @brief Override behavior for the QT press move event.
    ///
    /// @param event The QT mouse event.
    virtual void mousePressEvent(QMouseEvent* event);

    /// @brief Override behavior for the QT mouse release event.
    ///
    /// @param event The QT mouse event.
    virtual void mouseReleaseEvent(QMouseEvent* event);

    /// @brief Override behavior for the QT mouse double click event.
    ///
    /// @param event The QT mouse event.
    virtual void mouseDoubleClickEvent(QMouseEvent* event);

    /// @brief Override behavior for the QT mouse wheel event.
    ///
    /// @param event The QT mouse wheel event.
    void wheelEvent(QWheelEvent* event);

    /// @brief Create a rectangle to draw surrounding the selected pixels.
    ///
    /// @param rect The rectangle to be drawn.
    void CreateSelectionRect(const QRect& rect);

    /// @brief Clear the rectangle around the selected pixels.
    void ClearSelectionRect();

    /// @brief Crop the heatmap image.
    void CropHeatmap();

    /// @brief Update the icon scale / location marking the selected pixel.
    void UpdateSelectedPixelIcon();

    /// @brief Check if the clicked pixel is in an active portion of the heatmap.
    ///
    /// @return True if selected pixel is active.
    bool IsClickedPixelActive();

    /// @brief Do the actual zoom in or out.
    ///
    /// @param [in] zoom_in  If true, zoom in, else zoom out.
    void Zoom(bool zoom_in);

    /// @brief Update the state of the zoom buttons based on the current zoom state.
    void UpdateZoomButtonState();

    bool                 pan_{false};                  ///< True if currently panning.
    int                  pan_start_x_{0};              ///< The x value of the mouse the previous frame during panning.
    int                  pan_start_y_{0};              ///< The y value of the mouse the previous frame during panning.
    float                zoom_{1.0};                   ///< The current zoom of the viewer.
    QGraphicsScene*      graphics_scene_{};            ///< The graphics scene that is displayed by this graphics view.
    QGraphicsPixmapItem* heatmap_color_{};             ///< The colored heatmap.
    QGraphicsPixmapItem* heatmap_grayscale_{};         ///< The grayscale heatmap.
    QPixmap              heatmap_color_pixmap_{};      ///< The colored heatmap pixmap.
    QPixmap              heatmap_grayscale_pixmap_{};  ///< The grayscale heatmap pixmap.
    QGraphicsRectItem*   selection_rect_{};            ///< Graphical rectangle around selected pixels.
    bool                 left_mouse_held_{};           ///< True if the left mouse button is held down.
    QRect                crop_box_{};                  ///< Rectangle around cropped region of image.
    PixelSelectionInfo   pixel_selected_info_{};       ///< Info about currently selected pixel.
    std::function<void(uint32_t min_x, uint32_t min_y, uint32_t max_x, uint32_t max_y)>
                                                                   box_select_callback_{};    ///< The function called each time box select is change.
    std::function<void(uint32_t x, uint32_t y, bool double_click)> pixel_select_callback_{};  ///< The function called each time a pixel is selected.
    std::function<void(uint32_t x, uint32_t y, bool in_bounds)>    pixel_hover_callback_{};   ///< The function called each time a pixel is hovered over.
};

#endif  // RRA_VIEWS_CUSTOM_WIDGETS_RAY_HISTORY_GRAPHICS_VIEW_H_
