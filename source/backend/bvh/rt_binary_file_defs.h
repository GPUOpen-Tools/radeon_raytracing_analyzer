//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Header for the binary file definitions class.
//=============================================================================

#ifndef RRA_BACKEND_BVH_RT_BINARY_FILE_DEFS_H_
#define RRA_BACKEND_BVH_RT_BINARY_FILE_DEFS_H_

#include "gpu_def.h"

#include <cstdint>

namespace rdf
{
    class Stream;
}

namespace rta
{
    // Ray tracing binary file type enumeration
    enum class RayTracingBinaryFileType : std::uint32_t
    {
        kUnknown = 0,
        kRayHistory,
        kTraversalCounter,
        kBvhRaw,
        kBvhDecoded,
    };

    // Minor and major version is defined in gpurt.h
    struct RayTracingBinaryVersion final
    {
        /// @brief Constructor.
        RayTracingBinaryVersion() = default;

        /// @brief Destructor.
        RayTracingBinaryVersion(const std::uint32_t version);

        /// @brief Constructor.
        ///
        /// @param [in] major The major version number.
        /// @param [in] minor The minor version number.
        constexpr RayTracingBinaryVersion(const std::uint16_t major, const std::uint16_t minor)
            : version_minor(minor)
            , version_major(major)
        {
        }

        /// @brief Compare two ray tracing binary file header versions.
        ///
        /// @param [in] left  The first version to compare.
        /// @param [in] right The version to compare to the first.
        ///
        /// @return -1 if the left version is smaller than right, 0 if they are equal, or 1 if
        /// the left version is newer than right.
        static std::int32_t Compare(const RayTracingBinaryVersion& left, const RayTracingBinaryVersion& right);

        /// @brief equality operator.
        bool operator==(const RayTracingBinaryVersion& rhs) const;

        /// @brief not equal operator.
        bool operator!=(const RayTracingBinaryVersion& rhs) const;

        /// @brief less than or equal operator.
        bool operator<=(const RayTracingBinaryVersion& rhs) const;

        /// @brief greater than or equal operator.
        bool operator>=(const RayTracingBinaryVersion& rhs) const;

        /// @brief less than operator.
        bool operator<(const RayTracingBinaryVersion& rhs) const;

        /// @brief greater than operator.
        bool operator>(const RayTracingBinaryVersion& rhs) const;

        std::uint16_t version_minor = 0;  ///< minor version number.
        std::uint16_t version_major = 0;  ///< major version number.
    };

    constexpr RayTracingBinaryVersion kSupportedRayTracingBinaryHeaderVersion =
        RayTracingBinaryVersion(5, 1);  ///< The currently supported RT binary file version.

    constexpr RayTracingBinaryVersion kMinimumSupportedRayTracingBinaryHeaderVersion =
        RayTracingBinaryVersion(2, 0);  ///< The minimum supported RT binary file version.

    /// @brief The API driver format the dump was created with.
    enum class RayTracingBinaryFileApiFormat : std::int32_t
    {
        Dxc    = 0,
        Vulkan = 1
    };

    /// @brief Status of the RT binary file support check.
    enum class RtFileCheckStatus : std::int32_t
    {
        kIdentifierMismatch = -2,  ///< Required identifier could not be found. The file presumably does not contain any header.
        kFileTypeMismatch   = -1,  ///< File type mismatch
        kSupported,                ///< The version specified in the header is fully supported.
        kNewerVersionDetected  ///< A newer version has been detected and could imply major changes in the BVH format or its headers. Support is not guaranteed for these files.
    };

    /// @brief Get the driver's GPURT interface version from the stored binary RT file version.
    ///
    /// @param [in] rt_binary_file_version The binary file version.
    ///
    /// @return the GPURT interface version.
    RayTracingBinaryVersion GetDriverGpuRtVersionFromFileVersion(const RayTracingBinaryVersion& rt_binary_file_version);

}  // namespace rta

#endif  // RRA_BACKEND_BVH_RT_BINARY_FILE_DEFS_H_
