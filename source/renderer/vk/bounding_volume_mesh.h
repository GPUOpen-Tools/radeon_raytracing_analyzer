//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for static bounding volume mesh types.
//=============================================================================

#ifndef RRA_RENDERER_VK_BOUNDING_VOLUME_MESH_H_
#define RRA_RENDERER_VK_BOUNDING_VOLUME_MESH_H_

#include "mesh.h"

namespace rra
{
    namespace renderer
    {
        /// @brief This type is able to upload geometry for a solid cube.
        class SolidBoxMeshInfo : public TypedMesh<VertexPositionNormal>
        {
        public:
            /// @brief Constructor.
            SolidBoxMeshInfo() = default;

            /// @brief Destructor.
            virtual ~SolidBoxMeshInfo() = default;

        protected:
            /// @brief Populate the provided vertex and index buffer arrays with geometric data.
            ///
            /// @param [out] indices The index buffer data.
            /// @param [out] vertices The vertex buffer data.
            virtual void GenerateGeometry(std::vector<uint16_t>& indices, std::vector<VertexPositionNormal>& vertices) override;
        };

        /// @brief This type is able to upload geometry for a wireframe cube.
        class WireframeBoxMeshInfo : public TypedMesh<VertexPosition>
        {
        public:
            /// @brief Constructor.
            WireframeBoxMeshInfo() = default;

            /// @brief Destructor.
            virtual ~WireframeBoxMeshInfo() = default;

        protected:
            /// @brief Populate the provided vertex and index buffer arrays with geometric data.
            ///
            /// @param [out] indices The index buffer data.
            /// @param [out] vertices The vertex buffer data.
            virtual void GenerateGeometry(std::vector<uint16_t>& indices, std::vector<VertexPosition>& vertices) override;
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_BOUNDING_VOLUME_MESH_H_
