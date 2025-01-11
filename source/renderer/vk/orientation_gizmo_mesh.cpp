//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the orientation gizmo mesh types.
//=============================================================================

#include "orientation_gizmo_mesh.h"

namespace rra
{
    namespace renderer
    {
        static const float kPi = 3.14159265358979323846f;

        OrientationGizmoSubmeshInfo OrientationGizmoMeshInfo::GetSubmeshInfo(OrientationGizmoInstanceType type) const
        {
            if (type == OrientationGizmoInstanceType::kCylinder)
            {
                return cylinder_info_;
            }
            else if (type == OrientationGizmoInstanceType::kCircle)
            {
                return circle_info_;
            }
            else if (type == OrientationGizmoInstanceType::kX)
            {
                return x_info_;
            }
            else if (type == OrientationGizmoInstanceType::kY)
            {
                return y_info_;
            }
            else if (type == OrientationGizmoInstanceType::kZ)
            {
                return z_info_;
            }

            return cylinder_info_;
        }

        void OrientationGizmoMeshInfo::GenerateGeometry(std::vector<uint16_t>& indices, std::vector<VertexPosition>& vertices)
        {
            // The layout of each buffer is [cylinder, circle, X, Y, Z]
            const uint32_t cylinder_sides = 12;
            const uint32_t circle_sides   = 64;

            GenerateCylinderGeometry(cylinder_sides, indices, vertices);
            GenerateCircleGeometry(circle_sides, indices, vertices);
            GenerateXYZGeometry(indices, vertices);
        }

        void OrientationGizmoMeshInfo::GenerateCylinderGeometry(uint16_t n, std::vector<uint16_t>& indices, std::vector<VertexPosition>& vertices)
        {
            cylinder_info_.first_index   = static_cast<uint32_t>(indices.size());
            cylinder_info_.vertex_offset = static_cast<int32_t>(vertices.size());

            const float angle = (2 * kPi) / n;

            // Top circular face.
            for (uint16_t i = 0; i < n; ++i)
            {
                indices.push_back(2 * n);
                indices.push_back(i);
                indices.push_back((i + 1) % n);

                // Outer top vertices.
                vertices.push_back(VertexPosition(glm::vec3(-glm::cos(i * angle), 1.0f, glm::sin(i * angle))));
            }

            // Bottom circular face.
            for (uint16_t i = 0; i < n; ++i)
            {
                indices.push_back(2 * n + 1);
                indices.push_back(n + i);
                indices.push_back(n + (i + 1) % n);

                // Outer bottom vertices.
                vertices.push_back(VertexPosition(glm::vec3(-glm::cos(i * angle), 0.0f, glm::sin(i * angle))));
            }

            // Top and bottom middle vertices.
            vertices.push_back(VertexPosition(glm::vec3(0.0f, 1.0f, 0.0f)));
            vertices.push_back(VertexPosition(glm::vec3(0.0f, 0.0f, 0.0f)));

            // Side faces.
            for (uint16_t i = 0; i < n; ++i)
            {
                // Top triangle.
                indices.push_back(i);
                indices.push_back(n + i % n);
                indices.push_back((i + 1) % n);

                // Bottom triangle.
                indices.push_back(n + i);
                indices.push_back(n + (i + 1) % n);
                indices.push_back((i + 1) % n);
            }
            cylinder_info_.index_count = static_cast<uint32_t>(indices.size() - cylinder_info_.first_index);
        }

        void OrientationGizmoMeshInfo::GenerateCircleGeometry(uint16_t n, std::vector<uint16_t>& indices, std::vector<VertexPosition>& vertices)
        {
            circle_info_.first_index   = static_cast<uint32_t>(indices.size());
            circle_info_.vertex_offset = static_cast<int32_t>(vertices.size());

            const float angle = (2 * kPi) / n;

            for (uint16_t i = 0; i < n; ++i)
            {
                // Center vertex index.
                indices.push_back(n);
                // Outer indices.
                indices.push_back(i);
                indices.push_back((i + 1) % n);

                // Outer vertices.
                vertices.push_back(VertexPosition(glm::vec3(-glm::cos(i * angle), glm::sin(i * angle), 0.0f)));
            }

            // Center vertex
            vertices.push_back(VertexPosition(glm::vec3(0.0f, 0.0f, 0.0f)));

            circle_info_.index_count = static_cast<uint32_t>(indices.size() - circle_info_.first_index);
        }

        void OrientationGizmoMeshInfo::GenerateXYZGeometry(std::vector<uint16_t>& indices, std::vector<VertexPosition>& vertices)
        {
            // X geometry generation.
            std::vector<uint16_t> x_indices = {0, 1, 5, 1, 4, 5, 2, 6, 4, 2, 3, 6, 5, 4, 6, 5, 6, 7, 8, 5, 7, 8, 7, 9, 7, 11, 10, 7, 6, 11};

            std::vector<VertexPosition> x_vertices = {
                VertexPosition(glm::vec3(-0.486927f, 0.5f, 0.0f)),
                VertexPosition(glm::vec3(-0.31244f, 0.5f, 0.0f)),
                VertexPosition(glm::vec3(0.313662, 0.5f, 0.0f)),
                VertexPosition(glm::vec3(0.486683f, 0.5f, 0.0f)),
                VertexPosition(glm::vec3(0.001344f, 0.118767f, 0.0f)),
                VertexPosition(glm::vec3(-0.086633f, 0.017594f, 0.0f)),
                VertexPosition(glm::vec3(0.087855f, 0.017594f, 0.0f)),
                VertexPosition(glm::vec3(0.001344f, -0.086512, 0.0f)),
                VertexPosition(glm::vec3(-0.517719f, -0.5f, 0.0f)),
                VertexPosition(glm::vec3(-0.343232f, -0.5f, 0.0f)),
                VertexPosition(glm::vec3(0.341521f, -0.5f, 0.0f)),
                VertexPosition(glm::vec3(0.514542f, -0.5f, 0.0f)),
            };

            x_info_.first_index   = static_cast<uint32_t>(indices.size());
            x_info_.vertex_offset = static_cast<int32_t>(vertices.size());
            indices.insert(indices.end(), x_indices.begin(), x_indices.end());
            vertices.insert(vertices.end(), x_vertices.begin(), x_vertices.end());
            x_info_.index_count = static_cast<uint32_t>(indices.size() - x_info_.first_index);

            // Y geometry generation.
            std::vector<uint16_t> y_indices = {0, 1, 5, 1, 4, 5, 2, 6, 4, 2, 3, 6, 5, 4, 6, 5, 6, 7, 6, 8, 7};

            std::vector<VertexPosition> y_vertices = {
                VertexPosition(glm::vec3(-0.442654f, 0.500275f, 0.0f)),
                VertexPosition(glm::vec3(-0.269633f, 0.500275f, 0.0f)),
                VertexPosition(glm::vec3(0.269958f, 0.500275f, 0.0f)),
                VertexPosition(glm::vec3(0.44298f, 0.500275f, 0.0f)),
                VertexPosition(glm::vec3(-0.001303f, 0.155699f, 0.0f)),
                VertexPosition(glm::vec3(-0.071685f, 0.029599f, 0.0f)),
                VertexPosition(glm::vec3(0.072011f, 0.028133f, 0.0f)),
                VertexPosition(glm::vec3(-0.071685f, -0.499728f, 0.0f)),
                VertexPosition(glm::vec3(0.072011f, -0.499728f, 0.0f)),
            };

            y_info_.first_index   = static_cast<uint32_t>(indices.size());
            y_info_.vertex_offset = static_cast<int32_t>(vertices.size());
            indices.insert(indices.end(), y_indices.begin(), y_indices.end());
            vertices.insert(vertices.end(), y_vertices.begin(), y_vertices.end());
            y_info_.index_count = static_cast<uint32_t>(indices.size() - y_info_.first_index);

            // Z geometry generation.
            std::vector<uint16_t> z_indices = {0, 1, 2, 2, 1, 3, 3, 1, 4, 6, 3, 4, 6, 4, 5, 6, 5, 7};

            std::vector<VertexPosition> z_vertices = {
                VertexPosition(glm::vec3(-0.421189f, 0.500179f, 0.0f)),
                VertexPosition(glm::vec3(0.440983f, 0.500179f, 0.0f)),
                VertexPosition(glm::vec3(-0.421189f, 0.371146f, 0.0f)),
                VertexPosition(glm::vec3(0.175587f, 0.371146f, 0.0f)),
                VertexPosition(glm::vec3(-0.195382f, -0.370792f, 0.0f)),
                VertexPosition(glm::vec3(0.440983f, -0.370792f, 0.0f)),
                VertexPosition(glm::vec3(-0.460779f, -0.499824f, 0.0f)),
                VertexPosition(glm::vec3(0.440983f, -0.499824f, 0.0f)),
            };

            z_info_.first_index   = static_cast<uint32_t>(indices.size());
            z_info_.vertex_offset = static_cast<int32_t>(vertices.size());
            indices.insert(indices.end(), z_indices.begin(), z_indices.end());
            vertices.insert(vertices.end(), z_vertices.begin(), z_vertices.end());
            z_info_.index_count = static_cast<uint32_t>(indices.size() - z_info_.first_index);
        }

    }  // namespace renderer
}  // namespace rra
