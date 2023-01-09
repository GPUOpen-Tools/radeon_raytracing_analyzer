//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the BLAS properties model.
//=============================================================================

#include "models/blas/blas_properties_model.h"

#include "public/rra_bvh.h"
#include "public/rra_blas.h"
#include "public/rra_tlas.h"

#include "constants.h"
#include "util/string_util.h"
#include "settings/settings.h"

namespace rra
{
    BlasPropertiesModel::BlasPropertiesModel(int32_t num_model_widgets)
        : ModelViewMapper(num_model_widgets)
    {
    }

    BlasPropertiesModel::~BlasPropertiesModel()
    {
    }

    void BlasPropertiesModel::ResetModelValues()
    {
        SetModelData(kBlasPropertiesBaseAddress, "-");

        SetModelData(kBlasPropertiesNumNodes, "-");
        SetModelData(kBlasPropertiesNumBoxNodes, "-");
        SetModelData(kBlasPropertiesNumBox16Nodes, "-");
        SetModelData(kBlasPropertiesNumBox32Nodes, "-");
        SetModelData(kBlasPropertiesNumTriangleNodes, "-");
        SetModelData(kBlasPropertiesNumProceduralNodes, "-");
        SetModelData(kBlasPropertiesNumTriangles, "-");
        SetModelData(kBlasPropertiesNumInstances, "-");

        SetModelData(kBlasPropertiesBuildFlagAllowUpdate, false);
        SetModelData(kBlasPropertiesBuildFlagAllowCompaction, false);
        SetModelData(kBlasPropertiesBuildFlagLowMemory, false);
        SetModelData(kBlasPropertiesBuildFlagBuildType, "-");

        SetModelData(kBlasPropertiesMemory, "-");

        SetModelData(kBlasPropertiesRootSAH, "-");
        SetModelData(kBlasPropertiesMinSAH, "-");
        SetModelData(kBlasPropertiesMeanSAH, "-");
        SetModelData(kBlasPropertiesMaxDepth, "-");
        SetModelData(kBlasPropertiesAvgDepth, "-");
    }

    void BlasPropertiesModel::Update(uint64_t tlas_index, uint64_t blas_index)
    {
        uint64_t tlas_address = 0;
        if (RraBlasGetBaseAddress(blas_index, &tlas_address) == kRraOk)
        {
            QString address_string = "BLAS base address: 0x" + QString("%1").arg(tlas_address, 0, 16);
            SetModelData(kBlasPropertiesBaseAddress, address_string);
        }

        uint64_t node_count = 0;
        if (RraBlasGetTotalNodeCount(blas_index, &node_count) == kRraOk)
        {
            SetModelData(kBlasPropertiesNumNodes, rra::string_util::LocalizedValue(node_count));
        }

        uint64_t box_count = 0;
        if (RraBlasGetBoxNodeCount(blas_index, &box_count) == kRraOk)
        {
            SetModelData(kBlasPropertiesNumBoxNodes, rra::string_util::LocalizedValue(box_count));
        }

        uint32_t box16_count = 0;
        if (RraBlasGetBox16NodeCount(blas_index, &box16_count) == kRraOk)
        {
            SetModelData(kBlasPropertiesNumBox16Nodes, rra::string_util::LocalizedValue(box16_count));
        }

        uint32_t box32_count = 0;
        if (RraBlasGetBox32NodeCount(blas_index, &box32_count) == kRraOk)
        {
            SetModelData(kBlasPropertiesNumBox32Nodes, rra::string_util::LocalizedValue(box32_count));
        }

        uint32_t triangle_node_count = 0;
        if (RraBlasGetTriangleNodeCount(blas_index, &triangle_node_count) == kRraOk)
        {
            SetModelData(kBlasPropertiesNumTriangleNodes, rra::string_util::LocalizedValue(triangle_node_count));
        }

        uint32_t procedural_node_count = 0;
        if (RraBlasGetProceduralNodeCount(blas_index, &procedural_node_count) == kRraOk)
        {
            SetModelData(kBlasPropertiesNumProceduralNodes, rra::string_util::LocalizedValue(procedural_node_count));
        }

        uint32_t triangle_count = 0;
        if (RraBlasGetUniqueTriangleCount(blas_index, &triangle_count) == kRraOk)
        {
            SetModelData(kBlasPropertiesNumTriangles, rra::string_util::LocalizedValue(triangle_count));
        }

        uint64_t instance_count = 0;
        if (RraTlasGetInstanceCount(tlas_index, blas_index, &instance_count) == kRraOk)
        {
            SetModelData(kBlasPropertiesNumInstances, rra::string_util::LocalizedValue(instance_count));
        }

        VkBuildAccelerationStructureFlagBitsKHR build_flags;
        if (RraBlasGetBuildFlags(blas_index, &build_flags) == kRraOk)
        {
            SetModelData(kBlasPropertiesBuildFlagAllowUpdate, build_flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR ? true : false);
            SetModelData(kBlasPropertiesBuildFlagAllowCompaction, build_flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR ? true : false);
            SetModelData(kBlasPropertiesBuildFlagLowMemory, build_flags & VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR ? true : false);
            SetModelData(kBlasPropertiesBuildFlagBuildType, rra::string_util::GetBuildTypeString(build_flags));
        }

        uint32_t memory = 0;
        if (RraBlasGetSizeInBytes(blas_index, &memory) == kRraOk)
        {
            SetModelData(kBlasPropertiesMemory, rra::string_util::LocalizedValueMemory(static_cast<double>(memory), false, true));
        }

        uint32_t root_node = UINT32_MAX;
        if (RraBvhGetRootNodePtr(&root_node) == kRraOk)
        {
            int decimal_precision = rra::Settings::Get().GetDecimalPrecision();

            float root_sah = 0.0f;
            if (RraBlasGetSurfaceAreaHeuristic(blas_index, root_node, &root_sah) == kRraOk)
            {
                SetModelData(kBlasPropertiesRootSAH,
                             QString::number(root_sah, kQtFloatFormat, decimal_precision),
                             QString::number(root_sah, kQtFloatFormat, kQtTooltipFloatPrecision));
            }

            float max_sah = 0.0f;
            if (RraBlasGetMinimumSurfaceAreaHeuristic(blas_index, root_node, false, &max_sah) == kRraOk)
            {
                SetModelData(kBlasPropertiesMinSAH,
                             QString::number(max_sah, kQtFloatFormat, decimal_precision),
                             QString::number(max_sah, kQtFloatFormat, kQtTooltipFloatPrecision));
            }

            float mean_sah = 0.0f;
            if (RraBlasGetAverageSurfaceAreaHeuristic(blas_index, root_node, false, &mean_sah) == kRraOk)
            {
                SetModelData(kBlasPropertiesMeanSAH,
                             QString::number(mean_sah, kQtFloatFormat, decimal_precision),
                             QString::number(mean_sah, kQtFloatFormat, kQtTooltipFloatPrecision));
            }
        }

        uint32_t max_depth = 0;
        if (RraBlasGetMaxTreeDepth(blas_index, &max_depth) == kRraOk)
        {
            SetModelData(kBlasPropertiesMaxDepth, rra::string_util::LocalizedValue(max_depth));
        }

        uint32_t avg_depth = 0;
        if (RraBlasGetAvgTreeDepth(blas_index, &avg_depth) == kRraOk)
        {
            SetModelData(kBlasPropertiesAvgDepth, rra::string_util::LocalizedValue(avg_depth));
        }
    }

}  // namespace rra
