//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for RTA ray history.
//=============================================================================

#include "ray_history.h"
#include "loader.h"
#include "../bvh/flags_util.h"

#include "public/rra_assert.h"

#include <cassert>
#include <cstdio>
#include <cstring>

#include <limits>
#include <algorithm>
#include <stack>

#include <map>

namespace
{
    struct MetadataStore
    {
    public:
        static constexpr const char* ChunkIdentifier = "HistoryMetadata";

        void AddMetadata(const rta::RayHistoryMetadataKind kind, const std::size_t size, const std::byte* buffer)
        {
            metadata_[kind] = std::vector<std::byte>(buffer, buffer + size);
        }

        void RemoveMetadata(const rta::RayHistoryMetadataKind kind)
        {
            metadata_.erase(kind);
        }

        bool HasMetadata(const rta::RayHistoryMetadataKind kind) const
        {
            return metadata_.find(kind) != metadata_.end();
        }

        size_t GetMetadataSize(const rta::RayHistoryMetadataKind kind) const
        {
            return metadata_.find(kind)->second.size();
        }

        void GetMetadata(const rta::RayHistoryMetadataKind kind, std::byte* buffer) const
        {
            auto& data = metadata_.find(kind)->second;
            ::memcpy(buffer, data.data(), data.size());
        }

        std::vector<rta::RayHistoryMetadataKind> GetMetadataKeys() const
        {
            std::vector<rta::RayHistoryMetadataKind> result;
            for (const auto& kv : metadata_)
            {
                result.push_back(kv.first);
            }
            return result;
        }

        void SaveToFile(rdf::ChunkFileWriter& cfw, rdfCompression compression, const bool includeEmptyMetadata) const
        {
            if (const auto metadataKeys = GetMetadataKeys(); !metadataKeys.empty() || includeEmptyMetadata)
            {
                cfw.BeginChunk(ChunkIdentifier, 0, nullptr, compression);

                std::vector<std::byte> buffer;
                for (const auto& key : metadataKeys)
                {
                    buffer.resize(GetMetadataSize(key));
                    GetMetadata(key, buffer.data());

                    cfw.AppendToChunk(key);
                    // We enforce writing a 64-bit value to define the buffer size.
                    cfw.AppendToChunk(static_cast<std::uint64_t>(buffer.size()));
                    cfw.AppendToChunk(buffer.size(), buffer.data());
                }

                cfw.EndChunk();
            }
        }

        void LoadFromFile(rdf::ChunkFile& cf, const int chunkIndex)
        {
            if (!cf.ContainsChunk(ChunkIdentifier, chunkIndex) || cf.GetChunkDataSize(ChunkIdentifier, chunkIndex) == 0)
            {
                return;
            }

            cf.ReadChunkData(ChunkIdentifier, chunkIndex, [&](auto size, auto data) {
                auto                   stream = rdf::Stream::FromReadOnlyMemory(size, data);
                std::vector<std::byte> buffer;

                for (;;)
                {
                    std::uint32_t key;
                    if (!stream.Read(key))
                    {
                        break;
                    }

                    if (!stream.Read(size))
                    {
                        break;
                    }

                    buffer.resize(size);
                    if (!stream.Read(size, buffer.data()))
                    {
                        break;
                    }

                    metadata_[static_cast<rta::RayHistoryMetadataKind>(key)] = buffer;
                }
            });
        }

    private:
        std::unordered_map<rta::RayHistoryMetadataKind, std::vector<std::byte>> metadata_;
    };
}  // namespace

namespace rta
{
    class DxrRayTracingCounterInfo::Impl
    {
    public:
        GpuRt::CounterInfo info;
    };

    ///////////////////////////////////////////////////////////////////////////
    DxrRayTracingCounterInfo::DxrRayTracingCounterInfo(const std::size_t size, const void* buffer)
    {
        if (size != sizeof(impl_->info))
        {
            RRA_ASSERT_FAIL("Invalid metadata size");
        }

        impl_ = std::make_unique<Impl>();
        ::memcpy(&impl_->info, buffer, sizeof(impl_->info));
    }

    ///////////////////////////////////////////////////////////////////////////
    DxrRayTracingCounterInfo::~DxrRayTracingCounterInfo()
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    void DxrRayTracingCounterInfo::GetDispatchDimensions(std::uint32_t dimensions[3]) const
    {
        dimensions[0] = impl_->info.dispatchRayDimensionX;
        dimensions[1] = impl_->info.dispatchRayDimensionY;
        dimensions[2] = impl_->info.dispatchRayDimensionZ;
    }

    ///////////////////////////////////////////////////////////////////////////
    std::uint32_t DxrRayTracingCounterInfo::GetHitGroupShaderRecordCount() const
    {
        return impl_->info.hitGroupShaderRecordCount;
    }

    ///////////////////////////////////////////////////////////////////////////
    std::uint32_t DxrRayTracingCounterInfo::GetMissShaderRecordCount() const
    {
        return impl_->info.missShaderRecordCount;
    }

    ///////////////////////////////////////////////////////////////////////////
    std::uint32_t DxrRayTracingCounterInfo::GetPipelineShaderCount()
    {
        return impl_->info.pipelineShaderCount;
    }

    ///////////////////////////////////////////////////////////////////////////
    std::uint64_t DxrRayTracingCounterInfo::GetStateObjectHash() const
    {
        return impl_->info.stateObjectHash;
    }

    ///////////////////////////////////////////////////////////////////////////
    std::uint32_t DxrRayTracingCounterInfo::GetCounterMode() const
    {
        return impl_->info.counterMode;
    }

    ///////////////////////////////////////////////////////////////////////////
    std::uint32_t DxrRayTracingCounterInfo::GetCounterMask() const
    {
        return impl_->info.counterMask;
    }

    ///////////////////////////////////////////////////////////////////////////
    std::uint32_t DxrRayTracingCounterInfo::GetCounterStride() const
    {
        return impl_->info.counterStride;
    }

    ///////////////////////////////////////////////////////////////////////////
    std::uint32_t DxrRayTracingCounterInfo::GetRayCounterDataSize() const
    {
        return impl_->info.rayCounterDataSize;
    }

    ///////////////////////////////////////////////////////////////////////////
    std::uint32_t DxrRayTracingCounterInfo::GetLostTokenBytes() const
    {
        return impl_->info.lostTokenBytes;
    }

    ///////////////////////////////////////////////////////////////////////////

    std::string ToString(const RayHistoryTokenType tokenType)
    {
        switch (tokenType)
        {
        case RayHistoryTokenType::Begin:
            return "begin";
        case RayHistoryTokenType::TopLevel:
            return "top-level";
        case RayHistoryTokenType::BottomLevel:
            return "bottom-level";
        case RayHistoryTokenType::End:
            return "end";
        case RayHistoryTokenType::FunctionCall:
            return "function-call";
        case RayHistoryTokenType::Timestamp:
            return "timestamp";
        case RayHistoryTokenType::AnyHitStatus:
            return "any-hit-status";
        case RayHistoryTokenType::FunctionCallV2:
            return "function-call-v2";
        case RayHistoryTokenType::ProceduralIntersectionStatus:
            return "procedural-intersection-status";
        case RayHistoryTokenType::EndV2:
            return "end-v2";
        case RayHistoryTokenType::BeginV2:
            return "begin-v2";
        case RayHistoryTokenType::Unknown:
            return "unknown (nodeptr)";
        default:
            RRA_ASSERT_FAIL("unknown RayHistoryTokenType");
            return "";
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    const char* ToString(const RayHistoryFunctionCallType type)
    {
        switch (type)
        {
        case rta::RayHistoryFunctionCallType::Reserved:
            return "reserved";
        case rta::RayHistoryFunctionCallType::Miss:
            return "miss";
        case rta::RayHistoryFunctionCallType::Closest:
            return "closest";
        case rta::RayHistoryFunctionCallType::AnyHit:
            return "any-hit";
        case rta::RayHistoryFunctionCallType::Intersection:
            return "intersection";
        case rta::RayHistoryFunctionCallType::Unknown:
        default:
            return "unknown";
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    const char* ToString(const RayHistoryAnyHitStatus status)
    {
        switch (status)
        {
        case rta::RayHistoryAnyHitStatus::IgnoreHit:
            return "ignore-hit";
        case rta::RayHistoryAnyHitStatus::AcceptHit:
            return "accept-hit";
        case rta::RayHistoryAnyHitStatus::AcceptHitAndEndSearch:
            return "accept-hit-and-end-search";
        default:
            return "unknown";
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    const char* ToString(const NodeType nodeType)
    {
        switch (nodeType)
        {
        case NodeType::BOX_FLOAT16:
            return "box_float16";
        case NodeType::BOX_FLOAT32:
            return "box_float32";
        case NodeType::TRIANGLE_0:
            return "triangle_0";
        case NodeType::TRIANGLE_1:
            return "triangle_1";
        case NodeType::TRIANGLE_2:
            return "triangle_2";
        case NodeType::TRIANGLE_3:
            return "triangle_3";
        case NodeType::USER_NODE_INSTANCE:
            return "user_instance";
        case NodeType::USER_NODE_PROCEDURAL:
            return "user_procedural";
        default:
            return "undefined";
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    TileSize TileSize::CreateInvalid()
    {
        return {UINT32_MAX, UINT32_MAX};
    }

    ///////////////////////////////////////////////////////////////////////////
    bool TileSize::IsInvalid() const
    {
        return (width == UINT32_MAX || height == UINT32_MAX);
    }

    namespace io
    {
        struct Token
        {
            std::uint32_t rayId, data0, data1;
        };
    }  // namespace io

    ///////////////////////////////////////////////////////////////////////////
    struct RayHistoryTrace::Impl
    {
    public:
        static constexpr const char* TokenChunkIdentifier    = "HistoryTokens";
        static constexpr const char* IndexChunkIdentifier    = "HistoryIndex";
        static constexpr const char* RayIndexChunkIdentifier = "HistoryRayIndex";

        Impl(std::vector<std::byte>&& tokens, std::vector<TokenIndex>&& tokenIndices, std::vector<RayRange>&& rayRanges, const std::uint32_t highestRayId)
            : tokenData_(std::move(tokens))
            , tokenIndices_(std::move(tokenIndices))
            , rayRanges_(std::move(rayRanges))
            , lastRayId_(highestRayId)
        {
            CreateRayIndex();
        }

        Impl() = default;

        int GetRayCount() const
        {
            return (int)rayRanges_.size();
        }

        int GetHighestRayId() const
        {
            return static_cast<int>(lastRayId_);
        }

        RayHistory GetRayByIndex(const int index) const
        {
            const auto& range = rayRanges_[index];
            return RayHistory(tokenData_.data() + range.dataStart, tokenIndices_.data() + range.tokenStart, range.tokenCount, range.rayId);
        }

        RayHistory GetRayById(const int index) const
        {
            if (const auto it = rayIndices_.find(index); it != rayIndices_.end())
            {
                const auto& range = rayRanges_[it->second];

                return RayHistory(tokenData_.data() + range.dataStart, tokenIndices_.data() + range.tokenStart, range.tokenCount, range.rayId);
            }
            else
            {
                return RayHistory();
            }
        }

        MetadataStore& GetMetadataStore()
        {
            return metadataStore_;
        }

        void SaveToFile(rdf::Stream& stream, const rdfCompression compression, const bool includeEmptyMetadata) const
        {
            rdf::ChunkFileWriter cfw(stream);

            SaveToFile(cfw, compression, includeEmptyMetadata);

            cfw.Close();
        }

        void SaveToFile(rdf::ChunkFileWriter& cfw, const rdfCompression compression, const bool includeEmptyMetadata) const
        {
            cfw.BeginChunk(TokenChunkIdentifier, 0, nullptr, compression, 2);
            cfw.AppendToChunk(tokenData_.size(), tokenData_.data());
            cfw.EndChunk();

            cfw.BeginChunk(IndexChunkIdentifier, 0, nullptr, compression, 2);
            cfw.AppendToChunk(sizeof(RayHistoryTrace::TokenIndex) * tokenIndices_.size(), tokenIndices_.data());
            cfw.EndChunk();

            cfw.BeginChunk(RayIndexChunkIdentifier, 0, nullptr, compression, 2);
            cfw.AppendToChunk(sizeof(RayHistoryTrace::RayRange) * rayRanges_.size(), rayRanges_.data());
            cfw.EndChunk();

            metadataStore_.SaveToFile(cfw, compression, includeEmptyMetadata);
        }

        /**
     * @brief Checks if all chunks for a valid history file a present
     * This will throw exceptions if not.
    */
        static void CheckChunks(const rdf::ChunkFile& cf)
        {
            CheckChunkIsPresent(cf, TokenChunkIdentifier, 2);
            CheckChunkIsPresent(cf, IndexChunkIdentifier, 2);
            CheckChunkIsPresent(cf, RayIndexChunkIdentifier, 2);
        }

        /**
     * Check if a given stream contains a valid trace
    */
        static bool ContainsTrace(rdf::Stream& stream)
        {
            try
            {
                rdf::ChunkFile cf(stream);

                CheckChunks(cf);

                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        static std::int64_t GetTraceCount(const rdf::ChunkFile& cf)
        {
            try
            {
                // CheckChunks will throw if no chunks are found
                CheckChunks(cf);
            }
            catch (...)
            {
                return 0;
            }

            const auto tokenChunkCount    = cf.GetChunkCount(TokenChunkIdentifier);
            const auto indexChunkCount    = cf.GetChunkCount(IndexChunkIdentifier);
            const auto rayIndexChunkCount = cf.GetChunkCount(RayIndexChunkIdentifier);

            // ensure same number of chunks for each type
            if (tokenChunkCount != indexChunkCount || indexChunkCount != rayIndexChunkCount)
            {
                return 0;
            }

            return tokenChunkCount;
        }

        void LoadFromFile(rdf::ChunkFile& chunkFile, const int chunkIndex = 0)
        {
            CheckChunks(chunkFile);

            tokenData_.resize(chunkFile.GetChunkDataSize(TokenChunkIdentifier, chunkIndex));
            if (!tokenData_.empty())
            {
                chunkFile.ReadChunkDataToBuffer(TokenChunkIdentifier, chunkIndex, tokenData_.data());
            }

            tokenIndices_.resize(chunkFile.GetChunkDataSize(IndexChunkIdentifier, chunkIndex) / sizeof(RayHistoryTrace::TokenIndex));
            if (!tokenIndices_.empty())
            {
                chunkFile.ReadChunkDataToBuffer(IndexChunkIdentifier, chunkIndex, tokenIndices_.data());
            }

            rayRanges_.resize(chunkFile.GetChunkDataSize(RayIndexChunkIdentifier, chunkIndex) / sizeof(RayHistoryTrace::RayRange));
            if (!rayRanges_.empty())
            {
                chunkFile.ReadChunkDataToBuffer(RayIndexChunkIdentifier, chunkIndex, rayRanges_.data());
            }

            metadataStore_.LoadFromFile(chunkFile, chunkIndex);

            // Try to reconstruct the last ray ID from the dispatch size. If
            // this fails, the ray index will calculate it based on the
            // existing rays, which may result in a lower count if the dispatch
            // had empty rays at the end
            // Note: -1 because for a dispatch of size 8x8 the last ray ID is
            // 63!
            lastRayId_ = 0;
            if (metadataStore_.HasMetadata(RayHistoryMetadataKind::DispatchSize))
            {
                DispatchSize ds = {};
                metadataStore_.GetMetadata(RayHistoryMetadataKind::DispatchSize, static_cast<std::byte*>(static_cast<void*>(&ds)));
                lastRayId_ = ds.width * ds.height * ds.depth - 1;
            }
            else if (metadataStore_.HasMetadata(RayHistoryMetadataKind::DXC_RayTracingCounterInfo))
            {
                GpuRt::CounterInfo rtci = {};
                metadataStore_.GetMetadata(RayHistoryMetadataKind::DXC_RayTracingCounterInfo, static_cast<std::byte*>(static_cast<void*>(&rtci)));
                lastRayId_ = rtci.dispatchRayDimensionX * rtci.dispatchRayDimensionY * rtci.dispatchRayDimensionZ - 1;
            }
            else if (metadataStore_.HasMetadata(RayHistoryMetadataKind::DXC_RayHistoryTraversalFlags))
            {
                GpuRt::RayHistoryTraversalFlags rhtf = {};
                metadataStore_.GetMetadata(RayHistoryMetadataKind::DXC_RayTracingCounterInfo, static_cast<std::byte*>(static_cast<void*>(&rhtf)));
            }

            CreateRayIndex();
        }

    private:
        static void CheckChunkIsPresent(const rdf::ChunkFile& cf, const char* id, const int version)
        {
            if (!cf.ContainsChunk(id))
            {
                RRA_ASSERT_FAIL("Missing chunk");
            }

            if (cf.GetChunkVersion(id) != (uint32_t)version)
            {
                RRA_ASSERT_FAIL("Invalid chunk version");
            }
        }

        void CreateRayIndex()
        {
            for (std::size_t i = 0; i < rayRanges_.size(); ++i)
            {
                const auto id   = rayRanges_[i].rayId;
                rayIndices_[id] = i;
                lastRayId_      = std::max(id, lastRayId_);
            }
        }

        std::vector<std::byte>                   tokenData_;
        std::vector<RayHistoryTrace::TokenIndex> tokenIndices_;
        std::vector<RayHistoryTrace::RayRange>   rayRanges_;

        std::unordered_map<std::uint32_t, std::size_t> rayIndices_;
        std::uint32_t                                  lastRayId_ = 0;

        MetadataStore metadataStore_;
    };

    RayHistory RayHistoryTrace::Iterator::operator*() const
    {
        Validate();

        if (mode_ == IterationMode::IncludeEmptyRays)
        {
            return p_->GetRayById(i_);
        }
        else
        {
            return p_->GetRayByIndex(i_);
        }
    }

    void RayHistoryTrace::Iterator::Validate() const
    {
        if (p_ == nullptr)
        {
            RRA_ASSERT_FAIL("RayHistoryTrace Error: invalid iterator");
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    RayHistoryTrace::RayHistoryTrace(std::vector<std::byte>&&  tokens,
                                     std::vector<TokenIndex>&& tokenIndices,
                                     std::vector<RayRange>&&   rayRanges,
                                     const std::uint32_t       highestRayId)
        : impl_(new Impl(std::move(tokens), std::move(tokenIndices), std::move(rayRanges), highestRayId))
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    RayHistoryTrace::RayHistoryTrace()
        : impl_(new Impl())
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    RayHistory RayHistoryTrace::GetRayByIndex(const int index) const
    {
        return impl_->GetRayByIndex(index);
    }

    ///////////////////////////////////////////////////////////////////////////
    RayHistory RayHistoryTrace::GetRayById(const int id) const
    {
        return impl_->GetRayById(id);
    }

    ///////////////////////////////////////////////////////////////////////////
    int RayHistoryTrace::GetRayCount(IterationMode mode) const
    {
        if (mode == IncludeEmptyRays)
        {
            return impl_->GetHighestRayId() + 1;
        }
        else
        {
            return impl_->GetRayCount();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void RayHistoryTrace::AddMetadata(const RayHistoryMetadataKind kind, const std::size_t size, const void* buffer)
    {
        impl_->GetMetadataStore().AddMetadata(kind, size, static_cast<const std::byte*>(buffer));
    }

    //////////////////////////////////////////////////////////////////////////
    void RayHistoryTrace::RemoveMetadata(const RayHistoryMetadataKind kind)
    {
        impl_->GetMetadataStore().RemoveMetadata(kind);
    }

    //////////////////////////////////////////////////////////////////////////
    bool RayHistoryTrace::HasMetadata(const RayHistoryMetadataKind kind) const
    {
        return impl_->GetMetadataStore().HasMetadata(kind);
    }

    //////////////////////////////////////////////////////////////////////////
    size_t RayHistoryTrace::GetMetadataSize(const RayHistoryMetadataKind kind) const
    {
        return impl_->GetMetadataStore().GetMetadataSize(kind);
    }

    //////////////////////////////////////////////////////////////////////////
    void RayHistoryTrace::GetMetadata(const RayHistoryMetadataKind kind, void* buffer) const
    {
        impl_->GetMetadataStore().GetMetadata(kind, static_cast<std::byte*>(buffer));
    }

    //////////////////////////////////////////////////////////////////////////
    std::vector<RayHistoryMetadataKind> RayHistoryTrace::GetMetadataKeys() const
    {
        return impl_->GetMetadataStore().GetMetadataKeys();
    }

    ///////////////////////////////////////////////////////////////////////////
    void RayHistoryTrace::SaveToFile(rdf::Stream& stream, const bool compress, const bool includeEmptyMetadata) const
    {
        impl_->SaveToFile(stream, compress ? rdfCompressionZstd : rdfCompressionNone, includeEmptyMetadata);
    }

    ///////////////////////////////////////////////////////////////////////////
    void RayHistoryTrace::SaveToFile(rdf::ChunkFileWriter& writer, const bool compress, const bool includeEmptyMetadata) const
    {
        impl_->SaveToFile(writer, compress ? rdfCompressionZstd : rdfCompressionNone, includeEmptyMetadata);
    }

    ///////////////////////////////////////////////////////////////////////////
    void RayHistoryTrace::LoadFromFile(rdf::Stream& stream)
    {
        rdf::ChunkFile cf(stream);
        impl_->LoadFromFile(cf);
    }

    void RayHistoryTrace::LoadFromFile(rdf::ChunkFile& file, const int chunkIndex)
    {
        impl_->LoadFromFile(file, chunkIndex);
    }

    ///////////////////////////////////////////////////////////////////////////
    bool RayHistoryTrace::ContainsTrace(rdf::Stream& stream)
    {
        return Impl::ContainsTrace(stream);
    }

    ///////////////////////////////////////////////////////////////////////////
    std::int64_t RayHistoryTrace::GetTraceCount(const rdf::ChunkFile& cf)
    {
        return Impl::GetTraceCount(cf);
    }

    ///////////////////////////////////////////////////////////////////////////
    RayHistoryTrace::~RayHistoryTrace()
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    std::unique_ptr<RayHistoryTrace> ReadDxcRayHistoryTrace(const RayTracingBinaryLoader& loader)
    {
        struct RayState
        {
            std::vector<std::byte> tokenData;
            // Offsets of token starts
            std::vector<RayHistoryTrace::TokenIndex> index;

            bool beginToken = false;
            bool endToken   = false;
        };

        // Using a map here improves compression ratio on Windows by 2x, as
        // consecutive indices compress _much_ better. On Linux the effect is
        // minimal as the unordered_map implementation there is well sorted
        std::map<std::uint32_t, RayState> rayData;
        int                               totalTokenCount = 0;

        const auto bufferStart          = loader.GetBufferData();
        const auto bufferEnd            = loader.GetBufferData() + loader.GetBufferSize();
        auto       CheckOffsetIsInRange = [bufferStart, bufferEnd](const void* p, const size_t size) -> bool {
            return p >= bufferStart && (static_cast<const std::byte*>(p) + size) <= bufferEnd;
        };

        for (size_t offset = 0, end = loader.GetBufferSize(); offset < end;)
        {
            const void* readPointer = loader.GetBufferData() + offset;
            // We've reached the end of the stream, can't read anything here
            if (!CheckOffsetIsInRange(readPointer, sizeof(RayHistoryTokenId)))
            {
                break;
            }
            // Tokens
            const auto id = static_cast<const RayHistoryTokenId*>(readPointer);

            auto& rayState = rayData[id->id];

            ++totalTokenCount;

            readPointer = loader.GetBufferData() + offset + sizeof(RayHistoryTokenId);

            // The read pointer is pointing at the beginning of the control
            // token, so check if we can read the whole control token. If not the
            // trace is definitely broken
            if (!CheckOffsetIsInRange(readPointer, sizeof(RayHistoryTokenControl)))
            {
                break;
            }

            if (id->control)
            {
                // If it's a control word, then the payload comes right after
                // this one
                const auto control = static_cast<const RayHistoryTokenControl*>(readPointer);
                offset += sizeof(RayHistoryTokenId);

                // Check that we can actually read the whole token payload. Note
                // that the read pointer didn't move yet so we need to check for
                // both the token and the payload. At this point we know
                // that we can dereference control, because that's checked before
                // this loop
                if (!CheckOffsetIsInRange(readPointer, sizeof(RayHistoryTokenControl) + control->tokenLength))
                {
                    break;
                }

                const auto tokenStart = rayState.tokenData.size();

                assert(tokenStart <= std::numeric_limits<std::uint32_t>::max());

                const RayHistoryTrace::TokenIndex tokenIndex = {static_cast<uint32_t>(tokenStart), 1};
                rayState.index.push_back(tokenIndex);

                switch (control->type)
                {
                case RayHistoryTokenType::Begin:
                    assert(control->tokenLength * 4 == sizeof(RayHistoryTokenBeginData));
                    rayState.beginToken = true;
                    break;
                case RayHistoryTokenType::BeginV2:
                    assert(control->tokenLength * 4 == sizeof(RayHistoryTokenBeginDataV2));
                    rayState.beginToken = true;
                    break;
                case RayHistoryTokenType::AnyHitStatus:
                    assert(control->tokenLength == 0);
                    break;
                case RayHistoryTokenType::FunctionCallV2:
                    // The new function call control token has length 3
                    assert(control->tokenLength == 3);
                    break;
                case RayHistoryTokenType::End:
                    assert(control->tokenLength * 4 == sizeof(RayHistoryTokenEndData));
                    rayState.endToken = true;
                    break;
                case RayHistoryTokenType::EndV2:
                    assert(control->tokenLength * 4 == sizeof(RayHistoryTokenEndDataV2));
                    rayState.endToken = true;
                    break;
                default:
                    // All other tokens have length 2
                    assert(control->tokenLength == 2);
                    break;
                }

                rayState.tokenData.insert(
                    rayState.tokenData.end(),
                    loader.GetBufferData() + offset,
                    loader.GetBufferData() + offset + sizeof(RayHistoryTokenControl) + static_cast<std::size_t>(control->tokenLength) * 4 /* size in DWORDS */);

                offset += sizeof(RayHistoryTokenControl);
                offset += static_cast<std::size_t>(control->tokenLength) * 4 /* size in DWORDS */;
            }
            else
            {
                // No need to check anything here because the control token is
                // fully available
                offset += sizeof(RayHistoryTokenId);

                const auto tokenStart = rayState.tokenData.size();

                // Check if TokenId and TokenControl are invalid, i.e. all zero
                if (std::all_of(loader.GetBufferData() + offset - sizeof(RayHistoryTokenId),
                                loader.GetBufferData() + offset + sizeof(RayHistoryTokenControl),
                                [](const std::byte b) { return b == std::byte(); }))
                {
                    offset += sizeof(RayHistoryTokenControl);
                    // Do not count invalid tokens
                    --totalTokenCount;
                    continue;
                }

                assert(tokenStart <= std::numeric_limits<std::uint32_t>::max());

                const RayHistoryTrace::TokenIndex tokenIndex = {static_cast<uint32_t>(tokenStart), 0};

                rayState.index.push_back(tokenIndex);

                rayState.tokenData.insert(
                    rayState.tokenData.end(), loader.GetBufferData() + offset, loader.GetBufferData() + offset + sizeof(RayHistoryTokenControl));

                offset += sizeof(RayHistoryTokenControl);
            }
        }

        // At this point, we have the data in rayData. We now compact it such
        // that all tokenData entries get pasted together, and adjust the
        // indices accordingly.
        std::size_t totalCombinedTokenSize  = 0;
        bool        containsFullHistoryRays = false;

        for (const auto& kv : rayData)
        {
            totalCombinedTokenSize += kv.second.tokenData.size();
#if _DEBUG
            assert(kv.second.beginToken);
#endif
            if (kv.second.beginToken && kv.second.endToken)
            {
                containsFullHistoryRays = true;
            }
        }

        std::vector<std::byte> combinedTokenData;
        combinedTokenData.reserve(totalCombinedTokenSize);

        std::vector<RayHistoryTrace::TokenIndex> combinedTokenIndices;
        combinedTokenIndices.reserve(totalTokenCount);

        std::vector<RayHistoryTrace::RayRange> combinedRayRanges;
        combinedRayRanges.reserve(rayData.size());

        for (const auto& kv : rayData)
        {
            const RayHistoryTrace::RayRange range = {kv.first,
                                                     static_cast<std::uint32_t>(combinedTokenIndices.size()),
                                                     static_cast<std::uint32_t>(kv.second.index.size()),
                                                     combinedTokenData.size()};

            if (containsFullHistoryRays && IsFlagSet(loader.GetFlags(), RayTracingCounterLoadFlags::RemovePartialRays))
            {
                if (!(kv.second.beginToken && kv.second.endToken))
                {
                    // Skip this ray, as it doesn't have an end token
                    continue;
                }
            }

            combinedRayRanges.push_back(range);

            combinedTokenData.insert(combinedTokenData.end(), kv.second.tokenData.begin(), kv.second.tokenData.end());

            combinedTokenIndices.insert(combinedTokenIndices.end(), kv.second.index.begin(), kv.second.index.end());
        }

        auto result = std::make_unique<RayHistoryTrace>(
            std::move(combinedTokenData), std::move(combinedTokenIndices), std::move(combinedRayRanges), loader.GetHighestRayId());

        const auto& header = loader.GetHeader(0);
        result->AddMetadata(RayHistoryMetadataKind::DXC_RayTracingCounterInfo, sizeof(header.counterInfo), &header.counterInfo);
        result->AddMetadata(RayHistoryMetadataKind::DXC_RayHistoryTraversalFlags, sizeof(header.traversalFlags), &header.traversalFlags);

        const DispatchSize dispatchSize = {
            header.counterInfo.dispatchRayDimensionX, header.counterInfo.dispatchRayDimensionY, header.counterInfo.dispatchRayDimensionZ};

        result->AddMetadata(RayHistoryMetadataKind::DispatchSize, sizeof(dispatchSize), &dispatchSize);

        // Sanity check: #dispatched pixels = #rays
        if (const auto rayCount = result->GetRayCount(RayHistoryTrace::IncludeEmptyRays);
            (dispatchSize.width * dispatchSize.height * dispatchSize.depth) != (uint32_t)rayCount)
        {
            RRA_ASSERT_FAIL("dispatch dimension and ray count do not match");
        }
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////
    void WriteNativeRayHistoryTrace(const RayHistoryTrace& rht, const char* filename)
    {
        WriteNativeRayHistoryTrace(rht, filename, true);
    }

    ///////////////////////////////////////////////////////////////////////////
    void WriteNativeRayHistoryTrace(const RayHistoryTrace& rht, const char* filename, const bool compress)
    {
        rdf::Stream stream = rdf::Stream::CreateFile(filename);
        rht.SaveToFile(stream, compress);
    }

    ///////////////////////////////////////////////////////////////////////////////
    std::unique_ptr<RayHistoryTrace> ReadDxcRayHistoryTrace(const char* filename, RayTracingCounterLoadFlags flags)
    {
        return ReadDxcRayHistoryTrace(rdf::Stream::OpenFile(filename), flags);
    }

    ///////////////////////////////////////////////////////////////////////////////
    std::unique_ptr<RayHistoryTrace> ReadDxcRayHistoryTrace(rdf::Stream& stream, RayTracingCounterLoadFlags flags)
    {
        RayTracingBinaryLoader loader(flags);
        loader.AddRayHistoryTrace(stream);
        return ReadDxcRayHistoryTrace(loader);
    }

    ///////////////////////////////////////////////////////////////////////////////
    std::unique_ptr<RayHistoryTrace> ReadDxcRayHistoryTrace(rdf::Stream&& stream, RayTracingCounterLoadFlags flags)
    {
        return ReadDxcRayHistoryTrace(stream, flags);
    }

    ///////////////////////////////////////////////////////////////////////////////
    std::unique_ptr<RayHistoryTrace> ReadNativeRayHistoryTrace(const char* filename, int chunkIndex)
    {
        auto chunkFile = rdf::ChunkFile(filename);
        return ReadNativeRayHistoryTrace(chunkFile, chunkIndex);
    }

    ///////////////////////////////////////////////////////////////////////////////
    std::unique_ptr<RayHistoryTrace> ReadNativeRayHistoryTrace(rdf::Stream& stream, int chunkIndex)
    {
        auto chunkFile = rdf::ChunkFile(stream);
        return ReadNativeRayHistoryTrace(chunkFile, chunkIndex);
    }

    ///////////////////////////////////////////////////////////////////////////////
    std::unique_ptr<RayHistoryTrace> ReadNativeRayHistoryTrace(rdf::ChunkFile& file, int chunkIndex)
    {
        auto result = std::make_unique<RayHistoryTrace>();
        result->LoadFromFile(file, chunkIndex);
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////
    std::unique_ptr<RayHistoryTrace> ReadRayHistoryTrace(const char* filename, int chunkIndex, RayTracingCounterLoadFlags flags)
    {
        return ReadRayHistoryTrace(rdf::Stream::OpenFile(filename), chunkIndex, flags);
    }

    ///////////////////////////////////////////////////////////////////////////////
    std::unique_ptr<RayHistoryTrace> ReadRayHistoryTrace(rdf::Stream& stream, int chunkIndex, RayTracingCounterLoadFlags flags)
    {
        if (IsNativeRayHistoryTrace(stream))
        {
            return ReadNativeRayHistoryTrace(stream, chunkIndex);
        }
        else if (IsDxcRayHistoryTrace(stream))
        {
            return ReadDxcRayHistoryTrace(stream, flags);
        }
        else
        {
            RRA_ASSERT_FAIL("stream does not contain ray history trace.");
            return {};
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    std::unique_ptr<RayHistoryTrace> ReadRayHistoryTrace(rdf::Stream&& stream, int chunkIndex, RayTracingCounterLoadFlags flags)
    {
        return ReadRayHistoryTrace(stream, chunkIndex, flags);
    }

    ///////////////////////////////////////////////////////////////////////////////
    std::vector<std::unique_ptr<rta::RayHistoryTrace>> ReadRayHistoryTraces(const char* filename, const rta::RayTracingCounterLoadFlags flags)
    {
        auto stream = rdf::Stream::OpenFile(filename);
        return ReadRayHistoryTraces(stream, flags);
    }

    ///////////////////////////////////////////////////////////////////////////////
    std::vector<std::unique_ptr<rta::RayHistoryTrace>> ReadRayHistoryTraces(rdf::Stream& stream, const rta::RayTracingCounterLoadFlags flags)
    {
        std::vector<std::unique_ptr<rta::RayHistoryTrace>> result;

        if (IsDxcRayHistoryTrace(stream))
        {
            result.emplace_back(rta::ReadDxcRayHistoryTrace(stream, flags));
        }
        else if (IsNativeRayHistoryTrace(stream))
        {
            rdf::ChunkFile cf(stream);
            const auto     rayHistoryTraceCount = RayHistoryTrace::GetTraceCount(cf);

            if (rayHistoryTraceCount == 0)
            {
                RRA_ASSERT_FAIL("Ray history file does not contain any traces.");
            }

            for (std::int64_t ci = 0; ci < rayHistoryTraceCount; ++ci)
            {
                result.emplace_back(rta::ReadNativeRayHistoryTrace(cf, (int)ci));
            }
        }
        else
        {
            RRA_ASSERT_FAIL("File is not a valid ray history trace.");
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////
    std::vector<std::unique_ptr<rta::RayHistoryTrace>> ReadRayHistoryTraces(rdf::Stream&& stream, const rta::RayTracingCounterLoadFlags flags)
    {
        return ReadRayHistoryTraces(stream, flags);
    }

    ///////////////////////////////////////////////////////////////////////////////
    bool IsNativeRayHistoryTrace(rdf::Stream& stream)
    {
        return RayHistoryTrace::ContainsTrace(stream);
    }

    ///////////////////////////////////////////////////////////////////////////////
    bool IsNativeRayHistoryTrace(rdf::Stream&& stream)
    {
        return RayHistoryTrace::ContainsTrace(stream);
    }

    ///////////////////////////////////////////////////////////////////////////////
    bool IsNativeRayHistoryTrace(const char* filename)
    {
        rdf::Stream stream = rdf::Stream::OpenFile(filename);

        return RayHistoryTrace::ContainsTrace(stream);
    }

    ///////////////////////////////////////////////////////////////////////////////
    bool IsDxcRayHistoryTrace(rdf::Stream& stream)
    {
        return RayTracingBinaryLoader::ContainsRayHistoryTrace(stream);
    }

    ///////////////////////////////////////////////////////////////////////////////
    bool IsDxcRayHistoryTrace(rdf::Stream&& stream)
    {
        return RayTracingBinaryLoader::ContainsRayHistoryTrace(stream);
    }

    ///////////////////////////////////////////////////////////////////////////////
    bool IsDxcRayHistoryTrace(const char* filename)
    {
        return IsDxcRayHistoryTrace(rdf::Stream::OpenFile(filename));
    }

    ///////////////////////////////////////////////////////////////////////////////
    bool IsRayHistoryTrace(const char* filename)
    {
        auto stream = rdf::Stream::OpenFile(filename);
        return IsRayHistoryTrace(stream);
    }

    ///////////////////////////////////////////////////////////////////////////////
    bool IsRayHistoryTrace(rdf::Stream& stream)
    {
        if (IsDxcRayHistoryTrace(stream) || IsNativeRayHistoryTrace(stream))
        {
            return true;
        }

        return false;
    }

    ///////////////////////////////////////////////////////////////////////////////
    std::int64_t GetNativeRayHistoryTraceCount(const char* filename)
    {
        rdf::Stream    stream = rdf::Stream::OpenFile(filename);
        rdf::ChunkFile chunkFile(stream);

        return GetNativeRayHistoryTraceCount(chunkFile);
    }

    ///////////////////////////////////////////////////////////////////////////////
    std::int64_t GetNativeRayHistoryTraceCount(const rdf::ChunkFile& file)
    {
        return RayHistoryTrace::GetTraceCount(file);
    }

}  // namespace rta
