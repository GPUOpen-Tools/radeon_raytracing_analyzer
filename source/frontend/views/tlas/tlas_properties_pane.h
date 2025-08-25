//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the TLAS properties pane.
//=============================================================================

#ifndef RRA_VIEWS_TLAS_TLAS_PROPERTIES_PANE_H_
#define RRA_VIEWS_TLAS_TLAS_PROPERTIES_PANE_H_

#include "ui_tlas_properties_pane.h"

#include "models/tlas/tlas_properties_model.h"
#include "views/base_pane.h"

/// @brief Class declaration.
class TlasPropertiesPane : public BasePane
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit TlasPropertiesPane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~TlasPropertiesPane();

private slots:
    /// @brief Set the TLAS index.
    ///
    /// Called when the user chooses a different TLAS from the combo box.
    /// Will force a redraw of the table, since the TLAS index has changed.
    ///
    /// @param [in] tlas_index The TLAS index to set.
    void SetTlasIndex(uint64_t tlas_index);

private:
    Ui::TlasPropertiesPane* ui_;  ///< Pointer to the Qt UI design.

    rra::TlasPropertiesModel* model_;  ///< Container class for the widget models.
};

#endif  // RRA_VIEWS_TLAS_TLAS_PROPERTIES_PANE_H_
