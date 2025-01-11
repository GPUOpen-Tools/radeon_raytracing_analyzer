//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for back/fwd navigation manager.
//=============================================================================

#ifndef RRA_MANAGERS_NAVIGATION_MANAGER_H_
#define RRA_MANAGERS_NAVIGATION_MANAGER_H_

#include <QWidget>
#include <QList>
#include <QModelIndex>

#include "managers/pane_manager.h"

namespace rra
{
    /// @brief Class that handles back and forward navigation.
    class NavigationManager : public QWidget
    {
        Q_OBJECT

    public:
        /// @brief NavigationManager instance get function.
        ///
        /// @return a reference to the NavigationManager instance.
        static NavigationManager& Get();

        /// @brief Record a pane switch event.
        ///
        /// @param [in] pane The new pane.
        void RecordNavigationEventPaneSwitch(RRAPaneId pane);

        /// @brief Go back to starting state.
        void Reset();

    signals:
        /// @brief Signal to enable the navigation back button.
        ///
        /// @param [in] enable Should the button be enabled.
        void EnableBackNavButton(bool enable);

        /// @brief Signal to enable the navigation forward button.
        ///
        /// @param [in] enable Should the button be enabled.
        void EnableForwardNavButton(bool enable);

        /// @brief Signal to navigate to a specific pane via the navigation buttons.
        ///
        /// @param [in] pane The pane to navigate to.
        void NavigateButtonClicked(rra::RRAPaneId pane);

    public slots:
        /// @brief Go forward.
        void NavigateForward();

        /// @brief Go back.
        void NavigateBack();

    private:
        /// @brief Enum of navigation types.
        enum NavType
        {
            kNavigationTypeUndefined,
            kNavigationTypePaneSwitch,

            kNavigationTypeCount,
        };

        /// @brief Struct to hold navigation event data.
        struct NavEvent
        {
            NavType   type;  ///< Navigation type.
            RRAPaneId pane;  ///< Destination pane.
        };

        /// @brief Constructor.
        explicit NavigationManager();

        /// @brief Destructor.
        virtual ~NavigationManager();

        /// @brief Remove history if user went back towards the middle and then somewhere new.
        void DiscardObsoleteNavHistory();

        /// @brief Replay a previous navigation event.
        ///
        /// @param [in] event The navigation event.
        void AddNewEvent(const NavEvent& event);

        /// @brief Helper function to print a Navigation event.
        ///
        /// @param [in] event The navigation event.
        ///
        /// @return string version of the navigation event.
        const QString NavigationEventString(const NavEvent& event) const;

        /// @brief Register a new pane switch.
        ///
        /// @param [in] pane The new pane.
        void AddNewPaneSwitch(RRAPaneId pane);

        /// @brief Replay a previous navigation event.
        ///
        /// @param [in] event The navigation event.
        void ReplayNavigationEvent(const NavEvent& event);

        /// @brief Intelligently find the next navigation event.
        ///
        /// @return The next navigation event.
        NavEvent FindNextNavigationEvent();

        /// @brief Intelligently find the previous navigation event.
        ///
        /// @return The previous navigation event.
        NavEvent FindPrevNavigationEvent();

        /// @brief Helper function to print the backing structure.
        void PrintHistory() const;

        /// @brief Helper function to convert a Pane enum to string.
        ///
        /// @param [in] pane the pane.
        ///
        /// @return string version of pane name.
        QString GetPaneString(RRAPaneId pane) const;

        QList<NavEvent> navigation_history_;           ///< Track user navigation.
        int32_t         navigation_history_location_;  ///< Current location in navigation history.
    };
}  // namespace rra

#endif  // RRA_MANAGERS_NAVIGATION_MANAGER_H_
