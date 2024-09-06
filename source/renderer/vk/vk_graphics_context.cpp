//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the graphics context for the Vulkan API.
//=============================================================================

#include "vk_graphics_context.h"
#include "public/rra_error.h"
#include "public/rra_blas.h"
#include "framework/ext_debug_utils.h"
#include "vk/ray_history_offscreen_renderer.h"

/// Helper macro to bubble vulkan errors before the rendering starts. Used during uploads.
#define PRE_RENDER_CHECK_RESULT(code, extra) \
    {                                        \
        CheckResult(code, extra);            \
        if (code != VK_SUCCESS)              \
        {                                    \
            return false;                    \
        }                                    \
    }

/// Helper macro to bubble vulkan errors for functions that we can't check. Used during uploads.
#define PRE_RENDER_CHECK_HEALTH() \
    {                             \
        if (!this->IsHealthy())   \
        {                         \
            return false;         \
        }                         \
    }

namespace rra
{
    namespace renderer
    {
        /// @brief The Vulkan graphics context singleton instance.
        VkGraphicsContext* __global_vk_graphics_context = nullptr;

        void VkGraphicsContext::SetWindowInfo(WindowInfo window_info)
        {
            window_info_ = window_info;
        }

        BlasDrawInstruction VkGraphicsContext::GetBlasDrawInstruction(uint64_t blas_index)
        {
            return geometry_instructions_[blas_index];
        }

        std::vector<VkTraversalTree> VkGraphicsContext::GetBlases() const
        {
            return blases_;
        }

        Device& VkGraphicsContext::GetDevice()
        {
            return device_;
        }

        bool VkGraphicsContext::IsInitialized() const
        {
            return initialized_;
        }

        VkGraphicsContext* GetVkGraphicsContext()
        {
            if (__global_vk_graphics_context)
            {
                return __global_vk_graphics_context;
            }

            __global_vk_graphics_context = new VkGraphicsContext();
            return __global_vk_graphics_context;
        }

        void DeleteVkGraphicsContext()
        {
            if (__global_vk_graphics_context)
            {
                delete __global_vk_graphics_context;
                __global_vk_graphics_context = nullptr;
            }
        }

        bool VkGraphicsContext::Initialize(std::shared_ptr<GraphicsContextSceneInfo> info)
        {
// Enable validations in debug mode only.
#ifndef NDEBUG
            const bool kEnableCpuValidation = true;
            const bool kEnableGpuValidation = true;
#else
            const bool kEnableCpuValidation = false;
            const bool kEnableGpuValidation = false;
#endif

            initialized_ = device_.OnCreate("RRA", "RRA_Engine", kEnableCpuValidation, kEnableGpuValidation, &window_info_);

            if (initialized_)
            {
                initialized_ = CollectAndUploadTraversalTrees(info);
                rh_renderer_ = new RayHistoryOffscreenRenderer{};
                rh_renderer_->Initialize(&device_);
            }

            error_window_primed_ = initialized_;
            return initialized_;
        }

        void VkGraphicsContext::Cleanup()
        {
            if (initialized_)
            {
                device_.GPUFlush();

                for (auto& blas : blases_)
                {
                    device_.DestroyBuffer(blas.vertex_buffer, blas.vertex_allocation);
                    device_.DestroyBuffer(blas.volume_buffer, blas.volume_allocation);
                }

                rh_renderer_->CleanUp();
                device_.OnDestroy();

                geometry_instructions_.clear();
            }
        }

        bool VkGraphicsContext::IsErrorWindowPrimed()
        {
            return error_window_primed_;
        }

        bool VkGraphicsContext::IsHealthy()
        {
            return initialization_error_message_ == "";
        }

        std::string VkGraphicsContext::GetInitializationErrorMessage()
        {
            return initialization_error_message_;
        }

        void VkGraphicsContext::SetInitializationErrorMessage(const std::string& message)
        {
            initialization_error_message_ = message;
        }

        void VkGraphicsContext::SetSceneInfo(RendererSceneInfo* scene_info)
        {
            scene_info_ = scene_info;
        }

        void VkGraphicsContext::CreateRayHistoryStatsBuffer(uint32_t dispatch_id, DispatchIdData* out_max_count)
        {
            rh_renderer_->CreateStatsBuffer(dispatch_id, out_max_count);
        }

        QImage VkGraphicsContext::RenderRayHistoryImage(uint32_t             heatmap_min,
                                                        uint32_t             heatmap_max,
                                                        uint32_t             ray_index,
                                                        uint32_t             reshaped_x,
                                                        uint32_t             reshaped_y,
                                                        uint32_t             reshaped_z,
                                                        RayHistoryColorMode  color_mode,
                                                        uint32_t             slice_index,
                                                        renderer::SlicePlane slice_plane)
        {
            return rh_renderer_->Render(heatmap_min, heatmap_max, ray_index, reshaped_x, reshaped_y, reshaped_z, color_mode, slice_index, slice_plane);
        }

        void VkGraphicsContext::SetRayHistoryHeatmapData(const HeatmapData& heatmap_data)
        {
            rh_renderer_->SetHeatmapData(heatmap_data);
        }

        VulkanHeatmap VkGraphicsContext::CreateVulkanHeatmapResources(VkCommandBuffer cmd, Heatmap* heatmap, bool for_compute)
        {
            auto data = heatmap->GetData();

            VkDeviceSize buffer_size = data.size() * sizeof(glm::vec4);

            VkImageCreateInfo image_create_info = {};
            image_create_info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            image_create_info.format            = VK_FORMAT_R32G32B32A32_SFLOAT;
            image_create_info.usage             = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            image_create_info.imageType         = VK_IMAGE_TYPE_1D;
            image_create_info.extent            = {static_cast<uint32_t>(data.size()), 1, 1};
            image_create_info.mipLevels         = 1;
            image_create_info.arrayLayers       = 1;
            image_create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
            image_create_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
            image_create_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;

            VulkanHeatmap vulkan_heatmap{};
            device_.CreateImage(image_create_info, VMA_MEMORY_USAGE_GPU_ONLY, vulkan_heatmap.image, vulkan_heatmap.allocation);

            VkBuffer      staging_buffer     = VK_NULL_HANDLE;
            VmaAllocation staging_allocation = VK_NULL_HANDLE;

            device_.CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, staging_buffer, staging_allocation, data.data(), buffer_size);

            VkCommandBufferBeginInfo begin_info = {};
            begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            auto result                         = vkBeginCommandBuffer(cmd, &begin_info);
            CheckResult(result, "Upload command buffer failed to begin during heatmap upload.");

            VkImageMemoryBarrier undefined_to_transfer_barrier        = {};
            undefined_to_transfer_barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            undefined_to_transfer_barrier.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
            undefined_to_transfer_barrier.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            undefined_to_transfer_barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
            undefined_to_transfer_barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
            undefined_to_transfer_barrier.image                       = vulkan_heatmap.image;
            undefined_to_transfer_barrier.srcAccessMask               = VK_ACCESS_MEMORY_READ_BIT;
            undefined_to_transfer_barrier.dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
            undefined_to_transfer_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            undefined_to_transfer_barrier.subresourceRange.levelCount = 1;
            undefined_to_transfer_barrier.subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(
                cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &undefined_to_transfer_barrier);

            VkBufferImageCopy copy_region           = {};
            copy_region.bufferOffset                = 0;
            copy_region.bufferRowLength             = 0;
            copy_region.bufferImageHeight           = 0;
            copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy_region.imageSubresource.layerCount = 1;
            copy_region.imageOffset                 = {0, 0, 0};
            copy_region.imageExtent                 = {static_cast<uint32_t>(data.size()), 1, 1};

            vkCmdCopyBufferToImage(cmd, staging_buffer, vulkan_heatmap.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

            VkImageMemoryBarrier transfer_to_read_barrier        = {};
            transfer_to_read_barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            transfer_to_read_barrier.oldLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            transfer_to_read_barrier.newLayout                   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            transfer_to_read_barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
            transfer_to_read_barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
            transfer_to_read_barrier.image                       = vulkan_heatmap.image;
            transfer_to_read_barrier.srcAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
            transfer_to_read_barrier.dstAccessMask               = VK_ACCESS_MEMORY_READ_BIT;
            transfer_to_read_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            transfer_to_read_barrier.subresourceRange.levelCount = 1;
            transfer_to_read_barrier.subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(cmd,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 for_compute ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &transfer_to_read_barrier);

            result = vkEndCommandBuffer(cmd);
            CheckResult(result, "Upload command buffer failed to end during heatmap upload.");

            VkSubmitInfo submit_info       = {};
            submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers    = &cmd;

            result = vkQueueSubmit(for_compute ? device_.GetComputeQueue() : device_.GetGraphicsQueue(), 1, &submit_info, VK_NULL_HANDLE);
            CheckResult(result, "Failed to submit heatmap buffer uploads to the graphics queue.");

            result = vkDeviceWaitIdle(device_.GetDevice());
            CheckResult(result, "Failed to wait on device");

            device_.DestroyBuffer(staging_buffer, staging_allocation);

            VkImageViewCreateInfo image_view_create_info       = {};
            image_view_create_info.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            image_view_create_info.image                       = vulkan_heatmap.image;
            image_view_create_info.format                      = VK_FORMAT_R32G32B32A32_SFLOAT;
            image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_view_create_info.subresourceRange.levelCount = 1;
            image_view_create_info.subresourceRange.layerCount = 1;

            result = vkCreateImageView(device_.GetDevice(), &image_view_create_info, nullptr, &vulkan_heatmap.image_view);
            CheckResult(result, "Failed to create heatmap image view.");

            VkSamplerCreateInfo sampler_create_info = {};
            sampler_create_info.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sampler_create_info.magFilter           = VK_FILTER_LINEAR;
            sampler_create_info.minFilter           = VK_FILTER_LINEAR;
            sampler_create_info.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler_create_info.addressModeV        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler_create_info.addressModeW        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler_create_info.borderColor         = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

            result = vkCreateSampler(device_.GetDevice(), &sampler_create_info, nullptr, &vulkan_heatmap.sampler);
            CheckResult(result, "Failed to create heatmap image view.");

            vulkan_heatmap.image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            vulkan_heatmap.image_info.imageView   = vulkan_heatmap.image_view;
            vulkan_heatmap.image_info.sampler     = vulkan_heatmap.sampler;

            return vulkan_heatmap;
        }

        bool VkGraphicsContext::CollectAndUploadTraversalTrees(std::shared_ptr<GraphicsContextSceneInfo> info)
        {
            PRE_RENDER_CHECK_HEALTH();

            // Command setup
            VkResult result;

            VkCommandPool   command_pool;
            VkCommandBuffer command_buffer;
            VkFence         fence;

            VkCommandPoolCreateInfo pool_info = {};
            pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            pool_info.queueFamilyIndex        = device_.GetGraphicsQueueFamilyIndex();

            result = vkCreateCommandPool(device_.GetDevice(), &pool_info, nullptr, &command_pool);
            PRE_RENDER_CHECK_RESULT(result, "Failed to create global upload command buffer pool.");

            VkFenceCreateInfo fence_info{};
            fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            result           = vkCreateFence(device_.GetDevice(), &fence_info, nullptr, &fence);
            PRE_RENDER_CHECK_RESULT(result, "Failed to create fence for buffer upload.");

            VkCommandBufferAllocateInfo alloc_info = {};
            alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.commandPool                 = command_pool;
            alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            alloc_info.commandBufferCount          = 1;

            result = vkAllocateCommandBuffers(device_.GetDevice(), &alloc_info, &command_buffer);
            PRE_RENDER_CHECK_RESULT(result, "Failed to allocate global upload command buffer.");

            VkCommandBufferBeginInfo begin_info = {};
            begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            result                              = vkBeginCommandBuffer(command_buffer, &begin_info);
            PRE_RENDER_CHECK_RESULT(result, "Failed to begin global upload command buffer recording.");
            PRE_RENDER_CHECK_HEALTH();

            std::vector<VkBuffer>      volume_staging_buffers(info->acceleration_structures.size());
            std::vector<VmaAllocation> volume_staging_allocations(info->acceleration_structures.size());
            std::vector<VkBuffer>      triangle_staging_buffers(info->acceleration_structures.size());
            std::vector<VmaAllocation> triangle_staging_allocations(info->acceleration_structures.size());

            geometry_instructions_.reserve(info->acceleration_structures.size());
            blases_.reserve(info->acceleration_structures.size());

            for (size_t i = 0; i < info->acceleration_structures.size(); i++)
            {
                VkTraversalTree vk_tree;
                TraversalTree&  cpu_side = info->acceleration_structures[i];

                // We create a blank triangle since Vulkan does not like the 0 sized buffers and
                // we need valid buffers for a descriptor slot that holds multiple buffers.
                if (cpu_side.vertices.empty())
                {
                    cpu_side.vertices.resize(3);
                }

                {
                    // Upload volumes.
                    auto buffer_size = cpu_side.volumes.size() * sizeof(TraversalVolume);

                    PRE_RENDER_CHECK_HEALTH();
                    device_.CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                         VMA_MEMORY_USAGE_CPU_ONLY,
                                         volume_staging_buffers[i],
                                         volume_staging_allocations[i],
                                         cpu_side.volumes.data(),
                                         buffer_size);

                    PRE_RENDER_CHECK_HEALTH();
                    device_.CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                         VMA_MEMORY_USAGE_GPU_ONLY,
                                         vk_tree.volume_buffer,
                                         vk_tree.volume_allocation,
                                         nullptr,
                                         buffer_size);

                    VkBufferCopy copy_region = {};
                    copy_region.size         = buffer_size;
                    vkCmdCopyBuffer(command_buffer, volume_staging_buffers[i], vk_tree.volume_buffer, 1, &copy_region);
                }
                {
                    // Upload triangles.
                    auto buffer_size = cpu_side.vertices.size() * sizeof(RraVertex);

                    PRE_RENDER_CHECK_HEALTH();
                    device_.CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                         VMA_MEMORY_USAGE_CPU_ONLY,
                                         triangle_staging_buffers[i],
                                         triangle_staging_allocations[i],
                                         cpu_side.vertices.data(),
                                         buffer_size);

                    PRE_RENDER_CHECK_HEALTH();
                    device_.CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                         VMA_MEMORY_USAGE_GPU_ONLY,
                                         vk_tree.vertex_buffer,
                                         vk_tree.vertex_allocation,
                                         nullptr,
                                         buffer_size);

                    VkBufferCopy copy_region = {};
                    copy_region.size         = buffer_size;
                    vkCmdCopyBuffer(command_buffer, triangle_staging_buffers[i], vk_tree.vertex_buffer, 1, &copy_region);
                }

                // Create draw instructions for rasterization pipelines.
                BlasDrawInstruction geometry_instruction = {};
                geometry_instruction.vertex_index        = 0;
                geometry_instruction.vertex_count        = static_cast<uint32_t>(cpu_side.vertices.size());
                geometry_instruction.vertex_buffer       = vk_tree.vertex_buffer;
                geometry_instructions_.push_back(geometry_instruction);

                // Add structures to internal list.
                blases_.push_back(vk_tree);
            }

            result = vkEndCommandBuffer(command_buffer);
            PRE_RENDER_CHECK_RESULT(result, "Failed to end global upload command buffer recording.");

            VkSubmitInfo submit_info       = {};
            submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers    = &command_buffer;

            result = vkQueueSubmit(device_.GetGraphicsQueue(), 1, &submit_info, fence);
            PRE_RENDER_CHECK_RESULT(result, "Failed to submit global buffer uploads to the graphics queue.");

            PRE_RENDER_CHECK_HEALTH();
            result = vkWaitForFences(device_.GetDevice(), 1, &fence, VK_TRUE, 1000000000);
            PRE_RENDER_CHECK_RESULT(result, "Failed waiting for fence after buffer transfer.");

            vkDestroyCommandPool(device_.GetDevice(), command_pool, nullptr);

            for (uint32_t i = 0; i < (uint32_t)triangle_staging_buffers.size(); ++i)
            {
                PRE_RENDER_CHECK_HEALTH();
                device_.DestroyBuffer(triangle_staging_buffers[i], triangle_staging_allocations[i]);
                device_.DestroyBuffer(volume_staging_buffers[i], volume_staging_allocations[i]);
            }

            return true;
        }

    }  // namespace renderer

}  // namespace rra
