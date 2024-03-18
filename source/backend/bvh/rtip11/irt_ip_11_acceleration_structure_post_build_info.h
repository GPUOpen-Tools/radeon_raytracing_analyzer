//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the acceleration structure build info class.
//=============================================================================

#ifndef RRA_BACKEND_BVH_IRT_IP_11_ACCELERATION_STRUCTURE_POST_BUILD_INFO_H_
#define RRA_BACKEND_BVH_IRT_IP_11_ACCELERATION_STRUCTURE_POST_BUILD_INFO_H_

#include <cassert>
#include <memory>  // unique_ptr

#include "public/rra_macro.h"

#include "bvh/dxr_definitions.h"

namespace rta
{
    // This interface provides all the information about the pre-build settings for this acceleration
    // structure. Allows the client to query and set different settings.
    // This includes the general type of the BVH (top level, bottom level), the builder type (CPU / GPU),
    // the build mode (PLOC), triangle compression mode, fp16 box node mode, general DXR build flags set
    // by the app, and triangle splitting.
    class IRtIp11AccelerationStructurePostBuildInfo
    {
    public:
        /// @brief Constructor.
        IRtIp11AccelerationStructurePostBuildInfo() = default;

        /// @brief Destructor.
        virtual ~IRtIp11AccelerationStructurePostBuildInfo();

        /// @brief Set the BVH type.
        ///
        /// @param [in] type The BVH type.
        void SetBvhType(const BvhType type);

        /// @brief Get the BVH type.
        ///
        /// @return The BVH type.
        BvhType GetBvhType() const;

        /// @brief Is this a top level acceleration structure.
        ///
        /// @return true if this is a top level acceleration structure, false if not.
        bool IsTopLevel() const;

        /// @brief Is this a bottom level acceleration structure.
        ///
        /// @return true if this is a bottom level acceleration structure, false if not.
        bool IsBottomLevel() const;

        /// @brief Set the triangle compression mode for the acceleration structures.
        ///
        /// @param [in] compression_mode The triangle compression mode.
        void SetTriangleCompressionMode(const BvhTriangleCompressionMode compression_mode);

        /// @brief Get the triangle compression mode for the acceleration structures.
        ///
        /// @return The triangle compression mode.
        BvhTriangleCompressionMode GetTriangleCompressionMode() const;

        /// @brief Set the mode used for FP16 in the bottom level acceleration structures.
        ///
        /// @param [in] mode The FP16 mode.
        void SetBottomLevelFp16Mode(const BvhLowPrecisionInteriorNodeMode mode);

        /// @brief Get the mode used for FP16 in the bottom level acceleration structures.
        ///
        /// @return The FP16 mode.
        BvhLowPrecisionInteriorNodeMode GetBottomLevelFp16Mode() const;

        /// @brief Set the build flags used when constructing the BVH.
        ///
        /// @param [in] flags The build flags.
        void SetBuildFlags(const BvhBuildFlags flags);

        /// @brief Get the build flags used when constructing the BVH.
        ///
        /// @return The build flags.
        BvhBuildFlags GetBuildFlags() const;

        /// @brief Get whether rebraiding was enabled for this BVH.
        ///
        /// @return True if rebraiding enabled, false otherwise.
        bool GetRebraiding() const;

        /// @brief Get the if this BVH construction was with triangle splitting.
        ///
        /// @return The triangle splitting flag.
        std::uint32_t GetTriangleSplitting() const;

        /// @brief Load the data for this class from a buffer.
        ///
        /// @param [in] size The size of the buffer, in bytes.
        /// @param [in] buffer The buffer to load from.
        void LoadFromBuffer(std::size_t size, void* buffer);

        /// @brief Save the data in this class to a buffer.
        ///
        /// @param [in] buffer The buffer to save the data to.
        void SaveToBuffer(void* buffer) const;

        /// @brief Get whether fused instances was enabled for this BVH.
        ///
        /// @return True if fused instances enabled, false otherwise.
        bool GetFusedInstances() const;

    private:
        virtual void SetBvhTypeImpl(BvhType type) = 0;

        virtual BvhType GetBvhTypeImpl() const = 0;

        virtual void SetTriangleCompressionModeImpl(const BvhTriangleCompressionMode compression_mode) = 0;

        virtual BvhTriangleCompressionMode GetTriangleCompressionModeImpl() const = 0;

        virtual void SetBottomLevelFp16ModeImpl(const BvhLowPrecisionInteriorNodeMode mode) = 0;

        virtual BvhLowPrecisionInteriorNodeMode GetBottomLevelFp16ModeImpl() const = 0;

        virtual void SetBuildFlagsImpl(const BvhBuildFlags flags) = 0;

        virtual BvhBuildFlags GetBuildFlagsImpl() const = 0;

        virtual bool GetRebraidingImpl() const = 0;

        virtual bool GetFusedInstancesImpl() const = 0;

        virtual std::uint32_t GetTriangleSplittingImpl() const = 0;

        virtual void LoadFromBufferImpl(std::size_t size, void* buffer) = 0;

        virtual void SaveToBufferImpl(void* buffer) const = 0;
    };

    // Create a new RT IP 1.1 acceleration BVH post-build info. This information can be stored in the
    // acceleration structure header.
    std::unique_ptr<IRtIp11AccelerationStructurePostBuildInfo> CreateRtIp11AccelerationStructurePostBuildInfo();

}  // namespace rta

#endif  // RRA_BACKEND_BVH_IRT_IP_11_ACCELERATION_STRUCTURE_POST_BUILD_INFO_H_
