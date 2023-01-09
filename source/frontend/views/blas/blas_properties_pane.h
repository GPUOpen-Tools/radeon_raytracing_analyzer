//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the BLAS properties pane.
//=============================================================================

#ifndef RRA_VIEWS_BLAS_BLAS_PROPERTIES_PANE_H_
#define RRA_VIEWS_BLAS_BLAS_PROPERTIES_PANE_H_

#include "ui_blas_properties_pane.h"

#include "models/blas/blas_properties_model.h"
#include "views/base_pane.h"

/// @brief Class declaration.
class BlasPropertiesPane : public BasePane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit BlasPropertiesPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~BlasPropertiesPane();

private slots:
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

private:
    Ui::BlasPropertiesPane* ui_;  ///< Pointer to the Qt UI design.

    rra::BlasPropertiesModel* model_;  ///< Container class for the widget models.
    uint64_t                  tlas_index_;  ///< The currently selected TLAS index.
};

#endif  // RRA_VIEWS_BLAS_BLAS_PROPERTIES_PANE_H_
