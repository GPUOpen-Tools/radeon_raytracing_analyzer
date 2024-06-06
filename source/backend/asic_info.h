//=============================================================================
// Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for the ASIC Info class.
//=============================================================================

#ifndef RRA_BACKEND_ASIC_INFO_H_
#define RRA_BACKEND_ASIC_INFO_H_

#include "public/rra_error.h"

#include "rdf/rdf/inc/amdrdf.h"

namespace rra
{
    class AsicInfo
    {
    public:
        // Global identifier of the ASIC info in chunk files.
        static constexpr const char* kChunkIdentifier = "AsicInfo";

        static constexpr int TRACE_MAX_NUM_SE        = 32;
        static constexpr int TRACE_SA_PER_SE         = 2;
        static constexpr int TRACE_GPU_NAME_MAX_SIZE = 256;

        /// @brief Constructor.
        AsicInfo();

        /// @brief Destructor.
        ~AsicInfo();

        /// @brief Load the chunk.
        ///
        /// @param [in] chunk_file The chunk file to load from.
        ///
        /// @return kRraOk if loaded correctly, error code if not.
        RraErrorCode LoadChunk(rdf::ChunkFile& chunk_file);

        /// @brief Get the name of the GPU that the trace was taken on.
        ///
        /// @return Pointer to a string containing the device string, or nullptr if invalid.
        const char* GetDeviceName() const;

        /// @brief Get the device ID.
        ///
        /// @param [out] A variable to receive the device ID.
        ///
        /// @return kRraOk if successful or error code if not.
        RraErrorCode GetDeviceID(int32_t* device_id) const;

        /// @brief Get the device revision ID.
        ///
        /// @param [out] A variable to receive the device revision ID.
        ///
        /// @return kRraOk if successful or error code if not.
        RraErrorCode GetDeviceRevisionID(int32_t* device_revision_id) const;

        /// @brief Get the shader core clock frequency, in Hz.
        ///
        /// @param [out] A variable to receive the shader clock frequency.
        ///
        /// @return kRraOk if successful or error code if not.
        RraErrorCode GetShaderCoreClockFrequency(uint64_t* out_clk_frequency) const;

        /// @brief Get the maximum shader core clock frequency, in Hz.
        ///
        /// @param [out] A variable to receive the shader clock frequency.
        ///
        /// @return kRraOk if successful or error code if not.
        RraErrorCode GetMaxShaderCoreClockFrequency(uint64_t* out_clk_frequency) const;

        /// @brief Get the Video RAM size, in bytes.
        ///
        /// @param [out] A variable to receive the video RAM size.
        ///
        /// @return kRraOk if successful or error code if not.
        RraErrorCode GetVRAMSize(int64_t* out_vram_size) const;

        /// @brief Get the memory clock frequency, in Hz.
        ///
        /// @param [out] A variable to receive the memory clock frequency.
        ///
        /// @return kRraOk if successful or error code if not.
        RraErrorCode GetMemoryClockFrequency(uint64_t* out_clk_frequency) const;

        /// @brief Get the maximum memory clock frequency, in Hz.
        ///
        /// @param [out] A variable to receive the memory clock frequency.
        ///
        /// @return kRraOk if successful or error code if not.
        RraErrorCode GetMaxMemoryClockFrequency(uint64_t* out_clk_frequency) const;

        /// @brief Get the video memory bandwidth, in bytes per second.
        ///
        /// @param [out] A variable to receive the memory bandwidth.
        ///
        /// @return kRraOk if successful or error code if not.
        RraErrorCode GetVideoMemoryBandwidth(uint64_t* out_memory_bandwidth) const;

        /// @brief Get the video memory type as a string.
        ///
        /// @return Pointer to the video memory type string, or nullptr if invalid.
        const char* GetVideoMemoryType() const;

        /// @brief Get the video memory bus width, in bits.
        ///
        /// @param [out] A variable to receive the memory bus width.
        ///
        /// @return kRraOk if successful or error code if not.
        RraErrorCode GetVideoMemoryBusWidth(int32_t* out_bus_width) const;

        /// @brief Get the Gfx ip level major.
        ///
        /// @param [out] A variable to receive the gfx ip level major.
        ///
        /// @return kRraOk if successful or error code if not.
        RraErrorCode GetGfxIpLevelMajor(uint16_t* out_gfx_ip_level);

        /// @brief enum of GPU types.
        enum class TraceGpuType : uint32_t
        {
            Unknown,
            Integrated,
            Discrete,
            Virtual
        };

        /// @brief Structure describing the graphics IP level.
        struct TraceGfxIpLevel
        {
            uint16_t major;
            uint16_t minor;
            uint16_t stepping;
        };

        /// @brief enum of GPU memory types.
        enum class TraceMemoryType : uint32_t
        {
            Unknown,
            Ddr,
            Ddr2,
            Ddr3,
            Ddr4,
            Ddr5,
            Gddr3,
            Gddr4,
            Gddr5,
            Gddr6,
            Hbm,
            Hbm2,
            Hbm3,
            Lpddr4,
            Lpddr5
        };

        struct TraceChunkAsicInfo_v1
        {
            uint64_t        shader_core_clock_frequency;                 ///< Gpu core clock frequency in Hz.
            uint64_t        memory_clock_frequency;                      ///< Memory clock frequency in Hz.
            uint64_t        gpu_timestamp_frequency;                     ///< Frequency of the gpu timestamp clock in Hz.
            uint64_t        max_shader_core_clock;                       ///< Maximum shader core clock frequency in Hz.
            uint64_t        max_memory_clock;                            ///< Maximum memory clock frequency in Hz.
            int32_t         device_id;                                   ///< Device ID.
            int32_t         device_revision_id;                          ///< Device revision ID.
            int32_t         vgprs_per_simd;                              ///< Number of VGPRs per SIMD.
            int32_t         sgprs_per_simd;                              ///< Number of SGPRs per SIMD.
            int32_t         shader_engines;                              ///< Number of shader engines.
            int32_t         compute_units_per_shader_engine;             ///< Number of compute units per shader engine.
            int32_t         simds_per_compute_unit;                      ///< Number of SIMDs per compute unit.
            int32_t         wavefronts_per_simd;                         ///< Number of wavefronts per SIMD.
            int32_t         minimum_vgpr_allocation;                     ///< Minimum number of VGPRs per wavefront.
            int32_t         vgpr_allocation_granularity;                 ///< Allocation granularity of VGPRs.
            int32_t         minimum_sgpr_allocation;                     ///< Minimum number of SGPRs per wavefront.
            int32_t         sgpr_allocation_granularity;                 ///< Allocation granularity of SGPRs.
            int32_t         hardware_contexts;                           ///< Number of hardware contexts.
            TraceGpuType    gpu_type;                                    ///< The GPU type.
            TraceGfxIpLevel gfx_ip_level;                                ///< The graphics IP level.
            uint32_t        gpu_index;                                   ///< The GPU Index.
            int32_t         ce_ram_size;                                 ///< Max size in bytes of CE RAM space available.
            int32_t         ce_ram_size_graphics;                        ///< Max CE RAM size available to graphics engine in bytes.
            int32_t         ce_ram_size_compute;                         ///< Max CE RAM size available to Compute engine in bytes.
            int32_t         max_number_of_dedicatedc_us;                 ///< Number of CUs dedicated to real time audio queue.
            int64_t         vram_size;                                   ///< Total number of bytes to VRAM.
            int32_t         vram_bus_width;                              ///< Width of the bus to VRAM.
            int32_t         l2_cache_size;                               ///< Total number of bytes in L2 Cache.
            int32_t         l1_cache_size;                               ///< Total number of L1 cache bytes per CU.
            int32_t         lds_size;                                    ///< Total number of LDS bytes per CU.
            char            gpu_name[TRACE_GPU_NAME_MAX_SIZE];           ///< Name of the GPU, padded to 256 bytes.
            float           alu_per_clock;                               ///< Number of ALUs per clock.
            float           texture_per_clock;                           ///< Number of texture per clock.
            float           prims_per_clock;                             ///< Number of primitives per clock.
            float           pixels_per_clock;                            ///< Number of pixels per clock.
            uint32_t        memory_ops_per_clock;                        ///< Number of memory operations per memory clock cycle.
            TraceMemoryType memory_chip_type;                            ///< The GPU memory type.
            uint32_t        lds_allocation_granularity;                  ///< LDS allocation granularity expressed in bytes.
            uint16_t        cu_mask[TRACE_MAX_NUM_SE][TRACE_SA_PER_SE];  ///< Mask of present, non-harvested CUs (physical layout).
        };

        struct TraceChunkAsicInfo_v2
        {
            uint32_t        pci_id;                                      ///< The ID of the GPU queried.
            uint64_t        shader_core_clock_frequency;                 ///< Gpu core clock frequency in Hz.
            uint64_t        memory_clock_frequency;                      ///< Memory clock frequency in Hz.
            uint64_t        gpu_timestamp_frequency;                     ///< Frequency of the gpu timestamp clock in Hz.
            uint64_t        max_shader_core_clock;                       ///< Maximum shader core clock frequency in Hz.
            uint64_t        max_memory_clock;                            ///< Maximum memory clock frequency in Hz.
            int32_t         device_id;                                   ///< Device ID.
            int32_t         device_revision_id;                          ///< Device revision ID.
            int32_t         vgprs_per_simd;                              ///< Number of VGPRs per SIMD.
            int32_t         sgprs_per_simd;                              ///< Number of SGPRs per SIMD.
            int32_t         shader_engines;                              ///< Number of shader engines.
            int32_t         compute_units_per_shader_engine;             ///< Number of compute units per shader engine.
            int32_t         simds_per_compute_unit;                      ///< Number of SIMDs per compute unit.
            int32_t         wavefronts_per_simd;                         ///< Number of wavefronts per SIMD.
            int32_t         minimum_vgpr_allocation;                     ///< Minimum number of VGPRs per wavefront.
            int32_t         vgpr_allocation_granularity;                 ///< Allocation granularity of VGPRs.
            int32_t         minimum_sgpr_allocation;                     ///< Minimum number of SGPRs per wavefront.
            int32_t         sgpr_allocation_granularity;                 ///< Allocation granularity of SGPRs.
            int32_t         hardware_contexts;                           ///< Number of hardware contexts.
            TraceGpuType    gpu_type;                                    ///< The GPU type.
            TraceGfxIpLevel gfx_ip_level;                                ///< The graphics IP level.
            uint32_t        gpu_index;                                   ///< The GPU Index.
            int32_t         ce_ram_size;                                 ///< Max size in bytes of CE RAM space available.
            int32_t         ce_ram_size_graphics;                        ///< Max CE RAM size available to graphics engine in bytes.
            int32_t         ce_ram_size_compute;                         ///< Max CE RAM size available to Compute engine in bytes.
            int32_t         max_number_of_dedicatedc_us;                 ///< Number of CUs dedicated to real time audio queue.
            int64_t         vram_size;                                   ///< Total number of bytes to VRAM.
            int32_t         vram_bus_width;                              ///< Width of the bus to VRAM.
            int32_t         l2_cache_size;                               ///< Total number of bytes in L2 Cache.
            int32_t         l1_cache_size;                               ///< Total number of L1 cache bytes per CU.
            int32_t         lds_size;                                    ///< Total number of LDS bytes per CU.
            char            gpu_name[TRACE_GPU_NAME_MAX_SIZE];           ///< Name of the GPU, padded to 256 bytes.
            float           alu_per_clock;                               ///< Number of ALUs per clock.
            float           texture_per_clock;                           ///< Number of texture per clock.
            float           prims_per_clock;                             ///< Number of primitives per clock.
            float           pixels_per_clock;                            ///< Number of pixels per clock.
            uint32_t        memory_ops_per_clock;                        ///< Number of memory operations per memory clock cycle.
            TraceMemoryType memory_chip_type;                            ///< The GPU memory type.
            uint32_t        lds_allocation_granularity;                  ///< LDS allocation granularity expressed in bytes.
            uint16_t        cu_mask[TRACE_MAX_NUM_SE][TRACE_SA_PER_SE];  ///< Mask of present, non-harvested CUs (physical layout).
        };

        struct TraceChunkAsicInfo
        {
            uint32_t        pci_id;                                      ///< The ID of the GPU queried.
            uint64_t        shader_core_clock_frequency;                 ///< Gpu core clock frequency in Hz.
            uint64_t        memory_clock_frequency;                      ///< Memory clock frequency in Hz.
            uint64_t        gpu_timestamp_frequency;                     ///< Frequency of the gpu timestamp clock in Hz.
            uint64_t        max_shader_core_clock;                       ///< Maximum shader core clock frequency in Hz.
            uint64_t        max_memory_clock;                            ///< Maximum memory clock frequency in Hz.
            int32_t         device_id;                                   ///< PCIE Device ID.
            int32_t         device_revision_id;                          ///< PCIE Device revision ID.
            int32_t         vgprs_per_simd;                              ///< Number of VGPRs per SIMD.
            int32_t         sgprs_per_simd;                              ///< Number of SGPRs per SIMD.
            int32_t         shader_engines;                              ///< Number of shader engines.
            int32_t         compute_units_per_shader_engine;             ///< Number of compute units per shader engine.
            int32_t         simds_per_compute_unit;                      ///< Number of SIMDs per compute unit.
            int32_t         wavefronts_per_simd;                         ///< Number of wavefronts per SIMD.
            int32_t         minimum_vgpr_allocation;                     ///< Minimum number of VGPRs per wavefront.
            int32_t         vgpr_allocation_granularity;                 ///< Allocation granularity of VGPRs.
            int32_t         minimum_sgpr_allocation;                     ///< Minimum number of SGPRs per wavefront.
            int32_t         sgpr_allocation_granularity;                 ///< Allocation granularity of SGPRs.
            int32_t         hardware_contexts;                           ///< Number of hardware contexts.
            TraceGpuType    gpu_type;                                    ///< The GPU type.
            TraceGfxIpLevel gfx_ip_level;                                ///< The graphics IP level.
            uint32_t        gpu_index;                                   ///< The GPU Index.
            int32_t         ce_ram_size;                                 ///< Max size in bytes of CE RAM space available.
            int32_t         ce_ram_size_graphics;                        ///< Max CE RAM size available to graphics engine in bytes.
            int32_t         ce_ram_size_compute;                         ///< Max CE RAM size available to Compute engine in bytes.
            int32_t         max_number_of_dedicatedc_us;                 ///< Number of CUs dedicated to real time audio queue.
            int64_t         vram_size;                                   ///< Total number of bytes to VRAM.
            int32_t         vram_bus_width;                              ///< Width of the bus to VRAM.
            int32_t         l2_cache_size;                               ///< Total number of bytes in L2 Cache.
            int32_t         l1_cache_size;                               ///< Total number of L1 cache bytes per CU.
            int32_t         lds_size;                                    ///< Total number of LDS bytes per CU.
            char            gpu_name[TRACE_GPU_NAME_MAX_SIZE];           ///< Name of the GPU, padded to 256 bytes.
            float           alu_per_clock;                               ///< Number of ALUs per clock.
            float           texture_per_clock;                           ///< Number of texture per clock.
            float           prims_per_clock;                             ///< Number of primitives per clock.
            float           pixels_per_clock;                            ///< Number of pixels per clock.
            uint32_t        memory_ops_per_clock;                        ///< Number of memory operations per memory clock cycle.
            TraceMemoryType memory_chip_type;                            ///< The GPU memory type.
            uint32_t        lds_allocation_granularity;                  ///< LDS allocation granularity expressed in bytes.
            uint16_t        cu_mask[TRACE_MAX_NUM_SE][TRACE_SA_PER_SE];  ///< Mask of present, non-harvested CUs (physical layout).
            uint32_t        pixel_packer_mask[4];                        ///< Mask of present, non-harvested pixel packers -- 4 bits per shader engine (up to a max of 32 shader engines)
            uint32_t        gl1_cache_size;                              ///< Total number of GL1 cache bytes per shader array
            uint32_t        inst_cache_size;                             ///< Total number of Instruction cache bytes per CU
            uint32_t        scalar_cache_size;                           ///< Total number of Scalar cache (K$) bytes per CU
            uint32_t        mall_cache_size;                             ///< Total number of MALL cache (Infinity cache) bytes
        };

    private:
        TraceChunkAsicInfo chunk_data_ = {};   ///< The chunk data.
        bool               chunk_data_valid_;  ///< Is the API info data valid.
    };

}  // namespace rra

#endif  // RRA_BACKEND_ASIC_INFO_H_
