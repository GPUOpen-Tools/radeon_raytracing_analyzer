//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the Instances pane on the TLAS tab.
//=============================================================================

#ifndef RRA_VIEWS_TLAS_TLAS_INSTANCES_PANE_H_
#define RRA_VIEWS_TLAS_TLAS_INSTANCES_PANE_H_

#include "ui_tlas_instances_pane.h"

#include "models/tlas/tlas_instances_model.h"
#include "models/table_item_delegate.h"
#include "views/base_pane.h"

/// @brief Class declaration.
class TlasInstancesPane : public BasePane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit TlasInstancesPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~TlasInstancesPane();

    /// @brief Overridden pane key press event.
    ///
    /// @param [in] event The key press event object.
    virtual void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;

    /// @brief Overridden window show event.
    ///
    /// @param [in] event the show event object.
    virtual void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;

    /// @brief Trace closed.
    virtual void OnTraceClose() Q_DECL_OVERRIDE;

protected:
    /// @brief Filter events to catch right clicks to deselect rows in instance table.
    ///
    /// @param obj The object associated with the event.
    /// @param event The Qt event.
    ///
    /// @return True if the event should be filtered, false otherwise.
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    /// @brief Navigate to the TLAS pane from a selection within the BLAS Instances table.
    ///
    /// @param [in] index The model index of the item selected in the table.
    void GotoBlasInstanceFromTableSelect(const QModelIndex& index);

    /// @brief Set the TLAS index.
    ///
    /// Called when the user chooses a different TLAS from the combo box.
    /// Will force a redraw of the table, since the TLAS index has changed.
    void SetTlasIndex(uint64_t tlas_index);

    /// @brief Set the instance index.
    ///
    /// Called when the user chooses a different instance from the TLAS viewer tree view.
    ///
    /// @param [in] instance_index The instance index selected.
    void SetInstanceIndex(uint32_t instance_index);

    /// @brief Slot to handle what happens after the instance table is sorted.
    ///
    /// Make sure the selected item (if there is one) is visible.
    void ScrollToSelectedInstance();

private:
    Ui::TlasInstancesPane* ui_;  ///< Pointer to the Qt UI design.

    rra::TlasInstancesModel* model_;           ///< Container class for the widget models.
    uint64_t                 tlas_index_;      ///< The currently selected TLAS index.
    uint32_t                 instance_index_;  ///< The currently selected instance index.
    bool                     data_valid_;      ///< Is the trace data valid.
    TableItemDelegate*       table_delegate_;  ///< The delegate responsible for painting the table.
};

#endif  // #define RRA_VIEWS_TLAS_TLAS_INSTANCES_PANE_H_
