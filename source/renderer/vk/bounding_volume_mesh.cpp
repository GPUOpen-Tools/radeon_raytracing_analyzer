//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the static bounding volume mesh types.
//=============================================================================

#include "bounding_volume_mesh.h"

namespace rra
{
    namespace renderer
    {
        void SolidBoxMeshInfo::GenerateGeometry(std::vector<uint16_t>& indices, std::vector<VertexPositionNormal>& vertices)
        {
            // The box has 6 faces, with 2 triangles per face.
            // Each face must have its own normal, so we can't share vertices between faces.
            std::vector<uint16_t> face_indices = {
                0,  1,  2,   // Front
                0,  2,  3,   // Front
                4,  5,  6,   // Right
                4,  6,  7,   // Right
                8,  9,  10,  // Back
                8,  10, 11,  // Back
                12, 13, 14,  // Left
                12, 14, 15,  // Left
                16, 17, 18,  // Bottom
                16, 18, 19,  // Bottom
                20, 21, 22,  // Top
                20, 22, 23   // Top
            };

            indices = face_indices;

            // The positions for each cube vertex.
            static const glm::vec3 kLeftBottomBack   = {-1, -1, 1};
            static const glm::vec3 kRightBottomBack  = {1, -1, 1};
            static const glm::vec3 kRightTopBack     = {1, 1, 1};
            static const glm::vec3 kLeftTopBack      = {-1, 1, 1};
            static const glm::vec3 kLeftBottomFront  = {-1, -1, -1};
            static const glm::vec3 kRightBottomFront = {1, -1, -1};
            static const glm::vec3 kRightTopFront    = {1, 1, -1};
            static const glm::vec3 kLeftTopFront     = {-1, 1, -1};

            // Front
            static const glm::vec3 kForward = {0, 0, 1};
            vertices.push_back(VertexPositionNormal(kLeftBottomFront, kForward));
            vertices.push_back(VertexPositionNormal(kRightBottomFront, kForward));
            vertices.push_back(VertexPositionNormal(kRightTopFront, kForward));
            vertices.push_back(VertexPositionNormal(kLeftTopFront, kForward));

            // Right
            static const glm::vec3 kRight = {1, 0, 0};
            vertices.push_back(VertexPositionNormal(kRightBottomFront, kRight));
            vertices.push_back(VertexPositionNormal(kRightBottomBack, kRight));
            vertices.push_back(VertexPositionNormal(kRightTopBack, kRight));
            vertices.push_back(VertexPositionNormal(kRightTopFront, kRight));

            // Back
            static const glm::vec3 kBackward = {0, 0, -1};
            vertices.push_back(VertexPositionNormal(kRightBottomBack, kBackward));
            vertices.push_back(VertexPositionNormal(kLeftBottomBack, kBackward));
            vertices.push_back(VertexPositionNormal(kLeftTopBack, kBackward));
            vertices.push_back(VertexPositionNormal(kRightTopBack, kBackward));

            // Left
            static const glm::vec3 kLeft = {-1, 0, 0};
            vertices.push_back(VertexPositionNormal(kLeftBottomBack, kLeft));
            vertices.push_back(VertexPositionNormal(kLeftBottomFront, kLeft));
            vertices.push_back(VertexPositionNormal(kLeftTopFront, kLeft));
            vertices.push_back(VertexPositionNormal(kLeftTopBack, kLeft));

            // Bottom
            static const glm::vec3 kDown = {0, -1, 0};
            vertices.push_back(VertexPositionNormal(kLeftBottomBack, kDown));
            vertices.push_back(VertexPositionNormal(kRightBottomBack, kDown));
            vertices.push_back(VertexPositionNormal(kRightBottomFront, kDown));
            vertices.push_back(VertexPositionNormal(kLeftBottomFront, kDown));

            // Top
            static const glm::vec3 kUp = {0, 1, 0};
            vertices.push_back(VertexPositionNormal(kLeftTopFront, kUp));
            vertices.push_back(VertexPositionNormal(kRightTopFront, kUp));
            vertices.push_back(VertexPositionNormal(kRightTopBack, kUp));
            vertices.push_back(VertexPositionNormal(kLeftTopBack, kUp));
        }

        void WireframeBoxMeshInfo::GenerateGeometry(std::vector<uint16_t>& indices, std::vector<VertexPosition>& vertices)
        {
            std::vector<uint16_t> line_indices = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};
            indices                            = line_indices;

            vertices.push_back(VertexPosition({-1, -1, 1}));
            vertices.push_back(VertexPosition({1, -1, 1}));
            vertices.push_back(VertexPosition({1, 1, 1}));
            vertices.push_back(VertexPosition({-1, 1, 1}));
            vertices.push_back(VertexPosition({-1, -1, -1}));
            vertices.push_back(VertexPosition({1, -1, -1}));
            vertices.push_back(VertexPosition({1, 1, -1}));
            vertices.push_back(VertexPosition({-1, 1, -1}));
        }
    }  // namespace renderer
}  // namespace rra
