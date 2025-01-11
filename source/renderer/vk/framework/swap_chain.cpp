//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the Vulkan swapchain object.
//=============================================================================

#include <array>
#include <cassert>

#include <volk/volk.h>

#include "swap_chain.h"
#include "../util_vulkan.h"
#include "../framework/ext_debug_utils.h"

#include "public/rra_print.h"

namespace rra
{
    namespace renderer
    {
        VkFormat kDepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

        /// @brief Sets the swapchain format by querying the physical device and surface.
        void SetSwapchainFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkSurfaceFormatKHR& surface_format)
        {
            uint32_t surface_format_count;

            // Get the surface format count.
            CheckResult(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, nullptr),
                        "Could not retreive the surface format count.");

            // Get the supported surface formats.
            std::vector<VkSurfaceFormatKHR> format_pairs(surface_format_count);
            CheckResult(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, format_pairs.data()),
                        "Could not retreive the surface formats.");

            if (format_pairs.empty())
            {
                CheckResult(VK_ERROR_FORMAT_NOT_SUPPORTED, "The given surface has no supported formats");
            }

            // Use the desired format if available.
            for (auto pair : format_pairs)
            {
                if (pair.format == VK_FORMAT_B8G8R8A8_UNORM && pair.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    surface_format.format     = VK_FORMAT_B8G8R8A8_UNORM;
                    surface_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                    return;
                }
            }

            // If the desired format not available, pick the first one that is.
            surface_format.format     = format_pairs[0].format;
            surface_format.colorSpace = format_pairs[0].colorSpace;

            RraPrint("Desired surface format not available, defaulting to first available: %d(format), %d(color space)",
                     surface_format.format,
                     surface_format.colorSpace);
        }

        void SwapChain::OnCreate(Device* device, uint32_t back_buffer_count, const WindowInfo* window_info)
        {
            auto possible_msaa_samples = device->GetPossibleMSAASampleSettings();
            if (!possible_msaa_samples.empty())
            {
                msaa_samples_ = possible_msaa_samples.front();  // Settings are descending so 0th index is the highest possible setting.
            }

            if (msaa_samples_ == VK_SAMPLE_COUNT_1_BIT)
            {
                CheckResult(VK_ERROR_FORMAT_NOT_SUPPORTED, "No MSAA capabilities are present on this device.");
            }

            VkResult result;

#if defined(_WIN32)
            // Create a Win32 Surface.
            VkWin32SurfaceCreateInfoKHR surface_create_info = {};
            surface_create_info.sType                       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            surface_create_info.pNext                       = nullptr;
            surface_create_info.hinstance                   = nullptr;
            surface_create_info.hwnd                        = window_info->window_handle;
            result                                          = vkCreateWin32SurfaceKHR(device->GetInstance(), &surface_create_info, nullptr, &surface_);
            CheckResult(result, "Could not create Win32 surface.");
#elif defined(VK_USE_PLATFORM_XCB_KHR)
            // Create the XCB surface.
            VkXcbSurfaceCreateInfoKHR surface_create_info = {};
            surface_create_info.sType                     = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
            surface_create_info.pNext                     = nullptr;
            surface_create_info.flags                     = 0;
            surface_create_info.connection                = window_info->connection;
            surface_create_info.window                    = window_info->window;
            result                                        = vkCreateXcbSurfaceKHR(device->GetInstance(), &surface_create_info, nullptr, &surface_);
            CheckResult(result, "Could not create XCB surface.");
#endif

            // Used to supress validation layer warnings about surface.
            VkBool32 supports_present;
            vkGetPhysicalDeviceSurfaceSupportKHR(device->GetPhysicalDevice(), 0, surface_, &supports_present);

            window_info_          = window_info;
            device_               = device;
            back_buffer_count_    = back_buffer_count;
            semaphore_index_      = 0;
            prev_semaphore_index_ = 0;

            present_queue_ = device->GetPresentQueue();

            // Initialize the image format.
            SetSwapchainFormat(device->GetPhysicalDevice(), surface_, swapchain_format_);

            VkDevice device_handle = device_->GetDevice();

            // Resize the vectors of handles used to track frame completion.
            command_buffer_executed_fences_.resize(back_buffer_count_);
            image_available_semaphores_.resize(back_buffer_count_);
            render_finished_semaphores_.resize(back_buffer_count_);

            for (uint32_t back_buffer_index = 0; back_buffer_index < back_buffer_count_; back_buffer_index++)
            {
                // Create the fences used to check that command buffer execution is complete.
                VkFenceCreateInfo fence_create_info = {};
                fence_create_info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                fence_create_info.pNext             = nullptr;
                fence_create_info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

                result = vkCreateFence(device_handle, &fence_create_info, nullptr, &command_buffer_executed_fences_[back_buffer_index]);
                CheckResult(result, "Failed to create swapchain command buffer execution fence.");

                VkSemaphoreCreateInfo image_acquired_semaphore_create_info = {};
                image_acquired_semaphore_create_info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                image_acquired_semaphore_create_info.pNext                 = nullptr;
                image_acquired_semaphore_create_info.flags                 = 0;

                result = vkCreateSemaphore(device_handle, &image_acquired_semaphore_create_info, nullptr, &image_available_semaphores_[back_buffer_index]);
                CheckResult(result, "Failed to create swapchain image availability semaphore.");

                result = vkCreateSemaphore(device_handle, &image_acquired_semaphore_create_info, nullptr, &render_finished_semaphores_[back_buffer_index]);
                CheckResult(result, "Failed to create swapchain image acquisition semaphore.");
            }

            CreateRenderPass();
        }

        void SwapChain::OnDestroy()
        {
            OnDestroyWindowSizeDependentResources();

            if (surface_ != VK_NULL_HANDLE)
            {
                vkDestroySurfaceKHR(device_->GetInstance(), surface_, nullptr);
                surface_ = VK_NULL_HANDLE;
            }

            DestroyRenderPass();

            for (size_t image_index = 0; image_index < command_buffer_executed_fences_.size(); image_index++)
            {
                vkDestroyFence(device_->GetDevice(), command_buffer_executed_fences_[image_index], nullptr);
                vkDestroySemaphore(device_->GetDevice(), image_available_semaphores_[image_index], nullptr);
                vkDestroySemaphore(device_->GetDevice(), render_finished_semaphores_[image_index], nullptr);
            }
        }

        VkImage SwapChain::GetCurrentBackBuffer()
        {
            return images_[image_index_];
        }

        VkSwapchainKHR SwapChain::GetSwapChain() const
        {
            return swapchain_;
        }

        VkFormat SwapChain::GetFormat() const
        {
            return swapchain_format_.format;
        }

        VkRenderPass SwapChain::GetRenderPass(RenderPassHint render_pass_hint)
        {
            switch (render_pass_hint)
            {
            case RenderPassHint::kRenderPassHintClearColorAndDepth:
                return render_pass_clear_color_and_depth_;
            case RenderPassHint::kRenderPassHintClearDepthOnly:
                return render_pass_clear_depth_only_;
            case RenderPassHint::kRenderPassHintClearNone:
                return render_pass_clear_none_;
            case RenderPassHint::kRenderPassHintClearNoneDepthInput:
                return render_pass_clear_none_depth_input_;
            case RenderPassHint::kRenderPassHintResolve:
                return render_pass_resolve_;
            default:
                return render_pass_clear_none_;
            }
        }

        VkFramebuffer SwapChain::GetFramebuffer(RenderPassHint render_pass_hint, int image_index) const
        {
            switch (render_pass_hint)
            {
            case RenderPassHint::kRenderPassHintClearColorAndDepth:
                return frame_buffers_clear_color_and_depth_[image_index];
            case RenderPassHint::kRenderPassHintClearDepthOnly:
                return frame_buffers_clear_depth_only_[image_index];
            case RenderPassHint::kRenderPassHintClearNone:
                return frame_buffers_clear_none_[image_index];
            case RenderPassHint::kRenderPassHintClearNoneDepthInput:
                return frame_buffers_clear_none_depth_input_[image_index];
            case RenderPassHint::kRenderPassHintResolve:
                return frame_buffers_resolve_[image_index];
            default:
                return frame_buffers_clear_none_[image_index];
            }
        }

        uint32_t SwapChain::GetImageIndex() const
        {
            return image_index_;
        }

        VkImage SwapChain::GetDepthStencilImage() const
        {
            return depth_stencil_.image;
        }

        uint32_t SwapChain::GetBackBufferCount() const
        {
            return back_buffer_count_;
        }

        VkSampleCountFlagBits SwapChain::GetMSAASamples() const
        {
            return msaa_samples_;
        }

        VkImageView SwapChain::GetCurrentBackBufferRTV()
        {
            return image_views_[image_index_];
        }

        uint32_t SwapChain::WaitForSwapChain()
        {
            prev_semaphore_index_ = semaphore_index_;
            semaphore_index_++;

            if (semaphore_index_ >= back_buffer_count_)
            {
                semaphore_index_ = 0;
            }

            vkWaitForFences(device_->GetDevice(), 1, &command_buffer_executed_fences_[semaphore_index_], VK_TRUE, UINT64_MAX);
            vkResetFences(device_->GetDevice(), 1, &command_buffer_executed_fences_[semaphore_index_]);

            vkAcquireNextImageKHR(
                device_->GetDevice(), swapchain_, UINT64_MAX, image_available_semaphores_[prev_semaphore_index_], VK_NULL_HANDLE, &image_index_);

            return image_index_;
        }

        void SwapChain::GetSemaphores(VkSemaphore* image_available_semaphore, VkSemaphore* render_finished_semaphore, VkFence* cmd_buffer_executed_fences)
        {
            *image_available_semaphore  = image_available_semaphores_[prev_semaphore_index_];
            *render_finished_semaphore  = render_finished_semaphores_[semaphore_index_];
            *cmd_buffer_executed_fences = command_buffer_executed_fences_[semaphore_index_];
        }

        void SwapChain::Present()
        {
            VkPresentInfoKHR present_info   = {};
            present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            present_info.pNext              = nullptr;
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores    = &(render_finished_semaphores_[semaphore_index_]);
            present_info.swapchainCount     = 1;
            present_info.pSwapchains        = &swapchain_;
            present_info.pImageIndices      = &image_index_;
            present_info.pResults           = nullptr;

            VkResult result = vkQueuePresentKHR(present_queue_, &present_info);
            if (result != VK_SUBOPTIMAL_KHR)
            {
                CheckResult(result, "Present was not VK_SUCCESS.");
            }
        }

        void SwapChain::OnCreateWindowSizeDependentResources(uint32_t width, uint32_t height, bool v_sync_enabled)
        {
            SetSwapchainFormat(device_->GetPhysicalDevice(), surface_, swapchain_format_);
            v_sync_enabled_ = v_sync_enabled;

            swapchain_extent_.width  = width;
            swapchain_extent_.height = height;

            VkResult         result;
            VkDevice         device_handle  = device_->GetDevice();
            VkPhysicalDevice physicaldevice = device_->GetPhysicalDevice();

            // Get the surface capabilities.
            VkSurfaceCapabilitiesKHR surface_capabilities = {};

            result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicaldevice, surface_, &surface_capabilities);
            CheckResult(result, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR was not VK_SUCCESS.");

            // Clamp the swapchain extents to the physical hardware limits.
            if (swapchain_extent_.width < surface_capabilities.minImageExtent.width)
            {
                swapchain_extent_.width = surface_capabilities.minImageExtent.width;
            }
            else if (swapchain_extent_.width > surface_capabilities.maxImageExtent.width)
            {
                swapchain_extent_.width = surface_capabilities.maxImageExtent.width;
            }

            if (swapchain_extent_.height < surface_capabilities.minImageExtent.height)
            {
                swapchain_extent_.height = surface_capabilities.minImageExtent.height;
            }
            else if (swapchain_extent_.height > surface_capabilities.maxImageExtent.height)
            {
                swapchain_extent_.height = surface_capabilities.maxImageExtent.height;
            }

            // Destroy and re-create all resources they rely on a specific rendering resolution, since it has just changed.
            DestroyRenderPass();

            DestroyColorImage();

            DestroyDepthStencil();

            CreateColorImage();

            CreateDepthStencil();

            CreateRenderPass();

            // Set identity transform.
            VkSurfaceTransformFlagBitsKHR pre_ptransform = (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
                                                               ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
                                                               : surface_capabilities.currentTransform;

            // Find a supported composite alpha mode - one of these is guaranteed to be set
            VkCompositeAlphaFlagBitsKHR composite_alpha          = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            VkCompositeAlphaFlagBitsKHR composite_alpha_flags[4] = {
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
            };

            for (uint32_t flag_index = 0; flag_index < sizeof(flag_index); flag_index++)
            {
                if (surface_capabilities.supportedCompositeAlpha & composite_alpha_flags[flag_index])
                {
                    composite_alpha = composite_alpha_flags[flag_index];
                    break;
                }
            }

            // Get the present modes.
            uint32_t present_mode_count;
            result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicaldevice, surface_, &present_mode_count, nullptr);
            CheckResult(result, "Failed to enumerate physical device surface present modes.");

            std::vector<VkPresentModeKHR> present_modes(present_mode_count);
            result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicaldevice, surface_, &present_mode_count, &present_modes[0]);
            CheckResult(result, "Failed to populate physical device surface present mode array.");

            VkSwapchainCreateInfoKHR swapchain_create_info = {};
            swapchain_create_info.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapchain_create_info.pNext                    = nullptr;
            swapchain_create_info.surface                  = surface_;
            swapchain_create_info.imageFormat              = swapchain_format_.format;
            swapchain_create_info.minImageCount            = back_buffer_count_;
            swapchain_create_info.imageColorSpace          = swapchain_format_.colorSpace;
            swapchain_create_info.imageExtent.width        = swapchain_extent_.width;
            swapchain_create_info.imageExtent.height       = swapchain_extent_.height;
            swapchain_create_info.preTransform             = pre_ptransform;
            swapchain_create_info.compositeAlpha           = composite_alpha;
            swapchain_create_info.imageArrayLayers         = 1;
            swapchain_create_info.presentMode =
                v_sync_enabled_ ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;  // The FIFO present mode is guaranteed by the spec to be supported
            swapchain_create_info.oldSwapchain          = VK_NULL_HANDLE;
            swapchain_create_info.clipped               = true;
            swapchain_create_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            swapchain_create_info.queueFamilyIndexCount = 0;
            swapchain_create_info.pQueueFamilyIndices   = nullptr;

            uint32_t queue_family_indices[2] = {device_->GetGraphicsQueueFamilyIndex(), device_->GetPresentQueueFamilyIndex()};
            if (queue_family_indices[0] != queue_family_indices[1])
            {
                // If the graphics and present queues are from different queue families,
                // we either have to explicitly transfer ownership of images between the
                // queues, or we have to create the swapchain with imageSharingMode
                // as VK_SHARING_MODE_CONCURRENT.
                swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
                swapchain_create_info.queueFamilyIndexCount = 2;
                swapchain_create_info.pQueueFamilyIndices   = queue_family_indices;
            }

            result = vkCreateSwapchainKHR(device_handle, &swapchain_create_info, nullptr, &swapchain_);
            CheckResult(result, "Failed to create swapchain.");

            // We are querying the swapchain count so the next call doesn't generate a validation warning.
            uint32_t back_buffer_count;
            result = vkGetSwapchainImagesKHR(device_handle, swapchain_, &back_buffer_count, nullptr);
            CheckResult(result, "Failed to query swapchain image count.");
            assert(back_buffer_count == back_buffer_count_);

            images_.resize(back_buffer_count_);
            result = vkGetSwapchainImagesKHR(device_handle, swapchain_, &back_buffer_count_, images_.data());
            CheckResult(result, "Failed to populate swapchain image array.");

            CreateRTV();
            CreateFramebuffers(swapchain_extent_.width, swapchain_extent_.height);

            image_index_ = 0;
        }

        void SwapChain::OnDestroyWindowSizeDependentResources()
        {
            DestroyColorImage();
            DestroyDepthStencil();
            DestroyRenderPass();
            DestroyFramebuffers();
            DestroyRTV();

            if (swapchain_ != VK_NULL_HANDLE)
            {
                vkDestroySwapchainKHR(device_->GetDevice(), swapchain_, nullptr);
            }
        }

        void SwapChain::DestroyColorImage()
        {
            if (color_image_.view != VK_NULL_HANDLE)
            {
                vkDestroyImageView(device_->GetDevice(), color_image_.view, nullptr);
                color_image_.view = VK_NULL_HANDLE;
            }

            if (color_image_.image != VK_NULL_HANDLE)
            {
                vmaDestroyImage(device_->GetAllocator(), color_image_.image, color_image_.allocation);
                color_image_.image      = VK_NULL_HANDLE;
                color_image_.allocation = VK_NULL_HANDLE;
            }
        }

        void SwapChain::DestroyDepthStencil()
        {
            if (depth_stencil_.view != VK_NULL_HANDLE)
            {
                vkDestroyImageView(device_->GetDevice(), depth_stencil_.view, nullptr);
                depth_stencil_.view = VK_NULL_HANDLE;
            }

            if (depth_stencil_.image != VK_NULL_HANDLE)
            {
                vmaDestroyImage(device_->GetAllocator(), depth_stencil_.image, depth_stencil_.allocation);
                depth_stencil_.image      = VK_NULL_HANDLE;
                depth_stencil_.allocation = VK_NULL_HANDLE;
            }

            if (geometry_depth_copy_.view != VK_NULL_HANDLE)
            {
                vkDestroyImageView(device_->GetDevice(), geometry_depth_copy_.view, nullptr);
                geometry_depth_copy_.view = VK_NULL_HANDLE;
            }

            if (geometry_depth_copy_.image != VK_NULL_HANDLE)
            {
                vmaDestroyImage(device_->GetAllocator(), geometry_depth_copy_.image, geometry_depth_copy_.allocation);
                geometry_depth_copy_.image      = VK_NULL_HANDLE;
                geometry_depth_copy_.allocation = VK_NULL_HANDLE;
            }
        }

        void SwapChain::CreateColorImage()
        {
            VkImageCreateInfo image_create_info = {};
            image_create_info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            image_create_info.imageType         = VK_IMAGE_TYPE_2D;
            image_create_info.format            = swapchain_format_.format;
            image_create_info.extent            = {swapchain_extent_.width, swapchain_extent_.height, 1};
            image_create_info.mipLevels         = 1;
            image_create_info.arrayLayers       = 1;
            image_create_info.samples           = msaa_samples_;
            image_create_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
            image_create_info.usage             = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            VmaAllocationCreateInfo alloc_info = {};
            alloc_info.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;

            VkResult create_result =
                vmaCreateImage(device_->GetAllocator(), &image_create_info, &alloc_info, &color_image_.image, &color_image_.allocation, nullptr);
            CheckResult(create_result, "Failed to create color image.");

            VkImageViewCreateInfo image_view_create_info           = {};
            image_view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            image_view_create_info.image                           = color_image_.image;
            image_view_create_info.format                          = swapchain_format_.format;
            image_view_create_info.subresourceRange.baseMipLevel   = 0;
            image_view_create_info.subresourceRange.levelCount     = 1;
            image_view_create_info.subresourceRange.baseArrayLayer = 0;
            image_view_create_info.subresourceRange.layerCount     = 1;
            image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;

            create_result = vkCreateImageView(device_->GetDevice(), &image_view_create_info, nullptr, &color_image_.view);
            CheckResult(create_result, "Failed to create color image view.");
        }

        void SwapChain::CreateDepthStencil()
        {
            VkImageCreateInfo image_create_info = {};
            image_create_info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            image_create_info.imageType         = VK_IMAGE_TYPE_2D;
            image_create_info.format            = kDepthFormat;
            image_create_info.extent            = {swapchain_extent_.width, swapchain_extent_.height, 1};
            image_create_info.mipLevels         = 1;
            image_create_info.arrayLayers       = 1;
            image_create_info.samples           = msaa_samples_;
            image_create_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
            image_create_info.usage             = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

            VmaAllocationCreateInfo alloc_info = {};
            alloc_info.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;

            VkResult create_result =
                vmaCreateImage(device_->GetAllocator(), &image_create_info, &alloc_info, &depth_stencil_.image, &depth_stencil_.allocation, nullptr);
            CheckResult(create_result, "Failed to create depth stencil image.");

            image_create_info.usage = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

            create_result = vmaCreateImage(
                device_->GetAllocator(), &image_create_info, &alloc_info, &geometry_depth_copy_.image, &geometry_depth_copy_.allocation, nullptr);
            CheckResult(create_result, "Failed to create depth input attachment image.");

            VkImageViewCreateInfo image_view_create_info           = {};
            image_view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            image_view_create_info.image                           = depth_stencil_.image;
            image_view_create_info.format                          = kDepthFormat;
            image_view_create_info.subresourceRange.baseMipLevel   = 0;
            image_view_create_info.subresourceRange.levelCount     = 1;
            image_view_create_info.subresourceRange.baseArrayLayer = 0;
            image_view_create_info.subresourceRange.layerCount     = 1;
            image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;

            // Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
            if (kDepthFormat >= VK_FORMAT_D16_UNORM_S8_UINT)
            {
                image_view_create_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }

            create_result = vkCreateImageView(device_->GetDevice(), &image_view_create_info, nullptr, &depth_stencil_.view);
            CheckResult(create_result, "Failed to create depth stencil image view.");

            image_view_create_info.image                       = geometry_depth_copy_.image;
            image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            create_result = vkCreateImageView(device_->GetDevice(), &image_view_create_info, nullptr, &geometry_depth_copy_.view);
            CheckResult(create_result, "Failed to create depth input attachment image view.");
        }

        void SwapChain::CreateRenderPass()
        {
            // Regular render passes.
            {
                enum AttachmentIndexType
                {
                    kAttachmentIndexColor,
                    kAttachmentIndexDepthOut,
                    kAttachmentIndexDepthInput,
                };

                VkAttachmentReference color_reference = {};
                color_reference.attachment            = kAttachmentIndexColor;
                color_reference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentReference depth_reference = {};
                depth_reference.attachment            = kAttachmentIndexDepthOut;
                depth_reference.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkAttachmentReference depth_input_reference = {};
                depth_input_reference.attachment            = kAttachmentIndexDepthInput;
                depth_input_reference.layout                = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkSubpassDescription subpass_description    = {};
                subpass_description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass_description.colorAttachmentCount    = 1;
                subpass_description.pColorAttachments       = &color_reference;
                subpass_description.pDepthStencilAttachment = &depth_reference;
                subpass_description.inputAttachmentCount    = 0;
                subpass_description.pInputAttachments       = nullptr;
                subpass_description.preserveAttachmentCount = 0;
                subpass_description.pPreserveAttachments    = nullptr;
                subpass_description.pResolveAttachments     = nullptr;

                VkSubpassDescription subpass_depth_input_description{subpass_description};
                subpass_depth_input_description.inputAttachmentCount = 1;
                subpass_depth_input_description.pInputAttachments    = &depth_input_reference;

                // Subpass dependencies for layout transitions.
                std::array<VkSubpassDependency, 3> dependencies;

                dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
                dependencies[0].dstSubpass      = 0;
                dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
                dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                dependencies[1].srcSubpass      = 0;
                dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
                dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
                dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                dependencies[2].srcSubpass      = VK_SUBPASS_EXTERNAL;
                dependencies[2].dstSubpass      = 0;
                dependencies[2].srcStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependencies[2].dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependencies[2].srcAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                dependencies[2].dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                // Color attachment.
                VkAttachmentDescription color_attachment{};
                color_attachment.format         = swapchain_format_.format;
                color_attachment.samples        = msaa_samples_;
                color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
                color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
                color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                color_attachment.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                color_attachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                // Depth attachment.
                VkAttachmentDescription depth_attachment{};
                depth_attachment.format         = kDepthFormat;
                depth_attachment.samples        = msaa_samples_;
                depth_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
                depth_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
                depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                depth_attachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                // Depth input attachment.
                VkAttachmentDescription depth_input_attachment{};
                depth_input_attachment.format         = kDepthFormat;
                depth_input_attachment.samples        = msaa_samples_;
                depth_input_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
                depth_input_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                depth_input_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depth_input_attachment.initialLayout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                depth_input_attachment.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                std::array<VkAttachmentDescription, 2> attachments             = {color_attachment, depth_attachment};
                std::array<VkAttachmentDescription, 3> attachments_depth_input = {color_attachment, depth_attachment, depth_input_attachment};

                VkRenderPassCreateInfo render_pass_info = {};
                render_pass_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                render_pass_info.attachmentCount        = static_cast<uint32_t>(attachments.size());
                render_pass_info.pAttachments           = attachments.data();
                render_pass_info.subpassCount           = 1;
                render_pass_info.pSubpasses             = &subpass_description;
                render_pass_info.dependencyCount        = static_cast<uint32_t>(dependencies.size());
                render_pass_info.pDependencies          = dependencies.data();

                VkRenderPassCreateInfo render_pass_depth_input_info = {};
                render_pass_depth_input_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                render_pass_depth_input_info.attachmentCount        = static_cast<uint32_t>(attachments_depth_input.size());
                render_pass_depth_input_info.pAttachments           = attachments_depth_input.data();
                render_pass_depth_input_info.subpassCount           = 1;
                render_pass_depth_input_info.pSubpasses             = &subpass_depth_input_description;
                render_pass_depth_input_info.dependencyCount        = static_cast<uint32_t>(dependencies.size());
                render_pass_depth_input_info.pDependencies          = dependencies.data();

                VkResult create_result;

                // Transition layout to clear all, unique to the clear_color_and_depth render pass.
                attachments[kAttachmentIndexColor].initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
                attachments[kAttachmentIndexColor].finalLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachments[kAttachmentIndexDepthOut].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachments[kAttachmentIndexDepthOut].finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                // Clear both color and depth.
                attachments[kAttachmentIndexColor].loadOp    = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachments[kAttachmentIndexDepthOut].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                create_result = vkCreateRenderPass(device_->GetDevice(), &render_pass_info, nullptr, &render_pass_clear_color_and_depth_);
                CheckResult(create_result, "Render pass (render_pass_clear_color_and_depth_) creation failed");
                SetObjectName(device_->GetDevice(), VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)render_pass_clear_color_and_depth_, "renderPassClearColorAndDepth");

                // Retain layout for anything other than clear_color_and_depth.
                attachments[kAttachmentIndexColor].initialLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachments[kAttachmentIndexColor].finalLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachments[kAttachmentIndexDepthOut].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                attachments[kAttachmentIndexDepthOut].finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                // Clear depth only.
                attachments[kAttachmentIndexColor].loadOp    = VK_ATTACHMENT_LOAD_OP_LOAD;
                attachments[kAttachmentIndexDepthOut].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                create_result = vkCreateRenderPass(device_->GetDevice(), &render_pass_info, nullptr, &render_pass_clear_depth_only_);
                CheckResult(create_result, "Render pass (render_pass_clear_depth_only_) creation failed");
                SetObjectName(device_->GetDevice(), VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)render_pass_clear_depth_only_, "renderPassClearDepthOnly");

                // Clear none.
                attachments[kAttachmentIndexColor].loadOp    = VK_ATTACHMENT_LOAD_OP_LOAD;
                attachments[kAttachmentIndexDepthOut].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                create_result                                = vkCreateRenderPass(device_->GetDevice(), &render_pass_info, nullptr, &render_pass_clear_none_);
                CheckResult(create_result, "Render pass (render_pass_clear_none_) creation failed");
                SetObjectName(device_->GetDevice(), VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)render_pass_clear_none_, "renderPassClearNone");

                // Clear none and use depth input attachment.
                attachments_depth_input[kAttachmentIndexColor].loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD;
                attachments_depth_input[kAttachmentIndexDepthOut].loadOp   = VK_ATTACHMENT_LOAD_OP_LOAD;
                attachments_depth_input[kAttachmentIndexDepthInput].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                create_result = vkCreateRenderPass(device_->GetDevice(), &render_pass_depth_input_info, nullptr, &render_pass_clear_none_depth_input_);
                CheckResult(create_result, "Render pass (render_pass_clear_none_depth_input_) creation failed");
                SetObjectName(device_->GetDevice(), VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)render_pass_clear_none_depth_input_, "renderPassClearNone");
            }

            // Resolve render pass.
            {
                VkAttachmentReference color_reference = {};
                color_reference.attachment            = 0;
                color_reference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentReference depth_reference = {};
                depth_reference.attachment            = 1;
                depth_reference.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkAttachmentReference resolve_reference = {};
                resolve_reference.attachment            = 2;
                resolve_reference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkSubpassDescription subpass_description    = {};
                subpass_description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass_description.colorAttachmentCount    = 1;
                subpass_description.pColorAttachments       = &color_reference;
                subpass_description.pDepthStencilAttachment = &depth_reference;
                subpass_description.inputAttachmentCount    = 0;
                subpass_description.pInputAttachments       = nullptr;
                subpass_description.preserveAttachmentCount = 0;
                subpass_description.pPreserveAttachments    = nullptr;
                subpass_description.pResolveAttachments     = &resolve_reference;

                // Subpass dependencies for layout transitions.
                std::array<VkSubpassDependency, 3> dependencies;

                dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
                dependencies[0].dstSubpass      = 0;
                dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
                dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                dependencies[1].srcSubpass      = 0;
                dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
                dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
                dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                dependencies[2].srcSubpass      = VK_SUBPASS_EXTERNAL;
                dependencies[2].dstSubpass      = 0;
                dependencies[2].srcStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependencies[2].dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependencies[2].srcAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                dependencies[2].dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                std::array<VkAttachmentDescription, 3> attachments = {};

                // Color attachment.
                attachments[0].format         = swapchain_format_.format;
                attachments[0].samples        = msaa_samples_;
                attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

                // Depth attachment.
                attachments[1].format         = kDepthFormat;
                attachments[1].samples        = msaa_samples_;
                attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

                // Resolve attachment.
                attachments[2].format         = swapchain_format_.format;
                attachments[2].samples        = VK_SAMPLE_COUNT_1_BIT;
                attachments[2].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[2].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

                VkRenderPassCreateInfo render_pass_info = {};
                render_pass_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                render_pass_info.attachmentCount        = static_cast<uint32_t>(attachments.size());
                render_pass_info.pAttachments           = attachments.data();
                render_pass_info.subpassCount           = 1;
                render_pass_info.pSubpasses             = &subpass_description;
                render_pass_info.dependencyCount        = static_cast<uint32_t>(dependencies.size());
                render_pass_info.pDependencies          = dependencies.data();

                VkResult create_result;

                // Retain layout.
                attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachments[0].finalLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                attachments[1].finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachments[2].finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;  // Resolve attachment will always be on clear.

                create_result = vkCreateRenderPass(device_->GetDevice(), &render_pass_info, nullptr, &render_pass_resolve_);
                CheckResult(create_result, "Render pass (render_pass_resolve_) creation failed");
            }
        }

        void SwapChain::DestroyRenderPass()
        {
            if (render_pass_clear_color_and_depth_ != VK_NULL_HANDLE)
            {
                vkDestroyRenderPass(device_->GetDevice(), render_pass_clear_color_and_depth_, nullptr);
                render_pass_clear_color_and_depth_ = VK_NULL_HANDLE;
            }

            if (render_pass_clear_depth_only_ != VK_NULL_HANDLE)
            {
                vkDestroyRenderPass(device_->GetDevice(), render_pass_clear_depth_only_, nullptr);
                render_pass_clear_depth_only_ = VK_NULL_HANDLE;
            }

            if (render_pass_clear_none_ != VK_NULL_HANDLE)
            {
                vkDestroyRenderPass(device_->GetDevice(), render_pass_clear_none_, nullptr);
                render_pass_clear_none_ = VK_NULL_HANDLE;
            }

            if (render_pass_clear_none_depth_input_ != VK_NULL_HANDLE)
            {
                vkDestroyRenderPass(device_->GetDevice(), render_pass_clear_none_depth_input_, nullptr);
                render_pass_clear_none_depth_input_ = VK_NULL_HANDLE;
            }

            if (render_pass_resolve_ != VK_NULL_HANDLE)
            {
                vkDestroyRenderPass(device_->GetDevice(), render_pass_resolve_, nullptr);
                render_pass_resolve_ = VK_NULL_HANDLE;
            }
        }

        VkImage SwapChain::GetDepthInputImage() const
        {
            return geometry_depth_copy_.image;
        }

        const VkImageView& SwapChain::GetDepthInputImageView() const
        {
            return geometry_depth_copy_.view;
        }

        VkExtent2D SwapChain::GetSwapchainExtent() const
        {
            return swapchain_extent_;
        }

        void SwapChain::CreateRTV()
        {
            image_views_.resize(images_.size());
            for (uint32_t i = 0; i < images_.size(); i++)
            {
                VkImageViewCreateInfo color_image_view           = {};
                color_image_view.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                color_image_view.pNext                           = nullptr;
                color_image_view.format                          = swapchain_format_.format;
                color_image_view.components.r                    = VK_COMPONENT_SWIZZLE_R;
                color_image_view.components.g                    = VK_COMPONENT_SWIZZLE_G;
                color_image_view.components.b                    = VK_COMPONENT_SWIZZLE_B;
                color_image_view.components.a                    = VK_COMPONENT_SWIZZLE_A;
                color_image_view.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                color_image_view.subresourceRange.baseMipLevel   = 0;
                color_image_view.subresourceRange.levelCount     = 1;
                color_image_view.subresourceRange.baseArrayLayer = 0;
                color_image_view.subresourceRange.layerCount     = 1;
                color_image_view.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
                color_image_view.flags                           = 0;
                color_image_view.image                           = images_[i];

                VkResult result = vkCreateImageView(device_->GetDevice(), &color_image_view, nullptr, &image_views_[i]);
                CheckResult(result, "SwapChain image view creation failed.");
            }
        }

        void SwapChain::DestroyRTV()
        {
            for (uint32_t image_index = 0; image_index < image_views_.size(); image_index++)
            {
                vkDestroyImageView(device_->GetDevice(), image_views_[image_index], nullptr);
            }
        }

        void SwapChain::CreateFramebuffers(uint32_t width, uint32_t height)
        {
            frame_buffers_clear_color_and_depth_.resize(image_views_.size());
            frame_buffers_clear_depth_only_.resize(image_views_.size());
            frame_buffers_clear_none_.resize(image_views_.size());
            frame_buffers_clear_none_depth_input_.resize(image_views_.size());
            frame_buffers_resolve_.resize(image_views_.size());

            for (uint32_t image_index = 0; image_index < image_views_.size(); image_index++)
            {
                std::vector<VkImageView> attachments = {color_image_.view, depth_stencil_.view};

                VkFramebufferCreateInfo fb_info = {};
                fb_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                fb_info.pNext                   = nullptr;
                fb_info.attachmentCount         = static_cast<uint32_t>(attachments.size());
                fb_info.pAttachments            = attachments.data();
                fb_info.width                   = width;
                fb_info.height                  = height;
                fb_info.layers                  = 1;

                VkResult result;

                fb_info.renderPass = render_pass_clear_color_and_depth_;
                result             = vkCreateFramebuffer(device_->GetDevice(), &fb_info, nullptr, &frame_buffers_clear_color_and_depth_[image_index]);
                CheckResult(result, "Failed to create frame_buffers_clear_color_and_depth_.");

                fb_info.renderPass = render_pass_clear_depth_only_;
                result             = vkCreateFramebuffer(device_->GetDevice(), &fb_info, nullptr, &frame_buffers_clear_depth_only_[image_index]);
                CheckResult(result, "Failed to create frame_buffers_clear_depth_only_.");

                fb_info.renderPass = render_pass_clear_none_;
                result             = vkCreateFramebuffer(device_->GetDevice(), &fb_info, nullptr, &frame_buffers_clear_none_[image_index]);
                CheckResult(result, "Failed to create frame_buffers_clear_none_.");

                attachments             = {color_image_.view, depth_stencil_.view, geometry_depth_copy_.view};
                fb_info.attachmentCount = static_cast<uint32_t>(attachments.size());
                fb_info.pAttachments    = attachments.data();
                fb_info.renderPass      = render_pass_clear_none_depth_input_;
                result                  = vkCreateFramebuffer(device_->GetDevice(), &fb_info, nullptr, &frame_buffers_clear_none_depth_input_[image_index]);
                CheckResult(result, "Failed to create frame_buffers_clear_none_depth_input_.");

                attachments             = {color_image_.view, depth_stencil_.view, image_views_[image_index]};
                fb_info.attachmentCount = static_cast<uint32_t>(attachments.size());
                fb_info.pAttachments    = attachments.data();
                fb_info.renderPass      = render_pass_resolve_;
                result                  = vkCreateFramebuffer(device_->GetDevice(), &fb_info, nullptr, &frame_buffers_resolve_[image_index]);
                CheckResult(result, "Failed to create frame_buffers_resolve_.");
            }
        }

        void SwapChain::DestroyFramebuffers()
        {
            for (uint32_t frame_buffer_index = 0; frame_buffer_index < frame_buffers_clear_color_and_depth_.size(); frame_buffer_index++)
            {
                vkDestroyFramebuffer(device_->GetDevice(), frame_buffers_clear_color_and_depth_[frame_buffer_index], nullptr);
            }

            for (uint32_t frame_buffer_index = 0; frame_buffer_index < frame_buffers_clear_depth_only_.size(); frame_buffer_index++)
            {
                vkDestroyFramebuffer(device_->GetDevice(), frame_buffers_clear_depth_only_[frame_buffer_index], nullptr);
            }

            for (uint32_t frame_buffer_index = 0; frame_buffer_index < frame_buffers_clear_none_.size(); frame_buffer_index++)
            {
                vkDestroyFramebuffer(device_->GetDevice(), frame_buffers_clear_none_[frame_buffer_index], nullptr);
            }

            for (uint32_t frame_buffer_index = 0; frame_buffer_index < frame_buffers_clear_none_depth_input_.size(); frame_buffer_index++)
            {
                vkDestroyFramebuffer(device_->GetDevice(), frame_buffers_clear_none_depth_input_[frame_buffer_index], nullptr);
            }

            for (uint32_t frame_buffer_index = 0; frame_buffer_index < frame_buffers_resolve_.size(); frame_buffer_index++)
            {
                vkDestroyFramebuffer(device_->GetDevice(), frame_buffers_resolve_[frame_buffer_index], nullptr);
            }
        }
    }  // namespace renderer
}  // namespace rra
