//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition for a node pointer class.
//=============================================================================

#ifndef RRA_BACKEND_BVH_NODE_POINTER_H_
#define RRA_BACKEND_BVH_NODE_POINTER_H_

#include "bvh/dxr_definitions.h"

namespace dxr
{
    namespace amd
    {
        /// @brief uint32_t data containing the address (offset in bytes to the header) and the node type.
        ///
        /// NodePointer is used to traverse the BVH forward / backward.
        /// The class functions provide methods to check the type and obtain the ID
        /// of the node / primitive.
        class NodePointer final
        {
        public:
            /// @brief Constructor.
            NodePointer();

            /// @brief Constructor.
            ///
            /// @param type The node type.
            /// @param address The node address.
            NodePointer(const NodeType type, const std::uint32_t address);

            /// @brief Constructor.
            ///
            /// @param pointer The raw node pointer
            NodePointer(const std::uint32_t pointer);

            /// @brief Destructor.
            ~NodePointer() = default;

            /// @brief Get byte offset of the node pointer relative to the start of the acceleration structure header.
            ///
            /// @return The byte offset.
            std::uint32_t GetByteOffset() const;

            /// @brief Obtain the index to the link data using the aligned meta data size and the compression mode
            ///
            /// @param parent_data_size The size of the parent data.
            /// @param compression_mode The compression type.
            ///
            /// @return The link index.
            std::uint32_t CalculateParentLinkIndex(const std::uint32_t parent_data_size, const TriangleCompressionMode compression_mode) const;

            /// @brief Get type of node pointer.
            ///
            /// @return The node pointer type.
            NodeType GetType() const;

            /// @brief Get the node pointer virtual address.
            ///
            /// @return The virtual address.
            std::uint32_t GetGpuVirtualAddress() const;

            /// @brief Get the ID of this node (ID = type).
            ///
            /// @param offset The node pointer offset.
            ///
            /// @return The node ID.
            std::uint32_t GetID(const std::uint32_t offset = 0) const;

            /// @brief Get offset of the parent node in the parent data block starting
            /// from the end memory address of the meta data of each acceleration structure.
            ///
            /// @param compression_mode The compression type.
            ///
            /// @return The offset of the parent node.
            std::uint32_t GetParentByteOffset(const TriangleCompressionMode compression_mode) const;

            /// @brief Is this node a triangle node.
            ///
            /// @return true if it's a triangle node, false if not.
            bool IsTriangleNode() const;

            /// @brief Get the start of the triangle pair range.
            /// The end of the range is determined by the stop bit in the triangle pair descriptor.
            ///
            /// @return Triangle pair index.
            uint32_t GetTrianglePairIndex() const;

            /// @brief Is this node a box node.
            ///
            /// @return true if it's a box node, false if not.
            bool IsBoxNode() const;

            /// @brief Is this node a leaf node.
            ///
            /// @return true if it's a leaf node, false if not.
            bool IsLeafNode() const;

            /// @brief Is this node a box32 node.
            ///
            /// @return true if it's a box32 node, false if not.
            bool IsFp32BoxNode() const;

            /// @brief Is this node a box16 node.
            ///
            /// @return true if it's a box16 node, false if not.
            bool IsFp16BoxNode() const;

            /// @brief Is this node a root node.
            ///
            /// @return true if it's a root node, false if not.
            bool IsRoot() const;

            /// @brief Is this node an instance node.
            ///
            /// @return true if it's an instance node, false if not.
            bool IsInstanceNode() const;

            /// @brief Is this node invalid node.
            ///
            /// @return true if it's an invalid node, false if not.
            bool IsInvalid() const;

            /// @brief The the raw node pointer.
            ///
            /// @return The raw node pointer.
            std::uint32_t GetRawPointer() const;

        private:
            uint32_t GetAddress() const;

            uint32_t GetShiftedAddress() const;

            union
            {
                // This union makes it easier to quickly retrieve the node type
                // and index of each node pointer (especially for debugging).
                struct
                {
                    std::uint32_t type_ : 3;      ///< Type of node (internal, leaf).
                    std::uint32_t address_ : 29;  ///< Byte offset (uses 29 bits).
                };
                struct
                {
                    std::uint32_t rtip31_type_ : 4;
                    std::uint32_t rtip31_address_ : 28;
                };
                std::uint32_t ptr_;  ///< The node pointer.
            };
        };

    }  // namespace amd
}  // namespace dxr

#endif  // RRA_BACKEND_BVH_NODE_POINTER_H_
