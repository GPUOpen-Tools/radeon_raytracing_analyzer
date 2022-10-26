//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the BLAS list pane.
//=============================================================================

#ifndef RRA_VIEWS_TLAS_BLAS_LIST_PANE_H_
#define RRA_VIEWS_TLAS_BLAS_LIST_PANE_H_

#include "ui_blas_list_pane.h"

#include "models/tlas/blas_list_model.h"
#include "models/tlas/blas_list_table_item_delegate.h"
#include "views/base_pane.h"

/// @brief Class declaration.
class BlasListPane : public BasePane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit BlasListPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~BlasListPane();

    /// @brief Overridden pane key press event.
    ///
    /// @param [in] event The key press event object.
    virtual void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;

    /// @brief Overridden window show event.
    ///
    /// @param [in] event The show event object.
    virtual void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;

    /// @brief Trace closed.
    virtual void OnTraceClose() Q_DECL_OVERRIDE;

protected:
    /// @brief Filter events to catch right clicks to deselect rows in BLAS table.
    /// 
    /// @param obj The object associated with the event.
    /// @param event The Qt event.
    /// 
    /// @return True if the event should be filtered, false otherwise.
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    /// @brief Navigate to the BLAS pane from a table selection.
    ///
    /// The user has selected a BLAS instance from the UI, so select it in the BLAS
    /// combo box, set up the BLAS UI and navigate to the BLAS viewer pane.
    ///
    /// @param [in] index The model index of the item selected in the table.
    void GotoBlasPaneFromTableSelect(const QModelIndex& index);

    /// @brief Set the TLAS index.
    ///
    /// Called when the user chooses a different TLAS from the combo box.
    /// Will force a redraw of the table, since the TLAS index has changed.
    ///
    /// @param [in] tlas_index The TLAS index to set.
    void SetTlasIndex(uint64_t tlas_index);

    /// @brief Set the BLAS index.
    ///
    /// Called when the user chooses a different BLAS from the combo box.
    /// Will force a redraw of the table, since the BLAS index has changed.
    ///
    /// @param [in] blas_index The BLAS index to select.
    void SetBlasIndex(uint64_t blas_index);

    /// @brief Slot to handle what happens after the blas list table is sorted.
    ///
    /// Make sure the selected item (if there is one) is visible.
    void ScrollToSelectedBlas();

private:
    Ui::BlasListPane* ui_;  ///< Pointer to the Qt UI design.

    rra::BlasListModel*        model_;           ///< Container class for the widget models.
    uint64_t                   tlas_index_;      ///< The currently selected TLAS index.
    uint64_t                   blas_index_;      ///< The currently selected BLAS index.
    bool                       data_valid_;      ///< Is the trace data valid.
    BlasListTableItemDelegate* table_delegate_;  ///< The delegate responsible for painting the table.
};

#endif  // RRA_VIEWS_TLAS_BLAS_LIST_PANE_H_
