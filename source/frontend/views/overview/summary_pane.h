//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the summary pane.
//=============================================================================

#ifndef RRA_VIEWS_OVERVIEW_SUMMARY_PANE_H_
#define RRA_VIEWS_OVERVIEW_SUMMARY_PANE_H_

#include <vector>

#include <QWidget>

#include "ui_summary_pane.h"

#include "models/overview/summary_model.h"
#include "views/base_pane.h"

/// @brief Class declaration.
class SummaryPane : public BasePane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit SummaryPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    ~SummaryPane();

    /// @brief Overridden Qt show event. Fired when this pane is opened.
    ///
    /// @param [in] event The show event object.
    virtual void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;

    /// @brief Trace open.
    virtual void OnTraceOpen() Q_DECL_OVERRIDE;

    /// @brief Trace close.
    virtual void OnTraceClose() Q_DECL_OVERRIDE;

    /// @brief Reset UI state.
    virtual void Reset() Q_DECL_OVERRIDE;

public slots:
    /// @brief Select a TLAS.
    ///
    /// @param tlas_index The index of the selected TLAS.
    /// @param navigate_to_pane If true, navigate to the TLAS pane.
    void SelectTlas(uint32_t tlas_index, const bool navigate_to_pane);

private slots:
    /// @brief Set the TLAS index.
    ///
    /// Called when the user chooses a different TLAS.
    ///
    /// @param [in] tlas_index The TLAS index to set.
    void SetTlasIndex(uint64_t tlas_index);

private:
    /// @brief Populate the TLAS address to index map.
    void UpdateTlasMap();

    /// @brief Add all the dispatch panes to the summary pane.
    void AddDispatchPanes();

    /// @brief Add all the TLAS panes to the summary pane.
    void AddTlasPanes();

    Ui::SummaryPane*                       ui_;                     ///< Pointer to the Qt UI design.
    rra::SummaryModel*                     model_;                  ///< The model for this pane.
    uint64_t                               tlas_index_;             ///< The index in the table.
    std::vector<QWidget*>                  widget_deletion_queue_;  ///< A list of dynamic widgets in the pane which need manual deletion.
    std::unordered_map<uint64_t, uint32_t> tlas_address_to_index_;  ///< Maps the TLAS addresses to their indices.
};

#endif  // RRA_VIEWS_OVERVIEW_SUMMARY_PANE_H_

