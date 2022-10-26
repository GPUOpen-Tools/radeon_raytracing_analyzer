//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for static orientation gizmo mesh type.
//=============================================================================

#ifndef RRA_RENDERER_VK_ORIENTATION_GIZMO_MESH_H_
#define RRA_RENDERER_VK_ORIENTATION_GIZMO_MESH_H_

#include "mesh.h"

namespace rra
{
    namespace renderer
    {
        struct OrientationGizmoSubmeshInfo
        {
            uint32_t index_count;
            int32_t  first_index;
            uint32_t vertex_offset;
        };

        /// @brief This type is able to upload geometry for a wireframe cube.
        class OrientationGizmoMeshInfo : public TypedMesh<VertexPosition>
        {
        public:
            /// @brief Constructor.
            OrientationGizmoMeshInfo() = default;

            /// @brief Destructor.
            virtual ~OrientationGizmoMeshInfo() = default;

            /// @brief Populate the provided vertex and index buffer arrays with geometric data.
            ///
            /// @param [in] type The instance type.
            ///
            /// @returns The submesh info.
            OrientationGizmoSubmeshInfo GetSubmeshInfo(OrientationGizmoInstanceType type) const;

        protected:
            /// @brief Populate the provided vertex and index buffer arrays with geometric data.
            ///
            /// @param [out] indices The index buffer data.
            /// @param [out] vertices The vertex buffer data.
            virtual void GenerateGeometry(std::vector<uint16_t>& indices, std::vector<VertexPosition>& vertices) override;

        private:
            /// @brief Populate the provided vertex and index buffer arrays with cylinder geometry data.
            ///
            /// @param [in] n The number of sides the cylinder will have.
            /// @param [out] indices The index buffer data.
            /// @param [out] vertices The vertex buffer data.
            void GenerateCylinderGeometry(uint16_t n, std::vector<uint16_t>& indices, std::vector<VertexPosition>& vertices);

            /// @brief Populate the provided vertex and index buffer arrays with circle geometry data.
            ///
            /// @param [in] n The number of sides the circle will have.
            /// @param [out] indices The index buffer data.
            /// @param [out] vertices The vertex buffer data.
            void GenerateCircleGeometry(uint16_t n, std::vector<uint16_t>& indices, std::vector<VertexPosition>& vertices);

            /// @brief Populate the provided vertex and index buffer arrays with X, Y, and Z text geometry data.
            ///
            /// @param [out] indices The index buffer data.
            /// @param [out] vertices The vertex buffer data.
            void GenerateXYZGeometry(std::vector<uint16_t>& indices, std::vector<VertexPosition>& vertices);

            OrientationGizmoSubmeshInfo cylinder_info_ = {};  ///< Submesh info for the cylinder.
            OrientationGizmoSubmeshInfo circle_info_   = {};  ///< Submesh info for the circle.
            OrientationGizmoSubmeshInfo x_info_        = {};  ///< Submesh info for the X.
            OrientationGizmoSubmeshInfo y_info_        = {};  ///< Submesh info for the Y.
            OrientationGizmoSubmeshInfo z_info_        = {};  ///< Submesh info for the Z.
        };

    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_ORIENTATION_GIZMO_MESH_H_
