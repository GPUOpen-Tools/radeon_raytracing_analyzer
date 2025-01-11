//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of the DXR type conversion helper functions.
//=============================================================================

#include "dxr_type_conversion.h"

#include "flags_util.h"
#include "dxr_definitions.h"

namespace rta
{
    using DxrBuildFlags = dxr::amd::AccelerationStructureBuildFlags;

    namespace
    {
        /// @brief BVH type map structure.
        struct BvhTypeMap
        {
            BvhType                                bvhType;
            dxr::amd::AccelerationStructureBvhType dxrBvhType;
        } bvhTypeMap[] = {{BvhType::kTopLevel, dxr::amd::AccelerationStructureBvhType::kAmdTopLevel},
                          {BvhType::kBottomLevel, dxr::amd::AccelerationStructureBvhType::kAmdBottomLevel}};

        /// @brief BVH build flag map structure.
        struct BvhBuildFlagMap
        {
            BvhBuildFlags                             buildFlag;
            dxr::amd::AccelerationStructureBuildFlags dxrBuildFlag;
        } buildFlagMap[] = {{BvhBuildFlags::kAllowUpdate, DxrBuildFlags::kAllowUpdate},
                            {BvhBuildFlags::kAllowCompaction, DxrBuildFlags::kAllowCompaction},
                            {BvhBuildFlags::kFastTrace, DxrBuildFlags::kPreferFastTrace},
                            {BvhBuildFlags::kFastBuild, DxrBuildFlags::kPreferFastBuild},
                            {BvhBuildFlags::kPerformUpdate, DxrBuildFlags::kPerformUpdate},
                            {BvhBuildFlags::kMinimizeMemory, DxrBuildFlags::kMinimizeMemory}};

        /// @brief BVH triangle compression mode map structure.
        struct BvhTriangleCompressionModeMap
        {
            BvhTriangleCompressionMode        triangleCompressionMode;
            dxr::amd::TriangleCompressionMode dxrTriangleCompressionMode;
        } triangleCompressionModeMap[] = {
            {BvhTriangleCompressionMode::kNone, dxr::amd::TriangleCompressionMode::kAmdNoTriangleCompression},
            {BvhTriangleCompressionMode::kTwoTriangles, dxr::amd::TriangleCompressionMode::kAmdTwoTriangleCompression},
            {BvhTriangleCompressionMode::kPairTriangles, dxr::amd::TriangleCompressionMode::kAmdPairTriangleCompression},
            {BvhTriangleCompressionMode::kAutomaticNumberOfTriangles, dxr::amd::TriangleCompressionMode::kAmdAutoTriangleCompression}};

        /// @brief BVH FP16 interior node mode map structure.
        struct BvhLowPrecisionInteriorNodeModeMap
        {
            BvhLowPrecisionInteriorNodeMode lowPrecisionInteriorNodeMode;
            dxr::amd::BottomLevelFp16Mode   dxrLowPrecisionInteriorNodeMode;
        } lowPrecisionInteriorNodeModeMap[] = {{BvhLowPrecisionInteriorNodeMode::kNone, dxr::amd::BottomLevelFp16Mode::kNone},
                                               {BvhLowPrecisionInteriorNodeMode::kLeafNodesOnly, dxr::amd::BottomLevelFp16Mode::kLeafNodesOnly},
                                               {BvhLowPrecisionInteriorNodeMode::kMixedWithFp32, dxr::amd::BottomLevelFp16Mode::kMixedWithFp32},
                                               {BvhLowPrecisionInteriorNodeMode::kAll, dxr::amd::BottomLevelFp16Mode::kAll}};

        /// @brief Bottom level BVH geometry type map structure.
        struct BottomLevelBvhGeometryTypeMap
        {
            BottomLevelBvhGeometryType geometryType;
            dxr::GeometryType          dxrGeometryType;
        } bottomLevelBvhGeometryTypeMap[] = {{BottomLevelBvhGeometryType::kTriangle, dxr::GeometryType::kTriangle},
                                             {BottomLevelBvhGeometryType::kAABB, dxr::GeometryType::kAABB}};

    }  // namespace

    BvhBuildFlags ToBvhBuildFlags(const dxr::amd::AccelerationStructureBuildFlags flags)
    {
        BvhBuildFlags buildFlags = BvhBuildFlags::kNone;

        for (const auto& pair : buildFlagMap)
        {
            SetOrClearFlag(buildFlags, pair.buildFlag, IsFlagSet(flags, pair.dxrBuildFlag));
        }

        return buildFlags;
    }

    DxrBuildFlags ToDxrBuildFlags(const BvhBuildFlags flags)
    {
        DxrBuildFlags buildFlags = DxrBuildFlags::kNone;

        for (const auto& pair : buildFlagMap)
        {
            SetOrClearFlag(buildFlags, pair.dxrBuildFlag, IsFlagSet(flags, pair.buildFlag));
        }

        return buildFlags;
    }

    BvhType ToBvhType(const dxr::amd::AccelerationStructureBvhType type)
    {
        for (const auto& pair : bvhTypeMap)
        {
            if (pair.dxrBvhType == type)
            {
                return pair.bvhType;
            }
        }

        return BvhType::kTopLevel;
    }

    dxr::amd::AccelerationStructureBvhType ToDxrBvhType(const BvhType type)
    {
        for (const auto& pair : bvhTypeMap)
        {
            if (pair.bvhType == type)
            {
                return pair.dxrBvhType;
            }
        }

        return dxr::amd::AccelerationStructureBvhType::kAmdTopLevel;
    }

    BvhTriangleCompressionMode ToBvhTriangleCompressionMode(const dxr::amd::TriangleCompressionMode mode)
    {
        for (const auto& pair : triangleCompressionModeMap)
        {
            if (pair.dxrTriangleCompressionMode == mode)
            {
                return pair.triangleCompressionMode;
            }
        }

        return BvhTriangleCompressionMode::kNone;
    }

    dxr::amd::TriangleCompressionMode ToDxrTriangleCompressionMode(const BvhTriangleCompressionMode mode)
    {
        for (const auto& pair : triangleCompressionModeMap)
        {
            if (pair.triangleCompressionMode == mode)
            {
                return pair.dxrTriangleCompressionMode;
            }
        }

        return dxr::amd::TriangleCompressionMode::kAmdNoTriangleCompression;
    }

    BvhLowPrecisionInteriorNodeMode ToBvhLowPrecisionInteriorNodeMode(dxr::amd::BottomLevelFp16Mode mode)
    {
        for (const auto& pair : lowPrecisionInteriorNodeModeMap)
        {
            if (pair.dxrLowPrecisionInteriorNodeMode == mode)
            {
                return pair.lowPrecisionInteriorNodeMode;
            }
        }

        return BvhLowPrecisionInteriorNodeMode::kNone;
    }

    dxr::amd::BottomLevelFp16Mode ToDxrBottomLevelFp16Mode(const BvhLowPrecisionInteriorNodeMode mode)
    {
        for (const auto& pair : lowPrecisionInteriorNodeModeMap)
        {
            if (pair.lowPrecisionInteriorNodeMode == mode)
            {
                return pair.dxrLowPrecisionInteriorNodeMode;
            }
        }

        return dxr::amd::BottomLevelFp16Mode::kNone;
    }

    BottomLevelBvhGeometryType ToBottomLevelBvhGeometryType(const dxr::GeometryType type)
    {
        for (const auto& pair : bottomLevelBvhGeometryTypeMap)
        {
            if (pair.dxrGeometryType == type)
            {
                return pair.geometryType;
            }
        }

        return BottomLevelBvhGeometryType::kTriangle;
    }

    BvhNodeType ToBvhNodeType(const dxr::amd::NodeType type)
    {
        switch (type)
        {
        case dxr::amd::NodeType::kAmdNodeBoxFp32:
            return BvhNodeType::kHighPrecisionInteriorNode;
        case dxr::amd::NodeType::kAmdNodeBoxFp16:
            return BvhNodeType::kLowPrecisionInteriorNode;
        default:
            return BvhNodeType::kLeafNode;
        }
    }

}  // namespace rta
