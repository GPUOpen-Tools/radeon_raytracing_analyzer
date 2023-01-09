//==============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the file loading animation manager.
//==============================================================================

#include "managers/load_animation_manager.h"

#include <QApplication>

#include "qt_common/utils/scaling_manager.h"

namespace rra
{
    /// The single instance of the file loading animation manager.
    static LoadAnimationManager load_animation_manager;

    LoadAnimationManager& LoadAnimationManager::Get()
    {
        return load_animation_manager;
    }

    LoadAnimationManager::LoadAnimationManager(QObject* parent)
        : QObject(parent)
        , tab_widget_(nullptr)
        , file_menu_(nullptr)
        , file_load_animation_(nullptr)
    {
    }

    LoadAnimationManager::~LoadAnimationManager()
    {
    }

    void LoadAnimationManager::Initialize(TabWidget* tab_widget, QMenu* file_menu)
    {
        tab_widget_ = tab_widget;
        file_menu_  = file_menu;
    }

    void LoadAnimationManager::ResizeAnimation()
    {
        // Update Animation widget on window resize.
        if (file_load_animation_ != nullptr && tab_widget_ != nullptr)
        {
            double   height_offset = tab_widget_->TabHeight();
            QWidget* parent        = file_load_animation_->parentWidget();

            Resize(parent, height_offset);
        }
    }

    void LoadAnimationManager::StartAnimation(QWidget* parent, int height_offset)
    {
        if (file_load_animation_ == nullptr)
        {
            file_load_animation_ = new FileLoadingWidget(parent);
            Resize(parent, height_offset);

            file_load_animation_->show();
            tab_widget_->setDisabled(true);
            file_menu_->setDisabled(true);

            qApp->setOverrideCursor(Qt::BusyCursor);
        }
    }

    void LoadAnimationManager::StartAnimation()
    {
        StartAnimation(tab_widget_, tab_widget_->TabHeight());
    }

    void LoadAnimationManager::StopAnimation()
    {
        if (file_load_animation_ != nullptr)
        {
            delete file_load_animation_;
            file_load_animation_ = nullptr;

            tab_widget_->setEnabled(true);
            file_menu_->setEnabled(true);

            qApp->restoreOverrideCursor();
        }
    }

    void LoadAnimationManager::Resize(QWidget* parent, int height_offset)
    {
        if (parent != nullptr)
        {
            // Set overall size of the widget to cover the tab contents.
            int width  = parent->width();
            int height = parent->height() - height_offset;
            file_load_animation_->setGeometry(parent->x(), parent->y() + height_offset, width, height);

            // Set the contents margin so that the animated bars only cover a small area in the middle of the screen.
            const int desired_loading_dimension = ScalingManager::Get().Scaled(200);
            int       vertical_margin           = (height - desired_loading_dimension) / 2;
            int       horizontal_margin         = (width - desired_loading_dimension) / 2;
            file_load_animation_->setContentsMargins(horizontal_margin, vertical_margin, horizontal_margin, vertical_margin);
        }
    }

}  // namespace rra
