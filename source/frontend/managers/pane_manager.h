//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the pane manager.
//=============================================================================

#ifndef RRA_MANAGERS_PANE_MANAGER_H_
#define RRA_MANAGERS_PANE_MANAGER_H_

#include <vector>
#include <QObject>

#include "views/base_pane.h"

namespace rra
{
    /// @brief An enum of all the elements in the tab menu.
    enum MainPanes
    {
        kMainPaneNavigation,
        kMainPaneStart,
        kMainPaneOverview,
        kMainPaneTlas,
        kMainPaneBlas,
        kMainPaneRay,
        kMainPaneSpacer,
        kMainPaneSettings,

        kMainPaneCount,
    };

    /// @brief An enum of all the panes in the start menu.
    enum StartPanes
    {
        kStartPaneWelcome,
        kStartPaneRecentTraces,
        kStartPaneAbout,

        kStartPaneCount,
    };

    /// @brief An enum of all the panes in the Overview menu.
    enum OverviewPanes
    {
        kOverviewPaneSummary,
        kOverviewPaneDeviceConfig,

        kOverviewPaneCount,
    };

    /// @brief An enum of all the panes in the TLAS menu.
    enum TLASPanes
    {
        kTlasPaneViewer,
        kTlasPaneInstances,
        kTlasPaneBlasList,
        kTlasPaneProperties,

        kTlasPaneCount,
    };

    /// @brief An enum of all the panes in the BLAS menu.
    enum BLASPanes
    {
        kBlasPaneViewer,
        kBlasPaneInstances,
        kBlasPaneTriangles,
        kBlasPaneGeometries,
        kBlasPaneProperties,

        kBlasPaneCount,
    };

    /// @brief An enum of all the panes in the Ray menu.
    enum RayPanes
    {
        kRayPaneHistory,
        kRayPaneInspector,

        kRayPaneCount,
    };

    /// @brief An enum of all the panes in the settings menu.
    enum SettingsPanes
    {
        kSettingsPaneGeneral,
        kSettingsPaneThemesAndColors,
        kSettingsPaneKeyboardShortcuts,

        kSettingsPaneCount,
    };

    static const int32_t kPaneShift = 16;
    static const int32_t kPaneMask  = 0xffff;

    /// @brief Used to control and track user navigation.
    struct NavLocation
    {
        int32_t main_tab_index;     ///< Main tab index.
        int32_t start_list_row;     ///< Start list row.
        int32_t overview_list_row;  ///< Overview list row.
        int32_t tlas_list_row;      ///< TLAS list row.
        int32_t blas_list_row;      ///< BLAS list row.
        int32_t ray_list_row;       ///< RAY list row.
        int32_t settings_list_row;  ///< Settings list row.
    };

    /// @brief An enum of all the panes used in the tool.
    enum RRAPaneId
    {
        kPaneIdStartWelcome              = (kMainPaneStart << kPaneShift) | kStartPaneWelcome,
        kPaneIdStartRecentTraces         = (kMainPaneStart << kPaneShift) | kStartPaneRecentTraces,
        kPaneIdStartAbout                = (kMainPaneStart << kPaneShift) | kStartPaneAbout,
        kPaneIdOverviewSummary           = (kMainPaneOverview << kPaneShift) | kOverviewPaneSummary,
        kPaneIdOverviewDeviceConfig      = (kMainPaneOverview << kPaneShift) | kOverviewPaneDeviceConfig,
        kPaneIdTlasViewer                = (kMainPaneTlas << kPaneShift) | kTlasPaneViewer,
        kPaneIdTlasInstances             = (kMainPaneTlas << kPaneShift) | kTlasPaneInstances,
        kPaneIdTlasBlasList              = (kMainPaneTlas << kPaneShift) | kTlasPaneBlasList,
        kPaneIdTlasProperties            = (kMainPaneTlas << kPaneShift) | kTlasPaneProperties,
        kPaneIdBlasViewer                = (kMainPaneBlas << kPaneShift) | kBlasPaneViewer,
        kPaneIdBlasInstances             = (kMainPaneBlas << kPaneShift) | kBlasPaneInstances,
        kPaneIdBlasTriangles             = (kMainPaneBlas << kPaneShift) | kBlasPaneTriangles,
        kPaneIdBlasGeometries            = (kMainPaneBlas << kPaneShift) | kBlasPaneGeometries,
        kPaneIdBlasProperties            = (kMainPaneBlas << kPaneShift) | kBlasPaneProperties,
        kPaneIdRayHistory                = (kMainPaneRay << kPaneShift) | kRayPaneHistory,
        kPaneIdRayInspector              = (kMainPaneRay << kPaneShift) | kRayPaneInspector,
        kPaneIdSettingsGeneral           = (kMainPaneSettings << kPaneShift) | kSettingsPaneGeneral,
        kPaneIdSettingsThemesAndColors   = (kMainPaneSettings << kPaneShift) | kSettingsPaneThemesAndColors,
        kPaneIdSettingsKeyboardShortcuts = (kMainPaneSettings << kPaneShift) | kSettingsPaneKeyboardShortcuts,
        kPaneIdInvalid                   = (kMainPaneCount << kPaneShift)
    };

    /// @brief Hotkeys.
    static const int kGotoOverviewSummaryPane      = Qt::Key_1;
    static const int kGotoOverviewDeviceConfigPane = Qt::Key_2;

    static const int kGotoTlasViewerPane     = Qt::Key_Q;
    static const int kGotoTlasInstancesPane  = Qt::Key_W;
    static const int kGotoTlasBlasListPane   = Qt::Key_E;
    static const int kGotoTlasPropertiesPane = Qt::Key_T;

    static const int kGotoBlasViewerPane     = Qt::Key_A;
    static const int kGotoBlasInstancesPane  = Qt::Key_S;
    static const int kGotoBlasTrianglesPane  = Qt::Key_D;
    static const int kGotoBlasGeometriesPane = Qt::Key_F;
    static const int kGotoBlasPropertiesPane = Qt::Key_G;
    static const int kGotoRayHistoryPane   = Qt::Key_H;
    static const int kGotoRayInspectorPane = Qt::Key_J;

    static const int kGotoWelcomePane           = Qt::Key_X;
    static const int kGotoRecentTracesPane      = Qt::Key_C;
    static const int kGotoAboutPane             = Qt::Key_V;
    static const int kGotoGeneralSettingsPane   = Qt::Key_B;
    static const int kGotoThemesAndColorsPane   = Qt::Key_N;
    static const int kGotoKeyboardShortcutsPane = Qt::Key_M;

    static const int kKeyNavBackwardBackspace = Qt::Key_Backspace;
    static const int kKeyNavBackwardArrow     = Qt::Key_Left;
    static const int kKeyNavForwardArrow      = Qt::Key_Right;
    static const int kKeyNavUpArrow           = Qt::Key_Up;
    static const int kKeyNavDownArrow         = Qt::Key_Down;

    /// @brief Class to manage the panes and navigating between them.
    class PaneManager : public QObject
    {
        Q_OBJECT

    public:
        /// @brief Constructor.
        PaneManager();

        /// @brief Destructor.
        virtual ~PaneManager();

        /// @brief Take our navigation locations to starting state.
        ///
        /// @return The reset Navigation location.
        const NavLocation& ResetNavigation();

        /// @brief Navigate to a specific pane.
        ///
        /// @param [in] pane the pane to jump to.
        ///
        /// @return The Navigation location.
        NavLocation* SetupNextPane(rra::RRAPaneId pane);

        /// @brief Work out current pane from app state.
        ///
        /// Called every time there's a pane switch.
        ///
        /// @return The new current pane.
        RRAPaneId UpdateCurrentPane();

        /// @brief Store main tab index and update current pane.
        ///
        /// @param [in] tab_index The tab index.
        void UpdateMainTabIndex(const int tab_index);

        /// @brief Store start list row and update current pane.
        ///
        /// @param [in] row The row index.
        void UpdateStartListRow(const int row);

        /// @brief Store overview list row and update current pane.
        ///
        /// @param [in] row The row index.
        void UpdateOverviewListRow(const int row);

        /// @brief Store TLAS list index and update current pane.
        ///
        /// @param [in] index The index in the list selected.
        void UpdateTlasListIndex(const int index);

        /// @brief Store BLAS list index and update current pane.
        ///
        /// @param [in] index The index in the list selected.
        void UpdateBlasListIndex(const int index);

        /// @brief Store RAY list index and update current pane.
        ///
        /// @param [in] index The index in the list selected.
        void UpdateRayListIndex(const int index);

        /// @brief Store settings list row and update current pane.
        ///
        /// @param [in] row The row index.
        void UpdateSettingsListRow(const int row);

        /// @brief Get the main pane group from the pane.
        ///
        /// @param [in] pane The pane to check.
        ///
        /// @return The main pane.
        MainPanes GetMainPaneFromPane(RRAPaneId pane) const;

        /// @brief Get the current pane.
        ///
        /// @return The current pane.
        RRAPaneId GetCurrentPane() const;

        /// @brief Get the previous pane.
        ///
        /// @return The previous pane.
        RRAPaneId GetPreviousPane() const;

        /// @brief Add a pane to the group.
        ///
        /// @param [in] pane The pane to add.
        void AddPane(BasePane* pane);

        /// @brief Call OnTraceClose() for all panes.
        void OnTraceClose();

        /// @brief Call OnTraceOpen() for all panes.
        void OnTraceOpen();

        /// @brief Call Reset() for all panes.
        void Reset();

    private:
        /// @brief Update the current pane and record the change in the navigation manager.
        void UpdateAndRecordCurrentPane();

        NavLocation            nav_location_;   ///< Track current list and tab locations.
        RRAPaneId              current_pane_;   ///< Track current pane that is open.
        RRAPaneId              previous_pane_;  ///< Track previous pane that was open.
        std::vector<BasePane*> panes_;          ///< The group of panes to send messages to.
    };
}  // namespace rra

#endif  // RRA_MANAGERS_PANE_MANAGER_H_
