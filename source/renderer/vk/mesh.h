//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the instanced mesh renderer.
//=============================================================================

#ifndef RRA_RENDERER_VK_MESH_H_
#define RRA_RENDERER_VK_MESH_H_

#include "util_vulkan.h"
#include "../public/renderer_types.h"

#include "framework/device.h"
#include "framework/command_buffer_ring.h"
#include "framework/ext_debug_utils.h"

namespace rra
{
    namespace renderer
    {
        /// @brief A vertex with a position field.
        struct VertexPosition
        {
            /// @brief Constructor.
            ///
            /// @param [in] position The vertex position.
            VertexPosition(glm::vec3 position)
                : position(position)
            {
            }

            glm::vec3 position;  ///< The vertex position.
        };

        /// @brief A vertex with position and normal fields.
        struct VertexPositionNormal
        {
            /// @brief Constructor.
            ///
            /// @param [in] position The vertex position.
            /// @param [in] normal The vertex normal.
            VertexPositionNormal(glm::vec3 position, glm::vec3 normal)
                : position(position)
                , normal(normal)
            {
            }

            glm::vec3 position;  ///< The vertex position.
            glm::vec3 normal;    ///< The vertex normal.
        };

        /// @brief Declaration of the Mesh type.
        ///
        /// The Mesh type is templated to utilize any arbitrary Vertex structure type.
        /// It can upload geometric data data to buffer resources in device memory.
        template <typename Vertex_Type>
        class TypedMesh
        {
        public:
            /// @brief Constructor.
            TypedMesh() = default;

            /// @brief Destructor.
            virtual ~TypedMesh() = default;

            /// @brief Declare a typedef so the vertex type can be easily queried.
            typedef Vertex_Type VertexType;

        public:
            /// @brief Initialize the mesh geometry and upload to GPU memory.
            ///
            /// @param [in] device The GPU device to upload data to.
            /// @param [in] command_list_ring The ring to retrieve a new command buffer from.
            void Initialize(Device* device, CommandBufferRing* command_list_ring)
            {
                std::vector<uint16_t>    index_buffer;
                std::vector<Vertex_Type> vertex_buffer;

                // Fill the geometry arrays with vertex and index data.
                GenerateGeometry(index_buffer, vertex_buffer);

                if (vertex_buffer.size() > 0)
                {
                    // Upload the buffer data to GPU memory.
                    UploadGeometryBuffers(device, command_list_ring, index_buffer, vertex_buffer);
                }
            }

            /// @brief Cleanup the buffers in the device.
            ///
            /// @param [in] device The GPU device to cleanup data from.
            void Cleanup(Device* device)
            {
                device->DestroyBuffer(vertices_.buffer, vertices_.allocation);
                device->DestroyBuffer(indices_.buffer, indices_.allocation);
            }

            struct Vertices
            {
                int           count;
                VkBuffer      buffer     = VK_NULL_HANDLE;
                VmaAllocation allocation = VK_NULL_HANDLE;
            };

            struct Indices
            {
                int           count;
                VkBuffer      buffer     = VK_NULL_HANDLE;
                VmaAllocation allocation = VK_NULL_HANDLE;
            };

            /// @brief Retrieve a reference to the vertex data.
            ///
            /// @returns A reference to the vertex data.
            const Vertices& GetVertices() const
            {
                return vertices_;
            }

            /// @brief Retrieve a reference to the index data.
            ///
            /// @returns A reference to the index data.
            const Indices& GetIndices() const
            {
                return indices_;
            }

            /// @brief Get the triangle count for the mesh.
            ///
            /// @returns The triangle count for the mesh.
            uint32_t GetTriangleCount() const
            {
                return triangle_count_;
            }

        protected:
            /// @brief Populate the provided vertex and index buffer arrays with geometric data.
            ///
            /// @param [out] indices The index buffer data.
            /// @param [out] vertices The vertex buffer data.
            virtual void GenerateGeometry(std::vector<uint16_t>& indices, std::vector<Vertex_Type>& vertices) = 0;

            uint32_t triangle_count_ = 0;   ///< The number of triangles in the mesh.
            Vertices vertices_       = {};  ///< The vertex buffer resource info.
            Indices  indices_        = {};  ///< The index buffer resource info.

        private:
            /// @brief Use staging buffers to upload the provided vertex and index buffer data to GPU memory.
            ///
            /// @param [in] device The graphics device used to upload the data.
            /// @param [in] command_buffer_ring The command list ring used to retrieve a new command list from.
            /// @param [in] index_buffer The index buffer data to upload.
            /// @param [in] vertex_buffer The vertex buffer data to upload.
            void UploadGeometryBuffers(Device*                         device,
                                       CommandBufferRing*              command_buffer_ring,
                                       const std::vector<uint16_t>&    index_buffer,
                                       const std::vector<Vertex_Type>& vertex_buffer)
            {
                size_t vertex_buffer_size = vertex_buffer.size() * sizeof(Vertex_Type);
                size_t index_buffer_size  = index_buffer.size() * sizeof(uint16_t);
                indices_.count            = static_cast<uint32_t>(index_buffer.size());
                vertices_.count           = static_cast<uint32_t>(vertex_buffer.size());

                assert(vertex_buffer_size > 0);

                struct StagingBuffer
                {
                    VkBuffer      buffer;
                    VmaAllocation allocation;
                };

                StagingBuffer vertex_staging = {};
                StagingBuffer index_staging  = {};

                VkMemoryPropertyFlags memory_property_flags = 0;

                bool use_index_buffer = index_buffer_size > 0;
                if (use_index_buffer)
                {
                    // Create an index buffer upload staging resource.
                    device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                         VMA_MEMORY_USAGE_CPU_ONLY,
                                         index_staging.buffer,
                                         index_staging.allocation,
                                         index_buffer.data(),
                                         index_buffer_size);

                    // Create the destination index buffer.
                    device->CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | memory_property_flags,
                                         VMA_MEMORY_USAGE_GPU_ONLY,
                                         indices_.buffer,
                                         indices_.allocation,
                                         nullptr,
                                         index_buffer_size);
                }

                // Create a vertex buffer upload staging resource.
                device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                     VMA_MEMORY_USAGE_CPU_ONLY,
                                     vertex_staging.buffer,
                                     vertex_staging.allocation,
                                     vertex_buffer.data(),
                                     vertex_buffer_size);

                // Create the destination vertex buffer.
                device->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | memory_property_flags,
                                     VMA_MEMORY_USAGE_GPU_ONLY,
                                     vertices_.buffer,
                                     vertices_.allocation,
                                     nullptr,
                                     vertex_buffer_size);

                VkCommandBuffer copy_command_buffer = command_buffer_ring->GetUploadCommandBuffer();
                SetObjectName(device->GetDevice(), VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)copy_command_buffer, "copyCommandBuffer");

                VkCommandBufferBeginInfo begin_info = {};
                begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                VkResult begin_result               = vkBeginCommandBuffer(copy_command_buffer, &begin_info);
                CheckResult(begin_result, "Failed to begin command buffer.");

                SetObjectName(device->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)vertex_staging.buffer, "meshVertexStagingBuffer");
                SetObjectName(device->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)vertices_.buffer, "meshVertexBuffer");
                SetObjectName(device->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)index_staging.buffer, "meshIndexStagingBuffer");
                SetObjectName(device->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)indices_.buffer, "meshIndexBuffer");

                // Copy the resource data from staging buffers.
                VkBufferCopy copy_region = {};
                copy_region.size         = vertex_buffer_size;
                vkCmdCopyBuffer(copy_command_buffer, vertex_staging.buffer, vertices_.buffer, 1, &copy_region);

                if (use_index_buffer)
                {
                    copy_region.size = index_buffer_size;
                    vkCmdCopyBuffer(copy_command_buffer, index_staging.buffer, indices_.buffer, 1, &copy_region);
                }

                // Submit the command buffer and wait for execution to complete.
                device->FlushCommandBuffer(copy_command_buffer, device->GetGraphicsQueue(), command_buffer_ring->GetPool(), false);

                // Destroy the staging resources.
                device->DestroyBuffer(vertex_staging.buffer, vertex_staging.allocation);

                if (use_index_buffer)
                {
                    device->DestroyBuffer(index_staging.buffer, index_staging.allocation);
                }
            }
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_MESH_H_
