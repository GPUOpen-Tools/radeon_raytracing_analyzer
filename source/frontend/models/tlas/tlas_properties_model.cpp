//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the TLAS properties model.
//=============================================================================

#include "models/tlas/tlas_properties_model.h"

#include "public/rra_bvh.h"
#include "public/rra_blas.h"
#include "public/rra_tlas.h"

#include "util/string_util.h"

namespace rra
{
    TlasPropertiesModel::TlasPropertiesModel(int32_t num_model_widgets)
        : ModelViewMapper(num_model_widgets)
    {
    }

    TlasPropertiesModel::~TlasPropertiesModel()
    {
    }

    void TlasPropertiesModel::ResetModelValues()
    {
        SetModelData(kTlasPropertiesBaseAddress, "-");

        SetModelData(kTlasPropertiesNumNodes, "-");
        SetModelData(kTlasPropertiesNumBoxNodes, "-");
        SetModelData(kTlasPropertiesNumBox16Nodes, "-");
        SetModelData(kTlasPropertiesNumBox32Nodes, "-");
        SetModelData(kTlasPropertiesNumInstanceNodes, "-");
        SetModelData(kTlasPropertiesNumBlases, "-");
        SetModelData(kTlasPropertiesNumTriangles, "-");

        SetModelData(kTlasPropertiesBuildFlagAllowUpdate, false);
        SetModelData(kTlasPropertiesBuildFlagAllowCompaction, false);
        SetModelData(kTlasPropertiesBuildFlagLowMemory, false);
        SetModelData(kTlasPropertiesBuildFlagBuildType, "-");

        SetModelData(kTlasPropertiesMemoryTlas, "-");
        SetModelData(kTlasPropertiesMemoryTotal, "-");
    }

    void TlasPropertiesModel::Update(uint64_t tlas_index)
    {
        uint64_t tlas_address = 0;
        if (RraTlasGetBaseAddress(tlas_index, &tlas_address) == kRraOk)
        {
            QString address_string = "TLAS base address: 0x" + QString("%1").arg(tlas_address, 0, 16);
            SetModelData(kTlasPropertiesBaseAddress, address_string);
        }

        uint64_t node_count = 0;
        if (RraTlasGetTotalNodeCount(tlas_index, &node_count) == kRraOk)
        {
            SetModelData(kTlasPropertiesNumNodes, rra::string_util::LocalizedValue(node_count));
        }

        uint64_t box_node_count = 0;
        if (RraTlasGetBoxNodeCount(tlas_index, &box_node_count) == kRraOk)
        {
            SetModelData(kTlasPropertiesNumBoxNodes, rra::string_util::LocalizedValue(box_node_count));
        }

        uint32_t box16_node_count = 0;
        if (RraTlasGetBox16NodeCount(tlas_index, &box16_node_count) == kRraOk)
        {
            SetModelData(kTlasPropertiesNumBox16Nodes, rra::string_util::LocalizedValue(box16_node_count));
        }

        uint32_t box32_node_count = 0;
        if (RraTlasGetBox32NodeCount(tlas_index, &box32_node_count) == kRraOk)
        {
            SetModelData(kTlasPropertiesNumBox32Nodes, rra::string_util::LocalizedValue(box32_node_count));
        }

        uint64_t instance_node_count = 0;
        if (RraTlasGetInstanceNodeCount(tlas_index, &instance_node_count) == kRraOk)
        {
            SetModelData(kTlasPropertiesNumInstanceNodes, rra::string_util::LocalizedValue(instance_node_count));
        }

        uint64_t blas_count = 0;
        if (RraTlasGetBlasCount(tlas_index, &blas_count) == kRraOk)
        {
            SetModelData(kTlasPropertiesNumBlases, rra::string_util::LocalizedValue(blas_count));
        }

        uint64_t triangle_count = 0;
        if (RraTlasGetTotalTriangleCount(tlas_index, &triangle_count) == kRraOk)
        {
            SetModelData(kTlasPropertiesNumTriangles, rra::string_util::LocalizedValue(triangle_count));
        }

        VkBuildAccelerationStructureFlagBitsKHR build_flags;
        if (RraTlasGetBuildFlags(tlas_index, &build_flags) == kRraOk)
        {
            SetModelData(kTlasPropertiesBuildFlagAllowUpdate, build_flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR ? true : false);
            SetModelData(kTlasPropertiesBuildFlagAllowCompaction, build_flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR ? true : false);
            SetModelData(kTlasPropertiesBuildFlagLowMemory, build_flags & VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR ? true : false);
            SetModelData(kTlasPropertiesBuildFlagBuildType, rra::string_util::GetBuildTypeString(build_flags));
        }

        uint32_t tlas_memory = 0;
        if (RraTlasGetSizeInBytes(tlas_index, &tlas_memory) == kRraOk)
        {
            SetModelData(kTlasPropertiesMemoryTlas, rra::string_util::LocalizedValueMemory(static_cast<double>(tlas_memory), false, true ));
        }

        uint64_t total_memory = 0;
        if (RraTlasGetEffectiveSizeInBytes(tlas_index, &total_memory) == kRraOk)
        {
            SetModelData(kTlasPropertiesMemoryTotal, rra::string_util::LocalizedValueMemory(static_cast<double>(total_memory), false, true));
        }
    }

}  // namespace rra
