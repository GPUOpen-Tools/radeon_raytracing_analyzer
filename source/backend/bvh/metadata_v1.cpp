//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the V1 MetaData class.
//=============================================================================

#include "metadata_v1.h"

namespace dxr
{
    // Check if headers and descriptions are trivially copyable (memcpy support)
    static_assert(std::is_trivially_copyable<dxr::amd::MetaDataV1>::value, "DXR::AMD::MetaData must be a trivially copyable class.");

    // Check if the size of structs is as expected.
    static_assert(sizeof(rta::GpuVirtualAddress) == sizeof(std::uint64_t), "GpuVirtualAddress does not have the expected byte size");
    static_assert(sizeof(amd::MetaDataV1) == amd::kMetaDataV1Size, "Metadata does not have the expected byte size.");

}  // namespace dxr

namespace dxr
{
    namespace amd
    {
        void MetaDataV1::SetGpuVa(const rta::GpuVirtualAddress gpu_address)
        {
            std::uint32_t high_bits = gpu_address >> 32;
            std::uint32_t low_bits  = static_cast<uint32_t>(gpu_address);

            id_low_  = low_bits;
            id_high_ = high_bits;
        }

        rta::GpuVirtualAddress MetaDataV1::GetGpuVirtualAddress() const
        {
            rta::GpuVirtualAddress gpu_va = static_cast<std::uint64_t>(id_high_);
            gpu_va                        = (gpu_va << 32) | static_cast<std::uint64_t>(id_low_);

            return gpu_va;
        }

        std::uint32_t MetaDataV1::GetByteSize() const
        {
            return byte_size_;
        }

    }  // namespace amd
}  // namespace dxr
