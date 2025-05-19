//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the buffer safety helper.
//=============================================================================

#include "vk/buffer_guard.h"

#include <algorithm>

namespace rra
{
    namespace renderer
    {
        void BufferGuard::Initialize(uint32_t swap_chain_length)
        {
            swap_chain_length_ = swap_chain_length;
            current_buffer.frame_usage.clear();
            current_buffer.frame_usage.resize(swap_chain_length_, false);
        }

        void BufferGuard::Cleanup(Device* device)
        {
            for (size_t i = 0; i < safeties.size(); i++)
            {
                device->DestroyBuffer(safeties[i].buffer, safeties[i].allocation);
            }
            safeties.clear();
            device->DestroyBuffer(current_buffer.buffer, current_buffer.allocation);
            current_buffer.frame_usage.clear();
        }

        void BufferGuard::SetCurrentBuffer(VkBuffer buffer, VmaAllocation allocation)
        {
            safeties.push_back(current_buffer);
            current_buffer.buffer     = buffer;
            current_buffer.allocation = allocation;
            current_buffer.frame_usage.clear();
            current_buffer.frame_usage.resize(swap_chain_length_, false);
        }

        void BufferGuard::ProcessFrame(uint32_t current_frame, Device* device)
        {
            current_buffer.frame_usage[current_frame] = true;
            std::vector<BufferAllocation> swap_safeties;
            for (size_t i = 0; i < safeties.size(); i++)
            {
                safeties[i].frame_usage[current_frame] = false;
                if (std::find(safeties[i].frame_usage.begin(), safeties[i].frame_usage.end(), true) == safeties[i].frame_usage.end())
                {
                    device->DestroyBuffer(safeties[i].buffer, safeties[i].allocation);
                }
                else
                {
                    swap_safeties.push_back(safeties[i]);
                }
            }
            safeties = swap_safeties;
        }

        VkBuffer BufferGuard::GetCurrentBuffer() const
        {
            return current_buffer.buffer;
        }

        VmaAllocation BufferGuard::GetCurrentAllocation() const
        {
            return current_buffer.allocation;
        }

    }  // namespace renderer
}  // namespace rra

