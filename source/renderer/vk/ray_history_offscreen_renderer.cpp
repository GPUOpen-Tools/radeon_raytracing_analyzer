//=============================================================================
// Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for ray history offscreen renderer type.
//=============================================================================

#include "ray_history_offscreen_renderer.h"

#undef emit
#include <execution>
#include <algorithm>
#define emit

#include "framework/device.h"
#include "vk_graphics_context.h"

namespace rra
{
    namespace renderer
    {
        constexpr uint64_t render_timeout{1000000000};  ///< Maximum amount of time renderer can take in nanoseconds.
        constexpr uint32_t STATS_BUFFER_BINDING{0};
        constexpr uint32_t HEATMAP_BINDING{1};
        constexpr uint32_t COLOR_BUFFER_BINDING{2};
        constexpr uint32_t RAY_BUFFER_BINDING{3};

        void RayHistoryOffscreenRenderer::Initialize(Device* device)
        {
            device_ = device;
            CreatePipelineLayout();
            CreatePipeline();
            CreateDescriptorSet();
            AllocateCommandBuffer();
            CreateFence();
        }

        void RayHistoryOffscreenRenderer::CleanUp()
        {
            device_->DestroyBuffer(stats_buffer_.buffer, stats_buffer_.allocation);
            device_->DestroyBuffer(ray_data_buffer_.buffer, ray_data_buffer_.allocation);

            delete qt_image_data_;

            // Destroy heatmap image.
            delete heatmap_;
            vkDestroySampler(device_->GetDevice(), vulkan_heatmap_.sampler, nullptr);
            vkDestroyImageView(device_->GetDevice(), vulkan_heatmap_.image_view, nullptr);
            device_->DestroyImage(vulkan_heatmap_.image, vulkan_heatmap_.allocation);

            // Destroy other Vulkan objects.
            vkDestroyDescriptorPool(device_->GetDevice(), descriptor_pool_, nullptr);
            vkDestroyFence(device_->GetDevice(), fence_, nullptr);
            vkFreeCommandBuffers(device_->GetDevice(), command_pool_, 1, &cmd_);
            vkDestroyCommandPool(device_->GetDevice(), command_pool_, nullptr);
            vkDestroyDescriptorSetLayout(device_->GetDevice(), descriptor_set_layout_, nullptr);
            vkDestroyPipelineLayout(device_->GetDevice(), pipeline_layout_, nullptr);
            vkDestroyPipeline(device_->GetDevice(), pipeline_, nullptr);
        }

        void RayHistoryOffscreenRenderer::CreateStatsBuffer(uint32_t dispatch_id, DispatchIdData* out_max_count)
        {
            RraRayGetDispatchDimensions(dispatch_id, &width_, &height_, &depth_);

            // The dims are 0 so no buffer can be created here.
            if (width_ * height_ * depth_ == 0)
            {
                return;
            }

            uint32_t                    data_count{width_ * height_ * depth_};
            std::vector<DispatchIdData> data(data_count);

            *out_max_count = {};

            // Get ray counts.
            uint32_t ray_data_size{0};
            uint32_t max_ray_count{0};
            for (uint32_t x{0}; x < width_; ++x)
            {
                for (uint32_t y{0}; y < height_; ++y)
                {
                    for (uint32_t z{0}; z < depth_; ++z)
                    {
                        uint32_t ray_count{};
                        RraRayGetRayCount(dispatch_id, {x, y, z}, &ray_count);
                        uint32_t data_idx{x + y * width_ + z * width_ * height_};
                        data[data_idx].ray_count       = ray_count;
                        data[data_idx].first_ray_index = ray_data_size;
                        ray_data_size += ray_count;
                        max_ray_count = std::max(ray_count, max_ray_count);
                    }
                }
            }

            std::vector<DispatchRayData> ray_data(ray_data_size);
            std::vector<uint32_t>        x_coords(width_);
            std::iota(x_coords.begin(), x_coords.end(), 0);
            std::for_each(std::execution::par, x_coords.begin(), x_coords.end(), [&](uint32_t x) {
                std::vector<Ray> rays{};
                rays.resize(max_ray_count);
                for (uint32_t y{0}; y < height_; ++y)
                {
                    for (uint32_t z{0}; z < depth_; ++z)
                    {
                        uint32_t data_idx{x + y * width_ + z * width_ * height_};
                        uint32_t ray_count{data[data_idx].ray_count};

                        if (ray_count > out_max_count->ray_count)
                        {
                            out_max_count->ray_count = ray_count;
                        }

                        uint32_t traversal_count{0};
                        uint32_t instance_intersection_count{0};
                        uint32_t total_any_hit_count{0};
                        RraRayGetAnyHitInvocationCount(dispatch_id, {x, y, z}, &total_any_hit_count);

                        for (uint32_t i{0}; i < ray_count; ++i)
                        {
                            IntersectionResult intersection_result{};
                            RraRayGetIntersectionResult(dispatch_id, {x, y, z}, i, &intersection_result);
                            traversal_count += intersection_result.num_iterations;
                            instance_intersection_count += intersection_result.num_instance_intersections;
                        }
                        data[data_idx].traversal_count             = traversal_count;
                        data[data_idx].instance_intersection_count = instance_intersection_count;
                        data[data_idx].any_hit_invocation_count    = total_any_hit_count;

                        if (traversal_count > out_max_count->traversal_count)
                        {
                            out_max_count->traversal_count = traversal_count;
                        }

                        if (instance_intersection_count > out_max_count->instance_intersection_count)
                        {
                            out_max_count->instance_intersection_count = instance_intersection_count;
                        }

                        if (total_any_hit_count > out_max_count->any_hit_invocation_count)
                        {
                            out_max_count->any_hit_invocation_count = total_any_hit_count;
                        }

                        RraRayGetRays(dispatch_id, {x, y, z}, rays.data());

                        for (uint32_t ray_idx{0}; ray_idx < ray_count; ++ray_idx)
                        {
                            DispatchRayData individual_ray_data                        = {};
                            individual_ray_data.direction.x                            = rays[ray_idx].direction[0];
                            individual_ray_data.direction.y                            = rays[ray_idx].direction[1];
                            individual_ray_data.direction.z                            = rays[ray_idx].direction[2];
                            ray_data[(size_t)data[data_idx].first_ray_index + ray_idx] = individual_ray_data;
                        }
                    }
                }
            });

            if (ray_data.empty())
            {
                // There are no rays so we can't proceed.
                return;
            }

            // Destroy old buffers if they exist.
            device_->DestroyBuffer(stats_buffer_.buffer, stats_buffer_.allocation);

            // Create new buffers.
            device_->CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                  VMA_MEMORY_USAGE_GPU_ONLY,
                                  stats_buffer_.buffer,
                                  stats_buffer_.allocation,
                                  nullptr,  // Can't write directly to the GPU buffer.
                                  data.size() * sizeof(DispatchIdData));

            // Copy data to GPU buffer.
            device_->TransferBufferToDevice(cmd_, device_->GetComputeQueue(), stats_buffer_.buffer, data.data(), data.size() * sizeof(DispatchIdData));

            // Link buffers to descriptor set.
            VkDescriptorBufferInfo stats_buffer_info{};
            stats_buffer_info.buffer = stats_buffer_.buffer;
            stats_buffer_info.offset = 0;
            stats_buffer_info.range  = VK_WHOLE_SIZE;

            VkWriteDescriptorSet write_stats = {};
            write_stats.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_stats.dstSet               = descriptor_set_;
            write_stats.dstBinding           = STATS_BUFFER_BINDING;
            write_stats.dstArrayElement      = 0;
            write_stats.descriptorCount      = 1;
            write_stats.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write_stats.pImageInfo           = nullptr;
            write_stats.pBufferInfo          = &stats_buffer_info;
            write_stats.pTexelBufferView     = nullptr;

            // Destroy old buffers if they exist.
            device_->DestroyBuffer(ray_data_buffer_.buffer, ray_data_buffer_.allocation);

            // Create new buffers.
            device_->CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                  VMA_MEMORY_USAGE_GPU_ONLY,
                                  ray_data_buffer_.buffer,
                                  ray_data_buffer_.allocation,
                                  nullptr,  // Can't write directly to the GPU buffer.
                                  ray_data.size() * sizeof(DispatchRayData));

            // Copy data to GPU buffer.
            device_->TransferBufferToDevice(
                cmd_, device_->GetComputeQueue(), ray_data_buffer_.buffer, ray_data.data(), ray_data.size() * sizeof(DispatchRayData));

            // Link buffers to descriptor set.
            VkDescriptorBufferInfo ray_buffer_info{};
            ray_buffer_info.buffer = ray_data_buffer_.buffer;
            ray_buffer_info.offset = 0;
            ray_buffer_info.range  = VK_WHOLE_SIZE;

            VkWriteDescriptorSet write_rays = {};
            write_rays.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_rays.dstSet               = descriptor_set_;
            write_rays.dstBinding           = RAY_BUFFER_BINDING;
            write_rays.dstArrayElement      = 0;
            write_rays.descriptorCount      = 1;
            write_rays.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write_rays.pImageInfo           = nullptr;
            write_rays.pBufferInfo          = &ray_buffer_info;
            write_rays.pTexelBufferView     = nullptr;

            std::vector<VkWriteDescriptorSet> writes{write_stats, write_rays};

            vkUpdateDescriptorSets(device_->GetDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
        }

        QImage RayHistoryOffscreenRenderer::Render(uint32_t            heatmap_min,
                                                   uint32_t            heatmap_max,
                                                   uint32_t            ray_index,
                                                   uint32_t            reshaped_x,
                                                   uint32_t            reshaped_y,
                                                   uint32_t            reshaped_z,
                                                   RayHistoryColorMode color_mode,
                                                   uint32_t            slice_index,
                                                   SlicePlane          slice_plane)
        {
            width_  = reshaped_x;
            height_ = reshaped_y;
            depth_  = reshaped_z;

            // The dims are 0 so no image can be created here.
            if (width_ * height_ * depth_ == 0)
            {
                QImage image;
                return image;
            }

            // Temporary host-visible buffer to render to, which will be copied to CPU for QT.
            ImageBuffer color_buffer{CreateAndLinkColorBuffer(slice_plane)};

            VkCommandBufferBeginInfo begin_info = {};
            begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags                    = 0;
            vkBeginCommandBuffer(cmd_, &begin_info);
            vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
            vkCmdBindDescriptorSets(cmd_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_, 0, 1, &descriptor_set_, 0, nullptr);

            PushConstant push_constant{};
            push_constant.color_mode                = color_mode;
            push_constant.reshape_width             = width_;
            push_constant.reshape_height            = height_;
            push_constant.reshape_depth             = depth_;
            push_constant.slice_index               = slice_index;
            push_constant.slice_plane               = (uint32_t)slice_plane;
            push_constant.min_traversal_count_limit = heatmap_min;
            push_constant.max_traversal_count_limit = heatmap_max;
            push_constant.ray_index                 = ray_index;

            vkCmdPushConstants(cmd_, pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstant), &push_constant);
            vkCmdDispatch(cmd_, GetColorBufferWidth(slice_plane), GetColorBufferHeight(slice_plane), 1);
            vkEndCommandBuffer(cmd_);

            VkSubmitInfo submit_info         = {};
            submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.waitSemaphoreCount   = 0;
            submit_info.pWaitSemaphores      = nullptr;
            submit_info.pWaitDstStageMask    = nullptr;
            submit_info.commandBufferCount   = 1;
            submit_info.pCommandBuffers      = &cmd_;
            submit_info.signalSemaphoreCount = 0;
            submit_info.pSignalSemaphores    = nullptr;

            vkQueueSubmit(device_->GetComputeQueue(), 1, &submit_info, fence_);

            VkResult result{vkWaitForFences(device_->GetDevice(), 1, &fence_, VK_TRUE, render_timeout)};
            CheckResult(result, "Failed waiting for ray history offscreen renderer fence.");

            result = vkResetFences(device_->GetDevice(), 1, &fence_);
            CheckResult(result, "Failed resetting fence.");

            result = vkResetCommandPool(device_->GetDevice(), command_pool_, 0);
            CheckResult(result, "Failed to reset command pool.");

            // Copy render result to CPU.
            QImage image{ImageBufferToQImage(color_buffer, slice_plane)};
            device_->DestroyBuffer(color_buffer.buffer, color_buffer.allocation);

            return image;
        }

        void RayHistoryOffscreenRenderer::SetHeatmapData(const HeatmapData& heatmap_data)
        {
            // Destroy old heatmap resources.
            delete heatmap_;
            vkDestroySampler(device_->GetDevice(), vulkan_heatmap_.sampler, nullptr);
            vkDestroyImageView(device_->GetDevice(), vulkan_heatmap_.image_view, nullptr);
            device_->DestroyImage(vulkan_heatmap_.image, vulkan_heatmap_.allocation);

            // Create new heatmap resources.
            heatmap_        = new Heatmap(heatmap_data);
            vulkan_heatmap_ = GetVkGraphicsContext()->CreateVulkanHeatmapResources(cmd_, heatmap_, true);

            // Link new resources to descriptor set.
            VkWriteDescriptorSet write_color = {};
            write_color.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_color.dstSet               = descriptor_set_;
            write_color.dstBinding           = HEATMAP_BINDING;
            write_color.dstArrayElement      = 0;
            write_color.descriptorCount      = 1;
            write_color.descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write_color.pImageInfo           = &vulkan_heatmap_.image_info;
            write_color.pBufferInfo          = nullptr;
            write_color.pTexelBufferView     = nullptr;

            std::vector<VkWriteDescriptorSet> writes{write_color};

            vkUpdateDescriptorSets(device_->GetDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
        }

        void RayHistoryOffscreenRenderer::CreatePipelineLayout()
        {
            VkDescriptorSetLayoutBinding stats_buffer_binding{};
            stats_buffer_binding.binding         = STATS_BUFFER_BINDING;
            stats_buffer_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            stats_buffer_binding.descriptorCount = 1;
            stats_buffer_binding.stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

            VkDescriptorSetLayoutBinding heatmap_binding = {};
            heatmap_binding.binding                      = HEATMAP_BINDING;
            heatmap_binding.descriptorType               = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            heatmap_binding.descriptorCount              = 1;
            heatmap_binding.stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT;

            VkDescriptorSetLayoutBinding image_buffer_binding{};
            image_buffer_binding.binding         = COLOR_BUFFER_BINDING;
            image_buffer_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            image_buffer_binding.descriptorCount = 1;
            image_buffer_binding.stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

            VkDescriptorSetLayoutBinding ray_buffer_binding{};
            ray_buffer_binding.binding         = RAY_BUFFER_BINDING;
            ray_buffer_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            ray_buffer_binding.descriptorCount = 1;
            ray_buffer_binding.stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

            std::vector<VkDescriptorSetLayoutBinding> bindings{stats_buffer_binding, heatmap_binding, image_buffer_binding, ray_buffer_binding};

            VkDescriptorSetLayoutCreateInfo set_layout_info = {};
            set_layout_info.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            set_layout_info.flags                           = 0;
            set_layout_info.bindingCount                    = (uint32_t)bindings.size();
            set_layout_info.pBindings                       = bindings.data();

            VkResult result = vkCreateDescriptorSetLayout(device_->GetDevice(), &set_layout_info, nullptr, &descriptor_set_layout_);
            CheckResult(result, "Failed to create ray history offscreen renderer descriptor set layout.");

            VkPushConstantRange constant_range{};
            constant_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            constant_range.offset     = 0;
            constant_range.size       = sizeof(PushConstant);

            std::vector<VkPushConstantRange> push_constant_ranges{constant_range};

            VkPipelineLayoutCreateInfo layout_info = {};
            layout_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            layout_info.flags                      = 0;
            layout_info.setLayoutCount             = 1;
            layout_info.pSetLayouts                = &descriptor_set_layout_;
            layout_info.pushConstantRangeCount     = (uint32_t)push_constant_ranges.size();
            layout_info.pPushConstantRanges        = push_constant_ranges.data();

            result = vkCreatePipelineLayout(device_->GetDevice(), &layout_info, nullptr, &pipeline_layout_);
            CheckResult(result, "Failed to create ray history offscreen renderer pipeline layout.");
        }

        void RayHistoryOffscreenRenderer::CreateDescriptorSet()
        {
            // Descriptor pool.
            std::vector<VkDescriptorPoolSize> pool_sizes{{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};

            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags                      = 0;
            pool_info.maxSets                    = 1;
            pool_info.poolSizeCount              = (uint32_t)pool_sizes.size();
            pool_info.pPoolSizes                 = pool_sizes.data();

            VkResult result{vkCreateDescriptorPool(device_->GetDevice(), &pool_info, nullptr, &descriptor_pool_)};
            CheckResult(result, "Failed to create ray history offscreen renderer descriptor pool.");

            // Descriptor set.
            VkDescriptorSetAllocateInfo alloc_info = {};
            alloc_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.descriptorPool              = descriptor_pool_;
            alloc_info.descriptorSetCount          = 1;
            alloc_info.pSetLayouts                 = &descriptor_set_layout_;

            result = vkAllocateDescriptorSets(device_->GetDevice(), &alloc_info, &descriptor_set_);
            CheckResult(result, "Failed to allocate ray history offscreen renderer descriptor set.");

            // Don't link descriptor set to buffers yet, we will do that when image is created.
        }

        void RayHistoryOffscreenRenderer::CreatePipeline()
        {
            const char*                     compute_shader_path = "RayHistoryOffscreenRenderer.cs.spv";
            VkPipelineShaderStageCreateInfo shader_stage_info{};
            LoadShader(compute_shader_path, device_, VK_SHADER_STAGE_COMPUTE_BIT, "CSMain", shader_stage_info);

            VkComputePipelineCreateInfo pipeline_info = {};
            pipeline_info.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pipeline_info.flags                       = 0;
            pipeline_info.stage                       = shader_stage_info;
            pipeline_info.layout                      = pipeline_layout_;
            pipeline_info.basePipelineHandle          = VK_NULL_HANDLE;
            pipeline_info.basePipelineIndex           = -1;

            VkResult result = vkCreateComputePipelines(device_->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline_);
            CheckResult(result, "Failed to create ray history offscreen renderer compute pipeline.");

            vkDestroyShaderModule(device_->GetDevice(), shader_stage_info.module, nullptr);
        }

        void RayHistoryOffscreenRenderer::AllocateCommandBuffer()
        {
            // Create command pool.
            VkCommandPoolCreateInfo pool_info = {};
            pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            pool_info.flags                   = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            pool_info.queueFamilyIndex        = device_->GetComputeQueueFamilyIndex();

            VkResult result{vkCreateCommandPool(device_->GetDevice(), &pool_info, nullptr, &command_pool_)};
            CheckResult(result, "Failed to create ray history offscreen renderer command pool.");

            // Allocate the command buffer.
            VkCommandBufferAllocateInfo alloc_info = {};
            alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.commandPool                 = command_pool_;
            alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            alloc_info.commandBufferCount          = 1;

            result = vkAllocateCommandBuffers(device_->GetDevice(), &alloc_info, &cmd_);
            CheckResult(result, "Failed to allocate ray history offscreen renderer command buffer.");
        }

        void RayHistoryOffscreenRenderer::CreateFence()
        {
            VkFenceCreateInfo fence_info = {};
            fence_info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fence_info.flags             = 0;

            VkResult result{vkCreateFence(device_->GetDevice(), &fence_info, nullptr, &fence_)};
            CheckResult(result, "Failed to create ray history offscreen renderer fence.");
        }

        RayHistoryOffscreenRenderer::ImageBuffer RayHistoryOffscreenRenderer::CreateAndLinkColorBuffer(SlicePlane slice_plane)
        {
            ImageBuffer color_buffer{};
            uint32_t    data_count{GetColorBufferWidth(slice_plane) * GetColorBufferHeight(slice_plane)};

            device_->CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                  VMA_MEMORY_USAGE_GPU_TO_CPU,
                                  color_buffer.buffer,
                                  color_buffer.allocation,
                                  nullptr,                         // Don't write to color buffer yet since this will be done by compute shader.
                                  data_count * sizeof(uint32_t));  // 8 bit per channel.

            VkDescriptorBufferInfo color_buffer_info{};
            color_buffer_info.buffer = color_buffer.buffer;
            color_buffer_info.offset = 0;
            color_buffer_info.range  = VK_WHOLE_SIZE;

            VkWriteDescriptorSet write_color = {};
            write_color.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_color.dstSet               = descriptor_set_;
            write_color.dstBinding           = COLOR_BUFFER_BINDING;
            write_color.dstArrayElement      = 0;
            write_color.descriptorCount      = 1;
            write_color.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write_color.pImageInfo           = nullptr;
            write_color.pBufferInfo          = &color_buffer_info;
            write_color.pTexelBufferView     = nullptr;

            std::vector<VkWriteDescriptorSet> writes{write_color};

            vkUpdateDescriptorSets(device_->GetDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);

            return color_buffer;
        }

        QImage RayHistoryOffscreenRenderer::ImageBufferToQImage(ImageBuffer image_buffer, SlicePlane slice_plane)
        {
            void*    data{};
            VkResult result = vmaMapMemory(device_->GetAllocator(), image_buffer.allocation, &data);
            CheckResult(result, "Failed mapping memory.");

            size_t byte_size{sizeof(uint32_t) * GetColorBufferWidth(slice_plane) * GetColorBufferHeight(slice_plane)};
            delete qt_image_data_;
            qt_image_data_ = new uchar[byte_size];
            memcpy(qt_image_data_, data, byte_size);

            QImage image(qt_image_data_, GetColorBufferWidth(slice_plane), GetColorBufferHeight(slice_plane), QImage::Format_RGBA8888);

            vmaUnmapMemory(device_->GetAllocator(), image_buffer.allocation);

            return image;
        }

        uint32_t RayHistoryOffscreenRenderer::GetColorBufferWidth(SlicePlane slice_plane) const
        {
            switch (slice_plane)
            {
            case kSlicePlaneXY:
                return width_;
            case kSlicePlaneXZ:
                return width_;
            case kSlicePlaneYZ:
                return depth_;
            default:
                return 0xFFFFFFFF;
            }
        }

        uint32_t RayHistoryOffscreenRenderer::GetColorBufferHeight(SlicePlane slice_plane) const
        {
            switch (slice_plane)
            {
            case kSlicePlaneXY:
                return height_;
            case kSlicePlaneXZ:
                return depth_;
            case kSlicePlaneYZ:
                return height_;
            default:
                return 0xFFFFFFFF;
            }
        }
    }  // namespace renderer
}  // namespace rra
