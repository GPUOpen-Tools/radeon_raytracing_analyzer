//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the image safety helper.
//=============================================================================

#include "vk/image_guard.h"

#include <algorithm>

namespace rra
{
    namespace renderer
    {
        void ImageGuard::Initialize(uint32_t swap_chain_length)
        {
            swap_chain_length_ = swap_chain_length;
            current_image_and_view.frame_usage.clear();
            current_image_and_view.frame_usage.resize(swap_chain_length_, false);
        }

        void ImageGuard::Cleanup(Device* device)
        {
            for (size_t i = 0; i < safeties.size(); i++)
            {
                vkDestroyImageView(device->GetDevice(), safeties[i].image_view, nullptr);
                vmaDestroyImage(device->GetAllocator(), safeties[i].image, safeties[i].allocation);
            }
            safeties.clear();
            vkDestroyImageView(device->GetDevice(), current_image_and_view.image_view, nullptr);
            vmaDestroyImage(device->GetAllocator(), current_image_and_view.image, current_image_and_view.allocation);
            current_image_and_view.frame_usage.clear();
        }

        void ImageGuard::SetCurrentImage(VkImage image, VkImageView image_view, VmaAllocation allocation)
        {
            safeties.push_back(current_image_and_view);
            current_image_and_view.image      = image;
            current_image_and_view.image_view = image_view;
            current_image_and_view.allocation = allocation;
            current_image_and_view.frame_usage.clear();
            current_image_and_view.frame_usage.resize(swap_chain_length_, false);
        }

        void ImageGuard::ProcessFrame(uint32_t current_frame, Device* device)
        {
            current_image_and_view.frame_usage[current_frame] = true;
            std::vector<ImageAndViewAllocation> swap_safeties;
            for (size_t i = 0; i < safeties.size(); i++)
            {
                safeties[i].frame_usage[current_frame] = false;
                if (std::find(safeties[i].frame_usage.begin(), safeties[i].frame_usage.end(), true) == safeties[i].frame_usage.end())
                {
                    vkDestroyImageView(device->GetDevice(), safeties[i].image_view, nullptr);
                    vmaDestroyImage(device->GetAllocator(), safeties[i].image, safeties[i].allocation);
                }
                else
                {
                    swap_safeties.push_back(safeties[i]);
                }
            }
            safeties = swap_safeties;
        }

    }  // namespace renderer
}  // namespace rra

