//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief Source for the RGP histogram graphics view.
//=============================================================================

#include "views/custom_widgets/rgp_histogram_widget.h"

static const float    kSelectionRangeAlpha     = 0.125f;
static const QColor   kSelectionRangeColor     = QColor(128, 32, 32, 128);
static const QColor   kSelectedBucketColor     = QColor(0, 118, 215);
static const QColor   kUnselectedBucketColor   = Qt::gray;
static const QPen     kSelectionLinePenDefault = QPen(Qt::black, 0, Qt::SolidLine);
static const QPen     kSelectionLinePenActive  = QPen(kSelectionRangeColor, 3, Qt::SolidLine, Qt::FlatCap);
static const QBrush   kSelectionRangeBrush     = QBrush(kSelectionRangeColor);
static const QPen     kSelectionRangePen       = QPen(kSelectionRangeColor, 0, Qt::SolidLine);
static const QBrush   kSelectedBucketBrush     = QBrush(kSelectedBucketColor, Qt::SolidPattern);
static const QPen     kSelectedBucketPen       = QPen(kSelectedBucketColor, 0, Qt::SolidLine, Qt::SquareCap);
static const QBrush   kUnselectedBucketBrush   = QBrush(kUnselectedBucketColor, Qt::SolidPattern);
static const QPen     kUnselectedBucketPen     = QPen(kUnselectedBucketColor, 0, Qt::SolidLine, Qt::SquareCap);
static const uint32_t kHistogramSceneHeight    = 40;
static const uint32_t kHistogramSceneWidth     = 350;
static const QSize    kHistogramBaseSize       = QSize(kHistogramSceneWidth, kHistogramSceneHeight);
static const uint32_t kSelectionLinesZValue    = 2;
static const uint32_t kSelectionRangeZValue    = 1;
static const uint32_t kHistogramItemZValue     = 0;

RgpHistogramWidget::RgpHistogramWidget(QWidget* parent)
    : QGraphicsView(parent)
    , bounding_data_({UINT32_MAX, 0, 0})
    , selection_min_value_(0)
    , selection_max_value_(INT32_MAX)
    , selection_range_indicator_left_(nullptr)
    , selection_range_indicator_right_(nullptr)
    , selection_range_indicator_range_(nullptr)
    , active_selection_mode_(kHistogramSelectionModeRange)
{
    QGraphicsScene* graphics_scene = new QGraphicsScene(this);
    setScene(graphics_scene);

    selection_range_indicator_left_ = new QGraphicsLineItem();
    selection_range_indicator_left_->setPen(kSelectionLinePenDefault);
    selection_range_indicator_left_->setZValue(kSelectionLinesZValue);
    selection_range_indicator_left_->setFlag(QGraphicsItem::ItemIsSelectable, false);
    graphics_scene->addItem(selection_range_indicator_left_);

    selection_range_indicator_right_ = new QGraphicsLineItem();
    selection_range_indicator_right_->setPen(kSelectionLinePenDefault);
    selection_range_indicator_right_->setZValue(kSelectionLinesZValue);
    selection_range_indicator_right_->setFlag(QGraphicsItem::ItemIsSelectable, false);
    graphics_scene->addItem(selection_range_indicator_right_);

    selection_range_indicator_range_ = new QGraphicsRectItem();
    selection_range_indicator_range_->setBrush(kSelectionRangeBrush);
    selection_range_indicator_range_->setPen(kSelectionRangePen);
    selection_range_indicator_range_->setZValue(kSelectionRangeZValue);
    selection_range_indicator_range_->setOpacity(kSelectionRangeAlpha);
    selection_range_indicator_range_->setFlag(QGraphicsItem::ItemIsSelectable, false);
    graphics_scene->addItem(selection_range_indicator_range_);

    setFixedSize(kHistogramBaseSize);
}

RgpHistogramWidget::~RgpHistogramWidget()
{
}

void RgpHistogramWidget::Clear()
{
    histogram_elements_.clear();

    QGraphicsScene* graphics_scene = scene();
    if (graphics_scene)
    {
        graphics_scene->removeItem(selection_range_indicator_left_);
        graphics_scene->removeItem(selection_range_indicator_right_);
        graphics_scene->removeItem(selection_range_indicator_range_);
        graphics_scene->clear();
        graphics_scene->addItem(selection_range_indicator_left_);
        graphics_scene->addItem(selection_range_indicator_right_);
        graphics_scene->addItem(selection_range_indicator_range_);
    }
}

void RgpHistogramWidget::SetData(const RgpHistogramData* histogram_data, uint32_t num_data)
{
    Clear();

    bounding_data_.min_value = UINT32_MAX;
    bounding_data_.max_value = 0;
    bounding_data_.count     = 0;

    for (uint32_t i = 0; i < num_data; ++i)
    {
        RgpHistogramData hist_data = histogram_data[i];

        bounding_data_.min_value = std::min(bounding_data_.min_value, hist_data.min_value);
        bounding_data_.max_value = std::max(bounding_data_.max_value, hist_data.max_value);
        bounding_data_.count     = std::max(bounding_data_.count, hist_data.count);
    }

    if (bounding_data_.count == 0)
    {
        return;
    }

    QGraphicsScene* graphics_scene = scene();

    selection_min_value_ = bounding_data_.min_value;
    selection_max_value_ = bounding_data_.max_value;

    const float height_normalize = kHistogramSceneHeight / static_cast<float>(bounding_data_.count);
    const float item_width       = kHistogramSceneWidth / static_cast<float>(num_data);

    for (uint32_t i = 0; i < num_data; ++i)
    {
        RgpHistogramData hist_data = histogram_data[i];
        if (hist_data.count > 0)
        {
            const uint32_t height = hist_data.count * height_normalize;
            const uint32_t top    = kHistogramSceneHeight - height;

            QGraphicsRectItem* item = graphics_scene->addRect(item_width * i, top, item_width, height);
            item->setZValue(kHistogramItemZValue);
            histogram_elements_.push_back(item);
        }
    }

    const QRect scene_rect = QRect(0, 0, kHistogramSceneWidth, kHistogramSceneHeight);

    graphics_scene->setSceneRect(scene_rect);
    graphics_scene->update();

    selection_range_indicator_left_->setLine(selection_min_value_, 0, selection_min_value_, kHistogramSceneHeight);
    selection_range_indicator_right_->setLine(selection_max_value_, 0, selection_max_value_, kHistogramSceneHeight);
    selection_range_indicator_range_->setRect(QRect(0, kHistogramSceneHeight, kHistogramSceneWidth, kHistogramSceneHeight));

    show();
    fitInView(scene_rect);
}

void RgpHistogramWidget::SetSelectionMode(HistogramSelectionMode mode)
{
    active_selection_mode_ = mode;

    QPen active_pen = kSelectionLinePenActive;
    active_pen.setCosmetic(true);

    switch (mode)
    {
    case kHistogramSelectionModeRange:
    {
        selection_range_indicator_left_->setPen(kSelectionLinePenDefault);
        selection_range_indicator_right_->setPen(kSelectionLinePenDefault);
        break;
    }
    case kHistogramSelectionModeLeft:
    {
        selection_range_indicator_left_->setPen(active_pen);
        selection_range_indicator_right_->setPen(kSelectionLinePenDefault);
        break;
    }
    case kHistogramSelectionModeRight:
    {
        selection_range_indicator_left_->setPen(kSelectionLinePenDefault);
        selection_range_indicator_right_->setPen(active_pen);
        break;
    }
    }

    scene()->update();
}

void RgpHistogramWidget::SetSelectionAttributes(int32_t min_value, int32_t max_value)
{
    selection_min_value_ = min_value;
    selection_max_value_ = max_value;

    const float   inv_clk_range       = 1.0f / (bounding_data_.max_value - bounding_data_.min_value);
    const int32_t selection_min_coord = (min_value - bounding_data_.min_value) * kHistogramSceneWidth * inv_clk_range;
    const int32_t selection_max_coord = (max_value - bounding_data_.min_value) * kHistogramSceneWidth * inv_clk_range;

    const QRectF selection_rect(selection_min_coord, 0, selection_max_coord - selection_min_coord, kHistogramSceneHeight);

    selection_range_indicator_left_->setLine(selection_min_coord, 0, selection_min_coord, kHistogramSceneHeight);
    selection_range_indicator_right_->setLine(selection_max_coord, 0, selection_max_coord, kHistogramSceneHeight);
    selection_range_indicator_range_->setRect(selection_rect);

    for (QGraphicsRectItem* item : histogram_elements_)
    {
        if (selection_rect.intersects(item->boundingRect()))
        {
            item->setPen(kSelectedBucketPen);
            item->setBrush(kSelectedBucketBrush);
        }
        else
        {
            item->setPen(kUnselectedBucketPen);
            item->setBrush(kUnselectedBucketBrush);
        }
    }

    scene()->update();
}

void RgpHistogramWidget::resizeEvent(QResizeEvent* event)
{
    SetSelectionAttributes(selection_min_value_, selection_max_value_);
    fitInView(QRect(0, 0, kHistogramSceneWidth, kHistogramSceneHeight));

    QGraphicsView::resizeEvent(event);
}
