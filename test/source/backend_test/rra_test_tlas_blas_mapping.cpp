//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Backend test TLAS to BLAS mappings.
//=============================================================================

#include "rra_test_tlas_blas_mapping.h"

#include <limits.h>
#include <deque>

#include "glm/glm/glm.hpp"

#ifndef _WIN32
#include "public/linux/safe_crt.h"
#endif

#include "public/rra_blas.h"
#include "public/rra_blas_header.h"
#include "public/rra_bvh.h"
#include "public/rra_macro.h"
#include "public/rra_rtip_info.h"
#include "public/rra_tlas.h"
#include "public/rra_tlas_header.h"

#include "rra_test_config.h"

namespace backend_test
{
    RRATestTlasBlasMapping::RRATestTlasBlasMapping(const std::string& test_name)
        : RRATestBase(test_name)
    {
    }

    RRATestTlasBlasMapping::~RRATestTlasBlasMapping()
    {
    }

    bool RRATestTlasBlasMapping::RunTests(const RRATestConfig& config)
    {
        AddTestStartMessage(config.log);

        uint64_t     tlas_count = 0;
        RraErrorCode error_code = RraBvhGetTlasCount(&tlas_count);
        if (error_code != kRraOk)
        {
            config.log.Write(" ERROR - Can't get TLAS count");
            return false;
        }

        // Iterate over the TLAS interior nodes and for each instance, see what BLAS it's pointing to.
        for (size_t tlas_index = 0; tlas_index < tlas_count; tlas_index++)
        {
            uint32_t header_metadata_size = 0;
            uint32_t tlas_metadata_size   = 0;

            if (RraTlasHeaderGetMetaDataSize(tlas_index, &header_metadata_size) != kRraOk)
            {
                config.log.Write(" ERROR - Can't get metadata size from TLAS header");
                return false;
            }

            if (RraTlasGetMetaDataSize(tlas_index, &tlas_metadata_size) != kRraOk)
            {
                config.log.Write(" ERROR - Can't get metadata size from TLAS");
                return false;
            }

            if (header_metadata_size != tlas_metadata_size)
            {
                config.log.Write(
                    " ERROR - BLAS[%d] meta data sizes don't match (reader reports %d, TLAS reports %d)", tlas_index, header_metadata_size, tlas_metadata_size);
            }

            bool result = GetTLASStats(tlas_index, config);
            if (result == false)
            {
                AddTestResultMessage(result, config.log);
                return result;
            }
        }

        bool result = GetBLASStats(config);
        if (result == false)
        {
            AddTestResultMessage(result, config.log);
            return result;
        }

        uint64_t blas_count = 0;
        if (RraBvhGetBlasCount(&blas_count) != kRraOk)
        {
            config.log.Write(" ERROR - can't get blas count");
            return false;
        }

        AddTestResultMessage(true, config.log);
        return true;
    }

    bool RRATestTlasBlasMapping::GetTLASStats(const size_t tlas_index, const RRATestConfig& config)
    {
        uint32_t interior_count = 0;
        if (RraTlasHeaderGetInteriorNodeCount(tlas_index, &interior_count) != kRraOk)
        {
            config.log.Write(" ERROR - can't get interior node count from TLAS[%d] header", tlas_index);
            return false;
        }

        uint32_t leaf_count = 0;
        if (RraTlasHeaderGetLeafNodeCount(tlas_index, &leaf_count) != kRraOk)
        {
            config.log.Write(" ERROR - can't get leaf node count from TLAS[%d] header", tlas_index);
            return false;
        }

        rta::GpuVirtualAddress src_addr = 0;
        if (RraTlasGetBaseAddress(tlas_index, &src_addr) != kRraOk)
        {
            config.log.Write(" ERROR - can't get base address from TLAS[%d]", tlas_index);
            return false;
        }

        uint32_t meta_data_size = 0;
        if (RraTlasHeaderGetMetaDataSize(tlas_index, &meta_data_size) != kRraOk)
        {
            config.log.Write(" ERROR - Can't get metadata size from TLAS[%d] header", tlas_index);
            return false;
        }

        // For each BLAS in the TLAS, get the number of instances.
        uint64_t blas_count;
        if (RraTlasGetBlasCount(tlas_index, &blas_count) != kRraOk)
        {
            config.log.Write(" ERROR - Can't get blas count from TLAS[%d]", tlas_index);
            return false;
        }

        for (uint64_t blas_index = 0; blas_index < blas_count; blas_index++)
        {
            uint64_t instance_count = 0;
            RraTlasGetInstanceCount(tlas_index, blas_index, &instance_count);

            for (uint64_t instance_index = 0; instance_index < instance_count; instance_index++)
            {
                uint32_t node_ptr = 0;
                if (RraTlasGetInstanceNode(tlas_index, blas_index, instance_index, &node_ptr) != kRraOk)
                {
                    config.log.Write(" ERROR - Can't get instance node from TLAS[%d] and BLAS[%d]", tlas_index, blas_index);
                    return false;
                }

                glm::mat4 inverse_transform = {};
                if (RraTlasGetInstanceNodeTransform(tlas_index, node_ptr, reinterpret_cast<float*>(&inverse_transform)) != kRraOk)
                {
                    config.log.Write(
                        " ERROR - Can't get instance node transform from TLAS[%d] and BLAS[%d], instance[%d]", tlas_index, blas_index, instance_index);
                    return false;
                }

                glm::mat4 transform = {};

                if (RraTlasGetOriginalInstanceNodeTransform(tlas_index, node_ptr, reinterpret_cast<float*>(&transform)) != kRraOk)
                {
                    config.log.Write(
                        " ERROR - Can't get original instance node transform from TLAS[%d] and BLAS[%d], instance[%d]", tlas_index, blas_index, instance_index);
                    return false;
                }
            }
        }

        // Compare instance count with value reported in the header.
        uint64_t total_instance_count = 0;
        if (RraTlasGetInstanceNodeCount(tlas_index, &total_instance_count) != kRraOk)
        {
            config.log.Write(" ERROR - can't get instance node count from TLAS[%d]", tlas_index);
            return false;
        }

        uint32_t header_primitive_count = 0;
        if (RraTlasHeaderGetPrimitiveCount(tlas_index, &header_primitive_count) != kRraOk)
        {
            config.log.Write(" ERROR - can't get primitive count from TLAS[%d] header", tlas_index);
            return false;
        }

        if (total_instance_count != header_primitive_count)
        {
            config.log.Write(" WARNING - mismatch between instance count in header (%d) and instance count in TLAS[%d](%llu)",
                             header_primitive_count,
                             tlas_index,
                             total_instance_count);
        }

        return true;
    }

    bool RRATestTlasBlasMapping::GetBLASStats(const RRATestConfig& config)
    {
#ifdef RRA_INTRNAL
        if (config.dump_stats)
        {
            blas_dump_file_.Write("BLAS Data");
            blas_dump_file_.Write("=========");
        }
#endif

        struct NodeAttributes
        {
            uint32_t num_interior_nodes = 0;
            uint32_t num_leaf_nodes     = 0;
        };
        NodeAttributes max_leaf_count     = {};
        NodeAttributes max_interior_count = {};

        uint64_t     blas_count = 0;
        RraErrorCode error_code = RraBvhGetBlasCount(&blas_count);
        RRA_ASSERT(error_code == kRraOk);

        for (size_t i = 0; i < blas_count; i++)
        {
            uint32_t interior_count = 0;
            if (RraBlasHeaderGetInteriorNodeCount(i, &interior_count) != kRraOk)
            {
                config.log.Write(" ERROR - can't get interior node count from BLAS[%d] header", i);
                return false;
            }

            uint32_t leaf_count = 0;
            if (RraBlasHeaderGetLeafNodeCount(i, &leaf_count) != kRraOk)
            {
                config.log.Write(" ERROR - can't get leaf node count from BLAS[%d] header", i);
                return false;
            }

            rta::GpuVirtualAddress src_addr = 0;
            if (RraBlasGetBaseAddress(i, &src_addr) != kRraOk)
            {
                config.log.Write(" ERROR - can't get base address from BLAS[%d]", i);
                return false;
            }

            uint32_t meta_data_size = 0;
            if (RraBlasHeaderGetMetaDataSize(i, &meta_data_size) != kRraOk)
            {
                config.log.Write(" ERROR - Can't get metadata size from BLAS[%d] header", i);
                return false;
            }

            // Keep track of the max number of interior nodes (and its corresponding number of leaf nodes) and
            // the max number of leaf nodes (and its corresponding number of interior nodes).
            if (interior_count > max_interior_count.num_interior_nodes)
            {
                max_interior_count.num_interior_nodes = interior_count;
                max_interior_count.num_leaf_nodes     = leaf_count;
            }

            if (leaf_count > max_leaf_count.num_leaf_nodes)
            {
                max_leaf_count.num_leaf_nodes     = leaf_count;
                max_leaf_count.num_interior_nodes = interior_count;
            }
        }

        return true;
    }

}  // namespace backend_test
