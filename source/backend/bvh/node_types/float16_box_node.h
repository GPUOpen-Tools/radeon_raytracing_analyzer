//=============================================================================
// Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of a float 16 box node class.
//=============================================================================

#ifndef RRA_BACKEND_BVH_NODE_TYPES_FLOAT16_BOX_NODE_H_
#define RRA_BACKEND_BVH_NODE_TYPES_FLOAT16_BOX_NODE_H_

#include "bvh/dxr_definitions.h"
#include "bvh/node_pointer.h"

namespace dxr
{
    namespace amd
    {
        /// @brief Structure to describe a 3D vector of half-precision floats.
        struct HalfFloat3
        {
            HalfFloat x, y, z;
        };

        /// @brief Class to describe a half-precision float AABB.
        class Float16AxisAlignedBoundingBox final
        {
        public:
            Float16AxisAlignedBoundingBox() = default;

            HalfFloat3 min = {0, 0, 0};
            HalfFloat3 max = {0, 0, 0};
        };

        /// @brief Compact version of a box node, half the size of the FP32 box node.
        class Float16BoxNode final
        {
        public:
            /// @brief Constructor.
            Float16BoxNode() = default;

            /// @brief Destructor.
            ~Float16BoxNode() = default;

            /// @brief Get the child nodes.
            ///
            /// @return The child nodes.
            const std::array<NodePointer, 4>& GetChildren() const;

            /// @brief Get the bounding volumes.
            ///
            /// @return The bounding volumes.
            const std::array<AxisAlignedBoundingBox, 4> GetBoundingBoxes() const;

            /// @brief Get the number of valid child nodes. They can be scattered across all 4 positions.
            ///
            /// @return The number of valid child nodes.
            std::uint32_t GetValidChildCount() const;

        private:
            std::array<NodePointer, 4>                   children_       = {};  ///< Array of child nodes.
            std::array<Float16AxisAlignedBoundingBox, 4> bounding_boxes_ = {};  ///< Array of bounding volumes.
        };

    }  // namespace amd
}  // namespace dxr

#endif  // RRA_BACKEND_BVH_NODE_TYPES_FLOAT16_BOX_NODE_H_
