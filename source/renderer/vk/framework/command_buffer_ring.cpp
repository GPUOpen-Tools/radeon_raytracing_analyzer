//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the Vulkan Command Buffer Ring.
//=============================================================================

#include <cassert>

#include "device.h"
#include "command_buffer_ring.h"

namespace rra
{
    namespace renderer
    {
        void CommandBufferRing::OnCreate(Device* device, uint32_t back_buffer_count, uint32_t command_buffers_per_frame, bool compute)
        {
            device_                          = device;
            allocators_count_                = back_buffer_count;
            command_buffers_per_frame_count_ = command_buffers_per_frame;

            command_buffers_ring_ = new CommandBuffersPerFrame[allocators_count_]();

            // Create a separate allocation of command buffers for each frame in flight. Each frame will have its own pool of N buffers to use.
            for (uint32_t pool_index = 0; pool_index < allocators_count_; ++pool_index)
            {
                CommandBuffersPerFrame* per_frame_buffer = &command_buffers_ring_[pool_index];

                // Create the buffer pool.
                VkCommandPoolCreateInfo cmd_pool_info = {};
                cmd_pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                cmd_pool_info.pNext                   = nullptr;

                if (compute == false)
                {
                    cmd_pool_info.queueFamilyIndex = device->GetGraphicsQueueFamilyIndex();
                }
                else
                {
                    cmd_pool_info.queueFamilyIndex = device->GetComputeQueueFamilyIndex();
                }

                cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

                VkResult result = vkCreateCommandPool(device->GetDevice(), &cmd_pool_info, nullptr, &per_frame_buffer->command_pool_);
                CheckResult(result, "Failed to create command buffer pool.");

                // Create the command buffers in the current pool.
                per_frame_buffer->command_buffers_ = new VkCommandBuffer[command_buffers_per_frame_count_];
                VkCommandBufferAllocateInfo cmd    = {};
                cmd.sType                          = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                cmd.pNext                          = nullptr;
                cmd.commandPool                    = per_frame_buffer->command_pool_;
                cmd.level                          = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                cmd.commandBufferCount             = command_buffers_per_frame;

                result = vkAllocateCommandBuffers(device->GetDevice(), &cmd, per_frame_buffer->command_buffers_);
                CheckResult(result, "Failed to allocate command buffers.");

                cmd.commandBufferCount = 1;

                result = vkAllocateCommandBuffers(device->GetDevice(), &cmd, &per_frame_buffer->upload_command_buffer_);
                CheckResult(result, "Failed to allocate command buffers.");

                per_frame_buffer->num_buffers_used_ = 0;
            }

            current_frame_index_  = 0;
            current_ring_buffers_ = &command_buffers_ring_[current_frame_index_ % allocators_count_];
            current_frame_index_++;
            current_ring_buffers_->num_buffers_used_ = 0;
        }

        void CommandBufferRing::OnDestroy()
        {
            // Release and delete the command buffer pool.
            for (uint32_t pool_index = 0; pool_index < allocators_count_; pool_index++)
            {
                vkFreeCommandBuffers(device_->GetDevice(),
                                     command_buffers_ring_[pool_index].command_pool_,
                                     command_buffers_per_frame_count_,
                                     command_buffers_ring_[pool_index].command_buffers_);

                vkDestroyCommandPool(device_->GetDevice(), command_buffers_ring_[pool_index].command_pool_, nullptr);
            }

            delete[] command_buffers_ring_;
        }

        VkCommandBuffer CommandBufferRing::GetNewCommandBuffer()
        {
            VkCommandBuffer command_buffer = current_ring_buffers_->command_buffers_[current_ring_buffers_->num_buffers_used_++];

            // If this is hit, increase command_buffers_per_frame_count_.
            assert(current_ring_buffers_->num_buffers_used_ < command_buffers_per_frame_count_);

            return command_buffer;
        }

        VkCommandBuffer CommandBufferRing::GetUploadCommandBuffer()
        {
            return current_ring_buffers_->upload_command_buffer_;
        }

        VkCommandPool CommandBufferRing::GetPool() const
        {
            return command_buffers_ring_->command_pool_;
        }

        void CommandBufferRing::OnBeginFrame()
        {
            current_ring_buffers_ = &command_buffers_ring_[current_frame_index_ % allocators_count_];

            current_ring_buffers_->num_buffers_used_ = 0;

            current_frame_index_++;
        }

    }  // namespace renderer
}  // namespace rra
