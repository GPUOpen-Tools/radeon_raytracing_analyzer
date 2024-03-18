//=============================================================================
// Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the image safety helper.
//=============================================================================

#ifndef RRA_RENDERER_VK_IMAGE_GUARD_H_
#define RRA_RENDERER_VK_IMAGE_GUARD_H_

#include <vector>
#include "vk/framework/device.h"

namespace rra
{
    namespace renderer
    {
        /// @brief A helper class to keep images safe during rendering.
        class ImageGuard
        {
        public:
            /// @brief Initialize the safeties by the amount of swapchain elements.
            ///
            /// @param [in] swap_chain_length The length of the swap chain.
            void Initialize(uint32_t swap_chain_length);

            /// @brief Cleanup all the image data.
            ///
            /// @param [in] device The device to cleanup on.
            void Cleanup(Device* device);

            /// @brief Set the current buffer of interest that is being repeatedly used.
            ///
            /// @param [in] image The Vulkan image object we are currently using.
            /// @param [in] image_view The Vulkan image view object we are currently using.
            /// @param [in] allocation The VMA Allocation we are currently using.
            void SetCurrentImage(VkImage image, VkImageView image_view, VmaAllocation allocation);

            /// @brief Process the current frame and free unused buffers.
            ///
            /// @param [in] current_frame The index of the current frame.
            /// @param [in] device The device that the frames are rendered on.
            void ProcessFrame(uint32_t current_frame, Device* device);

        private:
            /// @brief The buffer allocation struct to check for buffer usage to ensure memory safety.
            struct ImageAndViewAllocation
            {
                VkImage           image      = VK_NULL_HANDLE;
                VkImageView       image_view = VK_NULL_HANDLE;
                VmaAllocation     allocation = VK_NULL_HANDLE;
                std::vector<bool> frame_usage;
            };

            std::vector<ImageAndViewAllocation> safeties;                     ///< The array to hold old buffers until their usage goes down to 0.
            ImageAndViewAllocation              current_image_and_view = {};  ///< The current image and view.
            uint32_t                            swap_chain_length_     = 0;   ///< The swap chain length.
        };
    }  // namespace renderer
}  // namespace rra

#endif
