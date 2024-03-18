//=============================================================================
// Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the geometries pane on the BLAS tab.
//=============================================================================

#ifndef RRA_VIEWS_BLAS_BLAS_GEOMETRIES_PANE_H_
#define RRA_VIEWS_BLAS_BLAS_GEOMETRIES_PANE_H_

#include "ui_blas_geometries_pane.h"

#include "models/blas/blas_geometries_model.h"
#include "models/table_item_delegate.h"
#include "views/base_pane.h"

/// @brief Class declaration.
class BlasGeometriesPane : public BasePane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit BlasGeometriesPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~BlasGeometriesPane();

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
    /// @brief Filter events to catch right clicks to deselect rows in geometry table.
    ///
    /// @param obj The object associated with the event.
    /// @param event The Qt event.
    ///
    /// @return True if the event should be filtered, false otherwise.
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    /// @brief Slot to handle what happens when a geometry is clicked on in the BLAS viewer.
    ///
    /// Select the correct row in the table.
    ///
    /// @param geometry_index The index of the geometry clicked on.
    void SelectGeometry(uint32_t geometry_index);

    /// @brief Set the BLAS index.
    ///
    /// Called when the user chooses a different BLAS from the combo box or
    /// from the BLAS list table.
    /// Will force a redraw of the table, since the BLAS index has changed.
    void SetBlasIndex(uint64_t blas_index);

    /// @brief Set the TLAS index.
    ///
    /// Called when the user chooses a different TLAS from the combo box.
    /// Will force a redraw of the table, since the TLAS index has changed.
    void SetTlasIndex(uint64_t tlas_index);

    /// @brief Slot to handle what happens after the geometry table is sorted.
    ///
    /// Make sure the selected item (if there is one) is visible.
    void ScrollToSelectedGeometry();

private:
    Ui::BlasGeometriesPane* ui_;  ///< Pointer to the Qt UI design.

    rra::BlasGeometriesModel* model_;                    ///< Container class for the widget models.
    uint64_t                  blas_index_;               ///< The currently selected BLAS index.
    uint64_t                  tlas_index_;               ///< The currently selected TLAS index.
    uint32_t                  selected_geometry_index_;  ///< The currently selected geometry index.
    bool                      data_valid_;               ///< Is the trace data valid.
    TableItemDelegate*        table_delegate_;           ///< The delegate responsible for painting the table.
};

#endif  // #define RRA_VIEWS_BLAS_BLAS_GEOMETRIES_PANE_H_
