//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the Partitions pane on the TLAS tab.
//=============================================================================

#ifndef RRA_VIEWS_TLAS_TLAS_PARTITIONS_PANE_H_
#define RRA_VIEWS_TLAS_TLAS_PARTITIONS_PANE_H_

#include "ui_tlas_partitions_pane.h"

#include "models/table_item_delegate.h"
#include "models/tlas/tlas_partitions_model.h"
#include "views/base_pane.h"

/// @brief Class declaration.
class TlasPartitionsPane : public BasePane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit TlasPartitionsPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~TlasPartitionsPane();

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
    /// @brief Filter events to catch right clicks to deselect rows in partition table.
    ///
    /// @param obj The object associated with the event.
    /// @param event The Qt event.
    ///
    /// @return True if the event should be filtered, false otherwise.
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    /// @brief Set the TLAS index.
    ///
    /// Called when the user chooses a different TLAS from the combo box.
    /// Will force a redraw of the table, since the TLAS index has changed.
    void SetTlasIndex(uint64_t tlas_index);

    /// @brief Focus the TLAS viewer camera on a partition when its row is double-clicked.
    ///
    /// @param [in] index The model index of the double-clicked row.
    void GotoPartitionFromTableSelect(const QModelIndex& index);

private:
    Ui::TlasPartitionsPane* ui_;  ///< Pointer to the Qt UI design.

    rra::TlasPartitionsModel* model_;           ///< Container class for the widget models.
    uint64_t                  tlas_index_;      ///< The currently selected TLAS index.
    bool                      data_valid_;      ///< Is the trace data valid.
    TableItemDelegate*        table_delegate_;  ///< The delegate responsible for painting the table.
};

#endif  // RRA_VIEWS_TLAS_TLAS_PARTITIONS_PANE_H_

