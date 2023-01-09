//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the buffer safety helper.
//=============================================================================

#ifndef RRA_RENDERER_VK_BUFFER_GUARD_H_
#define RRA_RENDERER_VK_BUFFER_GUARD_H_

#include <vector>
#include "vk/framework/device.h"

namespace rra
{
    namespace renderer
    {
        /// @brief A helper class to keep buffers safe during rendering.
        class BufferGuard
        {
        public:
            /// @brief Initialize the safeties by the amount of swapchain elements.
            ///
            /// @param [in] swap_chain_length The length of the swap chain.
            void Initialize(uint32_t swap_chain_length);

            /// @brief Cleanup all the buffers.
            ///
            /// @param [in] device The device to cleanup on.
            void Cleanup(Device* device);

            /// @brief Set the current buffer of interest that is being repeatedly used.
            ///
            /// @param [in] buffer The Vulkan buffer object we are currently using.
            /// @param [in] allocation The VMA Allocation we are currently using.
            void SetCurrentBuffer(VkBuffer buffer, VmaAllocation allocation);

            /// @brief Process the current frame and free unused buffers.
            ///
            /// @param [in] current_frame The index of the current frame.
            /// @param [in] device The device that the frames are rendered on.
            void ProcessFrame(uint32_t current_frame, Device* device);

            /// @brief Get the current buffer.
            ///
            /// @returns The current buffer.
            VkBuffer GetCurrentBuffer() const;

            /// @brief Get the current allocation.
            ///
            /// @returns The current allocations.
            VmaAllocation GetCurrentAllocation() const;

        private:
            /// @brief The buffer allocation struct to check for buffer usage to ensure memory safety.
            struct BufferAllocation
            {
                VkBuffer          buffer     = VK_NULL_HANDLE;
                VmaAllocation     allocation = VK_NULL_HANDLE;
                std::vector<bool> frame_usage;
            };

            std::vector<BufferAllocation> safeties;                 ///< The array to hold old buffers until their usage goes down to 0.
            BufferAllocation              current_buffer     = {};  ///< The current buffer
            uint32_t                      swap_chain_length_ = 0;   ///< The swap chain length.
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_BUFFER_SAFETY_H_
