//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the Vulkan Command Buffer Ring.
//=============================================================================

#ifndef RRA_RENDERER_VK_FRAMEWORK_COMMAND_BUFFER_RING_H_
#define RRA_RENDERER_VK_FRAMEWORK_COMMAND_BUFFER_RING_H_

namespace rra
{
    namespace renderer
    {
        /// @brief Declaration of the Command Buffer Ring class.
        ///
        /// The Command Buffer Ring manages the pool of command buffers used to assemble GPU operations per frame.
        /// Command buffers used in previous frames are recycled when they are no longer used by the GPU.
        class CommandBufferRing
        {
        public:
            /// @brief Initialize the pool of command buffers upon creation.
            ///
            /// @param [in] device The device used to create the command buffer pools.
            /// @param [in] back_buffer_count The back buffer count. Each frame will get its own command buffer pool.
            /// @param [in] command_buffers_per_frame The number of command buffers in each frame's pool.
            /// @param [in] compute When set to true, the command buffers should be submitted to the compute queue.
            void OnCreate(Device* device, uint32_t back_buffer_count, uint32_t command_buffers_per_frame, bool compute = false);

            /// @brief Handle destruction of the command buffer ring.
            void OnDestroy();

            /// @brief Begin a new frame. This advances the ring index to use the next set of frame command buffers.
            void OnBeginFrame();

            /// @brief Retrieve a new command buffer from the current frame's pool.
            ///
            /// @returns A command buffer ready to be used to build the current frame.
            VkCommandBuffer GetNewCommandBuffer();

            /// @brief Retrieve an existing command buffer from the current frame's pool.
            ///
            /// @returns A command buffer ready to be used to upload resources.
            VkCommandBuffer GetUploadCommandBuffer();

            /// @brief Retrieve the command pool used for the current frame.
            ///
            /// @returns The command pool for the current frame.
            VkCommandPool GetPool() const;

        private:
            uint32_t current_frame_index_             = {};  ///< The current frame index, which will be used to index the ring buffer.
            uint32_t allocators_count_                = {};  ///< The number of frames in flight that need a command buffer pool.
            uint32_t command_buffers_per_frame_count_ = {};  ///< The number of command buffers in each frame's pool.

            struct CommandBuffersPerFrame
            {
                VkCommandPool    command_pool_          = VK_NULL_HANDLE;  ///< The pool that command buffers are allocated from.
                VkCommandBuffer* command_buffers_       = VK_NULL_HANDLE;  ///< The array of command buffers in the pool.
                VkCommandBuffer  upload_command_buffer_ = VK_NULL_HANDLE;  ///< The command buffer to use for uploading resources.
                uint32_t         num_buffers_used_      = {};              ///< A count of how many command buffers have been used in the frame.
            };

            CommandBuffersPerFrame* command_buffers_ring_ = nullptr;  ///< The array of command buffer pools in the ring.
            CommandBuffersPerFrame* current_ring_buffers_ = nullptr;  ///< The current command buffer pool to use for the frame.
            Device*                 device_               = nullptr;  ///< The device used to create the pool of command buffers.
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_FRAMEWORK_COMMAND_BUFFER_RING_H_
