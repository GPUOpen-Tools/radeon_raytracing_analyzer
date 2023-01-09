//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of the parent ptr data block.
//=============================================================================

#ifndef RRA_BACKEND_BVH_PARENT_BLOCK_H_
#define RRA_BACKEND_BVH_PARENT_BLOCK_H_

#include <vector>

#include "bvh/dxr_definitions.h"
#include "bvh/node_pointer.h"

namespace dxr
{
    namespace amd
    {
        class ParentBlock final
        {
        public:
            /// @brief Constructor.
            ParentBlock() = default;

            /// @brief Constructor.
            ///
            /// @param internal_node_buffer_size The size of the internal node buffer.
            /// @param leaf_node_buffer_size The size of the leaf node buffer.
            /// @param compression_mode The compression type.
            ParentBlock(const std::uint32_t               internal_node_buffer_size,
                        const std::uint32_t               leaf_node_buffer_size,
                        dxr::amd::TriangleCompressionMode compression_mode);

            /// @brief Destructor.
            ~ParentBlock() = default;

            /// @brief Get the size of the links data.
            ///
            /// @return The links size, in bytes.
            std::uint32_t GetSizeInBytes() const;

            /// @brief Get the link count.
            ///
            /// @return The link count.
            std::uint32_t GetLinkCount() const;

            /// @brief Obtain the array of link data node pointers.
            ///
            /// @return The link data.
            std::vector<NodePointer>& GetLinkData();

            /// @brief Obtain the array of link data node pointers.
            ///
            /// @return The link data.
            const std::vector<NodePointer>& GetLinkData() const;

        private:
            /// @brief Initialize the parent block data.
            ///
            /// @param internal_node_buffer_size The size of the internal node buffer.
            /// @param leaf_node_buffer_size The size of the leaf node buffer.
            /// @param compression_mode The compression type.
            void Construct(const std::uint32_t                     internal_node_buffer_size,
                           const std::uint32_t                     leaf_node_buffer_size,
                           const dxr::amd::TriangleCompressionMode compression_mode);

            std::uint32_t             byte_size_             = 0;   ///< The size of the links array, in bytes.
            std::uint32_t             links_per_block_count_ = 0;   ///< The number of links per block.
            std::uint32_t             link_count_            = 0;   ///< The number of links.
            std::vector<NodePointer>  links_                 = {};  ///< The array of links.
            std::vector<std::uint8_t> sentry_values_         = {};  ///< Sentry values;
        };

    }  // namespace amd
}  // namespace dxr

#endif  // RRA_BACKEND_BVH_PARENT_BLOCK_H_
