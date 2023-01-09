//==============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Class definition for the file loading animation manager.
///
/// This class is responsible for managing the file load animation when loading
/// a trace or data-mining the trace file (ie getting resource details or
/// generating the timeline).
///
//==============================================================================

#ifndef RRA_MANAGERS_LOAD_ANIMATION_MANAGER_H_
#define RRA_MANAGERS_LOAD_ANIMATION_MANAGER_H_

#include <QObject>
#include <QMenu>

#include "qt_common/custom_widgets/file_loading_widget.h"
#include "qt_common/custom_widgets/tab_widget.h"

namespace rra
{
    /// @brief This class handles the trace loading animation.
    class LoadAnimationManager : public QObject
    {
        Q_OBJECT

    public:
        /// @brief Constructor.
        ///
        /// @param [in] parent The parent widget.
        explicit LoadAnimationManager(QObject* parent = nullptr);

        /// @brief Destructor.
        virtual ~LoadAnimationManager();

        /// @brief Accessor for singleton instance.
        ///
        /// @return The singleton instance.
        static LoadAnimationManager& Get();

        /// @brief Initialize the animation manager.
        ///
        /// @param [in] tab_widget The tab widget from the main window.
        /// @param [in] file_menu  The file menu widget from the main window.
        void Initialize(TabWidget* tab_widget, QMenu* file_menu);

        /// @brief Start the loading animation.
        ///
        /// Called when an animation needs to be loaded onto a window.
        ///
        /// @param [in] parent        The parent window.
        /// @param [in] height_offset The offset from the top of the parent widget.
        void StartAnimation(QWidget* parent, int height_offset);

        /// @brief Start the loading animation.
        ///
        /// Called when an animation needs to be loaded onto a window.
        ///
        /// The tab widget is used as the parent window.
        void StartAnimation();

        /// @brief Stop the loading animation.
        ///
        /// Called when trace file has loaded.
        void StopAnimation();

        /// @brief Resize the loading animation.
        ///
        /// Called when the main window is resized.
        /// Make sure that the load animation is also resized.
        void ResizeAnimation();

    private:
        /// @brief Resize the loading animation.
        ///
        /// @param [in] parent        The parent window.
        /// @param [in] height_offset The offset from the top of the parent widget.
        void Resize(QWidget* parent, int height_offset);

        TabWidget*         tab_widget_;           ///< The tab widget from the main window.
        QMenu*             file_menu_;            ///< The file menu widget from the main window.
        FileLoadingWidget* file_load_animation_;  ///< Widget to show animation.
    };
}  // namespace rra

#endif  // RRA_MANAGERS_LOAD_ANIMATION_MANAGER_H_
