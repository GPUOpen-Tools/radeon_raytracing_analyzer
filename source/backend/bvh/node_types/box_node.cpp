//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Box node helper functions.
//=============================================================================

#include "bvh/node_types/box_node.h"

#include "bvh/node_pointer.h"

namespace dxr
{
    namespace amd
    {
        std::uint32_t GetValidChildCountFromArray(const std::array<dxr::amd::NodePointer, 4>& children)
        {
            std::uint32_t num_children = 0;
            for (const auto& c : children)
            {
                num_children += static_cast<int>(!c.IsInvalid());
            }

            return num_children;
        }

    }  // namespace amd
}  // namespace dxr

