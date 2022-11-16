//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the acceleration structure build info class.
//=============================================================================

#include "bvh/irt_ip_11_acceleration_structure_post_build_info.h"
#include "bvh/dxr_type_conversion.h"

#include <string.h>  // for memcpy()

namespace rta
{
    IRtIp11AccelerationStructurePostBuildInfo::~IRtIp11AccelerationStructurePostBuildInfo()
    {
    }

    void IRtIp11AccelerationStructurePostBuildInfo::SetBvhType(const BvhType type)
    {
        SetBvhTypeImpl(type);
    }

    BvhType IRtIp11AccelerationStructurePostBuildInfo::GetBvhType() const
    {
        return GetBvhTypeImpl();
    }

    bool IRtIp11AccelerationStructurePostBuildInfo::IsTopLevel() const
    {
        return GetBvhTypeImpl() == BvhType::kTopLevel;
    }

    bool IRtIp11AccelerationStructurePostBuildInfo::IsBottomLevel() const
    {
        return GetBvhTypeImpl() == BvhType::kBottomLevel;
    }

    void IRtIp11AccelerationStructurePostBuildInfo::SetTriangleCompressionMode(const BvhTriangleCompressionMode compression_mode)
    {
        SetTriangleCompressionModeImpl(compression_mode);
    }

    BvhTriangleCompressionMode IRtIp11AccelerationStructurePostBuildInfo::GetTriangleCompressionMode() const
    {
        return GetTriangleCompressionModeImpl();
    }

    void IRtIp11AccelerationStructurePostBuildInfo::SetBottomLevelFp16Mode(const BvhLowPrecisionInteriorNodeMode mode)
    {
        SetBottomLevelFp16ModeImpl(mode);
    }

    BvhLowPrecisionInteriorNodeMode IRtIp11AccelerationStructurePostBuildInfo::GetBottomLevelFp16Mode() const
    {
        return GetBottomLevelFp16ModeImpl();
    }

    void IRtIp11AccelerationStructurePostBuildInfo::SetBuildFlags(const BvhBuildFlags flags)
    {
        SetBuildFlagsImpl(flags);
    }

    BvhBuildFlags IRtIp11AccelerationStructurePostBuildInfo::GetBuildFlags() const
    {
        return GetBuildFlagsImpl();
    }

    bool IRtIp11AccelerationStructurePostBuildInfo::GetRebraiding() const
    {
        return GetRebraidingImpl();
    }

    std::uint32_t IRtIp11AccelerationStructurePostBuildInfo::GetTriangleSplitting() const
    {
        return GetTriangleSplittingImpl();
    }

    void IRtIp11AccelerationStructurePostBuildInfo::LoadFromBuffer(std::size_t size, void* buffer)
    {
        return LoadFromBufferImpl(size, buffer);
    }

    void IRtIp11AccelerationStructurePostBuildInfo::SaveToBuffer(void* buffer) const
    {
        SaveToBufferImpl(buffer);
    }

    bool IRtIp11AccelerationStructurePostBuildInfo::GetFusedInstances() const
    {
        return GetFusedInstancesImpl();
    }

    // RT IP 1.1-specific implementation of build info.
    struct DxrAccelerationStructureBuildInfoDeprecatedVersion1
    {
        std::uint32_t type : 1;                    ///< Type of acceleration structure.
        std::uint32_t builder_type : 1;            ///< Type of bvh builder (CPU / GPU).
        std::uint32_t build_mode : 4;              ///< CPU/GPU build mode based on build type.
        std::uint32_t tri_compression : 2;         ///< Compression mode of triangles (Bottom level only).
        std::uint32_t bottom_level_fp16_mode : 2;  ///< Fp16 mode for bottom level bvhs.
        std::uint32_t reserved : 6;                ///< Unused bits.
        std::uint32_t build_flags : 16;            ///< Build flags of acceleration structure.
    };

    class DxrRtIp11AccelerationStructurePostBuildInfo final : public IRtIp11AccelerationStructurePostBuildInfo
    {
    public:
        DxrRtIp11AccelerationStructurePostBuildInfo() = default;

    private:
        void    SetBvhTypeImpl(BvhType type) override;
        BvhType GetBvhTypeImpl() const override;

        void                       SetTriangleCompressionModeImpl(const BvhTriangleCompressionMode compression_mode) override;
        BvhTriangleCompressionMode GetTriangleCompressionModeImpl() const override;

        void                            SetBottomLevelFp16ModeImpl(const BvhLowPrecisionInteriorNodeMode mode) override;
        BvhLowPrecisionInteriorNodeMode GetBottomLevelFp16ModeImpl() const override;

        void SetBuildFlagsImpl(const BvhBuildFlags flags) override;

        bool GetRebraidingImpl() const override;

        bool GetFusedInstancesImpl() const override;

        BvhBuildFlags GetBuildFlagsImpl() const override;

        std::uint32_t GetTriangleSplittingImpl() const override;

        void LoadFromBufferImpl(std::size_t size, void* buffer) override;

        void SaveToBufferImpl(void* buffer) const override;

    private:
        union
        {
            struct
            {
                std::uint32_t type : 1;                    ///< Type of acceleration structure.
                std::uint32_t builder_type : 1;            ///< Type of bvh builder (cpu / gpu).
                std::uint32_t build_mode : 4;              ///< CPU/GPU build mode based on build type (Linear=0, AC=1, PLOC=2).
                std::uint32_t tri_compression : 3;         ///< BLAS TriangleCompressionMode: None=0, Two=1, Pair=2.
                std::uint32_t bottom_level_fp16_mode : 2;  ///< BLAS FP16 box mode: None=0, Leaf=1, Mixed=2, All=3.
                std::uint32_t triangle_splitting : 1;      ///< Enable TriangleSplitting.
                std::uint32_t rebraid : 1;                 ///< Enable Rebraid.
                std::uint32_t fused_instance_node : 1;     ///< Enable fused instance nodes.
                std::uint32_t reserved : 2;                ///< Unused bits.
                std::uint32_t build_flags : 16;            ///< Build flags of acceleration structure.
            };
            std::uint32_t info_ = {};
        };
    };

    void DxrRtIp11AccelerationStructurePostBuildInfo::SetBvhTypeImpl(const BvhType bvh_type)
    {
        type = static_cast<std::uint32_t>(ToDxrBvhType(bvh_type));
    }

    BvhType DxrRtIp11AccelerationStructurePostBuildInfo::GetBvhTypeImpl() const
    {
        return ToBvhType(static_cast<dxr::amd::AccelerationStructureBvhType>(type));
    }

    void DxrRtIp11AccelerationStructurePostBuildInfo::SetTriangleCompressionModeImpl(const BvhTriangleCompressionMode compression_mode)
    {
        tri_compression = static_cast<std::uint32_t>(ToDxrTriangleCompressionMode(compression_mode));
    }

    BvhTriangleCompressionMode DxrRtIp11AccelerationStructurePostBuildInfo::GetTriangleCompressionModeImpl() const
    {
        return ToBvhTriangleCompressionMode(static_cast<dxr::amd::TriangleCompressionMode>(tri_compression));
    }

    void DxrRtIp11AccelerationStructurePostBuildInfo::SetBottomLevelFp16ModeImpl(const BvhLowPrecisionInteriorNodeMode mode)
    {
        bottom_level_fp16_mode = static_cast<std::uint32_t>(ToDxrBottomLevelFp16Mode(mode));
    }

    BvhLowPrecisionInteriorNodeMode DxrRtIp11AccelerationStructurePostBuildInfo::GetBottomLevelFp16ModeImpl() const
    {
        return ToBvhLowPrecisionInteriorNodeMode(static_cast<dxr::amd::BottomLevelFp16Mode>(bottom_level_fp16_mode));
    }

    void DxrRtIp11AccelerationStructurePostBuildInfo::SetBuildFlagsImpl(const BvhBuildFlags flags)
    {
        build_flags = static_cast<std::uint32_t>(ToDxrBuildFlags(flags));
    }

    bool DxrRtIp11AccelerationStructurePostBuildInfo::GetRebraidingImpl() const
    {
        return static_cast<bool>(rebraid);
    }

    bool DxrRtIp11AccelerationStructurePostBuildInfo::GetFusedInstancesImpl() const
    {
        return static_cast<bool>(fused_instance_node);
    }

    BvhBuildFlags DxrRtIp11AccelerationStructurePostBuildInfo::GetBuildFlagsImpl() const
    {
        return ToBvhBuildFlags(static_cast<dxr::amd::AccelerationStructureBuildFlags>(build_flags));
    }

    std::uint32_t DxrRtIp11AccelerationStructurePostBuildInfo::GetTriangleSplittingImpl() const
    {
        return triangle_splitting;
    }

    void DxrRtIp11AccelerationStructurePostBuildInfo::LoadFromBufferImpl(std::size_t size, void* buffer)
    {
        RRA_UNUSED(size);
        assert(size == sizeof(std::uint32_t));
        memcpy(&info_, buffer, sizeof(info_));

        DxrAccelerationStructureBuildInfoDeprecatedVersion1 deprecated_build_info = {};
        memcpy(&deprecated_build_info, &info_, sizeof(deprecated_build_info));

        SetBvhType(ToBvhType(static_cast<dxr::amd::AccelerationStructureBvhType>(deprecated_build_info.type)));
        SetTriangleCompressionMode(ToBvhTriangleCompressionMode(static_cast<dxr::amd::TriangleCompressionMode>(deprecated_build_info.tri_compression)));
        SetBottomLevelFp16Mode(ToBvhLowPrecisionInteriorNodeMode(static_cast<dxr::amd::BottomLevelFp16Mode>(deprecated_build_info.bottom_level_fp16_mode)));
        SetBuildFlags(ToBvhBuildFlags(static_cast<dxr::amd::AccelerationStructureBuildFlags>(deprecated_build_info.build_flags)));
    }

    void DxrRtIp11AccelerationStructurePostBuildInfo::SaveToBufferImpl(void* buffer) const
    {
        memcpy(buffer, &info_, sizeof(info_));
    }

    static_assert(sizeof(DxrAccelerationStructureBuildInfoDeprecatedVersion1) == sizeof(std::uint32_t),
                  "DxrAccelerationStructureBuildInfoDeprecatedVersion1 does not match the expected "
                  "size of uint32_t");

    std::unique_ptr<IRtIp11AccelerationStructurePostBuildInfo> CreateRtIp11AccelerationStructurePostBuildInfo()
    {
        return std::make_unique<DxrRtIp11AccelerationStructurePostBuildInfo>();
    }

}  // namespace rta
