//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation and linking for shared shader functions.
///
/// These functions will be shared between shaders and included in C++ code
/// to avoid code duplication.
//=============================================================================

#include "public/shared.h"

namespace rra
{
    namespace renderer
    {
#include "shaders/shared_impl.hlsl"
    }  // namespace renderer
}  // namespace rra

