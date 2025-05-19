//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for a triangle node class.
//=============================================================================

#ifndef RRA_BACKEND_BVH_NODE_TYPES_TRIANGLE_NODE_H_
#define RRA_BACKEND_BVH_NODE_TYPES_TRIANGLE_NODE_H_

#include "bvh/node_pointer.h"

namespace dxr
{
    namespace amd
    {
        /// @brief Triangle structure definition.
        struct Triangle
        {
            Float3 v0, v1, v2;

            const Float3& operator[](const std::int32_t i) const;
            Float3&       operator[](const std::int32_t i);
        };

        /// @brief Triangle node class definition.
        class TriangleNode final
        {
        public:
            /// @brief Constructor.
            TriangleNode();

            /// @brief Destructor.
            ~TriangleNode() = default;

            /// @brief Get triangle vertex.
            ///
            /// @param index The triangle index to retrieve.
            ///
            /// @return The triangle vertex.
            const Float3& GetVertex(const std::uint32_t index) const;

            /// @brief Get the three vertices of the triangle according to the node type.
            ///
            /// @param node_type The node type of the triangle.
            ///
            /// @return The triangle vertex data.
            Triangle GetTriangle(const NodeType node_type) const;

            /// @brief Get the actual (reconstructed) indices of the triangle.
            ///
            /// @param node_type The node type of the triangle.
            /// @param compression_mode The compression type.
            ///
            /// @return The triangle indices.
            std::array<std::uint32_t, 3> GetTriangleIndices(const NodeType node_type, const TriangleCompressionMode compression_mode) const;

            /// @brief Get the triangle, taking into account the compression mode.
            ///
            /// @param node_type The node type of the triangle.
            /// @param compression_mode The compression type.
            ///
            /// @return The triangle vertex data.
            Triangle GetTriangle(const NodeType node_type, const TriangleCompressionMode compression_mode) const;

            /// @brief Get all the triangle vertices.
            ///
            /// @return The triangle vertices.
            const std::array<Float3, 5>& GetVertices() const;

            /// @brief Get the triangle ID.
            ///
            /// @return The triangle ID.
            std::uint32_t GetTriangleId() const;

            /// @brief Obtain the index of the primitive of the triangle
            ///
            /// @param node_type The node type of the triangle.
            ///
            /// @return The primitive index.
            std::uint32_t GetPrimitiveIndex(const dxr::amd::NodeType node_type) const;

            /// @brief Set the primitive index.
            ///
            /// @param primitive_index The prinmitive index to set.
            /// @param node_type The node type of the triangle.
            void SetPrimitiveIndex(const std::uint32_t primitive_index, const dxr::amd::NodeType node_type);

            /// @brief Obtain the geometry index (The index of the geometry description and geometry flags (none, opaque, ...).
            ///
            /// @return The geometry index.
            std::uint32_t GetGeometryIndex() const;

            /// @brief Obtain the geometry flags.
            ///
            /// @return The geometry flags.
            dxr::GeometryFlags GetGeometryFlags() const;

            /// @brief Set the geometry index and geometry flags.
            ///
            /// @param geometry_index The geometry index.
            /// @param geometry_flags The geometry flags.
            void SetGeometryIndexAndFlags(const std::uint32_t geometry_index, const dxr::GeometryFlags geometry_flags);

            /// @brief Is the triangle inactive.
            ///
            /// Checks if the x-components of all triangle vertices are NaN according to the DXR 1.0 spec.
            ///
            /// @param node_type The node type of the triangle.
            ///
            /// @return true if inactive, false if not.
            bool IsInactive(NodeType node_type) const;

            /// @brief Check if any vertex is NaN.
            ///
            /// @param node_type The node type of the triangle.
            ///
            /// @return true if the triangle contains NaN, false otherwise.
            bool ContainsNaN(NodeType node_type) const;

        private:
            /// @brief Update the triangle ID.
            ///
            /// @param node_type The node type of the triangle.
            /// @param geometry_flags The geometry flags.
            /// @param id The current triangle id.
            /// @param rotation The triangle rotation.
            ///
            /// @return The triangle ID.
            void UpdateTriangleId(const dxr::amd::NodeType node_type,
                                  const dxr::GeometryFlags geometry_flags,
                                  const std::uint32_t      id       = 0,
                                  const std::uint32_t      rotation = 0);

            /// @brief Calculate the general triangle id of the triangle node based on the current triangle Id and the triangle rotation.
            ///
            /// @param node_type The node type of the triangle.
            /// @param geometry_flags The geometry flags.
            /// @param id The current triangle id.
            /// @param rotation The triangle rotation.
            ///
            /// @return The triangle ID.
            static std::uint32_t CalculateTriangleId(const dxr::amd::NodeType node_type,
                                                     const dxr::GeometryFlags geometry_flags,
                                                     const std::uint32_t      id       = 0,
                                                     const std::uint32_t      rotation = 0);

            std::array<Float3, 5> vertices_ = {};  ///< 5 Vertices to store quads (as triangle fans)
            std::uint32_t triangle_id_;  ///< ID of the triangle (defines rotation for compression mode)
        };
    }  // namespace amd
}  // namespace dxr

#endif  // RRA_BACKEND_BVH_NODE_TYPES_TRIANGLE_NODE_H_

