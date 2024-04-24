//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the binary file definitions class.
//=============================================================================

#include "rt_binary_file_defs.h"

#include <iostream>
#include <vector>

#include "rdf/rdf/inc/amdrdf.h"

namespace rta
{
    RayTracingBinaryVersion::RayTracingBinaryVersion(const std::uint32_t version)
    {
        this->version = version;
    }

    std::int32_t RayTracingBinaryVersion::Compare(const RayTracingBinaryVersion& left, const RayTracingBinaryVersion& right)
    {
        if (left.GetMajor() != right.GetMajor())
        {
            return (left.GetMajor() < right.GetMajor()) ? -1 : 1;
        }
        if (left.GetMinor() != right.GetMinor())
        {
            return (left.GetMinor() < right.GetMinor()) ? -1 : 1;
        }

        return 0;
    }

    bool RayTracingBinaryVersion::operator==(const RayTracingBinaryVersion& rhs) const
    {
        return Compare(*this, rhs) == 0;
    }

    bool RayTracingBinaryVersion::operator!=(const RayTracingBinaryVersion& rhs) const
    {
        return !(*this == rhs);
    }

    bool RayTracingBinaryVersion::operator<=(const RayTracingBinaryVersion& rhs) const
    {
        return Compare(*this, rhs) <= 0;
    }

    bool RayTracingBinaryVersion::operator>=(const RayTracingBinaryVersion& rhs) const
    {
        return Compare(*this, rhs) >= 0;
    }

    bool RayTracingBinaryVersion::operator<(const RayTracingBinaryVersion& rhs) const
    {
        return Compare(*this, rhs) < 0;
    }

    bool RayTracingBinaryVersion::operator>(const RayTracingBinaryVersion& rhs) const
    {
        return Compare(*this, rhs) > 0;
    }

    std::uint16_t RayTracingBinaryVersion::GetMinor() const
    {
        return static_cast<std::uint16_t>(version);
    }

    std::uint16_t RayTracingBinaryVersion::GetMajor() const
    {
        return static_cast<std::uint16_t>(version >> 16);
    }

    struct GpuRtDriverBinaryFileVersionTuple
    {
        RayTracingBinaryVersion binary_file_version, driver_interface_version;
    };

    const std::vector<GpuRtDriverBinaryFileVersionTuple> gpu_rt_driver_binary_file_version_tuples = {
        {RayTracingBinaryVersion(1, 0), RayTracingBinaryVersion(1, 0)},
        {RayTracingBinaryVersion(2, 0), RayTracingBinaryVersion(1, 0)},
        {RayTracingBinaryVersion(2, 1), RayTracingBinaryVersion(1, 0)},
        {RayTracingBinaryVersion(3, 0), RayTracingBinaryVersion(3, 0)},
        {RayTracingBinaryVersion(4, 0), RayTracingBinaryVersion(4, 0)},
        {RayTracingBinaryVersion(4, 0), RayTracingBinaryVersion(5, 0)},
        {RayTracingBinaryVersion(4, 0), RayTracingBinaryVersion(6, 0)},
        {RayTracingBinaryVersion(5, 0), RayTracingBinaryVersion(7, 0)},
        {RayTracingBinaryVersion(5, 1), RayTracingBinaryVersion(8, 0)},
        {RayTracingBinaryVersion(5, 1), RayTracingBinaryVersion(9, 0)},
        {RayTracingBinaryVersion(5, 1), RayTracingBinaryVersion(9, 1)}};

    RayTracingBinaryVersion GetDriverGpuRtVersionFromFileVersion(const RayTracingBinaryVersion& rt_binary_file_version)
    {
        for (auto it = gpu_rt_driver_binary_file_version_tuples.rbegin(); it != gpu_rt_driver_binary_file_version_tuples.rend(); ++it)
        {
            if (rt_binary_file_version == it->binary_file_version)
            {
                return it->driver_interface_version;
            }
        }

        return RayTracingBinaryVersion(0, 0);
    }
}  // namespace rta