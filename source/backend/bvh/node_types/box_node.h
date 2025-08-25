//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Box node helper functions.
//=============================================================================

#ifndef RRA_BACKEND_BVH_NODE_TYPES_BOX_NODE_H_
#define RRA_BACKEND_BVH_NODE_TYPES_BOX_NODE_H_

#include <array>
#include <cstdint>

#include "bvh/node_pointer.h"

namespace dxr
{
    namespace amd
    {
        /// @brief Get the number of valid child nodes.
        ///
        /// @param children An array of child nodes.
        ///
        /// @return The number of valid child nodes.
        std::uint32_t GetValidChildCountFromArray(const std::array<dxr::amd::NodePointer, 4>& children);

    }  // namespace amd
}  // namespace dxr

#endif  // RRA_BACKEND_BVH_NODE_TYPES_BOX_NODE_H_
