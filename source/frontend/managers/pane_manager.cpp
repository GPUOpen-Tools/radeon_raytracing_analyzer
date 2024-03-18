//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the pane manager.
//=============================================================================

#include "managers/pane_manager.h"

#include "public/rra_trace_loader.h"

#include "managers/navigation_manager.h"
#include "settings/settings.h"

namespace rra
{
    PaneManager::PaneManager()
        : nav_location_{}
        , current_pane_(kPaneIdStartWelcome)
        , previous_pane_(kPaneIdStartWelcome)
    {
        ResetNavigation();
    }

    PaneManager::~PaneManager()
    {
        for (auto it = panes_.begin(); it != panes_.end(); ++it)
        {
            if ((*it) != nullptr)
            {
                delete (*it);
            }
        }
    }

    const NavLocation& PaneManager::ResetNavigation()
    {
        nav_location_.main_tab_index    = kMainPaneStart;
        nav_location_.start_list_row    = kStartPaneWelcome;
        nav_location_.overview_list_row = kOverviewPaneSummary;
        nav_location_.tlas_list_row     = kTlasPaneViewer;
        nav_location_.blas_list_row     = kBlasPaneViewer;
        nav_location_.ray_list_row      = kRayPaneHistory;
        nav_location_.settings_list_row = kSettingsPaneGeneral;

        return nav_location_;
    }

    MainPanes PaneManager::GetMainPaneFromPane(RRAPaneId pane) const
    {
        return static_cast<MainPanes>(pane >> kPaneShift);
    }

    RRAPaneId PaneManager::GetCurrentPane() const
    {
        return current_pane_;
    }

    RRAPaneId PaneManager::GetPreviousPane() const
    {
        return previous_pane_;
    }

    NavLocation* PaneManager::SetupNextPane(rra::RRAPaneId pane)
    {
        rra::MainPanes main_pane = GetMainPaneFromPane(pane);

        if (main_pane == rra::kMainPaneOverview || main_pane == rra::kMainPaneTlas || main_pane == rra::kMainPaneBlas || main_pane == rra::kMainPaneRay)
        {
            // Make sure a trace is loaded before navigating.
            if (!RraTraceLoaderValid())
            {
                return nullptr;
            }
        }

        int32_t main_tab_index       = pane >> kPaneShift;
        int32_t list_row             = pane & kPaneMask;
        nav_location_.main_tab_index = main_tab_index;

        switch (main_tab_index)
        {
        case kMainPaneStart:
            nav_location_.start_list_row = list_row;
            break;

        case kMainPaneOverview:
            nav_location_.overview_list_row = list_row;
            break;

        case kMainPaneTlas:
            nav_location_.tlas_list_row = list_row;
            break;

        case kMainPaneBlas:
            nav_location_.blas_list_row = list_row;
            break;

        case kMainPaneRay:
            nav_location_.ray_list_row = list_row;
            break;

        case kMainPaneSettings:
            nav_location_.settings_list_row = list_row;
            break;

        default:
            break;
        }

        return &nav_location_;
    }

    void PaneManager::UpdateAndRecordCurrentPane()
    {
        UpdateCurrentPane();
        NavigationManager::Get().RecordNavigationEventPaneSwitch(current_pane_);
    }

    RRAPaneId PaneManager::UpdateCurrentPane()
    {
        // Create the combined component.
        int32_t current_pane = (nav_location_.main_tab_index << kPaneShift);

        switch (nav_location_.main_tab_index)
        {
        case kMainPaneStart:
            current_pane |= nav_location_.start_list_row;
            break;

        case kMainPaneOverview:
            current_pane |= nav_location_.overview_list_row;
            break;

        case kMainPaneTlas:
            current_pane |= nav_location_.tlas_list_row;
            break;

        case kMainPaneBlas:
            current_pane |= nav_location_.blas_list_row;
            break;

        case kMainPaneSettings:
            current_pane |= nav_location_.settings_list_row;
            break;

        case kMainPaneRay:
            current_pane |= nav_location_.ray_list_row;

        default:
            break;
        }

        // Only update the current pane if it's changed.
        if (current_pane != current_pane_)
        {
            previous_pane_ = current_pane_;
            current_pane_  = static_cast<RRAPaneId>(current_pane);
        }

        return current_pane_;
    }

    void PaneManager::UpdateMainTabIndex(const int tab_index)
    {
        if ((tab_index >= kMainPaneStart) && (tab_index < kMainPaneCount))
        {
            nav_location_.main_tab_index = tab_index;
            UpdateAndRecordCurrentPane();
        }
    }

    void PaneManager::UpdateStartListRow(const int row)
    {
        if (row < kStartPaneCount)
        {
            nav_location_.start_list_row = row;
            UpdateAndRecordCurrentPane();
        }
    }

    void PaneManager::UpdateOverviewListRow(const int row)
    {
        if (row < kOverviewPaneCount)
        {
            nav_location_.overview_list_row = row;
            UpdateAndRecordCurrentPane();
        }
    }

    void PaneManager::UpdateTlasListIndex(const int index)
    {
        if (index < kTlasPaneCount)
        {
            nav_location_.tlas_list_row = index;
            UpdateAndRecordCurrentPane();
        }
    }

    void PaneManager::UpdateBlasListIndex(const int index)
    {
        if (index < kBlasPaneCount)
        {
            nav_location_.blas_list_row = index;
            UpdateAndRecordCurrentPane();
        }
    }

    void PaneManager::UpdateRayListIndex(const int index)
    {
        if (index < kBlasPaneCount)
        {
            nav_location_.ray_list_row = index;
            UpdateAndRecordCurrentPane();
        }
    }

    void PaneManager::UpdateSettingsListRow(const int row)
    {
        if (row < kSettingsPaneCount)
        {
            nav_location_.settings_list_row = row;
            UpdateAndRecordCurrentPane();
        }
    }

    void PaneManager::AddPane(BasePane* pane)
    {
        panes_.push_back(pane);
    }

    void PaneManager::OnTraceClose()
    {
        for (auto it = panes_.begin(); it != panes_.end(); ++it)
        {
            if ((*it) != nullptr)
            {
                (*it)->OnTraceClose();
            }
        }
    }

    void PaneManager::OnTraceOpen()
    {
        if (!rra::Settings::Get().GetPersistentUIState())
        {
            Settings::Get().SetPersistentUIToDefault(rra::kPaneIdTlasViewer);
            Settings::Get().SetPersistentUIToDefault(rra::kPaneIdBlasViewer);
            Settings::Get().SetPersistentUIToDefault(rra::kPaneIdRayInspector);
            Settings::Get().SetPersistentUIToDefault();
        }

        for (auto it = panes_.begin(); it != panes_.end(); ++it)
        {
            if ((*it) != nullptr)
            {
                (*it)->OnTraceOpen();
            }
        }
    }

    void PaneManager::Reset()
    {
        for (auto it = panes_.begin(); it != panes_.end(); ++it)
        {
            if ((*it) != nullptr)
            {
                (*it)->Reset();
            }
        }
    }

}  // namespace rra
