//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 1.1 (Navi2x) specific bottom level acceleration structure
/// implementation.
//=============================================================================

#include "bvh/rtip_common/encoded_bottom_level_bvh.h"

#include <cassert>
#include <deque>
#include <iostream>
#include <limits>
#include <vector>

#include "public/rra_blas.h"

#include "bvh/rtip11/rt_ip_11_header.h"

namespace rta
{
    EncodedBottomLevelBvh::~EncodedBottomLevelBvh()
    {
    }

    const std::vector<dxr::amd::GeometryInfo>& EncodedBottomLevelBvh::GetGeometryInfos() const
    {
        return geom_infos_;
    }

    const std::vector<dxr::amd::NodePointer>& EncodedBottomLevelBvh::GetPrimitiveNodePtrs() const
    {
        return primitive_node_ptrs_;
    }

    bool EncodedBottomLevelBvh::HasBvhReferences() const
    {
        return false;
    }

    std::uint64_t EncodedBottomLevelBvh::GetBufferByteSizeImpl(const ExportOption export_option) const
    {
        auto file_size = header_->GetFileSize();
        if (export_option == ExportOption::kNoMetaData)
        {
            file_size -= meta_data_.GetByteSize();
        }
        auto min_file_size = kMinimumFileSize;
        return std::max(file_size, min_file_size);
    }

    bool EncodedBottomLevelBvh::Validate()
    {
        if (header_->GetGeometryType() == rta::BottomLevelBvhGeometryType::kTriangle)
        {
            // Make sure the number of primitives in the BLAS and header match.
            uint32_t total_triangle_count = 0;
            for (auto geom_iter = geom_infos_.begin(); geom_iter != geom_infos_.end(); ++geom_iter)
            {
                total_triangle_count += geom_iter->GetPrimitiveCount();
            }

            if (total_triangle_count != header_->GetPrimitiveCount())
            {
                return false;
            }
        }
        return true;
    }

    float EncodedBottomLevelBvh::GetSurfaceAreaHeuristic() const
    {
        return surface_area_heuristic_;
    }

    void EncodedBottomLevelBvh::SetSurfaceAreaHeuristic(float surface_area_heuristic)
    {
        surface_area_heuristic_ = surface_area_heuristic;
    }

    bool EncodedBottomLevelBvh::IsProcedural() const
    {
        return is_procedural_;
    }

    double EncodedBottomLevelBvh::GetLength(const dxr::amd::Float3& vert_1, const dxr::amd::Float3& vert_2) const
    {
        double delta_x   = vert_1.x - vert_2.x;
        double x_squared = delta_x * delta_x;
        double delta_y   = vert_1.y - vert_2.y;
        double y_squared = delta_y * delta_y;
        double delta_z   = vert_1.z - vert_2.z;
        double z_squared = delta_z * delta_z;

        return sqrt(x_squared + y_squared + z_squared);
    }

    float EncodedBottomLevelBvh::TriangleSurfaceArea(const dxr::amd::Float3& v0, const dxr::amd::Float3& v1, const dxr::amd::Float3& v2) const
    {
        // Calculate the surface area of the triangle using Heron's Formula.
        double length_a       = GetLength(v1, v0);
        double length_b       = GetLength(v2, v1);
        double length_c       = GetLength(v0, v2);
        double semi_perimeter = (length_a + length_b + length_c) * 0.5f;
        double area_squared   = semi_perimeter * (semi_perimeter - length_a) * (semi_perimeter - length_b) * (semi_perimeter - length_c);
        if (area_squared < 0.0)
        {
            return 0.0;
        }
        return static_cast<float>(sqrt(area_squared));
    }

    float EncodedBottomLevelBvh::GetTriangleSurfaceArea(const dxr::amd::TriangleNode& triangle_node, uint32_t tri_count) const
    {
        auto  tri0         = triangle_node.GetTriangle(dxr::amd::NodeType::kAmdNodeTriangle0);
        float surface_area = TriangleSurfaceArea(tri0.v0, tri0.v1, tri0.v2);

        if (tri_count == 2)
        {
            auto tri1 = triangle_node.GetTriangle(dxr::amd::NodeType::kAmdNodeTriangle1);
            surface_area += TriangleSurfaceArea(tri1.v0, tri1.v1, tri1.v2);
        }

        return surface_area;
    }

    RraErrorCode EncodedBottomLevelBvh::GetTriangleNodeCount(uint32_t* out_triangle_count) const
    {
        if (header_->GetGeometryType() == rta::BottomLevelBvhGeometryType::kTriangle)
        {
            *out_triangle_count = header_->GetLeafNodeCount();
        }
        else
        {
            *out_triangle_count = 0;
        }
        return kRraOk;
    }

    uint32_t EncodedBottomLevelBvh::GetTriangleCount() const
    {
        const auto& geometry_infos       = GetGeometryInfos();
        uint32_t    total_triangle_count = 0;
        for (auto geom_iter = geometry_infos.begin(); geom_iter != geometry_infos.end(); ++geom_iter)
        {
            total_triangle_count += geom_iter->GetPrimitiveCount();
        }
        return total_triangle_count;
    }

    uint32_t EncodedBottomLevelBvh::GetGeometryTriangleCount(uint32_t geometry_index) const
    {
        const auto& geometry_infos = GetGeometryInfos();
        if (geometry_index >= geometry_infos.size())
        {
            return 0;
        }
        return geometry_infos[geometry_index].GetPrimitiveCount();
    }

    RraErrorCode EncodedBottomLevelBvh::GetProceduralNodeCount(uint32_t* out_procedural_Node_count) const
    {
        if (header_->GetGeometryType() == rta::BottomLevelBvhGeometryType::kAABB)
        {
            *out_procedural_Node_count = header_->GetLeafNodeCount();
        }
        else
        {
            *out_procedural_Node_count = 0;
        }
        return kRraOk;
    }

}  // namespace rta

