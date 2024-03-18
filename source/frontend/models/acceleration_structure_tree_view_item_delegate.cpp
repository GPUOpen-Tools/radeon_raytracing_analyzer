//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of an acceleration structure (AS) tree-view item delegate.
//=============================================================================

#include "models/acceleration_structure_tree_view_item_delegate.h"
#include "models/acceleration_structure_tree_view_item.h"

#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QFontMetrics>
#include "public/rra_blas.h"

namespace rra
{
    AccelerationStructureTreeViewItemDelegate::AccelerationStructureTreeViewItemDelegate(Scene* scene, std::function<void()> update_function)
    {
        RRA_ASSERT(scene != nullptr);
        scene_           = scene;
        update_function_ = update_function;
    }

    void AccelerationStructureTreeViewItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        // Highlight background if selected.
        if (option.state & QStyle::State_Selected)
        {
            painter->fillRect(option.rect, option.palette.highlight());
        }

        auto item_data         = qvariant_cast<AccelerationStructureTreeViewItemData>(index.data(Qt::DisplayRole));
        auto horizontal_margin = option.rect.height() / 4;

        /// Draw the checkbox.
        QStyleOptionButton checkbox;
        checkbox.rect = GetCheckboxRect(option);
        checkbox.rect.moveRight(checkbox.rect.right() + horizontal_margin);
        auto node = scene_->GetNodeById(item_data.node_id);

        if (node && node->IsVisible())
        {
            checkbox.state |= QStyle::State_On;
            if (node->IsSelected())
            {
                painter->fillRect(option.rect, option.palette.highlight());
            }
        }
        else
        {
            checkbox.state |= QStyle::State_Off;
        }

        if (node && node->IsEnabled())
        {
            checkbox.state |= QStyle::State_Enabled;
        }
        QApplication::style()->drawControl(QStyle::CE_CheckBox, &checkbox, painter);

        /// Draw the text associated with the node.
        auto text_rect = option.rect;
        text_rect.setLeft(checkbox.rect.right() + horizontal_margin);
        auto text = item_data.display_name;

        // Make instance nodes with empty BLASes empty.
        if (node && !node->GetInstances().empty() && RraBlasIsEmpty(node->GetInstances()[0].blas_index))
        {
            painter->setPen(Qt::red);
        }
        else
        {
            painter->setPen(Qt::black);
        }

        if (node && (!node->IsVisible() || !node->IsEnabled()))
        {
            painter->setPen(Qt::gray);
        }

        painter->drawText(text_rect, text);
    }

    QSize AccelerationStructureTreeViewItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QSize hint = QStyledItemDelegate::sizeHint(option, index);

        auto item_data    = qvariant_cast<AccelerationStructureTreeViewItemData>(index.data(Qt::DisplayRole));
        int  string_width = option.fontMetrics.horizontalAdvance(item_data.display_name);
        auto dpi          = option.fontMetrics.fontDpi();
        int  margin_width = dpi / 3;

        return QSize(string_width + margin_width, hint.height());
    }

    /// @brief Toggle visibility of node and its children.
    /// 
    /// @param scene The scene containing the node.
    /// @param node The node to toggle.
    void ToggleNodeSelection(Scene* scene, SceneNode* node)
    {
        node->SetVisible(!node->IsVisible(), scene);
        if (!node->IsVisible())
        {
            node->ResetSelection(scene->GetSelectedNodeIDs());
        }
        else
        {
            node->ApplyNodeSelection(scene->GetSelectedNodeIDs());
        }
    }

    bool AccelerationStructureTreeViewItemDelegate::editorEvent(QEvent*                     event,
                                                                QAbstractItemModel*         model,
                                                                const QStyleOptionViewItem& option,
                                                                const QModelIndex&          index)
    {
        auto checkbox_rect     = GetCheckboxRect(option);
        auto horizontal_margin = option.rect.height() / 4;
        checkbox_rect.moveRight(checkbox_rect.right() + horizontal_margin);

        if (event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            auto         item_data  = qvariant_cast<AccelerationStructureTreeViewItemData>(index.data(Qt::DisplayRole));
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            int x_pos = mouseEvent->x();
#else
            int x_pos = mouseEvent->position().toPoint().x();
#endif
            if (checkbox_rect.left() <= x_pos && x_pos <= checkbox_rect.right())
            {
                auto node = scene_->GetNodeById(item_data.node_id);
                if (node && node->IsEnabled())
                {
                    auto instance = node->GetInstance();
                    // Select rebraided nodes in lockstep.
                    if (instance)
                    {
                        for (auto sibling : scene_->GetRebraidedInstances(instance->instance_index))
                        {
                            ToggleNodeSelection(scene_, sibling);
                        }
                    }
                    // Select split triangles in lockstep.
                    else if (!node->GetTriangles().empty())
                    {
                        for (auto sibling : scene_->GetSplitTriangles(node->GetGeometryIndex(), node->GetPrimitiveIndex()))
                        {
                            ToggleNodeSelection(scene_, sibling);
                        }
                    }
                    else
                    {
                        ToggleNodeSelection(scene_, node);
                    }

                    scene_->IncrementSceneIteration();
                    if (update_function_)
                    {
                        update_function_();
                    }
                }
            }
            return true;
        }

        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

    QRect AccelerationStructureTreeViewItemDelegate::GetCheckboxRect(const QStyleOptionViewItem& option)
    {
        QStyleOptionButton                dummy_option_button;
        dummy_option_button.QStyleOption::operator      =(option);
        QRect                             checkbox_size = QApplication::style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &dummy_option_button);

        QRect final_rectangle = option.rect;
        final_rectangle.setTop(final_rectangle.top() + (option.rect.height() - checkbox_size.height()) / 2);
        final_rectangle.setWidth(checkbox_size.width());
        final_rectangle.setHeight(checkbox_size.height());

        return final_rectangle;
    }

}  // namespace rra
