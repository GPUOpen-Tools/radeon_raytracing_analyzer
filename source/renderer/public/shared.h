//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Decleration for shared shader functions.
///
/// These functions and structures will be shared between shaders and included in C++ code
/// to avoid code duplication.
//=============================================================================

#ifndef RRA_SHADERS_SHARED_
#define RRA_SHADERS_SHARED_

#include "glm/glm/glm.hpp"

namespace rra
{
    namespace renderer
    {
        typedef glm::mat4 float4x4;
        typedef glm::vec4 float4;
        typedef glm::vec3 float3;
        typedef glm::vec2 float2;
        typedef uint32_t  uint;

#include "shaders/shared_definitions.hlsl"
    }  // namespace renderer
}  // namespace rra

#endif

