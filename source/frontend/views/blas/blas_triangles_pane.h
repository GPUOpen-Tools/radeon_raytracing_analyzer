//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the Triangles pane on the BLAS tab.
//=============================================================================

#ifndef RRA_VIEWS_BLAS_BLAS_TRIANGLES_PANE_H_
#define RRA_VIEWS_BLAS_BLAS_TRIANGLES_PANE_H_

#include "ui_blas_triangles_pane.h"

#include "models/blas/blas_triangles_model.h"
#include "models/table_item_delegate.h"
#include "views/base_pane.h"

/// @brief Class declaration.
class BlasTrianglesPane : public BasePane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit BlasTrianglesPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~BlasTrianglesPane();

    /// @brief Overridden pane key press event.
    ///
    /// @param [in] event The key press event object.
    virtual void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;

    /// @brief Overridden window show event.
    ///
    /// @param [in] event the show event object.
    virtual void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;

    /// @brief Trace opened.
    virtual void OnTraceOpen() Q_DECL_OVERRIDE;

    /// @brief Trace closed.
    virtual void OnTraceClose() Q_DECL_OVERRIDE;

protected:
    /// @brief Filter events to catch right clicks to deselect rows in triangle table.
    ///
    /// @param obj The object associated with the event.
    /// @param event The Qt event.
    ///
    /// @return True if the event should be filtered, false otherwise.
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    /// @brief Navigate to the BLAS viewr pane from a selection within the BLAS Triangles table.
    ///
    /// @param [in] index The model index of the item selected in the table.
    /// @param [in] navigate_to_pane If true, navigate to the BLAS viewer pane.
    void SelectTriangleInBlasViewer(const QModelIndex& index, bool navigate_to_pane) const;

    /// @brief Slot to handle what happens when a triangle is clicked on in the BLAS viewer.
    ///
    /// Select the correct row in the table.
    ///
    /// @param triangle_node_id The node id of the triangle clicked on.
    void SelectTriangle(uint32_t triangle_node_id);

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

    /// @brief Slot to handle what happens after the triangle table is sorted.
    ///
    /// Make sure the selected item (if there is one) is visible.
    void ScrollToSelectedTriangle();

private:
    Ui::BlasTrianglesPane* ui_;  ///< Pointer to the Qt UI design.

    rra::BlasTrianglesModel* model_;             ///< Container class for the widget models.
    uint64_t                 blas_index_;        ///< The currently selected BLAS index.
    uint64_t                 tlas_index_;        ///< The currently selected TLAS index.
    uint32_t                 triangle_node_id_;  ///< The currently selected triangle node id;
    bool                     data_valid_;        ///< Is the trace data valid.
    TableItemDelegate*       table_delegate_;    ///< The delegate responsible for painting the table.
};

#endif  // #define RRA_VIEWS_BLAS_BLAS_TRIANGLES_PANE_H_

