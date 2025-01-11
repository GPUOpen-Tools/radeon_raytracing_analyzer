//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for a base pane class.
//=============================================================================

#include "base_pane.h"

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
