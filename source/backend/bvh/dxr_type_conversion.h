//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of the DXR type conversion helper functions.
//=============================================================================

#ifndef RRA_BACKEND_BVH_TYPE_CONVERSION_H_
#define RRA_BACKEND_BVH_TYPE_CONVERSION_H_

#include <array>
#include <cstdint>

#include "bvh/gpu_def.h"
#include "bvh/node_types/instance_node.h"

namespace rta
{
    /// @brief Get the build flags in BVH format.
    ///
    /// @param The DXR build flags.
    ///
    /// @return The build flags in BVH format.
    BvhBuildFlags ToBvhBuildFlags(const dxr::amd::AccelerationStructureBuildFlags flags);

    /// @brief Get the build flags in DXR format.
    ///
    /// @param The BVH build flags.
    ///
    /// @return The build flags in DXR format.
    dxr::amd::AccelerationStructureBuildFlags ToDxrBuildFlags(const BvhBuildFlags flags);

    /// @brief Get the BVH type in BVH format.
    ///
    /// @param The DXR BVH type.
    ///
    /// @return The BVH type.
    BvhType ToBvhType(const dxr::amd::AccelerationStructureBvhType type);

    /// @brief Get the BVH type in DXR format.
    ///
    /// @param The BVH type.
    ///
    /// @return The DXR type.
    dxr::amd::AccelerationStructureBvhType ToDxrBvhType(const BvhType type);

    /// @brief Get the triangle compression mode in BVH format.
    ///
    /// @param The DXR triangle compression mode.
    ///
    /// @return The triangle compression mode in BVH format.
    BvhTriangleCompressionMode ToBvhTriangleCompressionMode(const dxr::amd::TriangleCompressionMode mode);

    /// @brief Get the triangle compression mode in DXR format.
    ///
    /// @param The BVH triangle compression mode.
    ///
    /// @return The triangle compression mode in DXR format.
    dxr::amd::TriangleCompressionMode ToDxrTriangleCompressionMode(const BvhTriangleCompressionMode mode);

    /// @brief Get the low precision interior node mode in BVH format.
    ///
    /// @param The DXR low precision interior node mode.
    ///
    /// @return The low precision interior node mode in BVH format.
    BvhLowPrecisionInteriorNodeMode ToBvhLowPrecisionInteriorNodeMode(dxr::amd::BottomLevelFp16Mode mode);

    /// @brief Get the low precision interior node mode in DXR format.
    ///
    /// @param The BVH low precision interior node mode.
    ///
    /// @return The low precision interior node mode in DXR format.
    dxr::amd::BottomLevelFp16Mode ToDxrBottomLevelFp16Mode(const BvhLowPrecisionInteriorNodeMode mode);

    /// @brief Get the bottom level BVH geometry type.
    ///
    /// @param The DXR geometry type.
    ///
    /// @return The bottom level BVH geometry type.
    BottomLevelBvhGeometryType ToBottomLevelBvhGeometryType(const dxr::GeometryType type);

    /// @brief Get the BVH node .
    ///
    /// @param The DXR node type.
    ///
    /// @return The BVH node type.
    BvhNodeType ToBvhNodeType(const dxr::amd::NodeType type);

}  // namespace rta

#endif  // RRA_BACKEND_BVH_TYPE_CONVERSION_H_

