//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
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

        uint64_t header_size  = chunk_file.GetChunkHeaderSize(identifier);
        uint64_t payload_size = chunk_file.GetChunkDataSize(identifier);

        std::vector<std::uint8_t> header(header_size);

        if (header_size > 0)
        {
            chunk_file.ReadChunkHeaderToBuffer(identifier, header.data());
        }

        if (payload_size > 0 && payload_size <= sizeof(TraceChunkApiInfo))
        {
            chunk_file.ReadChunkDataToBuffer(identifier, &chunk_data_);

            if (chunk_version == 1)
            {
                std::unordered_map<TraceApiType_v1, TraceApiType> api_map = {{TraceApiType_v1::DIRECTX_9, TraceApiType::DIRECTX_9},
                                                                             {TraceApiType_v1::DIRECTX_11, TraceApiType::DIRECTX_11},
                                                                             {TraceApiType_v1::DIRECTX_12, TraceApiType::DIRECTX_12},
                                                                             {TraceApiType_v1::VULKAN, TraceApiType::VULKAN},
                                                                             {TraceApiType_v1::OPENGL, TraceApiType::OPENGL},
                                                                             {TraceApiType_v1::OPENCL, TraceApiType::OPENCL},
                                                                             {TraceApiType_v1::MANTLE, TraceApiType::MANTLE},
                                                                             {TraceApiType_v1::GENERIC, TraceApiType::GENERIC}};

                // Map old API enum values to new ones.
                const auto& it = api_map.find(static_cast<TraceApiType_v1>(chunk_data_.api_type));
                if (it != api_map.end())
                {
                    chunk_data_.api_type = (*it).second;
                }
            }

            chunk_data_valid_ = true;
            return kRraOk;
        }
        else
        {
            return kRraMajorVersionIncompatible;
        }
        return kRraErrorMalformedData;
    }

    const char* ApiInfo::GetApiName() const
    {
        if (!chunk_data_valid_)
        {
            return nullptr;
        }

        switch (chunk_data_.api_type)
        {
        case TraceApiType::GENERIC:
            return "Generic";
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
        case TraceApiType::HIP:
            return "HIP";
        case TraceApiType::METAL:
            return "METAL";

        default:
            return "";
        }
    }

    bool ApiInfo::IsVulkan() const
    {
        return chunk_data_.api_type == TraceApiType::VULKAN;
    }

}  // namespace rra
