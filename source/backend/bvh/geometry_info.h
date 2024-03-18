//=============================================================================
// Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the geometry info class.
//=============================================================================

#ifndef RRA_BACKEND_BVH_GEOMETRY_INFO_H_
#define RRA_BACKEND_BVH_GEOMETRY_INFO_H_

#include "bvh/dxr_definitions.h"

namespace dxr
{
    namespace amd
    {
        class GeometryInfo final
        {
        public:
            /// @brief Constructor.
            GeometryInfo() = default;

            /// @brief Destructor.
            ~GeometryInfo() = default;

            /// @brief Obtain the geometry flags.
            ///
            /// @return The geometry flags.
            GeometryFlags GetGeometryFlags() const;

            /// @brief Obtain the primitive count.
            ///
            /// return The primitive count.
            std::uint32_t GetPrimitiveCount() const;

            /// @brief Get the offset to the primitive node pointers.
            ///
            /// @return The primitive node pointer offset.
            std::uint32_t GetPrimitiveNodePtrsOffset() const;

        private:
            union  ///< Encodes the geometry flags (opaque, non_opaque, ...) and number of primitives.
            {
                struct
                {
                    std::uint32_t primitive_count : 29;
                    std::uint32_t geometry_flags : 3;
                };
                std::uint32_t geometry_flags_and_primitive_count_ = 0;
            };

            std::uint32_t geometry_buffer_offset_ = 0;  ///< Byte offset to the geometry buffer (leaf nodes)

            std::uint32_t primitive_node_ptrs_offset_ =
                0;  ///< Byte offset from the base of all primitive node ptrs to this geometry's prim node ptrs. Each node pointer points to the primitive.
        };

    }  // namespace amd
}  // namespace dxr

#endif  // RRA_BACKEND_BVH_GEOMETRY_INFO_H_
