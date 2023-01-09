//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for back/fwd navigation manager.
//=============================================================================

#include "navigation_manager.h"

#include <QDebug>

#include "public/rra_assert.h"

#include "managers/message_manager.h"

namespace rra
{
    NavigationManager::NavigationManager()
        : navigation_history_location_(0)
    {
        Reset();
    }

    NavigationManager::~NavigationManager()
    {
    }

    NavigationManager& NavigationManager::Get()
    {
        static NavigationManager instance;

        return instance;
    }

    void NavigationManager::RecordNavigationEventPaneSwitch(RRAPaneId pane)
    {
        const NavEvent& curr_event = navigation_history_[navigation_history_location_];

        if (curr_event.type == kNavigationTypePaneSwitch)
        {
            // Prevent duplicates.
            if (curr_event.pane != pane)
            {
                AddNewPaneSwitch(pane);
            }
        }
        else
        {
            AddNewPaneSwitch(pane);
        }
    }

    void NavigationManager::Reset()
    {
        navigation_history_.clear();

        NavEvent first_event = {};
        first_event.type     = kNavigationTypePaneSwitch;
        first_event.pane     = kPaneIdStartWelcome;

        navigation_history_.push_back(first_event);
        navigation_history_location_ = 0;

        emit EnableBackNavButton(false);
        emit EnableForwardNavButton(false);
    }

    void NavigationManager::ReplayNavigationEvent(const NavEvent& event)
    {
        emit NavigateButtonClicked(event.pane);
    }

    NavigationManager::NavEvent NavigationManager::FindPrevNavigationEvent()
    {
        NavEvent out  = navigation_history_[navigation_history_location_];
        NavEvent curr = navigation_history_[navigation_history_location_];

        if (navigation_history_location_ > 0)
        {
            int32_t  idx  = navigation_history_location_ - 1;
            NavEvent prev = navigation_history_[idx];

            out                          = prev;
            navigation_history_location_ = idx;

            if ((prev.type == kNavigationTypePaneSwitch) && (curr.pane == prev.pane))
            {
                idx--;
                if (idx > 0)
                {
                    NavEvent prev_prev           = navigation_history_[idx];
                    out                          = prev_prev;
                    navigation_history_location_ = idx;
                }
            }
        }

        return out;
    }

    NavigationManager::NavEvent NavigationManager::FindNextNavigationEvent()
    {
        NavEvent out = navigation_history_[navigation_history_location_];

        const int32_t nav_limit = navigation_history_.size() - 1;

        if (navigation_history_location_ < nav_limit)
        {
            int32_t  idx  = navigation_history_location_ + 1;
            NavEvent next = navigation_history_[idx];

            out                          = next;
            navigation_history_location_ = idx;
        }

        return out;
    }

    void NavigationManager::NavigateBack()
    {
        if (navigation_history_location_ > 0)
        {
            RRA_ASSERT(navigation_history_.size() > 1);

            NavEvent prev_event = FindPrevNavigationEvent();

            ReplayNavigationEvent(prev_event);

            emit EnableForwardNavButton(true);
        }

        if (navigation_history_location_ <= 0)
        {
            emit EnableBackNavButton(false);
        }
    }

    void NavigationManager::NavigateForward()
    {
        const int32_t nav_limit = navigation_history_.size() - 1;

        if (navigation_history_location_ < nav_limit)
        {
            NavEvent next_event = FindNextNavigationEvent();

            ReplayNavigationEvent(next_event);

            emit EnableBackNavButton(true);
        }

        if (navigation_history_location_ >= nav_limit)
        {
            emit EnableForwardNavButton(false);
        }
    }

    void NavigationManager::DiscardObsoleteNavHistory()
    {
        for (int32_t i = navigation_history_.size() - 1; i > 0; i--)
        {
            if (i > navigation_history_location_)
            {
                navigation_history_.pop_back();

                emit EnableForwardNavButton(false);
            }
        }
    }

    void NavigationManager::AddNewEvent(const NavEvent& event)
    {
        navigation_history_.push_back(event);
        navigation_history_location_++;

        emit EnableBackNavButton(true);
    }

    void NavigationManager::AddNewPaneSwitch(RRAPaneId pane)
    {
        DiscardObsoleteNavHistory();

        NavEvent new_event = {};
        new_event.type     = kNavigationTypePaneSwitch;
        new_event.pane     = pane;

        AddNewEvent(new_event);
    }

    const QString NavigationManager::NavigationEventString(const NavEvent& event) const
    {
        QString out = "";

        if (event.type == kNavigationTypePaneSwitch)
        {
            out += GetPaneString(event.pane);
        }

        return out;
    }

    void NavigationManager::PrintHistory() const
    {
        QString out = "";

        const uint32_t nav_history_size = navigation_history_.size();

        for (uint32_t i = 0; i < nav_history_size; i++)
        {
            const NavEvent& curr_event = navigation_history_[i];

            out += "[" + QString::number(i) + "]=" + NavigationEventString(curr_event);

            if (i < nav_history_size - 1)
            {
                out += " | ";
            }
        }

        qDebug() << out;
    }

    QString NavigationManager::GetPaneString(RRAPaneId pane) const
    {
        QString out = "";

        switch (pane)
        {
        case kPaneIdStartWelcome:
            out = "Welcome";
            break;
        case kPaneIdStartRecentTraces:
            out = "Recent traces";
            break;
        case kPaneIdStartAbout:
            out = "About";
            break;
        case kPaneIdTlasViewer:
            out = "TLAS Viewer";
            break;
        case kPaneIdTlasInstances:
            out = "TLAS Instances";
            break;
        case kPaneIdTlasBlasList:
            out = "TLAS BLAS List";
            break;
        case kPaneIdTlasProperties:
            out = "TLAS Properties";
            break;
        case kPaneIdBlasViewer:
            out = "BLAS Viewer";
            break;
        case kPaneIdBlasInstances:
            out = "Instances";
            break;
        case kPaneIdBlasTriangles:
            out = "Triangles";
            break;
        case kPaneIdBlasProperties:
            out = "BLAS Properties";
            break;
        case kPaneIdSettingsGeneral:
            out = "General";
            break;
        case kPaneIdSettingsThemesAndColors:
            out = "Themes and colors";
            break;
        case kPaneIdSettingsKeyboardShortcuts:
            out = "Keyboard shortcuts";
            break;
        default:
            break;
        }

        return out;
    }
}  // namespace rra
