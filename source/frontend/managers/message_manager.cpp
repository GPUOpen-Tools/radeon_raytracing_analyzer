//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the MessageManager class.
///
/// Basically a bunch of signals that other UI elements can observe and react
/// to.
///
//=============================================================================

#include "managers/message_manager.h"

namespace rra
{
    // Single instance of the Message Manager.
    static MessageManager message_manager;

    MessageManager& MessageManager::Get()
    {
        return message_manager;
    }
}  // namespace rra
