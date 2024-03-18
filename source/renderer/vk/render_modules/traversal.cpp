//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the traversal module.
//=============================================================================

#include "traversal.h"

#include "vk/vk_graphics_context.h"
#include "../framework/ext_debug_utils.h"

#include <algorithm>

namespace rra
{
    namespace renderer
    {
        static const uint32_t kKernelSize        = 8;  ///< The kernel size for the traversal compute shader. Must match the shader value.
        static const uint32_t kSubsampleMinIndex = 0;  ///< The subsample min index.
        static const uint32_t kSubsampleMaxIndex = 1;  ///< The subsample max index.

        TraversalRenderModule::TraversalRenderModule()
            : RenderModule(RenderPassHint::kRenderPassHintClearDepthOnly)
        {
        }

        void TraversalRenderModule::Initialize(const RenderModuleContext* context)
        {
            // Set the local context.
            context_ = context;

            // Setup the sets and pipeline.
            SetupDescriptorSetLayoutAndPipelineLayout();

            // Populate the rasterization states.
            VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
            input_assembly_state.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            input_assembly_state.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            input_assembly_state.flags                                  = 0;
            input_assembly_state.primitiveRestartEnable                 = VK_FALSE;

            VkPipelineRasterizationStateCreateInfo rasterization_state = {};
            rasterization_state.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterization_state.polygonMode                            = VK_POLYGON_MODE_FILL;
            rasterization_state.cullMode                               = VK_CULL_MODE_NONE;
            rasterization_state.frontFace                              = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterization_state.flags                                  = 0;
            rasterization_state.depthClampEnable                       = VK_FALSE;
            rasterization_state.lineWidth                              = 1.0f;

            VkPipelineColorBlendAttachmentState blend_attachment_state = {};
            blend_attachment_state.colorWriteMask                      = 0xF;
            blend_attachment_state.blendEnable                         = VK_FALSE;

            VkPipelineColorBlendStateCreateInfo color_blend_state = {};
            color_blend_state.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blend_state.attachmentCount                     = 1;
            color_blend_state.pAttachments                        = &blend_attachment_state;

            VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {};
            depth_stencil_state.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil_state.depthTestEnable                       = VK_TRUE;
            depth_stencil_state.depthWriteEnable                      = VK_TRUE;
            depth_stencil_state.depthCompareOp                        = VK_COMPARE_OP_LESS_OR_EQUAL;
            depth_stencil_state.back.compareOp                        = VK_COMPARE_OP_ALWAYS;

            VkPipelineViewportStateCreateInfo viewport_state = {};
            viewport_state.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_state.viewportCount                     = 1;
            viewport_state.scissorCount                      = 1;
            viewport_state.flags                             = 0;

            VkPipelineMultisampleStateCreateInfo multisample_state = {};
            multisample_state.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisample_state.rasterizationSamples                 = context_->swapchain->GetMSAASamples();
            multisample_state.flags                                = 0;

            std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

            VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
            dynamic_state_info.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamic_state_info.pDynamicStates                   = dynamic_states.data();
            dynamic_state_info.dynamicStateCount                = static_cast<uint32_t>(dynamic_states.size());
            dynamic_state_info.flags                            = 0;

            // Load shaders.
            const char* full_vertex_shader_path    = "TraversalShader.vs.spv";
            const char* full_pixel_shader_path     = "TraversalShader.ps.spv";
            const char* full_compute_shader_path   = "TraversalShader.cs.spv";
            const char* full_subsample_shader_path = "TraversalShaderSubsample.cs.spv";

            std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
            LoadShader(full_vertex_shader_path, context->device, VK_SHADER_STAGE_VERTEX_BIT, "VSMain", shader_stages[0]);
            LoadShader(full_pixel_shader_path, context->device, VK_SHADER_STAGE_FRAGMENT_BIT, "PSMain", shader_stages[1]);

            VkPipelineShaderStageCreateInfo compute_shader_stage;
            LoadShader(full_compute_shader_path, context->device, VK_SHADER_STAGE_COMPUTE_BIT, "CSMain", compute_shader_stage);

            VkPipelineShaderStageCreateInfo subsample_stage;
            LoadShader(full_subsample_shader_path, context->device, VK_SHADER_STAGE_COMPUTE_BIT, "CSSubsample", subsample_stage);

            // Setup vertex inputs.
            VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
            vertex_input_state_info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_state_info.vertexBindingDescriptionCount        = 0;
            vertex_input_state_info.vertexAttributeDescriptionCount      = 0;

            // Combine state information for graphics.
            VkGraphicsPipelineCreateInfo pipeline_create_info = {};
            pipeline_create_info.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipeline_create_info.layout                       = trace_traversal_pipeline_layout_;
            pipeline_create_info.renderPass                   = context->swapchain->GetRenderPass(GetRenderPassHint());
            pipeline_create_info.flags                        = 0;
            pipeline_create_info.basePipelineIndex            = -1;
            pipeline_create_info.basePipelineHandle           = VK_NULL_HANDLE;
            pipeline_create_info.pInputAssemblyState          = &input_assembly_state;
            pipeline_create_info.pRasterizationState          = &rasterization_state;
            pipeline_create_info.pColorBlendState             = &color_blend_state;
            pipeline_create_info.pMultisampleState            = &multisample_state;
            pipeline_create_info.pViewportState               = &viewport_state;
            pipeline_create_info.pDepthStencilState           = &depth_stencil_state;
            pipeline_create_info.pDynamicState                = &dynamic_state_info;
            pipeline_create_info.stageCount                   = static_cast<uint32_t>(shader_stages.size());
            pipeline_create_info.pStages                      = shader_stages.data();
            pipeline_create_info.pStages                      = shader_stages.data();
            pipeline_create_info.pStages                      = shader_stages.data();
            pipeline_create_info.pVertexInputState            = &vertex_input_state_info;

            // Create the render pipeline.
            VkResult create_result =
                vkCreateGraphicsPipelines(context->device->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &trace_traversal_pipeline_);
            CheckResult(create_result, "Failed to create pipeline.");

            // Create the compute pipeline.
            VkComputePipelineCreateInfo compute_pipeline_create_info = {};
            compute_pipeline_create_info.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            compute_pipeline_create_info.layout                      = trace_traversal_pipeline_layout_;
            compute_pipeline_create_info.stage                       = compute_shader_stage;
            compute_pipeline_create_info.basePipelineIndex           = -1;
            compute_pipeline_create_info.basePipelineHandle          = VK_NULL_HANDLE;
            create_result =
                vkCreateComputePipelines(context->device->GetDevice(), VK_NULL_HANDLE, 1, &compute_pipeline_create_info, nullptr, &compute_pipeline_);
            CheckResult(create_result, "Failed to create compute pipeline.");

            // Create the subsample pipeline.
            VkComputePipelineCreateInfo subsample_pipeline_create_info = {};
            subsample_pipeline_create_info.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            subsample_pipeline_create_info.layout                      = trace_traversal_pipeline_layout_;
            subsample_pipeline_create_info.stage                       = subsample_stage;
            subsample_pipeline_create_info.basePipelineIndex           = -1;
            subsample_pipeline_create_info.basePipelineHandle          = VK_NULL_HANDLE;
            create_result =
                vkCreateComputePipelines(context->device->GetDevice(), VK_NULL_HANDLE, 1, &subsample_pipeline_create_info, nullptr, &subsample_pipeline_);
            CheckResult(create_result, "Failed to create subsample pipeline.");

            // Cleanup shader modules.
            vkDestroyShaderModule(context->device->GetDevice(), shader_stages[0].module, nullptr);
            vkDestroyShaderModule(context->device->GetDevice(), shader_stages[1].module, nullptr);
            vkDestroyShaderModule(context->device->GetDevice(), compute_shader_stage.module, nullptr);
            vkDestroyShaderModule(context->device->GetDevice(), subsample_stage.module, nullptr);

            // Initialize TLAS buffer guards.
            top_level_volumes_guard_.Initialize(context->swapchain->GetBackBufferCount());
            top_level_volumes_staging_guard_.Initialize(context->swapchain->GetBackBufferCount());

            top_level_vertices_guard_.Initialize(context->swapchain->GetBackBufferCount());
            top_level_vertices_staging_guard_.Initialize(context->swapchain->GetBackBufferCount());

            top_level_instances_guard_.Initialize(context->swapchain->GetBackBufferCount());
            top_level_instances_staging_guard_.Initialize(context->swapchain->GetBackBufferCount());

            // Setup the descriptor pool.
            SetupDescriptorPool();
        };

        void TraversalRenderModule::CreateCounterBuffers(const RenderFrameContext* context)
        {
            uint32_t swapchain_size = context_->swapchain->GetBackBufferCount();

            if (context->framebuffer_width == last_offscreen_image_width_ && context->framebuffer_height == last_offscreen_image_height_ &&
                !traversal_count_setting_changed_)
            {
                return;
            }
            traversal_count_setting_changed_ = false;

            for (auto& counter : counter_gpu_buffers_)
            {
                context_->device->DestroyBuffer(counter.buffer, counter.allocation);
            }
            for (auto& counter : counter_cpu_buffers_)
            {
                context_->device->DestroyBuffer(counter.buffer, counter.allocation);
            }
            for (auto& counter : histogram_gpu_buffers_)
            {
                context_->device->DestroyBuffer(counter.buffer, counter.allocation);
            }
            counter_gpu_buffers_.clear();
            counter_cpu_buffers_.clear();
            histogram_gpu_buffers_.clear();

            counter_gpu_buffers_.resize(swapchain_size);
            counter_cpu_buffers_.resize(swapchain_size);
            histogram_gpu_buffers_.resize(swapchain_size);

            last_offscreen_image_width_  = context->framebuffer_width;
            last_offscreen_image_height_ = context->framebuffer_height;

            counter_gpu_buffer_size_   = (2 + last_offscreen_image_width_ * last_offscreen_image_height_) * sizeof(TraversalResult);
            counter_cpu_buffer_size_   = 2 * sizeof(TraversalResult);
            histogram_gpu_buffer_size_ = max_traversal_count_setting_ * sizeof(uint32_t);

            for (uint32_t i = 0; i < swapchain_size; i++)
            {
                counter_cpu_buffers_[i].buffer     = VK_NULL_HANDLE;
                counter_cpu_buffers_[i].allocation = VK_NULL_HANDLE;
                counter_gpu_buffers_[i].buffer     = VK_NULL_HANDLE;
                counter_gpu_buffers_[i].allocation = VK_NULL_HANDLE;

                context->device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                              VMA_MEMORY_USAGE_GPU_ONLY,
                                              counter_gpu_buffers_[i].buffer,
                                              counter_gpu_buffers_[i].allocation,
                                              nullptr,
                                              counter_gpu_buffer_size_);

                context->device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                              VMA_MEMORY_USAGE_CPU_ONLY,
                                              counter_cpu_buffers_[i].buffer,
                                              counter_cpu_buffers_[i].allocation,
                                              nullptr,
                                              counter_cpu_buffer_size_);

                context->device->CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                              VMA_MEMORY_USAGE_GPU_TO_CPU,
                                              histogram_gpu_buffers_[i].buffer,
                                              histogram_gpu_buffers_[i].allocation,
                                              nullptr,
                                              histogram_gpu_buffer_size_);

                std::vector<VkWriteDescriptorSet> write_descriptor_sets;

                // Binding 6 : The counter buffer.
                VkDescriptorBufferInfo counter_info = {};
                counter_info.buffer                 = counter_gpu_buffers_[i].buffer;
                counter_info.offset                 = 0;
                counter_info.range                  = VK_WHOLE_SIZE;

                VkDescriptorBufferInfo histogram_info = {};
                histogram_info.buffer                 = histogram_gpu_buffers_[i].buffer;
                histogram_info.offset                 = 0;
                histogram_info.range                  = VK_WHOLE_SIZE;

                VkWriteDescriptorSet write_descriptor_6 = {};
                write_descriptor_6.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write_descriptor_6.dstSet               = traversal_descriptor_sets_[i];
                write_descriptor_6.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                write_descriptor_6.dstBinding           = 16;
                write_descriptor_6.pBufferInfo          = &counter_info;
                write_descriptor_6.descriptorCount      = 1;

                VkWriteDescriptorSet write_descriptor_7 = {};
                write_descriptor_7.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write_descriptor_7.dstSet               = traversal_descriptor_sets_[i];
                write_descriptor_7.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                write_descriptor_7.dstBinding           = 17;
                write_descriptor_7.pBufferInfo          = &histogram_info;
                write_descriptor_7.descriptorCount      = 1;

                write_descriptor_sets = {write_descriptor_6, write_descriptor_7};

                vkUpdateDescriptorSets(
                    context_->device->GetDevice(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
            }
        }

        void TraversalRenderModule::Draw(const RenderFrameContext* context)
        {
            // Update the counter range if requested
            if ((!traversal_counter_range_update_functions_.empty() || traversal_counter_range_continuous_update_function_) && counter_gpu_buffer_size_ > 0)
            {
                std::vector<TraversalResult> counters;
                counters.resize(2);  // Resize to 2 for min/max at the beginning.

                context->device->ReadFromBuffer(counter_cpu_buffers_[context->current_frame].allocation, counters.data(), counter_cpu_buffer_size_);

                uint32_t min_counter = counters[kSubsampleMinIndex].counter;
                uint32_t max_counter = counters[kSubsampleMaxIndex].counter;

                for (auto callback : traversal_counter_range_update_functions_)
                {
                    callback(min_counter, max_counter);
                }

                traversal_counter_range_update_functions_.clear();

                if (traversal_counter_range_continuous_update_function_)
                {
                    traversal_counter_range_continuous_update_function_(min_counter, max_counter);
                }
            }

            if (histogram_update_function_ && histogram_gpu_buffer_size_ > 0)
            {
                std::vector<uint32_t> histogram_data(histogram_gpu_buffer_size_ / sizeof(uint32_t));
                context->device->ReadFromBuffer(histogram_gpu_buffers_[context->current_frame].allocation, histogram_data.data(), histogram_gpu_buffer_size_);
                histogram_update_function_(histogram_data, last_offscreen_image_width_, last_offscreen_image_height_);
                context->device->ZeroOutBuffer(histogram_gpu_buffers_[context->current_frame].allocation, histogram_gpu_buffer_size_);
            }

            // This function is safe to use repeatedly.
            // Creates the offscreen image when necessary.
            // Checks for resolution differences as well.
            CreateCounterBuffers(context);

            // Check for updates in the scene and upload new data accordingly.
            if (last_scene_iteration_ != context->scene_info->scene_iteration || last_scene_ != context->scene_info)
            {
                UploadTraversalData(context);
                last_scene_iteration_ = context->scene_info->scene_iteration;
                last_scene_           = context->scene_info;
            }

            if (empty_scene_)
            {
                return;
            }

            UpdateDescriptorSet(context->current_frame);

            // Process buffer guards.
            top_level_volumes_guard_.ProcessFrame(context->current_frame, context->device);
            top_level_volumes_staging_guard_.ProcessFrame(context->current_frame, context->device);

            top_level_vertices_guard_.ProcessFrame(context->current_frame, context->device);
            top_level_vertices_staging_guard_.ProcessFrame(context->current_frame, context->device);

            top_level_instances_guard_.ProcessFrame(context->current_frame, context->device);
            top_level_instances_staging_guard_.ProcessFrame(context->current_frame, context->device);

            // Binding 0 : Scene UBO
            VkWriteDescriptorSet scene_descriptor = {};
            scene_descriptor.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            scene_descriptor.dstSet               = traversal_descriptor_sets_[context->current_frame];
            scene_descriptor.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            scene_descriptor.dstBinding           = 0;
            scene_descriptor.pBufferInfo          = &context->scene_ubo_info;
            scene_descriptor.descriptorCount      = 1;

            // Binding 1 : Heatmap
            VkWriteDescriptorSet heatmap_descriptor = {};
            heatmap_descriptor.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            heatmap_descriptor.dstSet               = traversal_descriptor_sets_[context->current_frame];
            heatmap_descriptor.descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            heatmap_descriptor.dstBinding           = 1;
            heatmap_descriptor.pImageInfo           = &context->heatmap_image_info;
            heatmap_descriptor.descriptorCount      = 1;

            // Per frame write descriptors.
            std::vector<VkWriteDescriptorSet> write_descritptors = {scene_descriptor, heatmap_descriptor};

            vkUpdateDescriptorSets(context->device->GetDevice(), static_cast<uint32_t>(write_descritptors.size()), write_descritptors.data(), 0, nullptr);

            // Find dispatch dims.
            uint32_t dispatch_x_count = 1 + last_offscreen_image_width_ / kKernelSize;
            uint32_t dispatch_y_count = 1 + last_offscreen_image_height_ / kKernelSize;

            // Launch compute work.
            vkCmdBindPipeline(context->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_);

            vkCmdBindDescriptorSets(context->command_buffer,
                                    VK_PIPELINE_BIND_POINT_COMPUTE,
                                    trace_traversal_pipeline_layout_,
                                    0,
                                    1,
                                    &traversal_descriptor_sets_[context->current_frame],
                                    0,
                                    nullptr);

            vkCmdDispatch(context->command_buffer, dispatch_x_count, dispatch_y_count, 1);

            // Halt transfer until the ray traversal is finished.
            if (counter_gpu_buffer_size_ > 0)
            {
                VkBufferMemoryBarrier buffer_barrier{};
                buffer_barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                buffer_barrier.srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
                buffer_barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
                buffer_barrier.srcQueueFamilyIndex = context_->device->GetComputeQueueFamilyIndex();
                buffer_barrier.dstQueueFamilyIndex = context_->device->GetComputeQueueFamilyIndex();
                buffer_barrier.buffer              = counter_gpu_buffers_[context->current_frame].buffer;
                buffer_barrier.offset              = 0;
                buffer_barrier.size                = VK_WHOLE_SIZE;

                // Make buffer memory available to transfer stage and the subsample that will read from it later.
                vkCmdPipelineBarrier(context->command_buffer,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     0,
                                     0,
                                     nullptr,
                                     1,
                                     &buffer_barrier,
                                     0,
                                     nullptr);
            }

            // Launch subsample work.
            vkCmdBindPipeline(context->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, subsample_pipeline_);

            vkCmdBindDescriptorSets(context->command_buffer,
                                    VK_PIPELINE_BIND_POINT_COMPUTE,
                                    trace_traversal_pipeline_layout_,
                                    0,
                                    1,
                                    &traversal_descriptor_sets_[context->current_frame],
                                    0,
                                    nullptr);

            vkCmdDispatch(context->command_buffer, dispatch_x_count, dispatch_y_count, 1);

            // Halt transfer until the subsample is finished.
            if (counter_gpu_buffer_size_ > 0)
            {
                VkBufferMemoryBarrier buffer_barrier{};
                buffer_barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                buffer_barrier.srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
                buffer_barrier.dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
                buffer_barrier.srcQueueFamilyIndex = context_->device->GetComputeQueueFamilyIndex();
                buffer_barrier.dstQueueFamilyIndex = context_->device->GetComputeQueueFamilyIndex();
                buffer_barrier.buffer              = counter_gpu_buffers_[context->current_frame].buffer;
                buffer_barrier.offset              = 0;
                buffer_barrier.size                = VK_WHOLE_SIZE;

                // Make buffer memory available to transfer stage and fragment shader that will read from it later.
                vkCmdPipelineBarrier(context->command_buffer,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                     0,
                                     0,
                                     nullptr,
                                     1,
                                     &buffer_barrier,
                                     0,
                                     nullptr);

                VkBufferCopy download_region = {};
                download_region.size         = counter_cpu_buffer_size_;
                vkCmdCopyBuffer(context->command_buffer,
                                counter_gpu_buffers_[context->current_frame].buffer,
                                counter_cpu_buffers_[context->current_frame].buffer,
                                1,
                                &download_region);
            }

            // Halt fragment buffer until the ray traversal is finished.
            vkCmdPipelineBarrier(
                context->command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);

            // Start rendering compute results.
            context->begin_render_pass();

            vkCmdBindPipeline(context->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trace_traversal_pipeline_);

            if (top_level_volumes_guard_.GetCurrentBuffer() != VK_NULL_HANDLE)
            {
                vkCmdBindDescriptorSets(context->command_buffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        trace_traversal_pipeline_layout_,
                                        0,
                                        1,
                                        &traversal_descriptor_sets_[context->current_frame],
                                        0,
                                        nullptr);
                vkCmdDraw(context->command_buffer, 3, 1, 0, 0);
            }

            context->end_render_pass();
        };

        void TraversalRenderModule::Cleanup(const RenderModuleContext* context)
        {
            // Cleanup buffer guards.
            top_level_volumes_guard_.Cleanup(context->device);
            top_level_volumes_staging_guard_.Cleanup(context->device);

            top_level_vertices_guard_.Cleanup(context->device);
            top_level_vertices_staging_guard_.Cleanup(context->device);

            top_level_instances_guard_.Cleanup(context->device);
            top_level_instances_staging_guard_.Cleanup(context->device);

            // Cleanup counters.
            for (auto& counter : counter_gpu_buffers_)
            {
                context_->device->DestroyBuffer(counter.buffer, counter.allocation);
            }
            for (auto& counter : counter_cpu_buffers_)
            {
                context_->device->DestroyBuffer(counter.buffer, counter.allocation);
            }
            for (auto& counter : histogram_gpu_buffers_)
            {
                context_->device->DestroyBuffer(counter.buffer, counter.allocation);
            }
            counter_gpu_buffers_.clear();
            counter_cpu_buffers_.clear();
            histogram_gpu_buffers_.clear();

            // Cleanup pipeline information.
            VkDevice device_handle = context->device->GetDevice();

            vkDestroyDescriptorSetLayout(device_handle, traversal_descriptor_set_layout_, nullptr);
            vkDestroyDescriptorPool(device_handle, descriptor_pool_, nullptr);

            vkDestroyPipeline(context->device->GetDevice(), compute_pipeline_, nullptr);
            vkDestroyPipeline(context->device->GetDevice(), subsample_pipeline_, nullptr);
            vkDestroyPipeline(context->device->GetDevice(), trace_traversal_pipeline_, nullptr);
            vkDestroyPipelineLayout(context->device->GetDevice(), trace_traversal_pipeline_layout_, nullptr);
        };

        void TraversalRenderModule::SetupDescriptorPool()
        {
            uint32_t swapchain_size = context_->swapchain->GetBackBufferCount();

            // Set pool sizes
            VkDescriptorPoolSize descriptor_pool_size_uniform_buffer = {};
            descriptor_pool_size_uniform_buffer.type                 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_pool_size_uniform_buffer.descriptorCount      = 1 * swapchain_size;

            VkDescriptorPoolSize descriptor_pool_size_image_sampler = {};
            descriptor_pool_size_image_sampler.type                 = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_pool_size_image_sampler.descriptorCount      = 1 * swapchain_size;

            VkDescriptorPoolSize descriptor_pool_size_storage_buffer = {};
            descriptor_pool_size_storage_buffer.type                 = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptor_pool_size_storage_buffer.descriptorCount = (6 + 2 * static_cast<uint32_t>(GetVkGraphicsContext()->GetBlases().size())) * swapchain_size;

            std::vector<VkDescriptorPoolSize> pool_sizes = {
                descriptor_pool_size_uniform_buffer,
                descriptor_pool_size_image_sampler,
                descriptor_pool_size_storage_buffer,
            };

            // Arrange pool creation info.
            VkDescriptorPoolCreateInfo descriptor_pool_info = {};
            descriptor_pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            descriptor_pool_info.poolSizeCount              = static_cast<uint32_t>(pool_sizes.size());
            descriptor_pool_info.pPoolSizes                 = pool_sizes.data();
            descriptor_pool_info.maxSets                    = swapchain_size;

            // Create descriptor pool.
            VkResult create_result = vkCreateDescriptorPool(context_->device->GetDevice(), &descriptor_pool_info, nullptr, &descriptor_pool_);
            CheckResult(create_result, "Failed to create descriptor pool.");

            traversal_descriptor_sets_.resize(swapchain_size);
            std::vector<VkDescriptorSetLayout> set_layouts(swapchain_size, traversal_descriptor_set_layout_);

            // Allocate sets.
            VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
            descriptor_set_allocate_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptor_set_allocate_info.descriptorPool              = descriptor_pool_;
            descriptor_set_allocate_info.pSetLayouts                 = set_layouts.data();
            descriptor_set_allocate_info.descriptorSetCount          = swapchain_size;

            VkResult alloc_result = vkAllocateDescriptorSets(context_->device->GetDevice(), &descriptor_set_allocate_info, traversal_descriptor_sets_.data());
            CheckResult(alloc_result, "Failed to allocate descriptor sets.");
        }

        void TraversalRenderModule::SetupDescriptorSetLayoutAndPipelineLayout()
        {
            // Construct binding information.
            VkDescriptorSetLayoutBinding scene_layout_binding_0 = {};
            scene_layout_binding_0.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            scene_layout_binding_0.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
            scene_layout_binding_0.binding                      = 0;
            scene_layout_binding_0.descriptorCount              = 1;

            VkDescriptorSetLayoutBinding scene_layout_binding_heatmap = {};
            scene_layout_binding_heatmap.descriptorType               = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            scene_layout_binding_heatmap.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
            scene_layout_binding_heatmap.binding                      = 1;
            scene_layout_binding_heatmap.descriptorCount              = 1;

            VkDescriptorSetLayoutBinding scene_layout_binding_1 = {};
            scene_layout_binding_1.descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            scene_layout_binding_1.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
            scene_layout_binding_1.binding                      = 11;
            scene_layout_binding_1.descriptorCount              = 1;

            VkDescriptorSetLayoutBinding scene_layout_binding_2 = {};
            scene_layout_binding_2.descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            scene_layout_binding_2.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
            scene_layout_binding_2.binding                      = 12;
            scene_layout_binding_2.descriptorCount              = 1;

            VkDescriptorSetLayoutBinding scene_layout_binding_3 = {};
            scene_layout_binding_3.descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            scene_layout_binding_3.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
            scene_layout_binding_3.binding                      = 13;
            scene_layout_binding_3.descriptorCount              = 1;

            auto blases = GetVkGraphicsContext()->GetBlases();

            VkDescriptorSetLayoutBinding scene_layout_binding_4 = {};
            scene_layout_binding_4.descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            scene_layout_binding_4.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
            scene_layout_binding_4.binding                      = 14;
            scene_layout_binding_4.descriptorCount              = static_cast<uint32_t>(blases.size());

            VkDescriptorSetLayoutBinding scene_layout_binding_5 = {};
            scene_layout_binding_5.descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            scene_layout_binding_5.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
            scene_layout_binding_5.binding                      = 15;
            scene_layout_binding_5.descriptorCount              = static_cast<uint32_t>(blases.size());

            VkDescriptorSetLayoutBinding scene_layout_binding_6 = {};
            scene_layout_binding_6.descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            scene_layout_binding_6.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
            scene_layout_binding_6.binding                      = 16;
            scene_layout_binding_6.descriptorCount              = 1;

            VkDescriptorSetLayoutBinding scene_layout_binding_7 = {};
            scene_layout_binding_7.descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            scene_layout_binding_7.stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT;
            scene_layout_binding_7.binding                      = 17;
            scene_layout_binding_7.descriptorCount              = 1;

            std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {scene_layout_binding_0,
                                                                             scene_layout_binding_heatmap,
                                                                             scene_layout_binding_1,
                                                                             scene_layout_binding_2,
                                                                             scene_layout_binding_3,
                                                                             scene_layout_binding_4,
                                                                             scene_layout_binding_5,
                                                                             scene_layout_binding_6,
                                                                             scene_layout_binding_7};

            // Create set layout.
            VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
            descriptor_set_layout_create_info.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptor_set_layout_create_info.pBindings                       = set_layout_bindings.data();
            descriptor_set_layout_create_info.pBindings                       = set_layout_bindings.data();
            descriptor_set_layout_create_info.bindingCount                    = static_cast<uint32_t>(set_layout_bindings.size());

            VkResult create_result =
                vkCreateDescriptorSetLayout(context_->device->GetDevice(), &descriptor_set_layout_create_info, nullptr, &traversal_descriptor_set_layout_);
            CheckResult(create_result, "Failed to create descriptor set layout.");

            // Create pipeline layout.
            VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
            pipeline_layout_create_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.setLayoutCount             = 1;
            pipeline_layout_create_info.pSetLayouts                = &traversal_descriptor_set_layout_;

            create_result = vkCreatePipelineLayout(context_->device->GetDevice(), &pipeline_layout_create_info, nullptr, &trace_traversal_pipeline_layout_);
            CheckResult(create_result, "Failed to create descriptor set layout.");
        }

        void TraversalRenderModule::UpdateDescriptorSet(uint32_t frame_id)
        {
            if (traversal_descriptor_set_update_flags_[frame_id])
            {
                return;
            }

            traversal_descriptor_set_update_flags_[frame_id] = true;

            std::vector<VkWriteDescriptorSet> write_descriptor_sets;

            // Binding 1 : Volume Storage
            VkDescriptorBufferInfo volume_info = {};
            volume_info.buffer                 = top_level_volumes_guard_.GetCurrentBuffer();
            volume_info.offset                 = 0;
            volume_info.range                  = VK_WHOLE_SIZE;

            VkWriteDescriptorSet write_descriptor_1 = {};
            write_descriptor_1.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_descriptor_1.dstSet               = traversal_descriptor_sets_[frame_id];
            write_descriptor_1.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write_descriptor_1.dstBinding           = 11;
            write_descriptor_1.pBufferInfo          = &volume_info;
            write_descriptor_1.descriptorCount      = 1;

            // Binding 2 : Vertex Storage
            VkDescriptorBufferInfo vertex_info = {};
            vertex_info.buffer                 = top_level_vertices_guard_.GetCurrentBuffer();
            vertex_info.offset                 = 0;
            vertex_info.range                  = VK_WHOLE_SIZE;

            VkWriteDescriptorSet write_descriptor_2 = {};
            write_descriptor_2.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_descriptor_2.dstSet               = traversal_descriptor_sets_[frame_id];
            write_descriptor_2.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write_descriptor_2.dstBinding           = 12;
            write_descriptor_2.pBufferInfo          = &vertex_info;
            write_descriptor_2.descriptorCount      = 1;

            // Binding 3 : Instance Storage
            VkDescriptorBufferInfo instance_info = {};
            instance_info.buffer                 = top_level_instances_guard_.GetCurrentBuffer();
            instance_info.offset                 = 0;
            instance_info.range                  = VK_WHOLE_SIZE;

            VkWriteDescriptorSet write_descriptor_3 = {};
            write_descriptor_3.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_descriptor_3.dstSet               = traversal_descriptor_sets_[frame_id];
            write_descriptor_3.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write_descriptor_3.dstBinding           = 13;
            write_descriptor_3.pBufferInfo          = &instance_info;
            write_descriptor_3.descriptorCount      = 1;

            auto blases = GetVkGraphicsContext()->GetBlases();

            std::vector<VkDescriptorBufferInfo> blas_volumes_info;
            blas_volumes_info.reserve(blases.size());

            // Binding 4 : Volume Storage for blasses
            for (auto& blas : blases)
            {
                VkDescriptorBufferInfo blas_volume_info = {};
                blas_volume_info.buffer                 = blas.volume_buffer;
                blas_volume_info.offset                 = 0;
                blas_volume_info.range                  = VK_WHOLE_SIZE;

                blas_volumes_info.push_back(blas_volume_info);
            }

            VkWriteDescriptorSet write_descriptor_4 = {};
            write_descriptor_4.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_descriptor_4.dstSet               = traversal_descriptor_sets_[frame_id];
            write_descriptor_4.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write_descriptor_4.dstBinding           = 14;
            write_descriptor_4.pBufferInfo          = blas_volumes_info.data();
            write_descriptor_4.descriptorCount      = static_cast<uint32_t>(blas_volumes_info.size());

            std::vector<VkDescriptorBufferInfo> blas_vertices_info;
            blas_vertices_info.reserve(blases.size());

            // Binding 5 : Vertex Storage for blasses
            for (auto& blas : blases)
            {
                VkDescriptorBufferInfo blas_vertex_info = {};
                blas_vertex_info.buffer                 = blas.vertex_buffer;
                blas_vertex_info.offset                 = 0;
                blas_vertex_info.range                  = VK_WHOLE_SIZE;

                blas_vertices_info.push_back(blas_vertex_info);
            }

            VkWriteDescriptorSet write_descriptor_5 = {};
            write_descriptor_5.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_descriptor_5.dstSet               = traversal_descriptor_sets_[frame_id];
            write_descriptor_5.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write_descriptor_5.dstBinding           = 15;
            write_descriptor_5.pBufferInfo          = blas_vertices_info.data();
            write_descriptor_5.descriptorCount      = static_cast<uint32_t>(blas_vertices_info.size());

            write_descriptor_sets = {write_descriptor_2, write_descriptor_3, write_descriptor_4, write_descriptor_5};

            if (volume_info.buffer != VK_NULL_HANDLE)
            {
                write_descriptor_sets.push_back(write_descriptor_1);
            }

            vkUpdateDescriptorSets(
                context_->device->GetDevice(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
        }

        void TraversalRenderModule::UploadTraversalData(const RenderFrameContext* context)
        {
            // Generate CPU side traversal information.
            auto& traversal_tree = context->scene_info->traversal_tree;

            empty_scene_ = traversal_tree.volumes.empty();

            // Padding to have a valid pipeline.
            traversal_tree.instances.emplace_back();
            traversal_tree.vertices.emplace_back();

            // Cycle guard information in case of no data.
            top_level_volumes_guard_.SetCurrentBuffer(VK_NULL_HANDLE, VK_NULL_HANDLE);
            top_level_vertices_guard_.SetCurrentBuffer(VK_NULL_HANDLE, VK_NULL_HANDLE);
            top_level_instances_guard_.SetCurrentBuffer(VK_NULL_HANDLE, VK_NULL_HANDLE);

            top_level_volumes_staging_guard_.SetCurrentBuffer(VK_NULL_HANDLE, VK_NULL_HANDLE);
            top_level_vertices_staging_guard_.SetCurrentBuffer(VK_NULL_HANDLE, VK_NULL_HANDLE);
            top_level_instances_staging_guard_.SetCurrentBuffer(VK_NULL_HANDLE, VK_NULL_HANDLE);

            // Upload tree information for volumes, instances and triangles.
            if (!traversal_tree.volumes.empty())
            {
                VkBuffer      volume_buffer     = VK_NULL_HANDLE;
                VmaAllocation volume_allocation = VK_NULL_HANDLE;

                VkBuffer      volume_staging_buffer     = VK_NULL_HANDLE;
                VmaAllocation volume_staging_allocation = VK_NULL_HANDLE;

                auto buffer_size = traversal_tree.volumes.size() * sizeof(TraversalVolume);

                context->device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                              VMA_MEMORY_USAGE_CPU_ONLY,
                                              volume_staging_buffer,
                                              volume_staging_allocation,
                                              traversal_tree.volumes.data(),
                                              buffer_size);

                context->device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                              VMA_MEMORY_USAGE_GPU_ONLY,
                                              volume_buffer,
                                              volume_allocation,
                                              nullptr,
                                              buffer_size);

                SetObjectName(context_->device->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)volume_staging_buffer, "volumeStagingBufferTraversal");
                SetObjectName(context_->device->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)volume_buffer, "volumeBufferTraversal");

                VkBufferCopy copy_region = {};
                copy_region.size         = buffer_size;
                vkCmdCopyBuffer(context->command_buffer, volume_staging_buffer, volume_buffer, 1, &copy_region);

                VkBufferMemoryBarrier buffer_barrier{};
                buffer_barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                buffer_barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
                buffer_barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
                buffer_barrier.srcQueueFamilyIndex = context_->device->GetComputeQueueFamilyIndex();
                buffer_barrier.dstQueueFamilyIndex = context_->device->GetComputeQueueFamilyIndex();
                buffer_barrier.buffer              = volume_buffer;
                buffer_barrier.offset              = 0;
                buffer_barrier.size                = VK_WHOLE_SIZE;

                vkCmdPipelineBarrier(context->command_buffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     0,
                                     0,
                                     nullptr,
                                     1,
                                     &buffer_barrier,
                                     0,
                                     nullptr);

                top_level_volumes_guard_.SetCurrentBuffer(volume_buffer, volume_allocation);
                top_level_volumes_staging_guard_.SetCurrentBuffer(volume_staging_buffer, volume_staging_allocation);
            }

            if (!traversal_tree.vertices.empty())
            {
                VkBuffer      vertex_buffer     = VK_NULL_HANDLE;
                VmaAllocation vertex_allocation = VK_NULL_HANDLE;

                VkBuffer      triangle_staging_buffer     = VK_NULL_HANDLE;
                VmaAllocation triangle_staging_allocation = VK_NULL_HANDLE;

                auto buffer_size = traversal_tree.vertices.size() * sizeof(RraVertex);

                context->device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                              VMA_MEMORY_USAGE_CPU_ONLY,
                                              triangle_staging_buffer,
                                              triangle_staging_allocation,
                                              traversal_tree.vertices.data(),
                                              buffer_size);

                context->device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                              VMA_MEMORY_USAGE_GPU_ONLY,
                                              vertex_buffer,
                                              vertex_allocation,
                                              nullptr,
                                              buffer_size);

                SetObjectName(context_->device->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)triangle_staging_buffer, "triangleStagingBufferTraversal");
                SetObjectName(context_->device->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)vertex_buffer, "vertexBufferTraversal");

                VkBufferCopy copy_region = {};
                copy_region.size         = buffer_size;
                vkCmdCopyBuffer(context->command_buffer, triangle_staging_buffer, vertex_buffer, 1, &copy_region);

                VkBufferMemoryBarrier buffer_barrier{};
                buffer_barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                buffer_barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
                buffer_barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
                buffer_barrier.srcQueueFamilyIndex = context_->device->GetGraphicsQueueFamilyIndex();
                buffer_barrier.dstQueueFamilyIndex = context_->device->GetGraphicsQueueFamilyIndex();
                buffer_barrier.buffer              = vertex_buffer;
                buffer_barrier.offset              = 0;
                buffer_barrier.size                = VK_WHOLE_SIZE;

                vkCmdPipelineBarrier(context->command_buffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     0,
                                     0,
                                     nullptr,
                                     1,
                                     &buffer_barrier,
                                     0,
                                     nullptr);

                top_level_vertices_guard_.SetCurrentBuffer(vertex_buffer, vertex_allocation);
                top_level_vertices_staging_guard_.SetCurrentBuffer(triangle_staging_buffer, triangle_staging_allocation);
            }

            if (!traversal_tree.instances.empty())
            {
                VkBuffer      instance_buffer     = VK_NULL_HANDLE;
                VmaAllocation instance_allocation = VK_NULL_HANDLE;

                VkBuffer      instance_staging_buffer     = VK_NULL_HANDLE;
                VmaAllocation instance_staging_allocation = VK_NULL_HANDLE;

                if (traversal_tree.instances.empty())
                {
                    traversal_tree.instances.push_back({});
                }

                auto buffer_size = traversal_tree.instances.size() * sizeof(TraversalInstance);

                context->device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                              VMA_MEMORY_USAGE_CPU_ONLY,
                                              instance_staging_buffer,
                                              instance_staging_allocation,
                                              traversal_tree.instances.data(),
                                              buffer_size);

                context->device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                              VMA_MEMORY_USAGE_GPU_ONLY,
                                              instance_buffer,
                                              instance_allocation,
                                              nullptr,
                                              buffer_size);

                SetObjectName(context_->device->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)instance_staging_buffer, "instanceStagingBufferTraversal");
                SetObjectName(context_->device->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)instance_buffer, "instanceBufferTraversal");

                VkBufferCopy copy_region = {};
                copy_region.size         = buffer_size;
                vkCmdCopyBuffer(context->command_buffer, instance_staging_buffer, instance_buffer, 1, &copy_region);

                VkBufferMemoryBarrier buffer_barrier{};
                buffer_barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                buffer_barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
                buffer_barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
                buffer_barrier.srcQueueFamilyIndex = context_->device->GetComputeQueueFamilyIndex();
                buffer_barrier.dstQueueFamilyIndex = context_->device->GetComputeQueueFamilyIndex();
                buffer_barrier.buffer              = instance_buffer;
                buffer_barrier.offset              = 0;
                buffer_barrier.size                = VK_WHOLE_SIZE;

                vkCmdPipelineBarrier(context->command_buffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     0,
                                     0,
                                     nullptr,
                                     1,
                                     &buffer_barrier,
                                     0,
                                     nullptr);

                top_level_instances_guard_.SetCurrentBuffer(instance_buffer, instance_allocation);
                top_level_instances_staging_guard_.SetCurrentBuffer(instance_staging_buffer, instance_staging_allocation);
            }

            traversal_descriptor_set_update_flags_.clear();
            traversal_descriptor_set_update_flags_.resize(context_->swapchain->GetBackBufferCount(), false);
        }

        void TraversalRenderModule::QueueTraversalCounterRangeUpdate(std::function<void(uint32_t min, uint32_t max)> update_function)
        {
            traversal_counter_range_update_functions_.push_back(update_function);
        }

        void TraversalRenderModule::SetTraversalCounterContinuousUpdateFunction(std::function<void(uint32_t min, uint32_t max)> update_function)
        {
            traversal_counter_range_continuous_update_function_ = update_function;
        }

        void TraversalRenderModule::SetHistogramUpdateFunction(std::function<void(const std::vector<uint32_t>&, uint32_t, uint32_t)> update_function,
                                                               uint32_t                                                              max_traversal_setting)
        {
            histogram_update_function_       = update_function;
            traversal_count_setting_changed_ = max_traversal_count_setting_ != max_traversal_setting;
            max_traversal_count_setting_     = max_traversal_setting;
        }

        bool TraversalRenderModule::IsTraversalCounterContinuousUpdateFunctionSet()
        {
            return traversal_counter_range_continuous_update_function_ != nullptr;
        }

    }  // namespace renderer
}  // namespace rra
