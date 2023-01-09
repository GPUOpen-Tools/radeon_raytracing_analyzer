//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the selection rendering module.
//=============================================================================

#include "selection_module.h"
#include "glm/glm/gtc/matrix_transform.hpp"

namespace rra
{
    namespace renderer
    {
        static const uint32_t kVertexBufferBindId   = 0;
        static const uint32_t kInstanceBufferBindId = 1;

        static const int kRenderOutlineEnabled   = 0x1;
        static const int kRenderTransformEnabled = 0x2;

        SelectionRenderModule::SelectionRenderModule()
            : RenderModule(RenderPassHint::kRenderPassHintClearDepthOnly)
        {
        }

        void SelectionRenderModule::Initialize(const RenderModuleContext* context)
        {
            context_ = context;

            wireframe_box_mesh_.Initialize(context->device, context->command_buffer_ring);

            SetupDescriptorSetLayout();

            VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
            pipeline_layout_create_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.setLayoutCount             = 1;
            pipeline_layout_create_info.pSetLayouts                = &descriptor_set_layout_;

            VkResult create_result = vkCreatePipelineLayout(context->device->GetDevice(), &pipeline_layout_create_info, nullptr, &pipeline_layout_);
            CheckResult(create_result, "Failed to create pipeline layout.");

            VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
            input_assembly_state.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            input_assembly_state.topology                               = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            input_assembly_state.flags                                  = 0;
            input_assembly_state.primitiveRestartEnable                 = VK_FALSE;

            VkPipelineRasterizationStateCreateInfo rasterization_state = {};
            rasterization_state.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterization_state.polygonMode                            = VK_POLYGON_MODE_FILL;
            rasterization_state.cullMode                               = VK_CULL_MODE_NONE;
            rasterization_state.frontFace                              = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterization_state.flags                                  = 0;
            rasterization_state.depthClampEnable                       = VK_FALSE;
            rasterization_state.lineWidth                              = 4.0f;

            VkPipelineColorBlendAttachmentState blend_attachment_state = {};
            blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            blend_attachment_state.blendEnable    = VK_TRUE;
            blend_attachment_state.alphaBlendOp   = VK_BLEND_OP_ADD;
            blend_attachment_state.colorBlendOp   = VK_BLEND_OP_ADD;
            blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

            VkPipelineColorBlendStateCreateInfo color_blend_state = {};
            color_blend_state.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blend_state.attachmentCount                     = 1;
            color_blend_state.pAttachments                        = &blend_attachment_state;

            VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {};
            depth_stencil_state.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil_state.depthTestEnable                       = VK_FALSE;
            depth_stencil_state.depthWriteEnable                      = VK_FALSE;
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

            std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;

            const char* full_vertex_shader_path = "SelectionVolume.vs.spv";
            const char* full_pixel_shader_path  = "SelectionVolume.ps.spv";

            LoadShader(full_vertex_shader_path, context->device, VK_SHADER_STAGE_VERTEX_BIT, "VSMain", shader_stages[0]);
            LoadShader(full_pixel_shader_path, context->device, VK_SHADER_STAGE_FRAGMENT_BIT, "PSMain", shader_stages[1]);

            VkGraphicsPipelineCreateInfo pipeline_create_info = {};
            pipeline_create_info.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipeline_create_info.layout                       = pipeline_layout_;
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

            // Binding point 0: Mesh vertex layout description at per-vertex rate
            VkVertexInputBindingDescription mesh_binding_description = {};
            mesh_binding_description.binding                         = kVertexBufferBindId;
            mesh_binding_description.stride                          = sizeof(WireframeBoxMeshInfo::VertexType);
            mesh_binding_description.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

            // Binding point 1: Instanced data at per-instance rate
            VkVertexInputBindingDescription instance_binding_description = {};
            instance_binding_description.binding                         = kInstanceBufferBindId;
            instance_binding_description.stride                          = sizeof(SelectedVolumeInstance);
            instance_binding_description.inputRate                       = VK_VERTEX_INPUT_RATE_INSTANCE;

            std::vector<VkVertexInputBindingDescription> binding_descriptions = {mesh_binding_description, instance_binding_description};

            std::vector<VkVertexInputAttributeDescription> attribute_descriptions;

            // Per-vertex attributes
            // These are advanced for each vertex fetched by the vertex shader.
            VkVertexInputAttributeDescription vertex_position_attrib = {
                0, kVertexBufferBindId, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexPosition, position)  // Location 0: Position
            };

            // Per-Instance attributes
            // These are fetched for each instance rendered.
            VkVertexInputAttributeDescription instance_min_attrib = {
                1, kInstanceBufferBindId, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(SelectedVolumeInstance, min)};
            VkVertexInputAttributeDescription instance_max_attrib = {
                2, kInstanceBufferBindId, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SelectedVolumeInstance, max)};
            VkVertexInputAttributeDescription instance_substrate_attrib = {
                3, kInstanceBufferBindId, VK_FORMAT_R32_UINT, offsetof(SelectedVolumeInstance, is_transform)};

            // A float4x4 must skip 4 byte location indices 4->7.
            VkVertexInputAttributeDescription transform_attrib_0 = {
                4, kInstanceBufferBindId, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(SelectedVolumeInstance, transform) + sizeof(glm::vec4) * 0};

            VkVertexInputAttributeDescription transform_attrib_1 = {
                5, kInstanceBufferBindId, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(SelectedVolumeInstance, transform) + sizeof(glm::vec4) * 1};

            VkVertexInputAttributeDescription transform_attrib_2 = {
                6, kInstanceBufferBindId, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(SelectedVolumeInstance, transform) + sizeof(glm::vec4) * 2};

            VkVertexInputAttributeDescription transform_attrib_3 = {
                7, kInstanceBufferBindId, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(SelectedVolumeInstance, transform) + sizeof(glm::vec4) * 3};

            attribute_descriptions = {
                vertex_position_attrib,
                instance_min_attrib,
                instance_max_attrib,
                instance_substrate_attrib,
                transform_attrib_0,
                transform_attrib_1,
                transform_attrib_2,
                transform_attrib_3,
            };

            VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
            vertex_input_state_info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_state_info.pVertexBindingDescriptions           = binding_descriptions.data();
            vertex_input_state_info.vertexBindingDescriptionCount        = static_cast<uint32_t>(binding_descriptions.size());
            vertex_input_state_info.pVertexAttributeDescriptions         = attribute_descriptions.data();
            vertex_input_state_info.vertexAttributeDescriptionCount      = static_cast<uint32_t>(attribute_descriptions.size());

            pipeline_create_info.pVertexInputState = &vertex_input_state_info;

            create_result = vkCreateGraphicsPipelines(context->device->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline_);
            CheckResult(create_result, "Failed to create pipeline.");

            SetupDescriptorPool();
            SetupDescriptorSet();

            vkDestroyShaderModule(context_->device->GetDevice(), shader_stages[0].module, nullptr);
            vkDestroyShaderModule(context_->device->GetDevice(), shader_stages[1].module, nullptr);

            instance_buffer_guard_.Initialize(context_->swapchain->GetBackBufferCount());
            instance_staging_buffer_guard_.Initialize(context_->swapchain->GetBackBufferCount());
        }  // namespace renderer

        void SelectionRenderModule::Draw(const RenderFrameContext* context)
        {
            if (!context->scene_info)
            {
                return;
            }

            if (context->scene_info && (context->scene_info->scene_iteration != last_scene_iteration_ || last_scene_ != context->scene_info) ||
                last_render_state_ != render_state_)
            {
                std::vector<SelectedVolumeInstance> selected_volume_instances;
                selected_volume_instances.reserve(context->scene_info->selected_volume_instances.size());

                // configure which instances are going to be rendered.
                for (const auto& instance : context->scene_info->selected_volume_instances)
                {
                    if (instance.is_transform && (render_state_ & kRenderTransformEnabled))
                    {
                        selected_volume_instances.push_back(instance);
                    }
                    else if (!instance.is_transform && (render_state_ & kRenderOutlineEnabled))
                    {
                        selected_volume_instances.push_back(instance);
                    }
                }

                CreateAndUploadInstanceBuffer(selected_volume_instances, context);

                last_scene_iteration_ = context->scene_info->scene_iteration;
                last_scene_           = context->scene_info;
                last_render_state_    = render_state_;
            }

            if (instance_buffer_.size == 0)
            {
                return;
            }

            instance_buffer_guard_.ProcessFrame(context->current_frame, context->device);
            instance_staging_buffer_guard_.ProcessFrame(context->current_frame, context->device);

            std::vector<VkWriteDescriptorSet> write_descriptor_sets;

            VkWriteDescriptorSet write_descriptor_set = {};
            write_descriptor_set.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_descriptor_set.dstSet               = descriptor_sets_[context->current_frame];
            write_descriptor_set.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write_descriptor_set.dstBinding           = 0;
            write_descriptor_set.pBufferInfo          = &context->scene_ubo_info;
            write_descriptor_set.descriptorCount      = 1;

            write_descriptor_sets = {write_descriptor_set};
            vkUpdateDescriptorSets(context->device->GetDevice(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);

            context->begin_render_pass();

            VkDeviceSize offsets[1] = {0};

            vkCmdBindDescriptorSets(
                context->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 0, 1, &descriptor_sets_[context->current_frame], 0, nullptr);

            vkCmdBindPipeline(context->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

            vkCmdBindVertexBuffers(context->command_buffer, kVertexBufferBindId, 1, &wireframe_box_mesh_.GetVertices().buffer, offsets);
            vkCmdBindVertexBuffers(context->command_buffer, kInstanceBufferBindId, 1, &instance_buffer_.buffer, offsets);
            vkCmdBindIndexBuffer(context->command_buffer, wireframe_box_mesh_.GetIndices().buffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdDrawIndexed(context->command_buffer, wireframe_box_mesh_.GetIndices().count, instance_buffer_.instance_count, 0, 0, 0);

            context->end_render_pass();
        }

        void SelectionRenderModule::Cleanup(const RenderModuleContext* context)
        {
            wireframe_box_mesh_.Cleanup(context->device);
            instance_buffer_guard_.Cleanup(context->device);
            instance_staging_buffer_guard_.Cleanup(context->device);
            vkDestroyDescriptorSetLayout(context->device->GetDevice(), descriptor_set_layout_, nullptr);
            vkDestroyDescriptorPool(context->device->GetDevice(), descriptor_pool_, nullptr);
            vkDestroyPipeline(context->device->GetDevice(), pipeline_, nullptr);
            vkDestroyPipelineLayout(context->device->GetDevice(), pipeline_layout_, nullptr);
        }

        void SelectionRenderModule::EnableOutlineRendering()
        {
            render_state_ = render_state_ | kRenderOutlineEnabled;
        }

        void SelectionRenderModule::DisableOutlineRendering()
        {
            render_state_ = render_state_ & (~kRenderOutlineEnabled);
        }

        bool SelectionRenderModule::IsTransformRenderingEnabled()
        {
            return render_state_ & kRenderTransformEnabled;
        }

        void SelectionRenderModule::EnableTransformRendering()
        {
            render_state_ = render_state_ | kRenderTransformEnabled;
        }

        void SelectionRenderModule::DisableTransformRendering()
        {
            render_state_ = render_state_ & (~kRenderTransformEnabled);
        }

        void SelectionRenderModule::CreateAndUploadInstanceBuffer(const std::vector<SelectedVolumeInstance>& selected_volume_instances,
                                                                  const RenderFrameContext*                  context)
        {
            auto device = context_->device;

            struct
            {
                VkBuffer      buffer     = VK_NULL_HANDLE;
                VmaAllocation allocation = VK_NULL_HANDLE;
            } staging_buffer = {};

            instance_buffer_.size           = selected_volume_instances.size() * sizeof(SelectedVolumeInstance);
            instance_buffer_.instance_count = static_cast<uint32_t>(selected_volume_instances.size());
            if (instance_buffer_.size == 0)
            {
                return;
            }

            device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VMA_MEMORY_USAGE_CPU_ONLY,
                                 staging_buffer.buffer,
                                 staging_buffer.allocation,
                                 selected_volume_instances.data(),
                                 instance_buffer_.size);

            instance_buffer_.buffer     = VK_NULL_HANDLE;
            instance_buffer_.allocation = VK_NULL_HANDLE;

            device->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 VMA_MEMORY_USAGE_GPU_ONLY,
                                 instance_buffer_.buffer,
                                 instance_buffer_.allocation,
                                 nullptr,
                                 instance_buffer_.size);

            // Prevent WRITE_AFTER_READ.
            VkBufferMemoryBarrier buffer_barrier1{};
            buffer_barrier1.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            buffer_barrier1.srcAccessMask       = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
            buffer_barrier1.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
            buffer_barrier1.srcQueueFamilyIndex = context_->device->GetGraphicsQueueFamilyIndex();
            buffer_barrier1.dstQueueFamilyIndex = context_->device->GetGraphicsQueueFamilyIndex();
            buffer_barrier1.buffer              = instance_buffer_.buffer;
            buffer_barrier1.offset              = 0;
            buffer_barrier1.size                = VK_WHOLE_SIZE;

            vkCmdPipelineBarrier(context->command_buffer,
                                 VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_DEPENDENCY_BY_REGION_BIT,
                                 0,
                                 nullptr,
                                 1,
                                 &buffer_barrier1,
                                 0,
                                 nullptr);

            VkBufferCopy copy_region = {};
            copy_region.size         = instance_buffer_.size;
            vkCmdCopyBuffer(context->command_buffer, staging_buffer.buffer, instance_buffer_.buffer, 1, &copy_region);

            // Prevent READ_AFTER_WRITE.
            VkBufferMemoryBarrier buffer_barrier2{};
            buffer_barrier2.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            buffer_barrier2.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
            buffer_barrier2.dstAccessMask       = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
            buffer_barrier2.srcQueueFamilyIndex = context_->device->GetGraphicsQueueFamilyIndex();
            buffer_barrier2.dstQueueFamilyIndex = context_->device->GetGraphicsQueueFamilyIndex();
            buffer_barrier2.buffer              = instance_buffer_.buffer;
            buffer_barrier2.offset              = 0;
            buffer_barrier2.size                = VK_WHOLE_SIZE;

            vkCmdPipelineBarrier(context->command_buffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                                 VK_DEPENDENCY_BY_REGION_BIT,
                                 0,
                                 nullptr,
                                 1,
                                 &buffer_barrier2,
                                 0,
                                 nullptr);

            instance_buffer_guard_.SetCurrentBuffer(instance_buffer_.buffer, instance_buffer_.allocation);
            instance_staging_buffer_guard_.SetCurrentBuffer(staging_buffer.buffer, staging_buffer.allocation);
        }

        void SelectionRenderModule::SetupDescriptorPool()
        {
            auto device = context_->device;

            VkDescriptorPoolSize descriptor_pool_size = {};
            descriptor_pool_size.type                 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_pool_size.descriptorCount      = context_->swapchain->GetBackBufferCount();

            std::vector<VkDescriptorPoolSize> pool_sizes = {descriptor_pool_size};

            VkDescriptorPoolCreateInfo descriptor_pool_info = {};
            descriptor_pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            descriptor_pool_info.poolSizeCount              = static_cast<uint32_t>(pool_sizes.size());
            descriptor_pool_info.pPoolSizes                 = pool_sizes.data();
            descriptor_pool_info.maxSets                    = context_->swapchain->GetBackBufferCount();

            VkResult create_result = vkCreateDescriptorPool(device->GetDevice(), &descriptor_pool_info, nullptr, &descriptor_pool_);
            CheckResult(create_result, "Failed to create descriptor pool.");
        }

        void SelectionRenderModule::SetupDescriptorSetLayout()
        {
            auto device = context_->device;

            VkDescriptorSetLayoutBinding set_layout_binding = {};
            set_layout_binding.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            set_layout_binding.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            set_layout_binding.binding                      = 0;
            set_layout_binding.descriptorCount              = 1;

            std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {set_layout_binding};

            VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
            descriptor_set_layout_create_info.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptor_set_layout_create_info.pBindings                       = set_layout_bindings.data();
            descriptor_set_layout_create_info.bindingCount                    = static_cast<uint32_t>(set_layout_bindings.size());

            VkResult create_result = vkCreateDescriptorSetLayout(device->GetDevice(), &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout_);
            CheckResult(create_result, "Failed to create descriptor set layout.");
        }

        void SelectionRenderModule::SetupDescriptorSet()
        {
            auto device = context_->device;

            descriptor_sets_.resize(context_->swapchain->GetBackBufferCount());
            std::vector<VkDescriptorSetLayout> set_layouts(context_->swapchain->GetBackBufferCount(), descriptor_set_layout_);

            VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
            descriptor_set_allocate_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptor_set_allocate_info.descriptorPool              = descriptor_pool_;
            descriptor_set_allocate_info.pSetLayouts                 = set_layouts.data();
            descriptor_set_allocate_info.descriptorSetCount          = static_cast<uint32_t>(descriptor_sets_.size());

            VkResult alloc_result = vkAllocateDescriptorSets(device->GetDevice(), &descriptor_set_allocate_info, descriptor_sets_.data());
            CheckResult(alloc_result, "Failed to allocate descriptor sets.");
        }

    }  // namespace renderer
}  // namespace rra
