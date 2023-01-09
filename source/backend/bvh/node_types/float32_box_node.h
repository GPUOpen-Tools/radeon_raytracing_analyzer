//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of a float 32 box node class.
//=============================================================================

#ifndef RRA_BACKEND_BVH_NODE_TYPES_FLOAT32_BOX_NODE_H_
#define RRA_BACKEND_BVH_NODE_TYPES_FLOAT32_BOX_NODE_H_

#include "bvh/node_pointer.h"

namespace dxr
{
    namespace amd
    {
        // Default layout of an internal node in Blas.
        class Float32BoxNode final
        {
        public:
            /// @brief Constructor.
            Float32BoxNode() = default;

            /// @brief Destructor.
            ~Float32BoxNode() = default;

            /// @brief Get the child nodes.
            ///
            /// @return The child nodes.
            const std::array<NodePointer, 4>& GetChildren() const;

            /// @brief Get the bounding volumes.
            ///
            /// @return The bounding volumes.
            const std::array<AxisAlignedBoundingBox, 4>& GetBoundingBoxes() const;

            /// @brief Get the number of valid child nodes. They can be scattered across all 4 positions.
            ///
            /// @return The number of valid child nodes.
            std::uint32_t GetValidChildCount() const;

        private:
            std::array<NodePointer, 4>            children_       = {};  ///< Array of child nodes.
            std::array<AxisAlignedBoundingBox, 4> bounding_boxes_ = {};  ///< Array of bounding volumes.
            std::uint32_t                         reserved_       = 0;   ///< Reserved.
            std::uint32_t                         reserved2_      = 0;   ///< Reserved.
            std::array<std::uint32_t, 2>          padding_        = {};  ///< Padding.
        };
    }  // namespace amd
}  // namespace dxr

#endif  // RRA_BACKEND_BVH_NODE_TYPES_FLOAT32_BOX_NODE_H_
