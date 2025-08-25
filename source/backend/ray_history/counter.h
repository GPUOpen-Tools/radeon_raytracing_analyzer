//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for RTA ray history counters.
//=============================================================================

#ifndef RRA_BACKEND_RAY_HISTORY_COUNTER_H_
#define RRA_BACKEND_RAY_HISTORY_COUNTER_H_

#include <cstdint>
#include <memory>
#include <stdexcept>

#define GPURT_COUNTER_MAJOR_VERSION 2

namespace rta
{
    // Indicates that the current ray is invalid or not tracing anything.
    constexpr std::uint32_t INVALID_RAY_ID = UINT32_MAX;

    enum class RayTracingCounterError
    {
        Unknown,
        PartialTrace,
        UnsupportedFormat,
    };

    class RayTracingCounterException final : public std::runtime_error
    {
    public:
        RayTracingCounterException(const std::string& message)
            : std::runtime_error(message)
            , error_(RayTracingCounterError::Unknown)
        {
        }

        RayTracingCounterException(const std::string& message, RayTracingCounterError error)
            : std::runtime_error(message)
            , error_(error)
        {
        }

        RayTracingCounterError GetError() const
        {
            return error_;
        }

    private:
        RayTracingCounterError error_;
    };

    // Fields of the 32-bit value returned by the AmdTraceRayGetHwWaveId intrinsic used by
    // raytracing shaders. This is effectively the hardware register HW_ID1 without padding
    // between the relevant fields.
    //
    // The layout for gfx10 and gfx11 is effectively identical; the only difference is that
    // gfx10 only uses 2 bits of se_id.
    struct HWWaveIdGfx10
    {
        unsigned int wave_id : 5;
        unsigned int simd_id : 2;
        unsigned int wgp_id : 4;
        unsigned int sa_id : 1;
        unsigned int se_id : 3;
        unsigned int padding : 17;

        // masks the required bits for identifying a hardware wave slot
        static constexpr std::uint32_t WaveSlotMask = 0x7fff;
        // masks the required bits for identifying compute unit (= row of a WGP)
        // The compute unit / row is identified by the _low_ bit of the simd_id.
        static constexpr std::uint32_t ComputeUnitMask = 0x7fa0;
        // masks the required bits for identifying a WGP
        static constexpr std::uint32_t WgpMask = 0x7f80;
        // masks the required bits for identifying an SA
        static constexpr std::uint32_t ShaderArrayMask = 0x7800;
        // masks the required bits for identifying an SE
        static constexpr std::uint32_t ShaderEngineMask = 0x7000;
    };
    // Ensure that the wave ID just consumes 32 bits.
    static_assert(sizeof(HWWaveIdGfx10) == (sizeof(std::uint32_t)));

    class DxrRayTracingCounterInfo final
    {
    public:
        DxrRayTracingCounterInfo(const std::size_t size, const void* buffer);
        ~DxrRayTracingCounterInfo();

        void          GetDispatchDimensions(std::uint32_t dimensions[3]) const;
        std::uint32_t GetHitGroupShaderRecordCount() const;
        std::uint32_t GetMissShaderRecordCount() const;
        std::uint32_t GetPipelineShaderCount();
        std::uint64_t GetStateObjectHash() const;

        std::uint32_t GetCounterMode() const;
        std::uint32_t GetCounterMask() const;
        std::uint32_t GetCounterStride() const;

        std::uint32_t GetRayCounterDataSize() const;
        std::uint32_t GetLostTokenBytes() const;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

    struct DispatchSize
    {
        std::uint32_t width, height, depth;
    };

    static_assert(sizeof(DispatchSize) == (sizeof(std::uint32_t) * 3));

    struct TileSize
    {
        std::uint32_t width;
        std::uint32_t height;

        static TileSize CreateInvalid();
        bool            IsInvalid() const;
    };

    enum class RayTracingCounterLoadFlags : std::uint32_t
    {
        None,
        AllowPartialTrace = 1 << 0,
        RemovePartialRays = 1 << 1,
        Default           = None
    };

}  // namespace rta

#endif
