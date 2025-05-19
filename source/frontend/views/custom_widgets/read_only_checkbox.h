//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for a read-only checkbox. Used for displaying boolean
/// values in tables.
//=============================================================================

#ifndef RRA_VIEWS_CUSTOM_WIDGETS_READ_ONLY_CHECKBOX_H_
#define RRA_VIEWS_CUSTOM_WIDGETS_READ_ONLY_CHECKBOX_H_

#include <QCheckBox>

/// @brief Helper class for read-only checkboxs.
class ReadOnlyCheckBox : public QCheckBox
{
public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit ReadOnlyCheckBox(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~ReadOnlyCheckBox();

    /// @brief Read-only checkbox paint event.
    ///
    /// @param [in] event Qt paint event.
    virtual void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;

private:
};

#endif  // RRA_VIEWS_CUSTOM_WIDGETS_READ_ONLY_CHECKBOX_H_

