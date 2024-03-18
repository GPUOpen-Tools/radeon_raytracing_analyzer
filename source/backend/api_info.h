//=============================================================================
// Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the API Info class.
//=============================================================================

#ifndef RRA_BACKEND_API_INFO_H_
#define RRA_BACKEND_API_INFO_H_

#include "public/rra_error.h"

#include "rdf/rdf/inc/amdrdf.h"

namespace rra
{
    class ApiInfo
    {
    public:
        // Global identifier of API info in chunk files.
        static constexpr const char* kChunkIdentifier = "ApiInfo";

        /// @brief Constructor.
        ApiInfo();

        /// @brief Destructor.
        ~ApiInfo();

        /// @brief Load the chunk.
        ///
        /// @return kRraOk if loaded correctly, error code if not.
        RraErrorCode LoadChunk(rdf::ChunkFile& chunk_file);

        /// @brief Get the API name.
        ///
        /// @return A pointer to a string containing the API name, or nullptr if invalid.
        const char* GetApiName() const;

        /// @brief Query whether or not the captured application uses Vulkan.
        ///
        /// @return True if the captured application uses Vulkan, false otherwise.
        bool IsVulkan() const;

        /// @brief enum of API types.
        enum class TraceApiType_v1 : uint32_t
        {
            DIRECTX_9  = 0,
            DIRECTX_11 = 1,
            DIRECTX_12 = 2,
            VULKAN     = 3,
            OPENGL     = 4,
            OPENCL     = 5,
            MANTLE     = 6,
            GENERIC    = 7
        };

        enum class TraceApiType : uint32_t
        {
            GENERIC    = 0,
            DIRECTX_9  = 1,
            DIRECTX_11 = 2,
            DIRECTX_12 = 3,
            VULKAN     = 4,
            OPENGL     = 5,
            OPENCL     = 6,
            MANTLE     = 7,
            HIP        = 8,
            METAL      = 9
        };

        /// @brief The API Info chunk data format.
        struct TraceChunkApiInfo
        {
            TraceApiType api_type;           ///< The API type.
            uint16_t     api_version_major;  ///< Major client API version.
            uint16_t     api_version_minor;  ///< Minor client API version.
        };

    private:
        TraceChunkApiInfo chunk_data_ = {};   ///< The chunk data.
        bool              chunk_data_valid_;  ///< Is the API info data valid.
    };

}  // namespace rra

#endif  // RRA_BACKEND_API_INFO_H_
