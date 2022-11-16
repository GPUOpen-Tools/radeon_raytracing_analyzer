//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the API Info class.
//=============================================================================

#include "api_info.h"

#include "rdf/rdf/inc/amdrdf.h"

static_assert(sizeof(rra::ApiInfo::TraceChunkApiInfo) == 8, "ApiInfo does not have the expected byte size.");

namespace rra
{
    ApiInfo::ApiInfo()
        : chunk_data_valid_(false)
    {
    }

    ApiInfo::~ApiInfo()
    {
    }

    RraErrorCode ApiInfo::LoadChunk(rdf::ChunkFile& chunk_file)
    {
        const auto identifier = rra::ApiInfo::kChunkIdentifier;

        chunk_data_valid_ = false;
        if (!chunk_file.ContainsChunk(identifier))
        {
            return kRraErrorMalformedData;
        }

        // Load in API info chunk.
        const auto chunk_count = chunk_file.GetChunkCount(identifier);
        if (chunk_count != 1)
        {
            return kRraErrorMalformedData;
        }

        std::uint32_t chunk_version = chunk_file.GetChunkVersion(identifier);
        RRA_UNUSED(chunk_version);

        uint64_t header_size  = chunk_file.GetChunkHeaderSize(identifier);
        uint64_t payload_size = chunk_file.GetChunkDataSize(identifier);

        std::vector<std::uint8_t> header(header_size);

        if (header_size > 0)
        {
            chunk_file.ReadChunkHeaderToBuffer(identifier, header.data());
        }

        if (payload_size > 0)
        {
            chunk_file.ReadChunkDataToBuffer(identifier, &chunk_data_);
        }

        chunk_data_valid_ = true;
        return kRraOk;
    }

    const char* ApiInfo::GetApiName() const
    {
        if (!chunk_data_valid_)
        {
            return nullptr;
        }

        switch (chunk_data_.api_type)
        {
        case TraceApiType::DIRECTX_9:
            return "DirectX 9";
        case TraceApiType::DIRECTX_11:
            return "DirectX 11";
        case TraceApiType::DIRECTX_12:
            return "DirectX 12";
        case TraceApiType::VULKAN:
            return "Vulkan";
        case TraceApiType::OPENGL:
            return "OpenGL";
        case TraceApiType::OPENCL:
            return "OpenCL";
        case TraceApiType::MANTLE:
            return "Mantle";
        case TraceApiType::GENERIC:
            return "Generic";

        default:
            return "";
        }
    }

    bool ApiInfo::IsVulkan() const
    {
        return chunk_data_.api_type == TraceApiType::VULKAN;
    }

}  // namespace rra
