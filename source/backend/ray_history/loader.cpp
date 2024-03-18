//=============================================================================
// Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for RTA ray history loader.
//=============================================================================

#include "loader.h"
#include "counter.h"
#include <rdf/rdf/inc/amdrdf.h>

#include <cassert>

#include "public/rra_assert.h"
#include "bvh/flags_util.h"

namespace GpuRt
{
    struct RayTracingBinaryHeaderBase
    {
        std::uint32_t            identifier;  // Must be GpuRt::RayTracingFileIdentifier
        std::uint32_t            version;     // Binary file format version
        RayTracingBinaryFileType fileType;    // Binary file type
    };

    struct RayTracingBinaryHeaderV0 : RayTracingBinaryHeaderBase
    {
        std::uint32_t headerSize;
    };
}  // namespace GpuRt

namespace
{

    // Compares counter info without comparing partial-trace specific fields:
    // counterRayIdRangeBegin, counterRayIdRangeEnd, lostTokenBytes, rayCounterDataSize
    bool CompareHeaderCounterData(const GpuRt::CounterInfo& a, const GpuRt::CounterInfo& b)
    {
        // compare only trace-identifiying field, ignore trace-specific fields,
        // i.e. fields which vary in partial traces
        return a.dispatchRayDimensionX == b.dispatchRayDimensionX &&          //
               a.dispatchRayDimensionY == b.dispatchRayDimensionY &&          //
               a.dispatchRayDimensionZ == b.dispatchRayDimensionZ &&          //
               a.hitGroupShaderRecordCount == b.hitGroupShaderRecordCount &&  //
               a.missShaderRecordCount == b.missShaderRecordCount &&          //
               a.pipelineShaderCount == b.pipelineShaderCount &&              //
               a.stateObjectHash == b.stateObjectHash &&                      //
               a.counterMode == b.counterMode &&                              //
               a.counterMask == b.counterMask && a.counterStride == b.counterStride;
    }
}  // namespace

namespace rta
{

    ///////////////////////////////////////////////////////////////////////////
    RayTracingBinaryLoader::RayTracingBinaryLoader(const RayTracingCounterLoadFlags flags)
        : flags_(flags)
    {
    }

    void RayTracingBinaryLoader::AddRayHistoryTrace(rdf::Stream& stream)
    {
        auto header = ReadHeader(stream);

        // check for partial trace
        if (!IsFlagSet(flags_, RayTracingCounterLoadFlags::AllowPartialTrace) && header.counterInfo.lostTokenBytes > 0)
        {
            throw RayTracingCounterException("Cannot load partial trace.", RayTracingCounterError::PartialTrace);
        }

        // compare incoming header against first header
        if (!headers_.empty() && !CompareHeaderCounterData(headers_.front().counterInfo, header.counterInfo))
        {
            RRA_ASSERT_FAIL("Header file mismatch");
        }

        const auto remainingFileSize = stream.GetSize() - stream.Tell();
        const auto dataSize          = header.counterInfo.rayCounterDataSize;

        if (remainingFileSize < dataSize)
        {
            RRA_ASSERT_FAIL("Invalid file size. File reports more bytes to read than available in the file.");
        }

        // add header
        headers_.push_back(std::move(header));

        const auto bufferOffset = buffer_.size();
        buffer_.resize(buffer_.size() + dataSize);

        stream.Read(dataSize, buffer_.data() + bufferOffset);
    }

    void RayTracingBinaryLoader::AddRayHistoryTrace(rdf::Stream&& stream)
    {
        AddRayHistoryTrace(stream);
    }

    size_t RayTracingBinaryLoader::GetBufferSize() const
    {
        return buffer_.size();
    }

    const std::byte* RayTracingBinaryLoader::GetBufferData() const
    {
        return buffer_.data();
    }

    std::uint32_t RayTracingBinaryLoader::GetHighestRayId() const
    {
        if (headers_.empty())
        {
            return INVALID_RAY_ID;
        }

        const auto& header = headers_.front();

        return header.counterInfo.dispatchRayDimensionX * header.counterInfo.dispatchRayDimensionY * header.counterInfo.dispatchRayDimensionZ
               // -1 because this is the ID of the last ray, not the ray count!
               - 1;
    }

    RayTracingCounterLoadFlags RayTracingBinaryLoader::GetFlags() const
    {
        return flags_;
    }

    const RayTracingCounterHeaderResult& RayTracingBinaryLoader::GetHeader(const size_t idx) const
    {
        if (idx >= headers_.size())
        {
            throw std::out_of_range("header index out of range.");
        }

        return headers_[idx];
    }

    bool RayTracingBinaryLoader::ContainsRayHistoryTrace(rdf::Stream& stream)
    {
        if ((uint64_t)stream.GetSize() < sizeof(std::uint32_t))
        {
            return false;
        }

        std::uint32_t fileId;
        stream.Seek(0);
        stream.Read(fileId);
        stream.Seek(0);

        if (fileId == GpuRt::RtCounterInfoIdentifier)
        {
            return true;
        }

        if ((fileId == GpuRt::RtBinaryHeaderIdentifier) && ((uint64_t)stream.GetSize() >= sizeof(GpuRt::RayTracingBinaryHeaderBase)))
        {
            GpuRt::RayTracingBinaryHeaderBase header = {};

            stream.Read(header);
            stream.Seek(0);

            return (header.fileType == GpuRt::RayTracingBinaryFileType::RayHistory) || (header.fileType == GpuRt::RayTracingBinaryFileType::TraversalCounter);
        }

        return false;
    }

    bool RayTracingBinaryLoader::ContainsRayHistoryTrace(rdf::Stream&& stream)
    {
        return ContainsRayHistoryTrace(stream);
    }

    RayTracingCounterHeaderResult RayTracingBinaryLoader::ReadHeader(rdf::Stream& stream)
    {
        RayTracingCounterHeaderResult result = {};

        std::uint32_t fileId;
        stream.Seek(0);
        stream.Read(fileId);

        if (fileId == GpuRt::RtCounterInfoIdentifier)
        {
            // Read header without resetting stream (skip fileId)
            if (!stream.Read(result.counterInfo))
            {
                RRA_ASSERT_FAIL("Error while reading header");
            }
        }
        else if (fileId == GpuRt::RtBinaryHeaderIdentifier)
        {
            // Reset stream
            stream.Seek(0);
            // Read header
            GpuRt::RayTracingBinaryHeaderBase baseHeader = {};
            stream.Read(baseHeader);

            if ((baseHeader.fileType != GpuRt::RayTracingBinaryFileType::RayHistory) &&
                (baseHeader.fileType != GpuRt::RayTracingBinaryFileType::TraversalCounter))
            {
                RRA_ASSERT_FAIL("Unsupported binary file type");
            }

            const std::uint16_t           majorVersion = static_cast<std::uint16_t>(baseHeader.version >> 16);
            GpuRt::RayTracingBinaryHeader header       = {};

            if (majorVersion <= 4)
            {
                // seek to end of V0 header
                stream.Seek(sizeof(GpuRt::RayTracingBinaryHeaderV0));

                header.fileType   = baseHeader.fileType;
                header.headerSize = RayTracingBinaryHeaderSizeRayHistory;
                header.identifier = baseHeader.identifier;
                header.version    = baseHeader.version;
            }
            else
            {
                stream.Seek(0);
                if (!stream.Read(header))
                {
                    RRA_ASSERT_FAIL("Error while reading header");
                }
            }

            if (header.fileType == GpuRt::RayTracingBinaryFileType::RayHistory)
            {
                if (header.headerSize != RayTracingBinaryHeaderSizeRayHistory)
                {
                    RRA_ASSERT_FAIL("Unexpected ray history header");
                }
            }
            else if (header.fileType == GpuRt::RayTracingBinaryFileType::TraversalCounter)
            {
                if (header.headerSize != RayTracingBinaryHeaderSizeTraversalCounter)
                {
                    RRA_ASSERT_FAIL("Unexpected traversal counter header");
                }
            }
            else
            {
                RRA_ASSERT_FAIL("Unsupported binary file type");
            }

            if (!stream.Read(result.counterInfo))
            {
                RRA_ASSERT_FAIL("Error while reading counter info");
            }

            if (header.fileType == GpuRt::RayTracingBinaryFileType::RayHistory)
            {
                if (majorVersion > 4)
                {
                    if (!stream.Read(result.traversalFlags))
                    {
                        RRA_ASSERT_FAIL("Error while reading traversal flags");
                    }

                    assert(stream.Tell() == RayTracingBinaryHeaderSizeRayHistory);
                }
            }
            else
            {
                assert(stream.Tell() == RayTracingBinaryHeaderSizeTraversalCounter);
            }
        }
        else
        {
            RRA_ASSERT_FAIL("Invalid ray tracing binary");
        }

        return result;
    }
}  // namespace rta