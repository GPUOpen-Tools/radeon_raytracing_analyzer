//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for RTA ray history.
//=============================================================================

#ifndef RRA_BACKEND_RAY_HISTORY_H_
#define RRA_BACKEND_RAY_HISTORY_H_

#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_set>

#include "raytracing_counter.h"
#include "counter.h"

#include <rdf/rdf/inc/amdrdf.h>

namespace rta
{
    class IDirectoryIterator;

    enum class NodeType : std::uint32_t
    {
        TRIANGLE_0           = 0,
        TRIANGLE_1           = 1,
        TRIANGLE_2           = 2,
        TRIANGLE_3           = 3,
        BOX_FLOAT16          = 4,
        BOX_FLOAT32          = 5,
        USER_NODE_INSTANCE   = 6,
        USER_NODE_PROCEDURAL = 7,
        UNDEFINED            = 0xFFFF
    };

    const char* ToString(NodeType nodeType);

    enum class RayHistoryTokenType : std::uint16_t
    {
        Begin,
        TopLevel,
        BottomLevel,
        End,
        FunctionCall,
        Timestamp,
        AnyHitStatus,
        FunctionCallV2,
        ProceduralIntersectionStatus,
        EndV2,
        BeginV2,

        // Anything with the top bit set is "reserved"
        Reserved = 0x8000,

        // We use unknown to indicate it's a "normal" token
        Unknown = 0xFFFF
    };

    std::string ToString(const RayHistoryTokenType tokenType);

    // Ray history function call types that fit within control token data field
    enum class RayHistoryFunctionCallType : std::uint8_t
    {
        Reserved     = 0,
        Miss         = 1,
        Closest      = 2,
        AnyHit       = 3,
        Intersection = 4,
        Unknown      = 0xFF
    };

    const char* ToString(const RayHistoryFunctionCallType callType);

    // Ray history any hit status.
    // Stored in control optional data for AnyHitStatus & ProceduralIntersectionStatus tokens
    // see https://github.amd.com/AMD-Radeon-Driver/gpurt/blob/amd/stg/gpurt/src/shaders/Common.hlsl#L83
    enum class RayHistoryAnyHitStatus : std::uint8_t
    {
        IgnoreHit             = 0,
        AcceptHit             = 1,
        AcceptHitAndEndSearch = 2
    };

    const char* ToString(const RayHistoryAnyHitStatus status);

    struct float3
    {
        float x, y, z;
    };

    struct RayHistoryTokenId final
    {
        std::uint32_t id : 30;      /// Unique identifier
        std::uint32_t unused : 1;   /// Reserved for future uses
        std::uint32_t control : 1;  /// Indicates that a control DWORD follows the RayID in token stream
    };

    struct RayHistoryTokenData
    {
    };

    struct RayHistoryTokenBeginData : public RayHistoryTokenData
    {
        std::uint32_t hwWaveId;              /// 32-bit unique identifier for a wave.
        std::uint32_t dispatchRaysIndex[3];  /// Dispatch rays index in 3-dimensional ray grid

        std::uint32_t accelStructAddrLo;  /// API top-level acceleration structure base address (lower 32-bits)
        std::uint32_t accelStructAddrHi;  /// API top-level acceleration structure base address (upper 32-bits)
        std::uint32_t rayFlags;           /// API ray flags (see API documentation for further details)

        union
        {
            struct
            {
                std::uint32_t instanceInclusionMask : 8;           /// API instance inclusion mask (see API documentation for further details)
                std::uint32_t rayContributionToHitGroupIndex : 4;  /// API ray hit group offset (see API documentation for further details)
                std::uint32_t geometryMultiplier : 4;              /// API ray geometry stride (see API documentation for further details)
                std::uint32_t missShaderIndex : 16;                /// API ray miss shader index (see API documentation for further details)
            };

            std::uint32_t packedTraceRayParams;  /// Packed parameters
        };

        struct
        {
            float3 origin;     /// Ray origin
            float  tMin;       /// Ray time minimum bounds
            float3 direction;  /// Ray direction
            float  tMax;       /// Ray time maximum bounds
        } rayDesc;
    };

    struct RayHistoryTokenBeginDataV2 final : public RayHistoryTokenBeginData
    {
        // 32-bit unique identifier, unique to a shader call site (TraceRay/RayQuery.TraceRayInline()
        uint32_t staticId;
        // 32-bit unique identifier generated on traversal begin that is uniform across the wave.
        uint32_t dynamicId;
        // 32-bit unique identifier for parent traversal that invoked recursive traversal.
        uint32_t parentId;
    };

    struct RayHistoryTokenEndData : public RayHistoryTokenData
    {
        std::uint32_t primitiveIndex;  /// Primitive index of the geometry hit by this ray (-1 for a miss)
        std::uint32_t geometryIndex;   /// Geometry index of the geometry hit by this ray (-1 for a miss)
    };

    struct RayHistoryTokenControl final : public RayHistoryTokenData
    {
        union
        {
            struct
            {
                RayHistoryTokenType type;         /// Token type
                std::uint8_t        tokenLength;  /// Length of tokens of this type, in DWORDS
                std::uint8_t        data;         /// Additional data (optional)
            };

            std::uint32_t u32;
        };
    };

    struct RayHistoryTokenTopLevelData final : public RayHistoryTokenData
    {
        std::uint64_t baseAddr;
    };

    struct RayHistoryTokenBottomLevelData final : public RayHistoryTokenData
    {
        std::uint64_t baseAddr;
    };

    struct RayHistoryTokenFunctionCallData final : public RayHistoryTokenData
    {
        std::uint64_t baseAddr;
    };

    struct RayHistoryTokenFunctionCallDataV2 final : public RayHistoryTokenData
    {
        std::uint64_t baseAddr;
        std::uint32_t tableIndex;
    };

    struct RayHistoryTokenProceduralIntersectionData final : public RayHistoryTokenData
    {
        // Hit T as reported by the intersection shader
        float hitT;
        // Hit kind as reported by the intersection shader (only lower 8-bits are valid)
        std::uint32_t hitKind;
    };

    struct RayHistoryTokenEndDataV2 final : public RayHistoryTokenEndData
    {
        uint32_t instanceIndex : 24;        /// Index of the intersected instance
        uint32_t hitKind : 8;               /// Front facing or back facing triangle hit
        uint32_t numIterations;             /// Traversal iterations
        uint32_t numInstanceIntersections;  /// Number of instance intersections
        float    hitT;                      /// Closest intersection distance on ray
    };

    struct RayHistoryTokenNodePtrData final : public RayHistoryTokenData
    {
        std::uint32_t nodePtr;
    };

    struct RayHistoryTokenTimestampData final : public RayHistoryTokenData
    {
        std::uint64_t gpuTimeStamp;
    };

    enum class RayHistoryMetadataKind : std::uint32_t
    {
        DXC_RayTracingCounterInfo    = 1,
        DispatchSize                 = 2,
        DXC_RayHistoryTraversalFlags = 3
    };

    using RayHistoryTokenFilter = std::unordered_set<RayHistoryTokenType>;

    struct RayHistoryMetadataInfo
    {
        rta::RayHistoryMetadataKind kind;
        uint64_t                    sizeInByte;
    };

    struct DispatchDimensions
    {
        uint32_t dimX;
        uint32_t dimY;
        uint32_t dimZ;
    };

    struct RayHistoryMetadata
    {
        RayHistoryMetadataInfo          counterInfo;
        GpuRt::CounterInfo              counter;
        RayHistoryMetadataInfo          dispatchDimsInfo;
        DispatchDimensions              dispatchDims;
        RayHistoryMetadataInfo          traversalFlagsInfo;
        GpuRt::RayHistoryTraversalFlags traversalFlags;
    };

    class RayHistory;

    class RayHistoryTrace final
    {
    private:
        struct Impl;

    public:
        /*
    Used in the HistoryRayIndex chunk, referencing the HistoryIndex and
    HistoryToken chunks
    */
        struct RayRange
        {
            std::uint32_t rayId;

            // Offset of the first token index in the token index buffer
            // Starting from that token, the next tokenCount indices belong to
            // this ray. We use this mechanism because each token can have varying
            // amounts of data, and we want random access to any given token,
            // which we achieve by indirecting through the fixed size token index
            // buffer.
            std::uint32_t tokenStart;
            std::uint32_t tokenCount;

            // Offset to the start of the token data. The token data is stored
            // as a stream of bytes, use the index to find out where each token
            // begins/ends as we remove the token header which would tell you
            // the length.
            std::size_t dataStart;
        };

        /**
    Used in the HistoryIndex chunk
    */
        struct TokenIndex
        {
            // Offset to the token data relative to the start of the tokens for
            // this ray. The base offset is stored in dataStart, and then the
            // offset in each token index provides the relative delta to the
            // base offset.
            std::uint32_t offset : 31;

            // Indicate this token is a control token
            std::uint32_t isControl : 1;
        };

        RayHistoryTrace(std::vector<std::byte>&&  tokens,
                        std::vector<TokenIndex>&& tokenIndices,
                        std::vector<RayRange>&&   rayRanges,
                        const std::uint32_t       highestRayId);
        RayHistoryTrace();
        ~RayHistoryTrace();

        enum IterationMode
        {
            IncludeEmptyRays,
            ExcludeEmptyRays
        };

        int GetRayCount(IterationMode mode) const;

        class Iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using difference_type   = int;
            using value_type        = RayHistory;

            Iterator() = default;
            Iterator(const RayHistoryTrace::Impl* p, IterationMode mode, int i)
                : p_(p)
                , i_(i)
                , mode_(mode)
            {
            }

            bool operator==(const Iterator& rhs) const
            {
                return p_ == rhs.p_ && i_ == rhs.i_ && mode_ == rhs.mode_;
            }

            Iterator& operator++()
            {
                Validate();

                ++i_;
                return *this;
            }

            Iterator operator++(int)
            {
                Validate();

                Iterator old = *this;
                ++(*this);
                return old;
            }

            Iterator& operator+=(const int offset)
            {
                Validate();

                i_ += offset;
                return *this;
            }

            Iterator operator+(const int offset) const
            {
                Validate();

                return Iterator(p_, mode_, i_ + offset);
            }

            Iterator& operator-=(const int offset)
            {
                Validate();

                i_ -= offset;
                return *this;
            }

            Iterator operator-(const int offset) const
            {
                Validate();

                return Iterator(p_, mode_, i_ - offset);
            }

            RayHistory operator*() const;

        private:
            void Validate() const;

            const RayHistoryTrace::Impl* p_ = nullptr;
            int                          i_ = -1;
            IterationMode                mode_;
        };

        using const_iterator = Iterator;
        struct Range
        {
        public:
            Range(Iterator begin, Iterator end)
                : begin_(begin)
                , end_(end)
            {
            }

            Iterator begin() const
            {
                return begin_;
            }

            Iterator end() const
            {
                return end_;
            }

        private:
            Iterator begin_;
            Iterator end_;
        };

        Range Iterate(IterationMode mode) const
        {
            if (mode == IncludeEmptyRays)
            {
                return {Iterator(impl_.get(), mode, 0), Iterator(impl_.get(), mode, GetRayCount(mode))};
            }
            else
            {
                return {Iterator(impl_.get(), mode, 0), Iterator(impl_.get(), mode, GetRayCount(mode))};
            }
        }

        /**
        * Retrieve non-empty rays by index. The amount of non-empty rays can
        * be queried using GetRayCount(ExcludeEmptyRays)
        */
        RayHistory GetRayByIndex(const int index) const;

        /**
        * Retrieve rays by id. The valid index range can be queried using
        * GetRayCount(IncludeEmptyRays)
        */
        RayHistory GetRayById(const int rayId) const;

        void AddMetadata(const RayHistoryMetadataKind kind, const std::size_t size, const void* buffer);
        void RemoveMetadata(const RayHistoryMetadataKind kind);

        bool   HasMetadata(const RayHistoryMetadataKind kind) const;
        size_t GetMetadataSize(const RayHistoryMetadataKind kind) const;
        void   GetMetadata(const RayHistoryMetadataKind kind, void* buffer) const;

        std::vector<RayHistoryMetadataKind> GetMetadataKeys() const;

        // includeEmptyMetadata writes metadata chunk, even if ray history trace does not contain any metadata
        void SaveToFile(rdf::Stream& stream, bool compress, bool includeEmptyMetadata = false) const;
        // includeEmptyMetadata writes metadata chunk, even if ray history trace does not contain any metadata
        void SaveToFile(rdf::ChunkFileWriter& writer, bool compress, bool includeEmptyMetadata = false) const;
        void LoadFromFile(rdf::Stream& stream);
        void LoadFromFile(rdf::ChunkFile& file, int chunkIndex);

        static bool         ContainsTrace(rdf::Stream& s);
        static std::int64_t GetTraceCount(const rdf::ChunkFile& cf);

    private:
        std::unique_ptr<Impl> impl_;
    };

    class RayHistoryToken final
    {
    public:
        RayHistoryToken(const void* data, bool hasControl)
            : data_(data)
            , hasControl_(hasControl)
        {
        }

        RayHistoryToken() = default;

        bool IsInterior() const
        {
            if (hasControl_)
            {
                return false;
            }

            return GetNodeType() == NodeType::BOX_FLOAT16 || GetNodeType() == NodeType::BOX_FLOAT32;
        }

        bool IsControl() const
        {
            return hasControl_;
        }

        bool IsNode() const
        {
            // Either we're a plain token, or a BLAS/TLAS token, in which
            // case we're also "node" tokens.
            return hasControl_ == false || IsBlas() || IsTlas();
        }

        NodeType GetNodeType() const
        {
            if (IsNode())
            {
                const auto data = static_cast<const RayHistoryTokenNodePtrData*>(GetPayload());

                return static_cast<NodeType>(data->nodePtr & 0x7);
            }
            else if (IsBlas())
            {
                const auto data = static_cast<const RayHistoryTokenBottomLevelData*>(GetPayload());

                return static_cast<NodeType>(data->baseAddr & 0x7);
            }
            else if (IsTlas())
            {
                if (GetType() == RayHistoryTokenType::TopLevel)
                {
                    const auto data = static_cast<const RayHistoryTokenTopLevelData*>(GetPayload());

                    return static_cast<NodeType>(data->baseAddr & 0x7);
                }
                else if (IsBegin())
                {
                    const auto data = static_cast<const RayHistoryTokenBeginData*>(GetPayload());

                    return static_cast<NodeType>(data->accelStructAddrLo & 0x7);
                }
            }

            return NodeType::UNDEFINED;
        }

        bool IsLeaf() const
        {
            if (hasControl_)
            {
                return false;
            }

            switch (GetNodeType())
            {
            case NodeType::TRIANGLE_0:
            case NodeType::TRIANGLE_1:
            case NodeType::TRIANGLE_2:
            case NodeType::TRIANGLE_3:
                return true;
            default:
                return false;
            }
        }

        bool IsInstance() const
        {
            if (hasControl_)
            {
                return false;
            }

            return GetNodeType() == NodeType::USER_NODE_INSTANCE;
        }

        bool IsProcedural() const
        {
            if (hasControl_)
            {
                return false;
            }

            return GetNodeType() == NodeType::USER_NODE_PROCEDURAL;
        }

        bool IsTlas() const
        {
            return GetType() == RayHistoryTokenType::TopLevel || IsBegin();
        }

        bool IsBlas() const
        {
            return GetType() == RayHistoryTokenType::BottomLevel;
        }

        bool IsClosestHit() const
        {
            if (!IsEnd())
            {
                return false;
            }

            const auto payload = static_cast<const RayHistoryTokenEndData*>(GetPayload());
            return payload->geometryIndex != 0xFFFFFFFF && payload->primitiveIndex != 0xFFFFFFFF;
        }

        bool IsHit() const
        {
            return IsEnd();
        }

        bool IsFunctionCall() const
        {
            return (GetType() == RayHistoryTokenType::FunctionCall) || (GetType() == RayHistoryTokenType::FunctionCallV2);
        }

        bool IsBegin() const
        {
            return (GetType() == RayHistoryTokenType::Begin) || (GetType() == RayHistoryTokenType::BeginV2);
        }

        bool IsEnd() const
        {
            return (GetType() == RayHistoryTokenType::End) || (GetType() == RayHistoryTokenType::EndV2);
        }

        bool IsAnyHitFunctionCall() const
        {
            if (IsFunctionCall())
            {
                const auto fType = static_cast<RayHistoryFunctionCallType>(GetControlTokenOptionalData());
                return fType == RayHistoryFunctionCallType::AnyHit;
            }
            else
            {
                return false;
            }
        }

        bool IsMiss() const
        {
            if (!IsEnd())
            {
                return false;
            }

            const auto payload = static_cast<const RayHistoryTokenEndData*>(GetPayload());
            return payload->geometryIndex == 0xFFFFFFFF && payload->primitiveIndex == 0xFFFFFFFF;
        }

        std::uint32_t GetPrimitiveId() const
        {
            const auto payload = static_cast<const RayHistoryTokenEndData*>(GetPayload());
            return payload->primitiveIndex;
        }

        std::uint32_t GetGeometryId() const
        {
            const auto payload = static_cast<const RayHistoryTokenEndData*>(GetPayload());
            return payload->geometryIndex;
        }

        /**
    Indicates that this token was emitted during the traversal loop.
    */
        bool IsInsideTraversal() const
        {
            return IsInstance() || IsProcedural() || IsLeaf() || IsInterior();
        }

        /**
    Indicates that this token was emitted outside of the traversal loop.
    */
        bool IsOutsideTraversal() const
        {
            return IsMiss() || IsClosestHit();
        }

        bool IsSidebandData() const
        {
            switch (GetType())
            {
            case RayHistoryTokenType::BottomLevel:
            case RayHistoryTokenType::TopLevel:
            case RayHistoryTokenType::Timestamp:
            case RayHistoryTokenType::AnyHitStatus:
            case RayHistoryTokenType::FunctionCall:
            case RayHistoryTokenType::FunctionCallV2:
            case RayHistoryTokenType::ProceduralIntersectionStatus:
                return true;
            default:
                return false;
            }
        }

        RayHistoryTokenType GetType() const
        {
            if (hasControl_ == false)
            {
                return RayHistoryTokenType::Unknown;
            }
            else
            {
                return static_cast<const RayHistoryTokenControl*>(data_)->type;
            }
        }

        const RayHistoryTokenData* GetPayload() const
        {
            if (hasControl_)
            {
                return static_cast<const RayHistoryTokenData*>(static_cast<const void*>(static_cast<const std::byte*>(data_) + sizeof(RayHistoryTokenControl)));
            }
            else
            {
                return static_cast<const RayHistoryTokenData*>(data_);
            }
        }

        std::uint32_t GetPayloadSize() const
        {
            if (hasControl_)
            {
                // token length is stored in DWORDs
                return static_cast<const RayHistoryTokenControl*>(data_)->tokenLength * sizeof(std::uint32_t);
            }
            else
            {
                return sizeof(RayHistoryTokenNodePtrData);
            }
        }

        // Gets the optional data field within control token. For other tokens, returns 0xFF
        std::uint8_t GetControlTokenOptionalData() const
        {
            if (hasControl_)
            {
                return static_cast<const RayHistoryTokenControl*>(data_)->data;
            }
            else
            {
                return 0xFF;
            }
        }

        /**
        Return 0xFFFF'FFFF'FFFF'FFFF if the token doesn't contain a node (i.e.
        if the token is not a TLAS, BLAS, or Node pointer)
        */
        std::uint64_t GetNodePointer() const
        {
            if (IsBegin())
            {
                const auto data = static_cast<const RayHistoryTokenBeginData*>(GetPayload());

                std::uint64_t result = data->accelStructAddrHi;
                result <<= 32;
                result |= data->accelStructAddrLo;

                return result;
            }
            else if (GetType() == RayHistoryTokenType::TopLevel)
            {
                const auto data = static_cast<const RayHistoryTokenTopLevelData*>(GetPayload());

                return data->baseAddr;
            }
            else if (GetType() == RayHistoryTokenType::BottomLevel)
            {
                const auto data = static_cast<const RayHistoryTokenBottomLevelData*>(GetPayload());

                return data->baseAddr;
            }
            else if (IsNode())
            {
                const auto data = static_cast<const RayHistoryTokenNodePtrData*>(GetPayload());

                return data->nodePtr;
            }

            return 0xFFFF'FFFF'FFFF'FFFF;
        }

        operator bool() const
        {
            return data_;
        }

        bool operator==(const RayHistoryToken& rhs) const
        {
            return data_ == rhs.data_ && hasControl_ == rhs.hasControl_;
        }

        bool operator!=(const RayHistoryToken& rhs) const
        {
            return !(*this == rhs);
        }

    private:
        const void* data_       = nullptr;
        bool        hasControl_ = false;
    };

    class RayHistory final
    {
    public:
        RayHistory(const std::byte* data, const RayHistoryTrace::TokenIndex* indices, const int count, const std::uint32_t rayId)
            : data_(data)
            , indices_(indices)
            , count_(count)
            , rayId_(rayId)
        {
        }

        RayHistory() = default;

        int GetTokenCount() const
        {
            return count_;
        }

        RayHistoryToken GetToken(const int index) const
        {
            const auto& indexData = indices_[index];
            return RayHistoryToken(data_ + indexData.offset, indexData.isControl);
        }

        std::uint32_t GetRayId() const
        {
            return rayId_;
        }

        class Iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = RayHistoryToken;

            Iterator(const RayHistory* p, int i)
                : p_(p)
                , i_(i)
            {
            }

            bool operator==(const Iterator& rhs) const
            {
                return p_ == rhs.p_ && i_ == rhs.i_;
            }

            bool operator!=(const Iterator& rhs) const
            {
                return !(*this == rhs);
            }

            Iterator& operator++()
            {
                ++i_;
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator old = *this;
                ++(*this);
                return old;
            }

            RayHistoryToken operator*() const
            {
                return p_->GetToken(i_);
            }

        private:
            const RayHistory* p_ = nullptr;
            int               i_ = -1;
        };

        using const_iterator = Iterator;
        const_iterator begin() const
        {
            return Iterator(this, 0);
        }

        const_iterator end() const
        {
            return Iterator(this, count_);
        }

        operator bool() const
        {
            return data_;
        }

    private:
        const std::byte*                   data_    = nullptr;
        const RayHistoryTrace::TokenIndex* indices_ = nullptr;
        int                                count_   = 0;
        std::uint32_t                      rayId_   = 0xFFFFFFFF;
    };

    // Loads ray history trace from file in binary format
    std::unique_ptr<RayHistoryTrace> ReadDxcRayHistoryTrace(const char* filename, RayTracingCounterLoadFlags flags = RayTracingCounterLoadFlags::Default);
    // Loads ray history trace from file in binary format
    std::unique_ptr<RayHistoryTrace> ReadDxcRayHistoryTrace(rdf::Stream& stream, RayTracingCounterLoadFlags flags = RayTracingCounterLoadFlags::Default);
    // Loads ray history trace from file in binary format
    std::unique_ptr<RayHistoryTrace> ReadDxcRayHistoryTrace(rdf::Stream&& stream, RayTracingCounterLoadFlags flags = RayTracingCounterLoadFlags::Default);

    // Loads native ray history trace from chunk file. chunkIndex can be used to select specific chunk
    std::unique_ptr<RayHistoryTrace> ReadNativeRayHistoryTrace(const char* filename, int chunkIndex = 0);
    // Loads native ray history trace from chunk file. chunkIndex can be used to select specific chunk
    std::unique_ptr<RayHistoryTrace> ReadNativeRayHistoryTrace(rdf::Stream& stream, int chunkIndex = 0);
    // Loads native ray history trace from chunk file. chunkIndex can be used to select specific chunk
    std::unique_ptr<RayHistoryTrace> ReadNativeRayHistoryTrace(rdf::ChunkFile& file, int chunkIndex = 0);

    // Loads ray history trace from binary or chunk file
    // chunkIndex is used to load specific ray history from chunk file
    // flags are used to load binary ray history traces
    std::unique_ptr<RayHistoryTrace> ReadRayHistoryTrace(const char*                filename,
                                                         int                        chunkIndex = 0,
                                                         RayTracingCounterLoadFlags flags      = RayTracingCounterLoadFlags::Default);
    // Loads ray history trace from binary or chunk file
    // chunkIndex is used to load specific ray history from chunk file
    // flags are used to load binary ray history traces
    std::unique_ptr<RayHistoryTrace> ReadRayHistoryTrace(rdf::Stream&               stream,
                                                         int                        chunkIndex = 0,
                                                         RayTracingCounterLoadFlags flags      = RayTracingCounterLoadFlags::Default);
    // Loads ray history trace from binary or chunk file
    // chunkIndex is used to load specific ray history from chunk file
    // flags are used to load binary ray history traces
    std::unique_ptr<RayHistoryTrace> ReadRayHistoryTrace(rdf::Stream&&              stream,
                                                         int                        chunkIndex = 0,
                                                         RayTracingCounterLoadFlags flags      = RayTracingCounterLoadFlags::Default);

    // Loads ray history traces from binary or chunk file.
    // Returns all ray history traces contained in the input file.
    std::vector<std::unique_ptr<rta::RayHistoryTrace>> ReadRayHistoryTraces(const char* filename, const rta::RayTracingCounterLoadFlags flags);

    // Loads ray history traces from binary or chunk file.
    // Returns all ray history traces contained in the input file.
    std::vector<std::unique_ptr<rta::RayHistoryTrace>> ReadRayHistoryTraces(rdf::Stream& stream, const rta::RayTracingCounterLoadFlags flags);

    // Loads ray history traces from binary or chunk file.
    // Returns all ray history traces contained in the input file.
    std::vector<std::unique_ptr<rta::RayHistoryTrace>> ReadRayHistoryTraces(rdf::Stream&& stream, const rta::RayTracingCounterLoadFlags flags);

    bool IsNativeRayHistoryTrace(const char* filename);
    bool IsNativeRayHistoryTrace(rdf::Stream& stream);
    bool IsNativeRayHistoryTrace(rdf::Stream&& stream);

    bool IsDxcRayHistoryTrace(const char* filename);
    bool IsDxcRayHistoryTrace(rdf::Stream& stream);
    bool IsDxcRayHistoryTrace(rdf::Stream&& stream);

    bool IsRayHistoryTrace(const char* filename);
    bool IsRayHistoryTrace(rdf::Stream& stream);

    std::int64_t GetNativeRayHistoryTraceCount(const char* filename);
    std::int64_t GetNativeRayHistoryTraceCount(const rdf::ChunkFile& file);

    void WriteNativeRayHistoryTrace(const RayHistoryTrace& rht, const char* filename);
    void WriteNativeRayHistoryTrace(const RayHistoryTrace& rht, const char* filename, const bool compress);
}  // namespace rta

#endif
