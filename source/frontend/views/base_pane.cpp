//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for a base pane class.
//=============================================================================

#include "views/base_pane.h"

BasePane::BasePane(QWidget* parent)
    : QWidget(parent)
{
}

BasePane::~BasePane()
{
}

void BasePane::OnTraceClose()
{
}

void BasePane::OnTraceOpen()
{
}

void BasePane::Reset()
{
}

