//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the BLAS geometries model.
//=============================================================================

#include "models/blas/blas_geometries_model.h"

#include <deque>

#include <QTableView>
#include <QScrollBar>
#include <QHeaderView>
#include <QSortFilterProxyModel>

#include "qt_common/utils/qt_util.h"
#include "qt_common/utils/scaling_manager.h"

#include "models/blas/blas_geometries_item_model.h"

#include "public/rra_bvh.h"
#include "public/rra_blas.h"
#include "public/rra_tlas.h"

namespace rra
{
    BlasGeometriesModel::BlasGeometriesModel(int32_t num_model_widgets)
        : ModelViewMapper(num_model_widgets)
        , table_model_(nullptr)
        , proxy_model_(nullptr)
    {
    }

    BlasGeometriesModel::~BlasGeometriesModel()
    {
        delete table_model_;
        delete proxy_model_;
    }

    void BlasGeometriesModel::ResetModelValues()
    {
        table_model_->removeRows(0, table_model_->rowCount());
        table_model_->SetRowCount(0);
    }

    bool BlasGeometriesModel::UpdateTable(uint64_t tlas_index, uint64_t blas_index)
    {
        uint64_t tlas_address = 0;
        if (RraTlasGetBaseAddress(tlas_index, &tlas_address) == kRraOk)
        {
            QString address_string = "TLAS base address: 0x" + QString("%1").arg(tlas_address, 0, 16);
            SetModelData(kTlasGeometriesBaseAddress, address_string);
        }

        uint64_t blas_address = 0;
        if (RraBlasGetBaseAddress(blas_index, &blas_address) == kRraOk)
        {
            QString address_string = "BLAS base address: 0x" + QString("%1").arg(blas_address, 0, 16);
            SetModelData(kBlasGeometriesBaseAddress, address_string);
        }

        std::vector<BlasGeometriesStatistics> stats_list;
        BlasGeometriesStatistics              stats = {};

        uint32_t geometry_count = 0;
        RRA_BUBBLE_ON_ERROR(RraBlasGetGeometryCount(blas_index, &geometry_count));

        for (uint32_t geometry_index = 0; geometry_index < geometry_count; ++geometry_index)
        {
            stats.geometry_index = geometry_index;

            uint32_t geometry_flags{};
            if (RraBlasGetGeometryFlags(blas_index, geometry_index, &geometry_flags) != kRraOk)
            {
                continue;
            }
            stats.geometry_flag_opaque               = geometry_flags & VK_GEOMETRY_OPAQUE_BIT_KHR;
            stats.geometry_flag_no_duplicate_any_hit = geometry_flags & VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;

            uint32_t primitive_count;
            if (RraBlasGetGeometryPrimitiveCount(blas_index, geometry_index, &primitive_count) != kRraOk)
            {
                continue;
            }
            stats.primitive_count = primitive_count;

            // Add this node and 0th index to the table.
            stats_list.push_back(stats);
        }

        table_model_->SetRowCount(static_cast<int>(stats_list.size()));
        for (auto& s : stats_list)
        {
            table_model_->AddGeometryStructure(s);
        }

        proxy_model_->invalidate();
        return !stats_list.empty();
    }

    void BlasGeometriesModel::InitializeTableModel(ScaledTableView* table_view, uint num_rows, uint num_columns)
    {
        if (proxy_model_ != nullptr)
        {
            delete proxy_model_;
            proxy_model_ = nullptr;
        }

        proxy_model_ = new BlasGeometriesProxyModel();
        table_model_ = proxy_model_->InitializeAccelerationStructureTableModels(table_view, num_rows, num_columns);
        table_model_->Initialize(table_view);
    }

    QModelIndex BlasGeometriesModel::FindGeometryIndex(uint32_t geometry_index) const
    {
        return proxy_model_->FindModelIndex(geometry_index, kBlasGeometriesColumnGeometryIndex);
    }

    void BlasGeometriesModel::SearchTextChanged(const QString& filter)
    {
        proxy_model_->SetSearchFilter(filter);
        proxy_model_->invalidate();
    }

    BlasGeometriesProxyModel* BlasGeometriesModel::GetProxyModel() const
    {
        return proxy_model_;
    }

}  // namespace rra
