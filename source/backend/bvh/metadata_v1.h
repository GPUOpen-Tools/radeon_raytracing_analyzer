//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of the V1 MetaData class.
//=============================================================================

#ifndef RRA_BACKEND_BVH_METADATA_V1_H_
#define RRA_BACKEND_BVH_METADATA_V1_H_

#include "dxr_definitions.h"

namespace dxr
{
    namespace amd
    {
        // Header of the meta data
        class MetaDataV1
        {
        public:
            /// @brief Constructor.
            MetaDataV1() = default;

            /// @brief Destructor.
            ~MetaDataV1() = default;

            /// @brief Set the GPU virtual address of the dump file.
            ///
            /// Should point to the start of the acceleration structure header.
            ///
            /// @param gpu_address The GPU address to set.
            void SetGpuVa(const rta::GpuVirtualAddress gpu_address);

            /// @brief Get the GPU virtual address pointing to the header (AsHeader) of the dump file.
            ///
            /// @return the GPU virtual address.
            rta::GpuVirtualAddress GetGpuVirtualAddress() const;

            /// @brief Obtain the size of the metadata.
            ///
            /// @return The size, in bytes.
            std::uint32_t GetByteSize() const;

        private:
            // GPU Address of the AS split in low and high bits
            std::uint32_t id_low_    = 0;
            std::uint32_t id_high_   = 0;
            std::uint32_t byte_size_ = 0;  // == Size of meta data header == offset to real as header

            std::uint32_t task_counter_   = 0;  // Task counter for dispatch-wide spin loop sync
            std::uint32_t num_tasks_done_ = 0;  // Number of tasks done
            std::uint32_t reserved0_[16] = {};  // Reserved
            std::uint32_t reserved1_[3] = {};  // Reserved - padding for 64-bit alignment
            std::uint32_t reserved2_[3] = {};  // Reserved
            std::uint32_t padding_[5] = {};  // 128-byte alignment padding
            std::uint32_t reserved3_[32] = {};  // BLAS KDOP data
        };

    }  // namespace amd
}  // namespace dxr

#endif  // RRA_BACKEND_BVH_METADATA_V1_H_
