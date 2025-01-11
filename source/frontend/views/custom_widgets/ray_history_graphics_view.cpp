//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of a widget that implements a custom graphics view.
//=============================================================================

#include "ray_history_graphics_view.h"

#include <algorithm>
#include <QMouseEvent>
#include <QScrollBar>
#include <QGraphicsLineItem>
#include <QGuiApplication>
#include <QScreen>

constexpr float viewer_width{1000.0f};
constexpr float viewer_height{1000.0f};
constexpr float max_zoom{20.0f};
constexpr float min_zoom{0.5f};

RayHistoryGraphicsView::RayHistoryGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
{
    setDragMode(QGraphicsView::RubberBandDrag);

    heatmap_color_ = new QGraphicsPixmapItem{};
    heatmap_color_->setZValue(2.0);

    heatmap_grayscale_ = new QGraphicsPixmapItem{};
    heatmap_grayscale_->setPixmap(heatmap_grayscale_pixmap_);
    heatmap_grayscale_->setZValue(0.0);

    graphics_scene_ = new QGraphicsScene(this);
    graphics_scene_->addItem(heatmap_grayscale_);
    graphics_scene_->addItem(heatmap_color_);

    pixel_selected_info_.triangle_brush.setColor(Qt::black);
    pixel_selected_info_.triangle_brush.setStyle(Qt::SolidPattern);
    pixel_selected_info_.triangle_pen.setColor(Qt::white);
    pixel_selected_info_.triangle_pen.setWidth(1);
    pixel_selected_info_.triangle_pen.setCosmetic(true);

    setScene(graphics_scene_);
    setMouseTracking(true);
    setRenderHint(QPainter::Antialiasing);
}

RayHistoryGraphicsView::~RayHistoryGraphicsView()
{
}

void RayHistoryGraphicsView::SetBoxSelectCallback(std::function<void(uint32_t min_x, uint32_t min_y, uint32_t max_x, uint32_t max_y)> callback)
{
    box_select_callback_ = callback;
}

void RayHistoryGraphicsView::SetPixelSelectCallback(std::function<void(uint32_t x, uint32_t y, bool double_click)> callback)
{
    pixel_select_callback_ = callback;
}

void RayHistoryGraphicsView::SetPixelHoverCallback(std::function<void(uint32_t x, uint32_t y, bool in_bounds)> callback)
{
    pixel_hover_callback_ = callback;
}

void RayHistoryGraphicsView::SetHeatmapImage(const QImage& image)
{
    heatmap_color_pixmap_.convertFromImage(image);
    heatmap_color_->setPixmap(heatmap_color_pixmap_);

    heatmap_grayscale_pixmap_.convertFromImage(image.convertToFormat(QImage::Format_Grayscale8));
    heatmap_grayscale_->setPixmap(heatmap_grayscale_pixmap_);

    graphics_scene_->setSceneRect(heatmap_grayscale_pixmap_.rect());

    CropHeatmap();
}

void RayHistoryGraphicsView::ClearBoxSelect()
{
    pixel_selected_info_.selected = false;
    UpdateSelectedPixelIcon();

    if (crop_box_ == QRect())
    {
        return;
    }

    crop_box_ = QRect{};
    heatmap_color_->setPixmap(heatmap_color_pixmap_);
    heatmap_color_->setPos(0, 0);
    box_select_callback_(0, 0, std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max());
    ClearSelectionRect();
}

void RayHistoryGraphicsView::SetSelectedPixel(uint32_t x, uint32_t y)
{
    pixel_selected_info_.x        = x;
    pixel_selected_info_.y        = y;
    pixel_selected_info_.selected = true;

    UpdateSelectedPixelIcon();
}

void RayHistoryGraphicsView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton)
    {
        pan_ = true;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        pan_start_x_ = event->x();
        pan_start_y_ = event->y();
#else
        pan_start_x_ = event->position().toPoint().x();
        pan_start_y_ = event->position().toPoint().y();
#endif
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton)
    {
        left_mouse_held_ = true;
        QGraphicsView::mousePressEvent(event);
    }
    event->ignore();
}

bool RectangleSmallerThanEpsilon(const QRect& rect)
{
    return rect.width() + rect.height() < 3.0f;
}

void RayHistoryGraphicsView::UpdateSelectedPixelIcon()
{
    if (pixel_selected_info_.triangle_icon != nullptr)
    {
        graphics_scene_->removeItem(pixel_selected_info_.triangle_icon);
        pixel_selected_info_.triangle_icon = nullptr;
    }

    if (!pixel_selected_info_.selected)
    {
        return;
    }

    // Create triangle.
    float height{20.0f / zoom_};
    float divot{6.0f / zoom_};
    float width{10.0f / zoom_};

    QPointF position = QPointF{pixel_selected_info_.x + 0.5f, pixel_selected_info_.y + 1.0f};

    QPolygonF triangle{};
    triangle.append(position + QPointF{-width, height});        // Bottom left.
    triangle.append(position + QPointF{0.0f, 0.0f});            // Top.
    triangle.append(position + QPointF{width, height});         // Bottom right.
    triangle.append(position + QPointF{0.0f, height - divot});  // Bottom middle.
    triangle.append(position + QPointF{-width, height});        // Bottom left again.

    pixel_selected_info_.triangle_icon = graphics_scene_->addPolygon(triangle, pixel_selected_info_.triangle_pen, pixel_selected_info_.triangle_brush);
    pixel_selected_info_.triangle_icon->setZValue(3.0);
}

bool RayHistoryGraphicsView::IsClickedPixelActive()
{
    QPoint clicked_position{(int)pixel_selected_info_.x, (int)pixel_selected_info_.y};
    return crop_box_.contains(clicked_position) || (crop_box_.isEmpty() && heatmap_color_pixmap_.rect().contains(clicked_position));
}

void RayHistoryGraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton)
    {
        pan_ = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton)
    {
        left_mouse_held_ = false;

        // Convert rubber band box to use heatmap coordinates.
        QRect rubber_band_rect = mapToScene(rubberBandRect()).boundingRect().toRect();

        // Single pixel was clicked.
        if (RectangleSmallerThanEpsilon(rubber_band_rect))
        {
            QPointF clicked_position = mapToScene(event->pos());
            pixel_selected_info_.x   = (uint32_t)clicked_position.x();
            pixel_selected_info_.y   = (uint32_t)clicked_position.y();

            if (IsClickedPixelActive())
            {
                pixel_select_callback_((uint32_t)clicked_position.x(), (uint32_t)clicked_position.y(), false);
                pixel_selected_info_.selected = true;
            }
            else
            {
                pixel_selected_info_.selected = false;
            }

            UpdateSelectedPixelIcon();
        }
        // Box was selected.
        else
        {
            crop_box_ = rubber_band_rect;

            // Constrain the box to be inside the heatmap.
            crop_box_.setX(std::max(crop_box_.x(), 0));
            crop_box_.setY(std::max(crop_box_.y(), 0));
            crop_box_.setWidth(std::min(crop_box_.width(), std::abs(crop_box_.x() - heatmap_color_pixmap_.width())));
            crop_box_.setHeight(std::min(crop_box_.height(), std::abs(crop_box_.y() - heatmap_color_pixmap_.height())));

            pixel_selected_info_.selected = false;
            CropHeatmap();
            UpdateSelectedPixelIcon();
            UpdateZoomButtonState();
        }

        QGraphicsView::mouseReleaseEvent(event);
        event->accept();
        return;
    }
    if (event->button() == Qt::RightButton)
    {
        if (left_mouse_held_)
        {
            // There is no way to directly cancel the rubber band box, so we disable and re-enable it.
            setDragMode(QGraphicsView::NoDrag);
            setDragMode(QGraphicsView::RubberBandDrag);
        }
        else
        {
            ClearBoxSelect();
            pixel_selected_info_.selected = false;
            UpdateSelectedPixelIcon();
            UpdateZoomButtonState();
        }

        event->accept();
        return;
    }
    event->ignore();
}

void RayHistoryGraphicsView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        QPointF clicked_position = mapToScene(event->pos());
        pixel_selected_info_.x   = (uint32_t)clicked_position.x();
        pixel_selected_info_.y   = (uint32_t)clicked_position.y();

        if (IsClickedPixelActive())
        {
            pixel_select_callback_((uint32_t)clicked_position.x(), (uint32_t)clicked_position.y(), true);
        }
    }
}

void RayHistoryGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    if (pan_)
    {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        int x_pos = event->x();
        int y_pos = event->y();
#else
        int x_pos = event->position().toPoint().x();
        int y_pos = event->position().toPoint().y();
#endif
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - (x_pos - pan_start_x_));
        verticalScrollBar()->setValue(verticalScrollBar()->value() - (y_pos - pan_start_y_));

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        pan_start_x_ = event->x();
        pan_start_y_ = event->y();
#else
        pan_start_x_ = event->position().toPoint().x();
        pan_start_y_ = event->position().toPoint().y();
#endif
        event->accept();
        return;
    }

    QPointF mouse_position = mapToScene(event->pos());
    pixel_hover_callback_((uint32_t)mouse_position.x(), (uint32_t)mouse_position.y(), heatmap_grayscale_->boundingRect().contains(mouse_position));

    QGraphicsView::mouseMoveEvent(event);
}

void RayHistoryGraphicsView::wheelEvent(QWheelEvent* event)
{
    if (event->angleDelta().y() != 0)
    {
        bool zoom_in{event->angleDelta().y() > 0};
        bool zoom_out{event->angleDelta().y() < 0};

        if ((zoom_in && zoom_ < max_zoom) || (zoom_out && zoom_ > min_zoom))
        {
            const ViewportAnchor anchor = transformationAnchor();
            setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

            Zoom(zoom_in);

            QPointF mouse_position = mapToScene(event->position().toPoint());
            pixel_hover_callback_((uint32_t)mouse_position.x(), (uint32_t)mouse_position.y(), heatmap_grayscale_->boundingRect().contains(mouse_position));

            setTransformationAnchor(anchor);
            event->accept();
        }

        // Send signals to update the zoom buttons.
        UpdateZoomButtonState();
    }
    event->ignore();
}

void RayHistoryGraphicsView::CreateSelectionRect(const QRect& rect)
{
    ClearSelectionRect();

    QPen pen{};
    pen.setWidth(5);
    pen.setCosmetic(true);
    pen.setDashPattern({2.0, 4.0});

    selection_rect_ = new QGraphicsRectItem{};
    selection_rect_->setRect(rect);
    selection_rect_->setZValue(1.0);
    selection_rect_->setPen(pen);
    graphics_scene_->addItem(selection_rect_);
}

void RayHistoryGraphicsView::ClearSelectionRect()
{
    if (selection_rect_)
    {
        graphics_scene_->removeItem(selection_rect_);
        delete selection_rect_;
        selection_rect_ = nullptr;
    }
}

void RayHistoryGraphicsView::CropHeatmap()
{
    QPixmap cropped{heatmap_color_pixmap_.copy(crop_box_)};

    if (cropped.size() != heatmap_color_pixmap_.size())
    {
        // Allows filtering rays by invocation ID.
        box_select_callback_(crop_box_.x(), crop_box_.y(), crop_box_.x() + crop_box_.width() - 1, crop_box_.y() + crop_box_.height() - 1);

        heatmap_color_->setPixmap(cropped);
        heatmap_color_->setPos(crop_box_.x(), crop_box_.y());

        // Draw rectangle around selection.
        CreateSelectionRect(crop_box_);
    }
}

void RayHistoryGraphicsView::Zoom(bool zoom_in)
{
    float factor = {zoom_in ? 1.1f : 0.9f};
    zoom_ *= factor;
    scale(factor, factor);

    CreateSelectionRect(crop_box_);
    UpdateSelectedPixelIcon();
}

void RayHistoryGraphicsView::ResetZoom()
{
    zoom_ = 1.0f;
    resetTransform();

    CreateSelectionRect(crop_box_);
    UpdateSelectedPixelIcon();

    UpdateZoomButtonState();
}

void RayHistoryGraphicsView::ZoomIn()
{
    Zoom(true);
    UpdateZoomButtonState();
}

void RayHistoryGraphicsView::ZoomOut()
{
    Zoom(false);
    UpdateZoomButtonState();
}

void RayHistoryGraphicsView::ZoomToSelection()
{
    resetTransform();

    float x_scale = (float)width() / (float)crop_box_.width();
    float y_scale = (float)height() / (float)crop_box_.height();
    zoom_         = std::min(x_scale, y_scale);
    zoom_         = std::min(zoom_, max_zoom);

    const ViewportAnchor anchor = transformationAnchor();
    setTransformationAnchor(QGraphicsView::NoAnchor);
    scale(zoom_, zoom_);
    float center_x = (float)crop_box_.x() + ((float)crop_box_.width() / 2.0f);
    float center_y = (float)crop_box_.y() + ((float)crop_box_.height() / 2.0f);
    QGraphicsView::centerOn(center_x, center_y);
    setTransformationAnchor(anchor);

    UpdateZoomButtonState();
}

void RayHistoryGraphicsView::UpdateZoomButtonState()
{
    constexpr float epsilon = 0.00001f;

    bool reset     = (zoom_ < 1.0f - epsilon || zoom_ > 1.0f + epsilon);
    bool selection = crop_box_ != QRect();
    emit UpdateZoomButtons(zoom_<max_zoom, zoom_> min_zoom, selection, reset);
}

void RayHistoryGraphicsView::HideSelectedPixelIcon()
{
    pixel_selected_info_.selected = false;
    UpdateSelectedPixelIcon();
}
