//=============================================================================
// Copyright (c) 2022-2026 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the BLAS triangles model.
//=============================================================================

#include "models/blas/blas_triangles_model.h"

#include <deque>

#include <QHeaderView>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QTableView>

#include "qt_common/utils/qt_util.h"

#include "public/rra_blas.h"
#include "public/rra_bvh.h"
#include "public/rra_rtip_info.h"
#include "public/rra_tlas.h"

#include "models/blas/blas_triangles_item_model.h"
#include "util/stack_vector.h"

namespace rra
{
    BlasTrianglesModel::BlasTrianglesModel(int32_t num_model_widgets)
        : ModelViewMapper(num_model_widgets)
        , table_model_(nullptr)
        , proxy_model_(nullptr)
    {
    }

    BlasTrianglesModel::~BlasTrianglesModel()
    {
        delete table_model_;
        delete proxy_model_;
    }

    void BlasTrianglesModel::ResetModelValues()
    {
        table_model_->removeRows(0, table_model_->rowCount());
        table_model_->SetRowCount(0);
        SetModelData(kBlasTrianglesBaseAddress, "-");
        SetModelData(kTlasTrianglesBaseAddress, "-");
    }

    bool BlasTrianglesModel::UpdateTable(uint64_t tlas_index, uint64_t blas_index)
    {
        uint64_t tlas_address = 0;
        if (RraTlasGetBaseAddress(tlas_index, &tlas_address) == kRraOk)
        {
            QString address_string = "TLAS base address: 0x" + QString("%1").arg(tlas_address, 0, 16);
            SetModelData(kTlasTrianglesBaseAddress, address_string);
        }

        uint64_t blas_address = 0;
        if (RraBlasGetBaseAddress(blas_index, &blas_address) == kRraOk)
        {
            QString address_string = "BLAS base address: 0x" + QString("%1").arg(blas_address, 0, 16);
            SetModelData(kBlasTrianglesBaseAddress, address_string);
        }

        std::vector<BlasTrianglesStatistics> stats_list;

        {
            uint32_t root_node = UINT32_MAX;
            RRA_BUBBLE_ON_ERROR(RraBvhGetRootNodePtr(&root_node));

            std::deque<std::pair<uint32_t, uint32_t>> traversal_stack;  // Pairs of (node_addr, child_index).
            traversal_stack.push_back({root_node, 0});
            uint32_t global_child_index = UINT32_MAX;

            // Create a temporary stack vector once and reuse it.
            StackVector<VertexPosition, 4> verts{};

            // Traverse the tree and add all triangle nodes to the table.
            while (!traversal_stack.empty())
            {
                BlasTrianglesStatistics stats       = {};
                auto&                   pair        = traversal_stack.back();
                uint32_t                node_addr   = pair.first;
                uint32_t                child_index = pair.second;
                traversal_stack.pop_back();
                ++global_child_index;

                bool is_internal_node = RraBlasHasChildren(blas_index, node_addr);
                if (is_internal_node)
                {
                    uint32_t child_node_count = 0;
                    RRA_BUBBLE_ON_ERROR(RraBlasGetChildNodeCount(blas_index, node_addr, &child_node_count));
                    std::vector<uint32_t> child_nodes(child_node_count);
                    RRA_BUBBLE_ON_ERROR(RraBlasGetChildNodes(blas_index, node_addr, child_nodes.data()));

                    for (uint32_t i = 0; i < child_node_count; i++)
                    {
                        uint32_t child_node = child_nodes[i];
                        traversal_stack.push_back({child_node, i});
                    }
                }
                else if (RraBlasIsTriangleNode(blas_index, node_addr))
                {
                    stats.global_node_id = ((uint64_t)global_child_index << 32) | node_addr;

                    // Gather triangle data.
                    if (RraBlasGetNodeBaseAddress(blas_index, node_addr, &stats.triangle_address) != kRraOk)
                    {
                        continue;
                    }
                    if (RraBvhGetNodeOffset(node_addr, &stats.triangle_offset) != kRraOk)
                    {
                        continue;
                    }
                    if (RraBlasGetGeometryIndex(blas_index, node_addr, child_index, global_child_index, &stats.geometry_index) != kRraOk)
                    {
                        continue;
                    }
                    uint32_t geometry_flags{};
                    if (RraBlasGetGeometryFlags(blas_index, stats.geometry_index, &geometry_flags) != kRraOk)
                    {
                        continue;
                    }
                    stats.geometry_flag_opaque               = geometry_flags & VK_GEOMETRY_OPAQUE_BIT_KHR;
                    stats.geometry_flag_no_duplicate_any_hit = geometry_flags & VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;

                    if (RraBlasGetIsInactive(blas_index, node_addr, child_index, global_child_index, &stats.is_inactive) != kRraOk)
                    {
                        continue;
                    }
                    if (RraBlasGetSurfaceArea(blas_index, node_addr, child_index, global_child_index, &stats.triangle_surface_area) != kRraOk)
                    {
                        continue;
                    }
                    if (RraBlasGetSurfaceAreaHeuristic(blas_index, node_addr, global_child_index, &stats.sah) != kRraOk)
                    {
                        continue;
                    }

                    uint32_t triangle_count{};
                    if (RraBlasGetNodeTriangleCount(blas_index, node_addr, child_index, global_child_index, &triangle_count) != kRraOk)
                    {
                        continue;
                    }
                    stats.triangle_count = triangle_count;

                    if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() <= rta::RayTracingIpLevel::RtIp2_0)
                    {
                        uint32_t vertex_count{};
                        RraBlasGetNodeVertexCount(blas_index, node_addr, child_index, global_child_index, &vertex_count);
                        verts.Resize(vertex_count);

                        if (RraBlasGetNodeVertices(blas_index, node_addr, child_index, global_child_index, verts.Data()) != kRraOk)
                        {
                            continue;
                        }
                        stats.vertex_0 = rra::renderer::float3(verts[0].x, verts[0].y, verts[0].z);
                        stats.vertex_1 = rra::renderer::float3(verts[1].x, verts[1].y, verts[1].z);
                        stats.vertex_2 = rra::renderer::float3(verts[2].x, verts[2].y, verts[2].z);
                    }

                    if (RraBlasGetPrimitiveIndex(blas_index, node_addr, child_index, global_child_index, 0, &stats.primitive_index) != kRraOk)
                    {
                        continue;
                    }

                    // Add this node and 0th index to the table.
                    stats_list.push_back(stats);

                    if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() <= rta::RayTracingIpLevel::RtIp2_0)
                    {
                        // Get the second triangle if node has more than one.
                        if (triangle_count == 2)
                        {
                            if (RraBlasGetPrimitiveIndex(blas_index, node_addr, child_index, global_child_index, 1, &stats.primitive_index) != kRraOk)
                            {
                                continue;
                            }

                            stats.vertex_0 = rra::renderer::float3(verts[1].x, verts[1].y, verts[1].z);
                            stats.vertex_1 = rra::renderer::float3(verts[2].x, verts[2].y, verts[2].z);
                            stats.vertex_2 = rra::renderer::float3(verts[3].x, verts[3].y, verts[3].z);
                            stats_list.push_back(stats);
                        }
                    }
                }
                else
                {
                    RRA_ASSERT(false);  // Unrecognized node type.
                }
            }
        }

        table_model_->SetRowCount(static_cast<int>(stats_list.size()));
        for (auto& stats : stats_list)
        {
            table_model_->AddTriangleStructure(stats);
        }

        proxy_model_->invalidate();
        return !stats_list.empty();
    }

    void BlasTrianglesModel::InitializeTableModel(QTableView* table_view, uint num_rows, uint num_columns)
    {
        if (proxy_model_ != nullptr)
        {
            delete proxy_model_;
            proxy_model_ = nullptr;
        }

        proxy_model_ = new BlasTrianglesProxyModel();
        table_model_ = proxy_model_->InitializeAccelerationStructureTableModels(table_view, num_rows, num_columns);
        table_model_->Initialize(table_view);
    }

    QModelIndex BlasTrianglesModel::FindTriangleIndex(uint64_t triangle_node_id, uint64_t blas_index) const
    {
        RRA_UNUSED(blas_index);
        return proxy_model_->FindModelIndex(triangle_node_id, kBlasTrianglesColumnPadding);
    }

    uint64_t BlasTrianglesModel::GetNodeId(int row) const
    {
        // The padding column is used to return the node id since it's not used for anything else.
        return proxy_model_->GetData(row, rra::kBlasTrianglesColumnPadding);
    }

    void BlasTrianglesModel::SearchTextChanged(const QString& filter)
    {
        proxy_model_->SetSearchFilter(filter);
        proxy_model_->invalidate();
    }

    BlasTrianglesProxyModel* BlasTrianglesModel::GetProxyModel() const
    {
        return proxy_model_;
    }

}  // namespace rra

