//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the base pane class.
//=============================================================================

#ifndef RRA_VIEWS_BASE_PANE_H_
#define RRA_VIEWS_BASE_PANE_H_

#include <QWidget>

/// @brief Base class for a pane in the UI.
class BasePane : public QWidget
{
    Q_OBJECT

public:
    /// @brief Constructor.
    ///
    /// @param [in] parent The parent widget.
    explicit BasePane(QWidget* parent = nullptr);

    /// @brief Destructor.
    virtual ~BasePane();

    /// @brief Trace closed.
    virtual void OnTraceClose();

    /// @brief Trace open.
    virtual void OnTraceOpen();

    /// @brief Reset the UI.
    virtual void Reset();
};

#endif  // RRA_VIEWS_BASE_PANE_H_
