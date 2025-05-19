//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the ASIC Info class.
//=============================================================================

#include "asic_info.h"

#include "rdf/rdf/inc/amdrdf.h"

static_assert(sizeof(rra::AsicInfo::TraceChunkAsicInfo) == 608, "AsicInfo does not have the expected byte size.");

namespace rra
{
    AsicInfo::AsicInfo()
        : chunk_data_valid_(false)
    {
    }

    AsicInfo::~AsicInfo()
    {
    }

    RraErrorCode AsicInfo::LoadChunk(rdf::ChunkFile& chunk_file)
    {
        const auto identifier = rra::AsicInfo::kChunkIdentifier;

        chunk_data_valid_ = false;
        if (!chunk_file.ContainsChunk(identifier))
        {
            return kRraErrorMalformedData;
        }

        // Load in ASIC info chunks.
        const auto chunk_count = chunk_file.GetChunkCount(identifier);
        if (chunk_count > 0)
        {
            //  For now, just load in the first chunk seen.
            auto chunk_index = 0;

            std::uint32_t chunk_version = chunk_file.GetChunkVersion(identifier, chunk_index);

            uint64_t header_size  = chunk_file.GetChunkHeaderSize(identifier, chunk_index);
            uint64_t payload_size = chunk_file.GetChunkDataSize(identifier, chunk_index);

            std::vector<std::uint8_t> header(header_size);

            if (header_size > 0)
            {
                chunk_file.ReadChunkHeaderToBuffer(identifier, header.data());
            }

            if (payload_size > 0 && payload_size <= sizeof(TraceChunkAsicInfo))
            {
                if (chunk_version == 1)
                {
                    // copy old data to new chunk data format.
                    chunk_file.ReadChunkDataToBuffer(identifier, chunk_index, &chunk_data_.shader_core_clock_frequency);
                    chunk_data_.pci_id = 0;
                }
                else if (chunk_version == 2)
                {
                    memset(&chunk_data_, 0, sizeof(TraceChunkAsicInfo));
                    chunk_file.ReadChunkDataToBuffer(identifier, chunk_index, &chunk_data_);
                }
                else
                {
                    chunk_file.ReadChunkDataToBuffer(identifier, chunk_index, &chunk_data_);
                }

                chunk_data_valid_ = true;
                return kRraOk;
            }
            else
            {
                return kRraMajorVersionIncompatible;
            }
        }

        return kRraErrorMalformedData;
    }

    const char* AsicInfo::GetDeviceName() const
    {
        if (!chunk_data_valid_)
        {
            return nullptr;
        }
        return chunk_data_.gpu_name;
    }

    RraErrorCode AsicInfo::GetDeviceID(int32_t* device_id) const
    {
        if (!chunk_data_valid_)
        {
            return kRraErrorMalformedData;
        }
        *device_id = chunk_data_.device_id;
        return kRraOk;
    }

    RraErrorCode AsicInfo::GetDeviceRevisionID(int32_t* device_revision_id) const
    {
        if (!chunk_data_valid_)
        {
            return kRraErrorMalformedData;
        }
        *device_revision_id = chunk_data_.device_revision_id;
        return kRraOk;
    }

    RraErrorCode AsicInfo::GetPciID(uint32_t* pci_id) const
    {
        if (!chunk_data_valid_)
        {
            return kRraErrorMalformedData;
        }
        *pci_id = chunk_data_.pci_id;
        return kRraOk;
    }

    RraErrorCode AsicInfo::GetShaderCoreClockFrequency(uint64_t* out_clk_frequency) const
    {
        if (!chunk_data_valid_)
        {
            return kRraErrorMalformedData;
        }
        *out_clk_frequency = chunk_data_.shader_core_clock_frequency;
        return kRraOk;
    }

    RraErrorCode AsicInfo::GetMaxShaderCoreClockFrequency(uint64_t* out_clk_frequency) const
    {
        if (!chunk_data_valid_)
        {
            return kRraErrorMalformedData;
        }
        *out_clk_frequency = chunk_data_.max_shader_core_clock;
        return kRraOk;
    }

    RraErrorCode AsicInfo::GetVRAMSize(int64_t* out_vram_size) const
    {
        if (!chunk_data_valid_)
        {
            return kRraErrorMalformedData;
        }
        *out_vram_size = chunk_data_.vram_size;
        return kRraOk;
    }

    RraErrorCode AsicInfo::GetMemoryClockFrequency(uint64_t* out_clk_frequency) const
    {
        if (!chunk_data_valid_)
        {
            return kRraErrorMalformedData;
        }
        *out_clk_frequency = chunk_data_.memory_clock_frequency;
        return kRraOk;
    }

    RraErrorCode AsicInfo::GetMaxMemoryClockFrequency(uint64_t* out_clk_frequency) const
    {
        if (!chunk_data_valid_)
        {
            return kRraErrorMalformedData;
        }
        *out_clk_frequency = chunk_data_.max_memory_clock;
        return kRraOk;
    }

    RraErrorCode AsicInfo::GetVideoMemoryBandwidth(uint64_t* out_memory_bandwidth) const
    {
        if (!chunk_data_valid_)
        {
            return kRraErrorMalformedData;
        }
        uint64_t result = chunk_data_.max_memory_clock;
        result *= chunk_data_.memory_ops_per_clock;
        result *= (chunk_data_.vram_bus_width / 8);
        *out_memory_bandwidth = result;

        return kRraOk;
    }

    const char* AsicInfo::GetVideoMemoryType() const
    {
        if (!chunk_data_valid_)
        {
            return nullptr;
        }

        switch (chunk_data_.memory_chip_type)
        {
        case TraceMemoryType::Unknown:
            return "Unknown";
        case TraceMemoryType::Ddr:
            return "DDR";
        case TraceMemoryType::Ddr2:
            return "DDR2";
        case TraceMemoryType::Ddr3:
            return "DDR3";
        case TraceMemoryType::Ddr4:
            return "DDR4";
        case TraceMemoryType::Ddr5:
            return "DDR5";
        case TraceMemoryType::Gddr3:
            return "GDDR3";
        case TraceMemoryType::Gddr4:
            return "GDDR4";
        case TraceMemoryType::Gddr5:
            return "GDDR5";
        case TraceMemoryType::Gddr6:
            return "GDDR6";
        case TraceMemoryType::Hbm:
            return "HBM";
        case TraceMemoryType::Hbm2:
            return "HBM2";
        case TraceMemoryType::Hbm3:
            return "HBM3";
        case TraceMemoryType::Lpddr4:
            return "LPDDR4";
        case TraceMemoryType::Lpddr5:
            return "LPDDR5";

        default:
            return "";
        }
    }

    RraErrorCode AsicInfo::GetVideoMemoryBusWidth(int32_t* out_bus_width) const
    {
        if (!chunk_data_valid_)
        {
            return kRraErrorMalformedData;
        }
        *out_bus_width = chunk_data_.vram_bus_width;
        return kRraOk;
    }

    RraErrorCode AsicInfo::GetGfxIpLevelMajor(uint16_t* out_gfx_ip_level)
    {
        if (!chunk_data_valid_)
        {
            return kRraErrorMalformedData;
        }
        *out_gfx_ip_level = chunk_data_.gfx_ip_level.major;
        return kRraOk;
    }

}  // namespace rra

