//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for an instance node class.
//=============================================================================

#include "bvh/node_types/instance_node.h"

namespace dxr
{
    // Check if headers and descriptions are trivially copyable (memcpy support).
    static_assert(std::is_trivially_copyable<dxr::amd::InstanceNode>::value, "DXR::AMD::InstanceNode must be a trivially copyable class.");
    static_assert(std::is_trivially_copyable<dxr::InstanceDesc>::value, "DXRInstanceDesc must be a trivially copyable class.");

    // Check if the size of structs is as expected.
    static_assert(sizeof(amd::FusedInstanceNode) == amd::kFusedInstanceNodeSize, "FusedInstanceNode size does not match the required byte size.");
    static_assert(sizeof(amd::InstanceNode) == amd::kInstanceNodeSize, "InstanceNode does not have the expected byte size.");
    static_assert(sizeof(amd::InstanceExtraData) == amd::kInstanceExtraDataSize, "Size of InstanceExtraData does not match the expected byte size");
    static_assert(sizeof(InstanceDesc) == kInstanceDescSize, "Size of InstanceDesc does not match the expected byte size");

    InstanceDesc::InstanceDesc(const InstanceDescType type)
    {
        // Set transform to identity.
        transform_[0] = transform_[5] = transform_[10] = 1;

        SetBottomLevelBvhAddress(0);
        SetHitGroupAndInstanceFlags(0, InstanceFlags::kFlagNone);

        if (type == InstanceDescType::kRaw)
        {
            SetHardwareInstanceFlags(HardwareInstanceFlags::kAmdFlagNone);
            SetGeometryType(GeometryType::kTriangle);
        }
    }

    void InstanceDesc::SetHardwareInstanceFlags(const HardwareInstanceFlags inst_flags)
    {
        hw_instance_flags = static_cast<std::uint32_t>(inst_flags);
    }

    void InstanceDesc::SetGeometryType(const GeometryType geometry_type)
    {
        if (geometry_type == GeometryType::kTriangle)
        {
            bottom_level_bvh_leaf_node_type = static_cast<std::uint32_t>(InstanceGeometryTypeFlag::kTriangle);
        }
        else
        {
            bottom_level_bvh_leaf_node_type = static_cast<std::uint32_t>(InstanceGeometryTypeFlag::kAABB);
        }
    }

    GeometryType InstanceDesc::GetGeometryType() const
    {
        if (bottom_level_bvh_leaf_node_type == static_cast<std::uint32_t>(InstanceGeometryTypeFlag::kTriangle))
        {
            return GeometryType::kTriangle;
        }
        else
        {
            return GeometryType::kAABB;
        }
    }

    uint64_t InstanceDesc::GetBottomLevelBvhGpuVa(const InstanceDescType type) const
    {
        std::uint64_t blas_index = static_cast<std::uint64_t>(bottom_level_bvh_address_high);
        blas_index               = (blas_index << 32) | static_cast<std::uint64_t>(bottom_level_bvh_address_low_);

        if (type == InstanceDescType::kDecoded)
        {
            return blas_index;
        }
        else
        {
            return blas_index << 3;
        }
    }

    void InstanceDesc::SetBottomLevelBvhGpuVa(const rta::GpuVirtualAddress blas_address, const InstanceDescType type)
    {
        if (type == InstanceDescType::kDecoded)
        {
            bottom_level_bvh_address_low_ = static_cast<std::uint32_t>(blas_address);
            bottom_level_bvh_address_high = blas_address >> 32;
        }
        else
        {
            const auto geometryType    = GetGeometryType();
            const auto hwInstanceFlags = GetHardwareInstanceFlags();

            SetBottomLevelBvhAddress(blas_address);
            SetHardwareInstanceFlags(hwInstanceFlags);
            SetGeometryType(geometryType);
        }
    }

    HardwareInstanceFlags InstanceDesc::GetHardwareInstanceFlags() const
    {
        return static_cast<HardwareInstanceFlags>(hw_instance_flags);
    }

    void InstanceDesc::SetBottomLevelBvhAddress(const rta::GpuVirtualAddress bottom_level_bvh_address)
    {
        const rta::GpuVirtualAddress stored_address = bottom_level_bvh_address >> 3;

        bottom_level_bvh_address_low_ = static_cast<std::uint32_t>(stored_address);
        bottom_level_bvh_address_high = stored_address >> 32;
    }

    void InstanceDesc::SetInstancePtr(const std::uint64_t instance_base_pointer)
    {
        bottom_level_bvh_address_low_            = static_cast<std::uint32_t>(instance_base_pointer);
        bottom_level_bvh_address_and_flags_high_ = instance_base_pointer >> 32;
    }

    void InstanceDesc::SetInstanceIdAndMask(const std::uint32_t id, const std::uint32_t in_mask)
    {
        instance_id = id;
        mask        = in_mask;
    }

    void InstanceDesc::SetHitGroupAndInstanceFlags(const std::uint32_t hit_group, const InstanceFlags flags)
    {
        contribution_to_hit_group = hit_group;
        instance_flags            = static_cast<uint32_t>(flags);
    }

    std::uint32_t InstanceDesc::GetInstanceID() const
    {
        return instance_id;
    }

    std::uint32_t InstanceDesc::GetMask() const
    {
        return mask;
    }

    bool InstanceDesc::IsActive() const
    {
        return GetMask() != 0;
    }

    std::uint32_t InstanceDesc::GetHitGroup() const
    {
        return contribution_to_hit_group;
    }

    InstanceFlags InstanceDesc::GetInstanceFlags() const
    {
        return static_cast<InstanceFlags>(instance_flags);
    }

    uint64_t InstanceDesc::GetInstancePtr() const
    {
        uint64_t addr = static_cast<std::uint64_t>(bottom_level_bvh_address_and_flags_high_);
        addr          = (addr << 32) | bottom_level_bvh_address_low_;
        return addr;
    }

    const Matrix3x4& InstanceDesc::GetTransform() const
    {
        return transform_;
    }

    void InstanceDesc::SetTransform(const Matrix3x4& transform_matrix)
    {
        transform_ = transform_matrix;
    }

    namespace amd
    {
        std::uint32_t InstanceExtraData::GetInstanceIndex() const
        {
            return index_;
        }

        std::uint32_t InstanceExtraData::GetBottomLevelBvhMetaDataSize() const
        {
            return bottom_level_bvh_meta_data_size_;
        }

        const Matrix3x4& InstanceExtraData::GetOriginalInstanceTransform() const
        {
            return original_instance_transform_;
        }

        InstanceDesc& InstanceNode::GetDesc()
        {
            return desc_;
        }

        const InstanceDesc& InstanceNode::GetDesc() const
        {
            return desc_;
        }

        const InstanceExtraData& InstanceNode::GetExtraData() const
        {
            return extra_data_;
        }

        bool InstanceNode::IsInactive() const
        {
            return desc_.GetBottomLevelBvhGpuVa(dxr::InstanceDescType::kDecoded) == 0 || extra_data_.GetBottomLevelBvhMetaDataSize() == 0 ||
                   desc_.GetMask() == 0;
        }

    }  // namespace amd
}  // namespace dxr

