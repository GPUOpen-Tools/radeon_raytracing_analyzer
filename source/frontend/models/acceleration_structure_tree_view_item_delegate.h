//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of an acceleration structure (AS) tree-view item delegate.
//=============================================================================

#ifndef RRA_MODELS_ACCELERATION_STRUCTURE_TREE_VIEW_ITEM_DELEGATE_H_
#define RRA_MODELS_ACCELERATION_STRUCTURE_TREE_VIEW_ITEM_DELEGATE_H_

#include <QStyledItemDelegate>
#include <QString>

#include "scene.h"

namespace rra
{
    /// @brief The item delegate to use when rendering tree view items.
    class AccelerationStructureTreeViewItemDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        using QStyledItemDelegate::QStyledItemDelegate;

        /// @brief The constructor.
        explicit AccelerationStructureTreeViewItemDelegate(Scene* scene, std::function<void()> update_function);

        /// @brief Paint the item by using model index.
        ///
        /// @param [in] painter The painter.
        /// @param [in] option The style option.
        /// @param [in] index The model index.
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        /// @brief Determines the size of the tree view item.
        /// 
        /// @param [in] option Additional parameters affecting the size.
        /// @param [in] index Index of the item.
        /// 
        /// @return 
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        /// @brief Handle event on the delegate.
        ///
        /// @param [in] event The event.
        /// @param [in] model The model.
        /// @param [in] option The style option.
        /// @param [in] index The model index.
        ///
        /// @returns True if the event has been handled.
        bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

        /// @brief Get the checkbox rectangle based on Qt internal styling.
        ///
        /// @param [in] option The item option to gather size information from.
        ///
        /// @returns The checkbox rectangle from the left.
        static QRect GetCheckboxRect(const QStyleOptionViewItem& option);

    private:
        Scene*                scene_           = nullptr;  ///< The scene to widthdraw render data from.
        std::function<void()> update_function_ = nullptr;  ///< The function to execute when an update is made.
    };
}  // namespace rra

#endif
