//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of a procedural node class.
//=============================================================================

#ifndef RRA_BACKEND_BVH_NODE_TYPES_PROCEDURAL_NODE_H_
#define RRA_BACKEND_BVH_NODE_TYPES_PROCEDURAL_NODE_H_

#include "bvh/dxr_definitions.h"

namespace dxr
{
    namespace amd
    {
        class ProceduralNode final
        {
        public:
            /// @brief Constructor.
            ProceduralNode() = default;

            /// @brief Destructor.
            ~ProceduralNode() = default;

            /// @brief Get the bounding volume.
            ///
            /// @return The bounding volume.
            const AxisAlignedBoundingBox& GetBoundingBox() const;

            /// @brief Obtain the geometry index.
            ///
            /// @return The geometry index.
            std::uint32_t GetGeometryIndex() const;

            /// @brief Obtain the geometry flags.
            ///
            /// @return The geometry flags.
            GeometryFlags GetGeometryFlags() const;

            /// @brief Obtain the primitive index.
            ///
            /// @return The primitive index.
            std::uint32_t GetPrimitiveIndex() const;

            /// @brief Check if the x-component of the bounding box minimum is NaN.
            ///
            /// @param node_type The node type of the triangle.
            ///
            /// @return true if inactive, false if not.
            bool IsInactive() const;

            /// @brief Check if any vertex is NaN.
            ///
            /// @param node_type The node type of the triangle.
            ///
            /// @return true if the triangle contains NaN, false otherwise.
            bool ContainsNaN() const;

        private:
            AxisAlignedBoundingBox       bounding_box_ = {};  ///< The bounding volume.
            std::array<std::uint32_t, 3> padding_      = {};  ///< Padding.
            std::uint32_t                unused_       = 0;   ///< Unused.
            std::array<std::uint32_t, 2> padding2_     = {};  ///< Padding.

            union  ///< The geometry index and flags.
            {
                struct
                {
                    std::uint32_t geometry_index : 24;
                    std::uint32_t geometry_flags : 8;
                };
                std::uint32_t geometry_index_and_flags_ = 0;
            };

            std::uint32_t                primitive_index_ = 0;   ///< The primitive index.
            std::array<std::uint32_t, 2> padding3_        = {};  ///< Padding.
        };

    }  // namespace amd
}  // namespace dxr

#endif  // RRA_BACKEND_BVH_NODE_TYPES_PROCEDURAL_NODE_H_

