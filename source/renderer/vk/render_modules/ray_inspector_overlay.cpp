//=============================================================================
// Copyright (c) 2022-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the selection rendering module.
//=============================================================================

#include "ray_inspector_overlay.h"
#include <math.h>

#define RAY_FLAGS_TERMINATE_ON_FIRST_HIT 4U

namespace rra::renderer
{

    RayInspectorOverlayRenderModule::RayInspectorOverlayRenderModule()
        : RenderModule(RenderPassHint::kRenderPassHintClearNoneDepthInput)
    {
    }

    void RayInspectorOverlayRenderModule::Initialize(const RenderModuleContext* context)
    {
        module_context_ = context;

        SetupRayLinesDescriptorSetLayout();
        SetupRayLinesDescriptorPool();

        SetupIconDescriptorSetLayout();
        SetupIconDescriptorPool();

        CreateRayLinesPipelineAndLayout();
        CreateIconPipelineAndLayout();

        ray_buffer_guard_.Initialize(context->swapchain->GetBackBufferCount());
        ray_staging_buffer_guard_.Initialize(context->swapchain->GetBackBufferCount());

        icon_buffer_guard_.Initialize(context->swapchain->GetBackBufferCount());
        icon_staging_buffer_guard_.Initialize(context->swapchain->GetBackBufferCount());

        depth_input_buffer_view_ = &context->swapchain->GetDepthInputImageView();
    }

    void RayInspectorOverlayRenderModule::Draw(const RenderFrameContext* context)
    {
        frame_context_ = context;

        SetRays(context->scene_info->ray_inspector_rays);
        UpdateRayBuffer(context->scene_info);
        UpdateIconBuffer();

        ray_staging_buffer_guard_.ProcessFrame(context->current_frame, context->device);
        ray_buffer_guard_.ProcessFrame(context->current_frame, context->device);

        icon_staging_buffer_guard_.ProcessFrame(context->current_frame, context->device);
        icon_buffer_guard_.ProcessFrame(context->current_frame, context->device);

        if (number_of_rays_ == 0)
        {
            return;
        }

        UpdateRayLinesDescriptorSet();
        UpdateIconDescriptorSet();

        context->begin_render_pass();

        RenderRayLinesPipeline();
        RenderIconPipeline();

        context->end_render_pass();
    }

    void RayInspectorOverlayRenderModule::Cleanup(const RenderModuleContext* context)
    {
        vkDestroyPipeline(context->device->GetDevice(), ray_lines_pipeline_, nullptr);
        vkDestroyPipelineLayout(context->device->GetDevice(), ray_lines_pipeline_layout_, nullptr);

        vkDestroyPipeline(context->device->GetDevice(), icon_pipeline_, nullptr);
        vkDestroyPipelineLayout(context->device->GetDevice(), icon_pipeline_layout_, nullptr);

        vkDestroyDescriptorPool(context->device->GetDevice(), ray_lines_descriptor_pool_, nullptr);
        vkDestroyDescriptorSetLayout(context->device->GetDevice(), ray_lines_descriptor_set_layout_, nullptr);

        vkDestroyDescriptorPool(context->device->GetDevice(), icon_descriptor_pool_, nullptr);
        vkDestroyDescriptorSetLayout(context->device->GetDevice(), icon_descriptor_set_layout_, nullptr);

        ray_staging_buffer_guard_.Cleanup(context->device);
        ray_buffer_guard_.Cleanup(context->device);

        icon_staging_buffer_guard_.Cleanup(context->device);
        icon_buffer_guard_.Cleanup(context->device);
    }

    void RayInspectorOverlayRenderModule::SetRays(std::vector<RayInspectorRay> rays)
    {
        rays_              = rays;
        update_ray_buffer_ = true;

        uint32_t first_ray_outline = frame_context_->scene_info->first_ray_outline;

        uint64_t selected_ray_tlas_address = first_ray_outline < (uint32_t)rays_.size() ? rays_[first_ray_outline].tlas_address : 0;

        rays_in_other_tlas_ = 0;
        for (uint32_t i = 0; i < first_ray_outline; ++i)
        {
            if (rays_[i].tlas_address != selected_ray_tlas_address)
            {
                ++rays_in_other_tlas_;
            }
        }

        // Filter rays that are not on the same acceleration structure as the selected ray.
        auto end_of_vector = std::remove_if(
            rays_.begin(), rays_.end(), [selected_ray_tlas_address](RayInspectorRay& ray) { return ray.tlas_address != selected_ray_tlas_address; });
        rays_.erase(end_of_vector, rays_.end());
    }

    void RayInspectorOverlayRenderModule::SetupRayLinesDescriptorSetLayout()
    {
        // Construct binding information.
        VkDescriptorSetLayoutBinding scene_layout_binding_0 = {};
        scene_layout_binding_0.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        scene_layout_binding_0.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        scene_layout_binding_0.binding                      = 0;
        scene_layout_binding_0.descriptorCount              = 1;

        VkDescriptorSetLayoutBinding scene_layout_binding_1 = {};
        scene_layout_binding_1.descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        scene_layout_binding_1.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        scene_layout_binding_1.binding                      = 1;
        scene_layout_binding_1.descriptorCount              = 1;

        VkDescriptorSetLayoutBinding scene_layout_binding_2 = {};
        scene_layout_binding_2.descriptorType               = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        scene_layout_binding_2.stageFlags                   = VK_SHADER_STAGE_FRAGMENT_BIT;
        scene_layout_binding_2.binding                      = 2;
        scene_layout_binding_2.descriptorCount              = 1;

        std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
            scene_layout_binding_0,
            scene_layout_binding_1,
            scene_layout_binding_2,
        };

        // Create set layout.
        VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
        descriptor_set_layout_create_info.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_layout_create_info.pBindings                       = set_layout_bindings.data();
        descriptor_set_layout_create_info.bindingCount                    = static_cast<uint32_t>(set_layout_bindings.size());

        VkResult create_result =
            vkCreateDescriptorSetLayout(module_context_->device->GetDevice(), &descriptor_set_layout_create_info, nullptr, &ray_lines_descriptor_set_layout_);
        CheckResult(create_result, "Failed to create descriptor set layout.");
    }

    void RayInspectorOverlayRenderModule::SetupRayLinesDescriptorPool()
    {
        uint32_t swapchain_size = module_context_->swapchain->GetBackBufferCount();

        // Set pool sizes
        VkDescriptorPoolSize descriptor_pool_size_uniform_buffer = {};
        descriptor_pool_size_uniform_buffer.type                 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_pool_size_uniform_buffer.descriptorCount      = 1 * swapchain_size;

        VkDescriptorPoolSize descriptor_pool_size_image_sampler = {};
        descriptor_pool_size_image_sampler.type                 = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptor_pool_size_image_sampler.descriptorCount      = 1 * swapchain_size;

        VkDescriptorPoolSize descriptor_pool_size_input_attachment = {};
        descriptor_pool_size_input_attachment.type                 = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptor_pool_size_input_attachment.descriptorCount      = 1 * swapchain_size;

        std::vector<VkDescriptorPoolSize> pool_sizes = {
            descriptor_pool_size_uniform_buffer,
            descriptor_pool_size_image_sampler,
            descriptor_pool_size_input_attachment,
        };

        // Arrange pool creation info.
        VkDescriptorPoolCreateInfo descriptor_pool_info = {};
        descriptor_pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_info.poolSizeCount              = static_cast<uint32_t>(pool_sizes.size());
        descriptor_pool_info.pPoolSizes                 = pool_sizes.data();
        descriptor_pool_info.maxSets                    = swapchain_size;

        // Create descriptor pool.
        VkResult create_result = vkCreateDescriptorPool(module_context_->device->GetDevice(), &descriptor_pool_info, nullptr, &ray_lines_descriptor_pool_);
        CheckResult(create_result, "Failed to create descriptor pool.");

        ray_lines_descriptor_sets_.resize(swapchain_size);
        std::vector<VkDescriptorSetLayout> set_layouts(swapchain_size, ray_lines_descriptor_set_layout_);

        // Allocate sets.
        VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
        descriptor_set_allocate_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_allocate_info.descriptorPool              = ray_lines_descriptor_pool_;
        descriptor_set_allocate_info.pSetLayouts                 = set_layouts.data();
        descriptor_set_allocate_info.descriptorSetCount          = swapchain_size;

        VkResult alloc_result =
            vkAllocateDescriptorSets(module_context_->device->GetDevice(), &descriptor_set_allocate_info, ray_lines_descriptor_sets_.data());
        CheckResult(alloc_result, "Failed to allocate descriptor sets.");
    }

    void RayInspectorOverlayRenderModule::UpdateRayBuffer(RendererSceneInfo* scene_info)
    {
        if (!update_ray_buffer_)
        {
            return;
        }
        update_ray_buffer_ = false;
        number_of_rays_    = static_cast<uint32_t>(rays_.size());

        int32_t ray_idx{};
        for (auto& ray : rays_)
        {
            ray.color = scene_info->ray_color;
            if (ray.is_outline)
            {
                ray.color = scene_info->selected_ray_color;
            }
            else if (ray.cull_mask == 0)
            {
                ray.color = scene_info->zero_mask_ray_color;
            }
            else if (ray.ray_flags & RAY_FLAGS_TERMINATE_ON_FIRST_HIT)
            {
                ray.color = scene_info->shadow_ray_color;
            }
            ++ray_idx;
        }

        if (number_of_rays_ == 0)
        {
            return;
        }

        current_ray_staging_memory_.buffer     = VK_NULL_HANDLE;
        current_ray_staging_memory_.allocation = VK_NULL_HANDLE;
        current_ray_memory_.buffer             = VK_NULL_HANDLE;
        current_ray_memory_.allocation         = VK_NULL_HANDLE;

        frame_context_->device->CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                             VMA_MEMORY_USAGE_CPU_ONLY,
                                             current_ray_staging_memory_.buffer,
                                             current_ray_staging_memory_.allocation,
                                             rays_.data(),
                                             number_of_rays_ * sizeof(RayInspectorRay));

        ray_staging_buffer_guard_.SetCurrentBuffer(current_ray_staging_memory_.buffer, current_ray_staging_memory_.allocation);

        frame_context_->device->CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                             VMA_MEMORY_USAGE_GPU_ONLY,
                                             current_ray_memory_.buffer,
                                             current_ray_memory_.allocation,
                                             nullptr,
                                             number_of_rays_ * sizeof(RayInspectorRay));

        ray_buffer_guard_.SetCurrentBuffer(current_ray_memory_.buffer, current_ray_memory_.allocation);

        VkBufferCopy copy_region = {};
        copy_region.size         = number_of_rays_ * sizeof(RayInspectorRay);
        vkCmdCopyBuffer(frame_context_->command_buffer, current_ray_staging_memory_.buffer, current_ray_memory_.buffer, 1, &copy_region);

        VkBufferMemoryBarrier buffer_barrier{};
        buffer_barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        buffer_barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        buffer_barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
        buffer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        buffer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        buffer_barrier.buffer              = current_ray_memory_.buffer;
        buffer_barrier.offset              = 0;
        buffer_barrier.size                = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(
            frame_context_->command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 1, &buffer_barrier, 0, nullptr);
    }

    std::vector<IconDescription> MakeIconDescriptions(std::vector<RayInspectorRay> rays,
                                                      glm::mat4                    view_projection,
                                                      glm::vec2                    screen_size,
                                                      RendererSceneInfo*           scene_info)
    {
        std::vector<IconDescription> icons;
        const float                  icon_size = 0.04f;

        for (auto& ray : rays)
        {
            glm::vec4 ray_color = scene_info->ray_color;

            if (ray.is_outline)
            {
                ray_color = scene_info->selected_ray_color;
            }
            else if (ray.cull_mask == 0)
            {
                ray_color = scene_info->zero_mask_ray_color;
            }
            else if (ray.ray_flags & RAY_FLAGS_TERMINATE_ON_FIRST_HIT)
            {
                ray_color = scene_info->shadow_ray_color;
            }

            IconDescription origin         = {};
            origin.type                    = 0;
            glm::vec4 screen_mapped_origin = view_projection * glm::vec4(ray.origin, 1.0f);
            screen_mapped_origin /= screen_mapped_origin.w;
            origin.position     = {screen_mapped_origin.x, screen_mapped_origin.y};
            origin.aspect_ratio = screen_size.x / screen_size.y;
            origin.color        = ray_color;
            origin.size         = glm::vec2(icon_size);
            if (screen_mapped_origin.z > 1.0f)
            {
                origin.size = {};
            }

            IconDescription direction         = {};
            direction.type                    = 1;
            glm::vec4 screen_mapped_direction = view_projection * glm::vec4(ray.origin + ray.direction, 1.0f);
            screen_mapped_direction /= screen_mapped_direction.w;
            glm::vec4 screen_mapped_direction_extension =
                view_projection * glm::vec4(ray.origin + ray.direction - (ray.direction * 0.01f), 1.0f);
            screen_mapped_direction_extension /= screen_mapped_direction_extension.w;
            direction.position     = {screen_mapped_direction.x, screen_mapped_direction.y};
            direction.aspect_ratio = screen_size.x / screen_size.y;
            direction.color        = ray_color;
            direction.size         = glm::vec2(icon_size * 1.5f);
            auto diff              = screen_mapped_direction - screen_mapped_direction_extension;
            direction.angle        = std::atan2(diff.y, diff.x);
            if (screen_mapped_direction_extension.z > 1.0f)
            {
                direction.angle = -direction.angle;
            }
            if (screen_mapped_direction.z > 1.0)
            {
                direction.size = {};
            }

            IconDescription hit         = {};
            hit.type                    = 2;
            glm::vec4 screen_mapped_hit = view_projection * glm::vec4(ray.origin + ray.direction * ray.hit_distance, 1.0f);
            screen_mapped_hit /= screen_mapped_hit.w;
            hit.position     = {screen_mapped_hit.x, screen_mapped_hit.y};
            hit.aspect_ratio = screen_size.x / screen_size.y;
            hit.color        = ray_color;
            hit.size         = glm::vec2(icon_size / 2.0f);
            if (screen_mapped_hit.z > 1.0f || ray.hit_distance < 0.0f)
            {
                hit.size = {};
            }

            icons.push_back(origin);
            icons.push_back(direction);
            icons.push_back(hit);
        }

        return icons;
    }

    void RayInspectorOverlayRenderModule::UpdateIconBuffer()
    {
        number_of_icons_ = 0;
        if (number_of_rays_ == 0)
        {
            return;
        }

        icons_ = MakeIconDescriptions(
            rays_, frame_context_->view_projection, {frame_context_->framebuffer_width, frame_context_->framebuffer_height}, frame_context_->scene_info);

        number_of_icons_ = static_cast<uint32_t>(icons_.size());

        current_icon_staging_memory_.buffer     = VK_NULL_HANDLE;
        current_icon_staging_memory_.allocation = VK_NULL_HANDLE;
        current_icon_memory_.buffer             = VK_NULL_HANDLE;
        current_icon_memory_.allocation         = VK_NULL_HANDLE;

        frame_context_->device->CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                             VMA_MEMORY_USAGE_CPU_ONLY,
                                             current_icon_staging_memory_.buffer,
                                             current_icon_staging_memory_.allocation,
                                             icons_.data(),
                                             number_of_icons_ * sizeof(IconDescription));

        icon_staging_buffer_guard_.SetCurrentBuffer(current_icon_staging_memory_.buffer, current_icon_staging_memory_.allocation);

        frame_context_->device->CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                             VMA_MEMORY_USAGE_GPU_ONLY,
                                             current_icon_memory_.buffer,
                                             current_icon_memory_.allocation,
                                             nullptr,
                                             number_of_icons_ * sizeof(IconDescription));

        icon_buffer_guard_.SetCurrentBuffer(current_icon_memory_.buffer, current_icon_memory_.allocation);

        VkBufferCopy copy_region = {};
        copy_region.size         = number_of_icons_ * sizeof(IconDescription);
        vkCmdCopyBuffer(frame_context_->command_buffer, current_icon_staging_memory_.buffer, current_icon_memory_.buffer, 1, &copy_region);

        VkBufferMemoryBarrier buffer_barrier{};
        buffer_barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        buffer_barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        buffer_barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
        buffer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        buffer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        buffer_barrier.buffer              = current_icon_memory_.buffer;
        buffer_barrier.offset              = 0;
        buffer_barrier.size                = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(
            frame_context_->command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 1, &buffer_barrier, 0, nullptr);
    }

    void RayInspectorOverlayRenderModule::CreateRayLinesPipelineAndLayout()
    {
        VkPushConstantRange push_constant_range = {};
        push_constant_range.size                = sizeof(uint32_t);
        push_constant_range.stageFlags          = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
        pipeline_layout_create_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.setLayoutCount             = 1;
        pipeline_layout_create_info.pSetLayouts                = &ray_lines_descriptor_set_layout_;
        pipeline_layout_create_info.pushConstantRangeCount     = 1;
        pipeline_layout_create_info.pPushConstantRanges        = &push_constant_range;

        // Create the pipeline layout.
        VkResult create_result =
            vkCreatePipelineLayout(module_context_->device->GetDevice(), &pipeline_layout_create_info, nullptr, &ray_lines_pipeline_layout_);
        CheckResult(create_result, "Failed to create ray inspector overlay pipeline layout.");

        // Construct pipeline information structs.

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
        rasterization_state.lineWidth                              = 5.0f;

        VkPipelineColorBlendAttachmentState blend_attachment_state = {};
        blend_attachment_state.colorWriteMask                      = 0xF;
        blend_attachment_state.blendEnable                         = VK_TRUE;
        // Below is for alpha if blendEnable is set to VK_TRUE
        blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
        blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;

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
        multisample_state.rasterizationSamples                 = module_context_->swapchain->GetMSAASamples();
        multisample_state.flags                                = 0;

        std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH};

        VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
        dynamic_state_info.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_info.pDynamicStates                   = dynamic_states.data();
        dynamic_state_info.dynamicStateCount                = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state_info.flags                            = 0;

        VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
        vertex_input_state_info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        // Load shaders.
        std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
        const char*                                    full_vertex_shader_path = "RayInspectorOverlay.vs.spv";
        const char*                                    full_pixel_shader_path  = "RayInspectorOverlay.ps.spv";
        LoadShader(full_vertex_shader_path, module_context_->device, VK_SHADER_STAGE_VERTEX_BIT, "VSMain", shader_stages[0]);
        LoadShader(full_pixel_shader_path, module_context_->device, VK_SHADER_STAGE_FRAGMENT_BIT, "PSMain", shader_stages[1]);

        // Use the constructed data to describe the pipeline.
        VkGraphicsPipelineCreateInfo pipeline_create_info = {};
        pipeline_create_info.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.layout                       = ray_lines_pipeline_layout_;
        pipeline_create_info.renderPass                   = module_context_->swapchain->GetRenderPass(GetRenderPassHint());
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
        pipeline_create_info.pVertexInputState            = &vertex_input_state_info;

        // Create the pipeline.
        create_result =
            vkCreateGraphicsPipelines(module_context_->device->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &ray_lines_pipeline_);
        CheckResult(create_result, "Failed to create ray inspector overlay pipeline.");

        // Destroy shader modules.
        vkDestroyShaderModule(module_context_->device->GetDevice(), shader_stages[0].module, nullptr);
        vkDestroyShaderModule(module_context_->device->GetDevice(), shader_stages[1].module, nullptr);
    }

    void RayInspectorOverlayRenderModule::UpdateRayLinesDescriptorSet()
    {
        // Binding 0 : Scene UBO
        VkWriteDescriptorSet scene_descriptor = {};
        scene_descriptor.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        scene_descriptor.dstSet               = ray_lines_descriptor_sets_[frame_context_->current_frame];
        scene_descriptor.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        scene_descriptor.dstBinding           = 0;
        scene_descriptor.pBufferInfo          = &frame_context_->scene_ubo_info;
        scene_descriptor.descriptorCount      = 1;

        VkDescriptorBufferInfo rays_buffer_info = {};
        rays_buffer_info.range                  = VK_WHOLE_SIZE;
        rays_buffer_info.buffer                 = current_ray_memory_.buffer;

        // Binding 1 : Rays
        VkWriteDescriptorSet rays_descriptor = {};
        rays_descriptor.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        rays_descriptor.dstSet               = ray_lines_descriptor_sets_[frame_context_->current_frame];
        rays_descriptor.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        rays_descriptor.dstBinding           = 1;
        rays_descriptor.pBufferInfo          = &rays_buffer_info;
        rays_descriptor.descriptorCount      = 1;

        VkDescriptorImageInfo rays_image_info{};
        rays_image_info.sampler     = VK_NULL_HANDLE;
        rays_image_info.imageView   = *depth_input_buffer_view_;
        rays_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Binding 2 : Input attachment
        VkWriteDescriptorSet depth_input_descriptor = {};
        depth_input_descriptor.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        depth_input_descriptor.dstSet               = ray_lines_descriptor_sets_[frame_context_->current_frame];
        depth_input_descriptor.descriptorType       = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        depth_input_descriptor.dstBinding           = 2;
        depth_input_descriptor.pImageInfo           = &rays_image_info;
        depth_input_descriptor.descriptorCount      = 1;

        // Per frame write descriptors.
        std::vector<VkWriteDescriptorSet> write_descritptors = {scene_descriptor, rays_descriptor, depth_input_descriptor};

        vkUpdateDescriptorSets(frame_context_->device->GetDevice(), static_cast<uint32_t>(write_descritptors.size()), write_descritptors.data(), 0, nullptr);
    }

    void RayInspectorOverlayRenderModule::RenderRayLinesPipeline()
    {
        vkCmdBindDescriptorSets(frame_context_->command_buffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                ray_lines_pipeline_layout_,
                                0,
                                1,
                                &ray_lines_descriptor_sets_[frame_context_->current_frame],
                                0,
                                nullptr);

        vkCmdBindPipeline(frame_context_->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ray_lines_pipeline_);

        vkCmdPushConstants(frame_context_->command_buffer, ray_lines_pipeline_layout_, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &number_of_rays_);

        // Draw deselected rays.
        uint32_t first_ray_outline{frame_context_->scene_info->first_ray_outline};
        uint32_t first_ray_outline_adjusted =
            first_ray_outline - rays_in_other_tlas_;  // Ray buffer has removed rays from other TLASes, so subtract that from first index.
        uint32_t deselected_ray_count = first_ray_outline_adjusted;  // The deselected rays are grouped contiguously before the ray outlines.
        vkCmdSetLineWidth(frame_context_->command_buffer, 5.0f);
        vkCmdDraw(frame_context_->command_buffer, 2, deselected_ray_count, 0, 0);

        // Draw outline of selected rays.
        uint32_t ray_outline_count{frame_context_->scene_info->ray_outline_count};
        if (ray_outline_count > 0)
        {
            vkCmdSetLineWidth(frame_context_->command_buffer, 10.0f);
            vkCmdDraw(frame_context_->command_buffer, 2, ray_outline_count, 0, first_ray_outline_adjusted);
        }

        // Draw selected rays.
        vkCmdSetLineWidth(frame_context_->command_buffer, 5.0f);
        uint32_t selected_ray_count = ray_outline_count;  // One selected ray per outline.
        vkCmdDraw(frame_context_->command_buffer,
                  2,
                  selected_ray_count,
                  0,
                  first_ray_outline_adjusted + ray_outline_count);  // The selected rays begin where ray outlines end.
    }

    void RayInspectorOverlayRenderModule::CreateIconPipelineAndLayout()
    {
        VkPushConstantRange push_constant_range = {};
        push_constant_range.size                = sizeof(uint32_t);
        push_constant_range.stageFlags          = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
        pipeline_layout_create_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.setLayoutCount             = 1;
        pipeline_layout_create_info.pSetLayouts                = &icon_descriptor_set_layout_;
        pipeline_layout_create_info.pushConstantRangeCount     = 1;
        pipeline_layout_create_info.pPushConstantRanges        = &push_constant_range;

        // Create the pipeline layout.
        VkResult create_result = vkCreatePipelineLayout(module_context_->device->GetDevice(), &pipeline_layout_create_info, nullptr, &icon_pipeline_layout_);
        CheckResult(create_result, "Failed to create ray inspector overlay pipeline layout.");

        // Construct pipeline information structs.

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
        rasterization_state.lineWidth                              = 5.0f;  // Ignored, since we have this set to dynamic state.

        VkPipelineColorBlendAttachmentState blend_attachment_state = {};
        blend_attachment_state.colorWriteMask                      = 0xF;
        blend_attachment_state.blendEnable                         = VK_TRUE;
        // Below is for alpha if blendEnable is set to VK_TRUE
        blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
        blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;

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
        multisample_state.rasterizationSamples                 = module_context_->swapchain->GetMSAASamples();
        multisample_state.flags                                = 0;

        std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
        dynamic_state_info.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_info.pDynamicStates                   = dynamic_states.data();
        dynamic_state_info.dynamicStateCount                = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state_info.flags                            = 0;

        VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
        vertex_input_state_info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        // Load shaders.
        std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
        const char*                                    full_vertex_shader_path = "RayInspectorOverlayIcons.vs.spv";
        const char*                                    full_pixel_shader_path  = "RayInspectorOverlayIcons.ps.spv";
        LoadShader(full_vertex_shader_path, module_context_->device, VK_SHADER_STAGE_VERTEX_BIT, "VSMain", shader_stages[0]);
        LoadShader(full_pixel_shader_path, module_context_->device, VK_SHADER_STAGE_FRAGMENT_BIT, "PSMain", shader_stages[1]);

        // Use the constructed data to describe the pipeline.
        VkGraphicsPipelineCreateInfo pipeline_create_info = {};
        pipeline_create_info.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.layout                       = icon_pipeline_layout_;
        pipeline_create_info.renderPass                   = module_context_->swapchain->GetRenderPass(GetRenderPassHint());
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
        pipeline_create_info.pVertexInputState            = &vertex_input_state_info;

        // Create the pipeline.
        create_result = vkCreateGraphicsPipelines(module_context_->device->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &icon_pipeline_);
        CheckResult(create_result, "Failed to create ray inspector overlay pipeline.");

        // Destroy shader modules.
        vkDestroyShaderModule(module_context_->device->GetDevice(), shader_stages[0].module, nullptr);
        vkDestroyShaderModule(module_context_->device->GetDevice(), shader_stages[1].module, nullptr);
    }

    void RayInspectorOverlayRenderModule::UpdateIconDescriptorSet()
    {
        // Binding 0 : Scene UBO
        VkWriteDescriptorSet scene_descriptor = {};
        scene_descriptor.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        scene_descriptor.dstSet               = icon_descriptor_sets_[frame_context_->current_frame];
        scene_descriptor.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        scene_descriptor.dstBinding           = 0;
        scene_descriptor.pBufferInfo          = &frame_context_->scene_ubo_info;
        scene_descriptor.descriptorCount      = 1;

        VkDescriptorBufferInfo icons_buffer_info = {};
        icons_buffer_info.range                  = VK_WHOLE_SIZE;
        icons_buffer_info.buffer                 = current_icon_memory_.buffer;

        // Binding 1 : Icons
        VkWriteDescriptorSet icons_descriptor = {};
        icons_descriptor.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        icons_descriptor.dstSet               = icon_descriptor_sets_[frame_context_->current_frame];
        icons_descriptor.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        icons_descriptor.dstBinding           = 1;
        icons_descriptor.pBufferInfo          = &icons_buffer_info;
        icons_descriptor.descriptorCount      = 1;

        // Per frame write descriptors.
        std::vector<VkWriteDescriptorSet> write_descritptors = {scene_descriptor, icons_descriptor};

        vkUpdateDescriptorSets(frame_context_->device->GetDevice(), static_cast<uint32_t>(write_descritptors.size()), write_descritptors.data(), 0, nullptr);
    }

    void RayInspectorOverlayRenderModule::RenderIconPipeline()
    {
        vkCmdBindDescriptorSets(frame_context_->command_buffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                icon_pipeline_layout_,
                                0,
                                1,
                                &icon_descriptor_sets_[frame_context_->current_frame],
                                0,
                                nullptr);

        // Bind pipeline and draw a triangle. The shader generates a triangle big enough to saturate the fragment shader.
        vkCmdBindPipeline(frame_context_->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, icon_pipeline_);

        vkCmdPushConstants(frame_context_->command_buffer, icon_pipeline_layout_, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &number_of_rays_);

        vkCmdDraw(frame_context_->command_buffer, 6, number_of_icons_, 0, 0);
    }

    void RayInspectorOverlayRenderModule::SetupIconDescriptorSetLayout()
    {
        // Construct binding information.
        VkDescriptorSetLayoutBinding scene_layout_binding_0 = {};
        scene_layout_binding_0.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        scene_layout_binding_0.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        scene_layout_binding_0.binding                      = 0;
        scene_layout_binding_0.descriptorCount              = 1;

        VkDescriptorSetLayoutBinding scene_layout_binding_1 = {};
        scene_layout_binding_1.descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        scene_layout_binding_1.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        scene_layout_binding_1.binding                      = 1;
        scene_layout_binding_1.descriptorCount              = 1;

        std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
            scene_layout_binding_0,
            scene_layout_binding_1,
        };

        // Create set layout.
        VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
        descriptor_set_layout_create_info.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_layout_create_info.pBindings                       = set_layout_bindings.data();
        descriptor_set_layout_create_info.bindingCount                    = static_cast<uint32_t>(set_layout_bindings.size());

        VkResult create_result =
            vkCreateDescriptorSetLayout(module_context_->device->GetDevice(), &descriptor_set_layout_create_info, nullptr, &icon_descriptor_set_layout_);
        CheckResult(create_result, "Failed to create descriptor set layout.");
    }

    void RayInspectorOverlayRenderModule::SetupIconDescriptorPool()
    {
        uint32_t swapchain_size = module_context_->swapchain->GetBackBufferCount();

        // Set pool sizes
        VkDescriptorPoolSize descriptor_pool_size_uniform_buffer = {};
        descriptor_pool_size_uniform_buffer.type                 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_pool_size_uniform_buffer.descriptorCount      = 1 * swapchain_size;

        VkDescriptorPoolSize descriptor_pool_size_image_sampler = {};
        descriptor_pool_size_image_sampler.type                 = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptor_pool_size_image_sampler.descriptorCount      = 1 * swapchain_size;

        std::vector<VkDescriptorPoolSize> pool_sizes = {
            descriptor_pool_size_uniform_buffer,
            descriptor_pool_size_image_sampler,
        };

        // Arrange pool creation info.
        VkDescriptorPoolCreateInfo descriptor_pool_info = {};
        descriptor_pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_info.poolSizeCount              = static_cast<uint32_t>(pool_sizes.size());
        descriptor_pool_info.pPoolSizes                 = pool_sizes.data();
        descriptor_pool_info.maxSets                    = swapchain_size;

        // Create descriptor pool.
        VkResult create_result = vkCreateDescriptorPool(module_context_->device->GetDevice(), &descriptor_pool_info, nullptr, &icon_descriptor_pool_);
        CheckResult(create_result, "Failed to create descriptor pool.");

        icon_descriptor_sets_.resize(swapchain_size);
        std::vector<VkDescriptorSetLayout> set_layouts(swapchain_size, icon_descriptor_set_layout_);

        // Allocate sets.
        VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
        descriptor_set_allocate_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_allocate_info.descriptorPool              = icon_descriptor_pool_;
        descriptor_set_allocate_info.pSetLayouts                 = set_layouts.data();
        descriptor_set_allocate_info.descriptorSetCount          = swapchain_size;

        VkResult alloc_result = vkAllocateDescriptorSets(module_context_->device->GetDevice(), &descriptor_set_allocate_info, icon_descriptor_sets_.data());
        CheckResult(alloc_result, "Failed to allocate descriptor sets.");
    }

}  // namespace rra::renderer