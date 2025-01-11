//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the parent ptr data block.
//=============================================================================

#include "parent_block.h"

#include <cassert>

#include "node_pointer.h"
#include "public/rra_rtip_info.h"

namespace dxr
{
    namespace amd
    {
        ParentBlock::ParentBlock(const std::uint32_t               internal_node_buffer_size,
                                 const std::uint32_t               leaf_node_buffer_size,
                                 dxr::amd::TriangleCompressionMode compression_mode)
        {
            Construct(internal_node_buffer_size, leaf_node_buffer_size, compression_mode);
        }

        std::uint32_t ParentBlock::GetSizeInBytes() const
        {
            return byte_size_;
        }

        std::uint32_t ParentBlock::GetLinkCount() const
        {
            return link_count_;
        }

        std::vector<NodePointer>& ParentBlock::GetLinkData()
        {
            return links_;
        }

        const std::vector<NodePointer>& ParentBlock::GetLinkData() const
        {
            return links_;
        }

        void ParentBlock::Construct(const std::uint32_t                     internal_node_buffer_size,
                                    const std::uint32_t                     leaf_node_buffer_size,
                                    const dxr::amd::TriangleCompressionMode compression_mode)
        {
            uint32_t parent_chunk_size{kParentChunkSize};

            if ((rta::RayTracingIpLevel)RraRtipInfoGetRaytracingIpLevel() == rta::RayTracingIpLevel::RtIp3_1)
            {
                parent_chunk_size = kParentChunkSizeRtIp31;
            }

            links_per_block_count_ =
                (compression_mode == TriangleCompressionMode::kAmdPairTriangleCompression) ? 1 : 1 << static_cast<uint32_t>(compression_mode);
            const auto num_64_byte_links = (internal_node_buffer_size + leaf_node_buffer_size) / parent_chunk_size;
            link_count_                  = num_64_byte_links * links_per_block_count_;
            byte_size_                   = link_count_ * sizeof(NodePointer);

            // Resize links buffer and zero out all entries.
            links_.resize(link_count_, NodePointer(NodeType::kAmdNodeTriangle0, 0));
        }

    }  // namespace amd
}  // namespace dxr
