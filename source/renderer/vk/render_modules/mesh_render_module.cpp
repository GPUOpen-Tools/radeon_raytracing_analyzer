//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the mesh render module.
//=============================================================================

#include "vk/render_modules/mesh_render_module.h"

#include <type_traits>
#include <unordered_map>
#include <vector>

#include <QCoreApplication>

#include "public/rra_assert.h"

#include "vk/framework/ext_debug_utils.h"
#include "vk/vk_graphics_context.h"

// We can't use std::max or glm::max since the windows macro ends up overriding the max keyword.
// So we underfine max for this file only.
#undef max

#define VERTEX_ATTRIBUTE(loc, attr)                                                                                                     \
    VkVertexInputAttributeDescription                                                                                                   \
    {                                                                                                                                   \
        loc, kVertexBufferBindId, GetVulkanFormat<decltype(rra::renderer::RraVertex::attr)>(), offsetof(rra::renderer::RraVertex, attr) \
    }

#define INSTANCE_ATTRIBUTE(loc, attr)                                                                                                                   \
    VkVertexInputAttributeDescription                                                                                                                   \
    {                                                                                                                                                   \
        loc, kInstanceBufferBindId, GetVulkanFormat<decltype(rra::renderer::MeshInstanceData::attr)>(), offsetof(rra::renderer::MeshInstanceData, attr) \
    }

#define INSTANCE_ATTRIBUTE_FOUR_SLOTS(loc, attr)                                                                    \
    VkVertexInputAttributeDescription{loc + 0,                                                                      \
                                      kInstanceBufferBindId,                                                        \
                                      GetVulkanFormat<decltype(rra::renderer::MeshInstanceData::attr)>(),           \
                                      offsetof(rra::renderer::MeshInstanceData, attr) + sizeof(glm::vec4) * 0},     \
        VkVertexInputAttributeDescription{loc + 1,                                                                  \
                                          kInstanceBufferBindId,                                                    \
                                          GetVulkanFormat<decltype(rra::renderer::MeshInstanceData::attr)>(),       \
                                          offsetof(rra::renderer::MeshInstanceData, attr) + sizeof(glm::vec4) * 1}, \
        VkVertexInputAttributeDescription{loc + 2,                                                                  \
                                          kInstanceBufferBindId,                                                    \
                                          GetVulkanFormat<decltype(rra::renderer::MeshInstanceData::attr)>(),       \
                                          offsetof(rra::renderer::MeshInstanceData, attr) + sizeof(glm::vec4) * 2}, \
        VkVertexInputAttributeDescription{loc + 3,                                                                  \
                                          kInstanceBufferBindId,                                                    \
                                          GetVulkanFormat<decltype(rra::renderer::MeshInstanceData::attr)>(),       \
                                          offsetof(rra::renderer::MeshInstanceData, attr) + sizeof(glm::vec4) * 3}

template <typename T>
VkFormat GetVulkanFormat()
{
    if (std::is_same<float, T>::value)
    {
        return VK_FORMAT_R32_SFLOAT;
    }
    else if (std::is_same<glm::vec2, T>::value)
    {
        return VK_FORMAT_R32G32_SFLOAT;
    }
    else if (std::is_same<glm::vec3, T>::value)
    {
        return VK_FORMAT_R32G32B32_SFLOAT;
    }
    else if (std::is_same<glm::vec4, T>::value)
    {
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    }
    else if (std::is_same<int, T>::value)
    {
        return VK_FORMAT_R32_SINT;
    }
    else if (std::is_same<glm::ivec2, T>::value)
    {
        return VK_FORMAT_R32G32_SINT;
    }
    else if (std::is_same<glm::ivec3, T>::value)
    {
        return VK_FORMAT_R32G32B32_SINT;
    }
    else if (std::is_same<glm::ivec4, T>::value)
    {
        return VK_FORMAT_R32G32B32A32_SINT;
    }
    else if (std::is_same<uint32_t, T>::value)
    {
        return VK_FORMAT_R32_UINT;
    }
    else if (std::is_same<glm::uvec2, T>::value)
    {
        return VK_FORMAT_R32G32_UINT;
    }
    else if (std::is_same<glm::uvec3, T>::value)
    {
        return VK_FORMAT_R32G32B32_UINT;
    }
    else if (std::is_same<glm::uvec4, T>::value)
    {
        return VK_FORMAT_R32G32B32A32_UINT;
    }
    else if (std::is_same<glm::mat4, T>::value)
    {
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    }

    return VK_FORMAT_UNDEFINED;
}

namespace rra
{
    namespace renderer
    {
        static const uint32_t kVertexBufferBindId   = 0;
        static const uint32_t kInstanceBufferBindId = 1;
        static const uint32_t kPerSceneUboBindId    = 2;

        static const float kWireframeWidth = 1.25f;  ///< The wireframe width.

        MeshRenderModule::MeshRenderModule()
            : RenderModule(RenderPassHint::kRenderPassHintClearDepthOnly)
        {
        }

        void MeshRenderModule::Initialize(const RenderModuleContext* context)
        {
            // Setup the pipeline.
            context_ = context;
            SetupDescriptorSetLayout();
            InitializePipelines();
            InitializeDefaultRenderState();
            SetupDescriptorPool();
            SetupDescriptorSet();

            // Initialize the instance buffer guard.
            instance_guard.Initialize(context->swapchain->GetBackBufferCount());
            instance_staging_guard.Initialize(context->swapchain->GetBackBufferCount());

            // Initialize the custom triangle buffer guard.
            custom_triangles_guard.Initialize(context->swapchain->GetBackBufferCount());
            custom_triangles_staging_guard.Initialize(context->swapchain->GetBackBufferCount());
        }

        void MeshRenderModule::Draw(const RenderFrameContext* draw_context)
        {
            // If the scene has changed, up date internal state.
            if (current_scene_info_ != draw_context->scene_info)
            {
                current_scene_info_   = draw_context->scene_info;
                render_state_.updated = true;
            }

            // Update custom triangles if the state has updated.
            if (draw_context->scene_info != nullptr && (render_state_.updated || draw_context->scene_info->scene_iteration != last_scene_iteration_))
            {
                UploadCustomTriangles(draw_context->command_buffer);
            }

            // Process the other scene data if the state has updated.
            if (draw_context->scene_info != nullptr && (render_state_.updated || draw_context->scene_info->scene_iteration != last_scene_iteration_ ||
                                                        (last_view_projection_matrix_ != draw_context->view_projection)))
            {
                last_view_projection_matrix_ = draw_context->view_projection;
                ProcessSceneData(draw_context->command_buffer, draw_context->camera_position);
            }

            if (per_blas_instance_buffer_.buffer)
            {
                VkBufferMemoryBarrier buffer_barrier{};
                buffer_barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                buffer_barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
                buffer_barrier.dstAccessMask       = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
                buffer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                buffer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                buffer_barrier.buffer              = per_blas_instance_buffer_.buffer;
                buffer_barrier.offset              = 0;
                buffer_barrier.size                = VK_WHOLE_SIZE;

                // This barrier needs to be moved outside of ProcessSceneData (called in the if-statement above) or validation errors appear.
                // I believe it's because ProcessSceneData isn't called each time Draw is called, but we want this barrier to be present
                // each time we call vkCmdDraw.
                vkCmdPipelineBarrier(draw_context->command_buffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                                     VK_DEPENDENCY_BY_REGION_BIT,
                                     0,
                                     nullptr,
                                     1,
                                     &buffer_barrier,
                                     0,
                                     nullptr);
            }

            // Save the last scene iteration if the scene is available.
            if (draw_context->scene_info != nullptr)
            {
                last_scene_iteration_ = draw_context->scene_info->scene_iteration;
            }

            // Mark state as not updated after running necessary updates.
            render_state_.updated = false;

            // Process the instance data by the frame. (We update the custom triangles separately in UploadCustomTriangles)
            instance_guard.ProcessFrame(draw_context->current_frame, context_->device);
            instance_staging_guard.ProcessFrame(draw_context->current_frame, context_->device);

            std::vector<VkWriteDescriptorSet> write_descriptor_sets;

            // Binding 0 : Vertex shader uniform buffer.
            VkWriteDescriptorSet write_scene_ubo_descriptor = {};
            write_scene_ubo_descriptor.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_scene_ubo_descriptor.dstSet               = blas_mesh_descriptor_sets_[draw_context->current_frame];
            write_scene_ubo_descriptor.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write_scene_ubo_descriptor.dstBinding           = 0;
            write_scene_ubo_descriptor.pBufferInfo          = &draw_context->scene_ubo_info;
            write_scene_ubo_descriptor.descriptorCount      = 1;

            // Binding 1 : Heatmap
            VkWriteDescriptorSet write_heatmap_descriptor = {};
            write_heatmap_descriptor.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_heatmap_descriptor.dstSet               = blas_mesh_descriptor_sets_[draw_context->current_frame];
            write_heatmap_descriptor.descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write_heatmap_descriptor.dstBinding           = 1;
            write_heatmap_descriptor.pImageInfo           = &draw_context->heatmap_image_info;
            write_heatmap_descriptor.descriptorCount      = 1;

            write_descriptor_sets = {write_scene_ubo_descriptor, write_heatmap_descriptor};
            vkUpdateDescriptorSets(
                draw_context->device->GetDevice(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);

            draw_context->begin_render_pass();

            // Step over and draw all instanced meshes in the scene.
            if (current_scene_info_ != nullptr && current_scene_info_ == draw_context->scene_info)
            {
                // Bind sets and buffers.
                vkCmdBindDescriptorSets(draw_context->command_buffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        pipeline_layout_,
                                        0,
                                        1,
                                        &blas_mesh_descriptor_sets_[draw_context->current_frame],
                                        0,
                                        nullptr);

                VkDeviceSize offsets[1] = {0};

                if (!render_instructions_.empty())
                {
                    auto current_instance_buffer = instance_guard.GetCurrentBuffer();
                    vkCmdBindVertexBuffers(draw_context->command_buffer, kInstanceBufferBindId, 1, &current_instance_buffer, offsets);
                }

                // Draw the instructions.
                for (auto& render_instruction : render_instructions_)
                {
                    if (render_instruction.vertex_count == 0 || render_instruction.vertex_buffer == VK_NULL_HANDLE)
                    {
                        continue;
                    }

                    bool draw = false;

                    TriangleCullPipelines cull_pipelines{};

                    if (render_state_.render_geometry)
                    {
                        cull_pipelines = geometry_color_pipelines_[coloring_mode_];
                        draw           = true;
                    }
                    else if (render_state_.render_wireframe)
                    {
                        cull_pipelines = geometry_wireframe_only_pipeline_;
                        draw           = true;
                    }

                    if (draw)
                    {
                        switch (render_state_.culling_mode)
                        {
                        case VK_CULL_MODE_NONE:
                            vkCmdBindPipeline(draw_context->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cull_pipelines.cull_none);
                            break;
                        case VK_CULL_MODE_FRONT_BIT:
                            vkCmdBindPipeline(draw_context->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cull_pipelines.cull_front);
                            break;
                        case VK_CULL_MODE_BACK_BIT:
                            vkCmdBindPipeline(draw_context->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cull_pipelines.cull_back);
                            break;
                        }

                        // Binding point 0 : Mesh vertex buffer.
                        vkCmdBindVertexBuffers(draw_context->command_buffer, kVertexBufferBindId, 1, &render_instruction.vertex_buffer, offsets);

                        // Render instances.
                        vkCmdDraw(draw_context->command_buffer,
                                  render_instruction.vertex_count,
                                  render_instruction.instance_count,
                                  render_instruction.vertex_index,
                                  render_instruction.instance_index);
                    }
                }
            }

            draw_context->end_render_pass();
        }

        void MeshRenderModule::Cleanup(const RenderModuleContext* context)
        {
            VkDevice device_handle = context->device->GetDevice();

            for (auto& pipeline_pair : geometry_color_pipelines_)
            {
                vkDestroyPipeline(device_handle, pipeline_pair.second.cull_none, nullptr);
                vkDestroyPipeline(device_handle, pipeline_pair.second.cull_front, nullptr);
                vkDestroyPipeline(device_handle, pipeline_pair.second.cull_back, nullptr);
            }

            vkDestroyPipeline(device_handle, geometry_wireframe_only_pipeline_.cull_none, nullptr);
            vkDestroyPipeline(device_handle, geometry_wireframe_only_pipeline_.cull_front, nullptr);
            vkDestroyPipeline(device_handle, geometry_wireframe_only_pipeline_.cull_back, nullptr);

            vkDestroyPipelineLayout(device_handle, pipeline_layout_, nullptr);
            vkDestroyDescriptorSetLayout(device_handle, descriptor_set_layout_, nullptr);
            vkDestroyDescriptorPool(device_handle, descriptor_pool_, nullptr);

            instance_guard.Cleanup(context->device);
            instance_staging_guard.Cleanup(context->device);

            custom_triangles_guard.Cleanup(context->device);
            custom_triangles_staging_guard.Cleanup(context->device);
        }

        bool MeshRenderModule::ShouldCopyDepthBuffer() const
        {
            return true;
        }

        RenderState& MeshRenderModule::GetRenderState()
        {
            return render_state_;
        }

        void MeshRenderModule::SetupDescriptorPool()
        {
            // Initialize a single uniform buffer.
            VkDescriptorPoolSize descriptor_pool_size_scene_ubo = {};
            descriptor_pool_size_scene_ubo.type                 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_pool_size_scene_ubo.descriptorCount      = context_->swapchain->GetBackBufferCount();

            VkDescriptorPoolSize descriptor_pool_size_image_sampler = {};
            descriptor_pool_size_image_sampler.type                 = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_pool_size_image_sampler.descriptorCount      = context_->swapchain->GetBackBufferCount();

            std::vector<VkDescriptorPoolSize> pool_sizes = {descriptor_pool_size_scene_ubo, descriptor_pool_size_image_sampler};

            VkDescriptorPoolCreateInfo descriptor_pool_info = {};
            descriptor_pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            descriptor_pool_info.poolSizeCount              = static_cast<uint32_t>(pool_sizes.size());
            descriptor_pool_info.pPoolSizes                 = pool_sizes.data();
            descriptor_pool_info.maxSets                    = context_->swapchain->GetBackBufferCount();

            VkResult create_result = vkCreateDescriptorPool(context_->device->GetDevice(), &descriptor_pool_info, nullptr, &descriptor_pool_);
            CheckResult(create_result, "Failed to create descriptor pool.");
        }

        void MeshRenderModule::SetupDescriptorSetLayout()
        {
            // Binding 0 points to a UBO containing scene-wide uniform data (ex: lighting and display configuration).
            VkDescriptorSetLayoutBinding scene_layout_binding_scene_ubo = {};
            scene_layout_binding_scene_ubo.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            scene_layout_binding_scene_ubo.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            scene_layout_binding_scene_ubo.binding                      = 0;
            scene_layout_binding_scene_ubo.descriptorCount              = 1;

            VkDescriptorSetLayoutBinding scene_layout_binding_heatmap = {};
            scene_layout_binding_heatmap.descriptorType               = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            scene_layout_binding_heatmap.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
            scene_layout_binding_heatmap.binding                      = 1;
            scene_layout_binding_heatmap.descriptorCount              = 1;

            std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {scene_layout_binding_scene_ubo, scene_layout_binding_heatmap};

            VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
            descriptor_set_layout_create_info.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptor_set_layout_create_info.pBindings                       = set_layout_bindings.data();
            descriptor_set_layout_create_info.bindingCount                    = static_cast<uint32_t>(set_layout_bindings.size());

            VkResult create_result =
                vkCreateDescriptorSetLayout(context_->device->GetDevice(), &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout_);
            CheckResult(create_result, "Failed to create descriptor set layout.");

            VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
            pipeline_layout_create_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.setLayoutCount             = 1;
            pipeline_layout_create_info.pSetLayouts                = &descriptor_set_layout_;

            create_result = vkCreatePipelineLayout(context_->device->GetDevice(), &pipeline_layout_create_info, nullptr, &pipeline_layout_);
            CheckResult(create_result, "Failed to create descriptor set layout.");
        }

        void MeshRenderModule::SetupDescriptorSet()
        {
            blas_mesh_descriptor_sets_.resize(context_->swapchain->GetBackBufferCount());
            std::vector<VkDescriptorSetLayout> set_layouts(context_->swapchain->GetBackBufferCount(), descriptor_set_layout_);

            VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
            descriptor_set_allocate_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptor_set_allocate_info.descriptorPool              = descriptor_pool_;
            descriptor_set_allocate_info.pSetLayouts                 = set_layouts.data();
            descriptor_set_allocate_info.descriptorSetCount          = static_cast<uint32_t>(blas_mesh_descriptor_sets_.size());

            VkResult alloc_result = vkAllocateDescriptorSets(context_->device->GetDevice(), &descriptor_set_allocate_info, blas_mesh_descriptor_sets_.data());
            CheckResult(alloc_result, "Failed to allocate descriptor sets.");
        }

        VkPipeline MeshRenderModule::InitializeMeshPipeline(const VkPipelineShaderStageCreateInfo&                vertex_stage,
                                                            const VkPipelineShaderStageCreateInfo&                fragment_stage,
                                                            const std::vector<VkVertexInputBindingDescription>&   vertex_input_bindings,
                                                            const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions,
                                                            VkCullModeFlags                                       cull_mode,
                                                            bool                                                  wireframe_only) const
        {
            VkPipeline result_pipeline = VK_NULL_HANDLE;

            VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
            input_assembly_state.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            input_assembly_state.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            input_assembly_state.flags                                  = 0;
            input_assembly_state.primitiveRestartEnable                 = VK_FALSE;

            VkPipelineRasterizationStateCreateInfo rasterization_state = {};
            rasterization_state.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterization_state.polygonMode                            = VK_POLYGON_MODE_FILL;
            rasterization_state.frontFace                              = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterization_state.lineWidth                              = 1.0f;
            rasterization_state.cullMode                               = cull_mode;

            VkPipelineColorBlendAttachmentState blend_attachment_state = {};
            blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            blend_attachment_state.blendEnable    = wireframe_only ? VK_TRUE : VK_FALSE;
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

            std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
            shader_stages.push_back(vertex_stage);
            shader_stages.push_back(fragment_stage);

            VkGraphicsPipelineCreateInfo pipeline_create_info = {};
            pipeline_create_info.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipeline_create_info.layout                       = pipeline_layout_;
            pipeline_create_info.renderPass                   = context_->swapchain->GetRenderPass(GetRenderPassHint());
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

            VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
            vertex_input_state_info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_state_info.pVertexBindingDescriptions           = vertex_input_bindings.data();
            vertex_input_state_info.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
            vertex_input_state_info.pVertexAttributeDescriptions         = vertex_attribute_descriptions.data();
            vertex_input_state_info.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_attribute_descriptions.size());

            pipeline_create_info.pVertexInputState = &vertex_input_state_info;

            // Create the pipeline.
            VkResult create_result =
                vkCreateGraphicsPipelines(context_->device->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &result_pipeline);
            CheckResult(create_result, "Failed to create pipeline.");

            return result_pipeline;
        }

        void MeshRenderModule::InitializeGeometryColorPipeline(const std::string&                                    vert_shader,
                                                               const std::string&                                    frag_shader,
                                                               const std::vector<VkVertexInputAttributeDescription>& attributes,
                                                               GeometryColoringMode                                  coloring_mode)
        {
            // Binding point 0: Mesh vertex layout description at per-vertex rate.
            VkVertexInputBindingDescription mesh_binding_description = {};
            mesh_binding_description.binding                         = kVertexBufferBindId;
            mesh_binding_description.stride                          = sizeof(RraVertex);
            mesh_binding_description.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

            VkVertexInputBindingDescription instance_binding_description = {};
            instance_binding_description.binding                         = kInstanceBufferBindId;
            instance_binding_description.stride                          = sizeof(MeshInstanceData);
            instance_binding_description.inputRate                       = VK_VERTEX_INPUT_RATE_INSTANCE;

            std::vector<VkVertexInputBindingDescription> input_binding_descriptions = {mesh_binding_description, instance_binding_description};

            // Load the SPV shader binaries used to render solid TLAS + BLAS geometry.
            VkPipelineShaderStageCreateInfo preview_shader_vs;
            LoadShader(vert_shader.c_str(), context_->device, VK_SHADER_STAGE_VERTEX_BIT, "VSMain", preview_shader_vs);

            VkPipelineShaderStageCreateInfo preview_shader_ps;
            LoadShader(frag_shader.c_str(), context_->device, VK_SHADER_STAGE_FRAGMENT_BIT, "PSMain", preview_shader_ps);

            VkPipeline cull_none_pipeline =
                InitializeMeshPipeline(preview_shader_vs, preview_shader_ps, input_binding_descriptions, attributes, VK_CULL_MODE_NONE, false);
            geometry_color_pipelines_[coloring_mode].cull_none = cull_none_pipeline;

            VkPipeline cull_front_pipeline =
                InitializeMeshPipeline(preview_shader_vs, preview_shader_ps, input_binding_descriptions, attributes, VK_CULL_MODE_FRONT_BIT, false);
            geometry_color_pipelines_[coloring_mode].cull_front = cull_front_pipeline;

            VkPipeline cull_back_pipeline =
                InitializeMeshPipeline(preview_shader_vs, preview_shader_ps, input_binding_descriptions, attributes, VK_CULL_MODE_BACK_BIT, false);
            geometry_color_pipelines_[coloring_mode].cull_back = cull_back_pipeline;

            // Destroy each render module after the pipelines have been created.
            vkDestroyShaderModule(context_->device->GetDevice(), preview_shader_vs.module, nullptr);
            vkDestroyShaderModule(context_->device->GetDevice(), preview_shader_ps.module, nullptr);
        }

        void MeshRenderModule::InitializeWireframePipeline(const std::string&                                    vert_shader,
                                                           const std::string&                                    frag_shader,
                                                           const std::vector<VkVertexInputAttributeDescription>& attributes)
        {
            // Binding point 0: Mesh vertex layout description at per-vertex rate.
            VkVertexInputBindingDescription mesh_binding_description = {};
            mesh_binding_description.binding                         = kVertexBufferBindId;
            mesh_binding_description.stride                          = sizeof(RraVertex);
            mesh_binding_description.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

            VkVertexInputBindingDescription instance_binding_description = {};
            instance_binding_description.binding                         = kInstanceBufferBindId;
            instance_binding_description.stride                          = sizeof(MeshInstanceData);
            instance_binding_description.inputRate                       = VK_VERTEX_INPUT_RATE_INSTANCE;

            std::vector<VkVertexInputBindingDescription> input_binding_descriptions = {mesh_binding_description, instance_binding_description};

            // Load the SPV shader binaries used to render solid TLAS + BLAS geometry.
            VkPipelineShaderStageCreateInfo preview_shader_vs;
            LoadShader(vert_shader.c_str(), context_->device, VK_SHADER_STAGE_VERTEX_BIT, "VSMain", preview_shader_vs);

            VkPipelineShaderStageCreateInfo preview_shader_ps;
            LoadShader(frag_shader.c_str(), context_->device, VK_SHADER_STAGE_FRAGMENT_BIT, "PSMain", preview_shader_ps);

            VkPipeline cull_none_pipeline =
                InitializeMeshPipeline(preview_shader_vs, preview_shader_ps, input_binding_descriptions, attributes, VK_CULL_MODE_NONE, true);
            geometry_wireframe_only_pipeline_.cull_none = cull_none_pipeline;

            VkPipeline cull_front_pipeline =
                InitializeMeshPipeline(preview_shader_vs, preview_shader_ps, input_binding_descriptions, attributes, VK_CULL_MODE_FRONT_BIT, true);
            geometry_wireframe_only_pipeline_.cull_front = cull_front_pipeline;

            VkPipeline cull_back_pipeline =
                InitializeMeshPipeline(preview_shader_vs, preview_shader_ps, input_binding_descriptions, attributes, VK_CULL_MODE_BACK_BIT, true);
            geometry_wireframe_only_pipeline_.cull_back = cull_back_pipeline;

            // Destroy each render module after the pipelines have been created.
            vkDestroyShaderModule(context_->device->GetDevice(), preview_shader_vs.module, nullptr);
            vkDestroyShaderModule(context_->device->GetDevice(), preview_shader_ps.module, nullptr);
        }

        GeometryColoringMode MeshRenderModule::GetGeometryColoringMode() const
        {
            return coloring_mode_;
        }

        void MeshRenderModule::SetGeometryColoringMode(GeometryColoringMode coloring_mode)
        {
            coloring_mode_ = coloring_mode;
        }

        void MeshRenderModule::InitializePipelines()
        {
            std::vector<VkVertexInputAttributeDescription> wireframe_attr{
                VERTEX_ATTRIBUTE(0, position),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(1, instance_transform),
                INSTANCE_ATTRIBUTE(5, wireframe_metadata),
            };
            InitializeWireframePipeline("GeometryColorWireframe.vs.spv", "GeometryColorWireframe.ps.spv", wireframe_attr);

            std::vector<VkVertexInputAttributeDescription> tree_level_attr{
                VERTEX_ATTRIBUTE(0, position),
                VERTEX_ATTRIBUTE(1, geometry_index_depth_split_opaque),
                VERTEX_ATTRIBUTE(2, triangle_sah_and_selected),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(3, instance_transform),
                INSTANCE_ATTRIBUTE(7, wireframe_metadata),
            };
            InitializeGeometryColorPipeline(
                "GeometryColorTreeLevel.vs.spv", "GeometryColorTreeLevel.ps.spv", tree_level_attr, GeometryColoringMode::kTreeLevel);

            std::vector<VkVertexInputAttributeDescription> blas_instance_id_attr{
                VERTEX_ATTRIBUTE(0, position),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(1, instance_transform),
                INSTANCE_ATTRIBUTE(5, blas_index),
                INSTANCE_ATTRIBUTE(6, wireframe_metadata),
            };
            InitializeGeometryColorPipeline(
                "GeometryColorBlasInstanceId.vs.spv", "GeometryColorBlasInstanceId.ps.spv", blas_instance_id_attr, GeometryColoringMode::kBlasInstanceId);

            std::vector<VkVertexInputAttributeDescription> geometry_index_attr{
                VERTEX_ATTRIBUTE(0, position),
                VERTEX_ATTRIBUTE(1, geometry_index_depth_split_opaque),
                VERTEX_ATTRIBUTE(2, triangle_sah_and_selected),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(3, instance_transform),
                INSTANCE_ATTRIBUTE(7, wireframe_metadata),
            };
            InitializeGeometryColorPipeline(
                "GeometryColorGeometryIndex.vs.spv", "GeometryColorGeometryIndex.ps.spv", geometry_index_attr, GeometryColoringMode::kGeometryIndex);

            std::vector<VkVertexInputAttributeDescription> opacity_attr{
                VERTEX_ATTRIBUTE(0, position),
                VERTEX_ATTRIBUTE(1, geometry_index_depth_split_opaque),
                VERTEX_ATTRIBUTE(2, triangle_sah_and_selected),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(3, instance_transform),
                INSTANCE_ATTRIBUTE(7, wireframe_metadata),
            };
            InitializeGeometryColorPipeline("GeometryColorOpacity.vs.spv", "GeometryColorOpacity.ps.spv", opacity_attr, GeometryColoringMode::kOpacity);

            std::vector<VkVertexInputAttributeDescription> final_opacity_attr{
                VERTEX_ATTRIBUTE(0, position),
                VERTEX_ATTRIBUTE(1, geometry_index_depth_split_opaque),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(2, instance_transform),
                INSTANCE_ATTRIBUTE(6, flags),
                INSTANCE_ATTRIBUTE(7, wireframe_metadata),
            };
            InitializeGeometryColorPipeline(
                "GeometryColorFinalOpacity.vs.spv", "GeometryColorFinalOpacity.ps.spv", final_opacity_attr, GeometryColoringMode::kFinalOpacity);

            std::vector<VkVertexInputAttributeDescription> instance_mask_attr{
                VERTEX_ATTRIBUTE(0, position),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(1, instance_transform),
                INSTANCE_ATTRIBUTE(5, wireframe_metadata),
                INSTANCE_ATTRIBUTE(6, mask),
            };
            InitializeGeometryColorPipeline(
                "GeometryColorInstanceMask.vs.spv", "GeometryColorInstanceMask.ps.spv", instance_mask_attr, GeometryColoringMode::kInstanceMask);

            std::vector<VkVertexInputAttributeDescription> lit_attr{
                VERTEX_ATTRIBUTE(0, position),
                VERTEX_ATTRIBUTE(1, normal),
                VERTEX_ATTRIBUTE(2, triangle_sah_and_selected),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(3, instance_transform),
                INSTANCE_ATTRIBUTE(7, wireframe_metadata),
            };
            InitializeGeometryColorPipeline("GeometryColorLit.vs.spv", "GeometryColorLit.ps.spv", lit_attr, GeometryColoringMode::kLit);

            std::vector<VkVertexInputAttributeDescription> technical_attr{
                VERTEX_ATTRIBUTE(0, position),
                VERTEX_ATTRIBUTE(1, normal),
                VERTEX_ATTRIBUTE(2, triangle_sah_and_selected),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(3, instance_transform),
                INSTANCE_ATTRIBUTE(7, wireframe_metadata),
            };
            InitializeGeometryColorPipeline("GeometryColorTechnical.vs.spv", "GeometryColorTechnical.ps.spv", technical_attr, GeometryColoringMode::kTechnical);

            std::vector<VkVertexInputAttributeDescription> average_sah_attr{
                VERTEX_ATTRIBUTE(0, position),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(1, instance_transform),
                INSTANCE_ATTRIBUTE(5, average_triangle_sah),
                INSTANCE_ATTRIBUTE(6, wireframe_metadata),
            };
            InitializeGeometryColorPipeline(
                "GeometryColorBlasAverageSAH.vs.spv", "GeometryColorBlasAverageSAH.ps.spv", average_sah_attr, GeometryColoringMode::kBlasAverageSAH);

            std::vector<VkVertexInputAttributeDescription> min_sah_attr{
                VERTEX_ATTRIBUTE(0, position),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(1, instance_transform),
                INSTANCE_ATTRIBUTE(5, min_triangle_sah),
                INSTANCE_ATTRIBUTE(6, wireframe_metadata),
            };
            InitializeGeometryColorPipeline(
                "GeometryColorBlasMinSAH.vs.spv", "GeometryColorBlasMinSAH.ps.spv", min_sah_attr, GeometryColoringMode::kBlasMinSAH);

            std::vector<VkVertexInputAttributeDescription> triangle_sah_attr{
                VERTEX_ATTRIBUTE(0, position),
                VERTEX_ATTRIBUTE(1, triangle_sah_and_selected),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(2, instance_transform),
                INSTANCE_ATTRIBUTE(6, wireframe_metadata),
            };
            InitializeGeometryColorPipeline(
                "GeometryColorTriangleSAH.vs.spv", "GeometryColorTriangleSAH.ps.spv", triangle_sah_attr, GeometryColoringMode::kTriangleSAH);

            std::vector<VkVertexInputAttributeDescription> blas_instance_count_attr{
                VERTEX_ATTRIBUTE(0, position),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(1, instance_transform),
                INSTANCE_ATTRIBUTE(5, instance_count),
                INSTANCE_ATTRIBUTE(6, wireframe_metadata),
            };
            InitializeGeometryColorPipeline("GeometryColorBlasInstanceCount.vs.spv",
                                            "GeometryColorBlasInstanceCount.ps.spv",
                                            blas_instance_count_attr,
                                            GeometryColoringMode::kBlasInstanceCount);

            std::vector<VkVertexInputAttributeDescription> blas_triangle_count_attr{
                VERTEX_ATTRIBUTE(0, position),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(1, instance_transform),
                INSTANCE_ATTRIBUTE(5, triangle_count),
                INSTANCE_ATTRIBUTE(6, wireframe_metadata),
            };
            InitializeGeometryColorPipeline("GeometryColorBlasTriangleCount.vs.spv",
                                            "GeometryColorBlasTriangleCount.ps.spv",
                                            blas_triangle_count_attr,
                                            GeometryColoringMode::kBlasTriangleCount);

            std::vector<VkVertexInputAttributeDescription> blas_max_depth_attr{
                VERTEX_ATTRIBUTE(0, position),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(1, instance_transform),
                INSTANCE_ATTRIBUTE(5, max_depth),
                INSTANCE_ATTRIBUTE(6, wireframe_metadata),
            };
            InitializeGeometryColorPipeline(
                "GeometryColorBlasMaxDepth.vs.spv", "GeometryColorBlasMaxDepth.ps.spv", blas_max_depth_attr, GeometryColoringMode::kBlasMaxDepth);

            std::vector<VkVertexInputAttributeDescription> blas_average_depth_attr{
                VERTEX_ATTRIBUTE(0, position),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(1, instance_transform),
                INSTANCE_ATTRIBUTE(5, average_depth),
                INSTANCE_ATTRIBUTE(6, wireframe_metadata),
            };
            InitializeGeometryColorPipeline("GeometryColorBlasAverageDepth.vs.spv",
                                            "GeometryColorBlasAverageDepth.ps.spv",
                                            blas_average_depth_attr,
                                            GeometryColoringMode::kBlasAverageDepth);

            std::vector<VkVertexInputAttributeDescription> instance_index_attr{
                VERTEX_ATTRIBUTE(0, position),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(1, instance_transform),
                INSTANCE_ATTRIBUTE(5, instance_index),
                INSTANCE_ATTRIBUTE(6, blas_index),
                INSTANCE_ATTRIBUTE(7, wireframe_metadata),
            };
            InitializeGeometryColorPipeline(
                "GeometryColorInstanceIndex.vs.spv", "GeometryColorInstanceIndex.ps.spv", instance_index_attr, GeometryColoringMode::kInstanceIndex);

            std::vector<VkVertexInputAttributeDescription> instance_rebraiding{
                VERTEX_ATTRIBUTE(0, position),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(1, instance_transform),
                INSTANCE_ATTRIBUTE(5, rebraided),
                INSTANCE_ATTRIBUTE(6, wireframe_metadata),
            };
            InitializeGeometryColorPipeline(
                "GeometryColorRebraiding.vs.spv", "GeometryColorRebraiding.ps.spv", instance_rebraiding, GeometryColoringMode::kInstanceRebraiding);

            std::vector<VkVertexInputAttributeDescription> triangle_splitting{
                VERTEX_ATTRIBUTE(0, position),
                VERTEX_ATTRIBUTE(1, geometry_index_depth_split_opaque),
                VERTEX_ATTRIBUTE(2, triangle_sah_and_selected),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(3, instance_transform),
                INSTANCE_ATTRIBUTE(7, wireframe_metadata),
            };
            InitializeGeometryColorPipeline(
                "GeometryColorTriangleSplitting.vs.spv", "GeometryColorTriangleSplitting.ps.spv", triangle_splitting, GeometryColoringMode::kTriangleSplitting);

            std::vector<VkVertexInputAttributeDescription> build_flags_attr{
                VERTEX_ATTRIBUTE(0, position),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(1, instance_transform),
                INSTANCE_ATTRIBUTE(5, build_flags),
                INSTANCE_ATTRIBUTE(6, wireframe_metadata),
            };

            InitializeGeometryColorPipeline("GeometryColorPreferFastBuildOrTrace.vs.spv",
                                            "GeometryColorPreferFastBuildOrTrace.ps.spv",
                                            build_flags_attr,
                                            GeometryColoringMode::kFastBuildOrTraceFlag);

            InitializeGeometryColorPipeline("GeometryColorAllowCompactionFlag.vs.spv",
                                            "GeometryColorAllowCompactionFlag.ps.spv",
                                            build_flags_attr,
                                            GeometryColoringMode::kAllowCompactionFlag);

            InitializeGeometryColorPipeline(
                "GeometryColorAllowUpdateFlag.vs.spv", "GeometryColorAllowUpdateFlag.ps.spv", build_flags_attr, GeometryColoringMode::kAllowUpdateFlag);

            InitializeGeometryColorPipeline(
                "GeometryColorLowMemoryFlag.vs.spv", "GeometryColorLowMemoryFlag.ps.spv", build_flags_attr, GeometryColoringMode::kLowMemoryFlag);

            std::vector<VkVertexInputAttributeDescription> instance_flags_attr{
                VERTEX_ATTRIBUTE(0, position),

                INSTANCE_ATTRIBUTE_FOUR_SLOTS(1, instance_transform),
                INSTANCE_ATTRIBUTE(5, flags),
                INSTANCE_ATTRIBUTE(6, wireframe_metadata),
            };

            InitializeGeometryColorPipeline("GeometryInstanceFacingCullDisable.vs.spv",
                                            "GeometryInstanceFacingCullDisable.ps.spv",
                                            instance_flags_attr,
                                            GeometryColoringMode::kInstanceFacingCullDisableBit);

            InitializeGeometryColorPipeline(
                "GeometryInstanceFlipFacing.vs.spv", "GeometryInstanceFlipFacing.ps.spv", instance_flags_attr, GeometryColoringMode::kInstanceFlipFacingBit);

            InitializeGeometryColorPipeline("GeometryInstanceForceOpaqueOrNoOpaque.vs.spv",
                                            "GeometryInstanceForceOpaqueOrNoOpaque.ps.spv",
                                            instance_flags_attr,
                                            GeometryColoringMode::kInstanceForceOpaqueOrNoOpaqueBits);
        }

        void MeshRenderModule::UploadCustomTriangles(VkCommandBuffer command_buffer)
        {
            // Upload any custom triangle data.
            struct
            {
                VmaAllocation allocation = VK_NULL_HANDLE;
                VkBuffer      buffer     = VK_NULL_HANDLE;
            } custom_triangle_staging;

            auto vertex_list = current_scene_info_->custom_triangles;

            if (vertex_list != nullptr && vertex_list->size() > 0)
            {
                auto custom_triangle_byte_size = vertex_list->size() * sizeof(RraVertex);

                context_->device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                               VMA_MEMORY_USAGE_CPU_ONLY,
                                               custom_triangle_staging.buffer,
                                               custom_triangle_staging.allocation,
                                               vertex_list->data(),
                                               custom_triangle_byte_size);

                custom_triangle_buffer.buffer     = VK_NULL_HANDLE;
                custom_triangle_buffer.allocation = VK_NULL_HANDLE;

                context_->device->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                               VMA_MEMORY_USAGE_GPU_ONLY,
                                               custom_triangle_buffer.buffer,
                                               custom_triangle_buffer.allocation,
                                               nullptr,
                                               custom_triangle_byte_size);

                SetObjectName(context_->device->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)custom_triangle_staging.buffer, "customTriangleStagingBuffer");
                SetObjectName(context_->device->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)custom_triangle_buffer.buffer, "customTriangleBuffer");

                VkBufferCopy copy_region = {};
                copy_region.size         = custom_triangle_byte_size;
                vkCmdCopyBuffer(command_buffer, custom_triangle_staging.buffer, custom_triangle_buffer.buffer, 1, &copy_region);

                if (custom_triangle_buffer.buffer)
                {
                    VkBufferMemoryBarrier buffer_barrier{};
                    buffer_barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                    buffer_barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
                    buffer_barrier.dstAccessMask       = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
                    buffer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    buffer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    buffer_barrier.buffer              = custom_triangle_buffer.buffer;
                    buffer_barrier.offset              = 0;
                    buffer_barrier.size                = VK_WHOLE_SIZE;

                    vkCmdPipelineBarrier(command_buffer,
                                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                                         VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                                         VK_DEPENDENCY_BY_REGION_BIT,
                                         0,
                                         nullptr,
                                         1,
                                         &buffer_barrier,
                                         0,
                                         nullptr);
                }

                custom_triangles_guard.SetCurrentBuffer(custom_triangle_buffer.buffer, custom_triangle_buffer.allocation);
                custom_triangles_staging_guard.SetCurrentBuffer(custom_triangle_staging.buffer, custom_triangle_staging.allocation);
                custom_triangle_buffer.vertex_count = static_cast<uint32_t>(vertex_list->size());
            }
            else
            {
                custom_triangles_guard.SetCurrentBuffer(VK_NULL_HANDLE, VK_NULL_HANDLE);
                custom_triangles_staging_guard.SetCurrentBuffer(VK_NULL_HANDLE, VK_NULL_HANDLE);
                custom_triangle_buffer.vertex_count = 0;
            }
        }

        static uint32_t GetTotalInstanceCountForBlas(uint64_t blas_index, const std::map<uint64_t, uint32_t>* instance_counts)
        {
            auto result = instance_counts->find(blas_index);
            if (result != instance_counts->end())
            {
                return result->second;
            }
            return 0;
        }

        static glm::vec4 GetWireframeColor(bool render_wireframe, bool selected, const rra::renderer::RendererSceneInfo* info)
        {
            const glm::vec4 wireframe_normal   = info->wireframe_normal_color;
            const glm::vec4 wireframe_selected = info->wireframe_selected_color;
            if (render_wireframe)
            {
                if (selected)
                {
                    return {wireframe_selected.r, wireframe_selected.g, wireframe_selected.b, kWireframeWidth};
                }
                else
                {
                    return {wireframe_normal.r, wireframe_normal.g, wireframe_normal.b, kWireframeWidth};
                }
            }
            else
            {
                return {wireframe_selected.r, wireframe_selected.g, wireframe_selected.b, 0.0f};
            }
        }

        float MeshRenderModule::ProcessSceneData(VkCommandBuffer command_buffer, glm::vec3 camera_position)
        {
            // Clear the old render instructions.
            render_instructions_.clear();

            // Pour the instances into one big buffer that will be uploaded to the device.
            std::vector<MeshInstanceData> mesh_info_buffer;
            mesh_info_buffer.reserve(current_scene_info_->max_instance_count);

            for (auto& instance_iter : current_scene_info_->instance_map)
            {
                //auto instance_count_for_blas = current_scene_info_->GetTotalInstanceCountForBlas(instance_iter.first);
                auto        instance_count_for_blas = GetTotalInstanceCountForBlas(instance_iter.first, current_scene_info_->instance_counts);
                const auto& instance_transforms     = instance_iter.second;

                auto mesh = GetVkGraphicsContext()->GetBlasDrawInstruction(instance_iter.first);

                std::vector<MeshInstanceData> temp_buffer;
                temp_buffer.reserve(instance_transforms.size());

                MeshInstanceData mesh_instance_data{};

                for (uint32_t i = 0; i < instance_transforms.size(); i++)
                {
                    mesh_instance_data.instance_transform   = instance_transforms[i].transform;
                    mesh_instance_data.instance_index       = instance_transforms[i].instance_unique_index;
                    mesh_instance_data.instance_node        = instance_transforms[i].instance_node;
                    mesh_instance_data.instance_count       = instance_count_for_blas;
                    mesh_instance_data.blas_index           = instance_transforms[i].blas_index;
                    mesh_instance_data.triangle_count       = mesh.vertex_count / 3;
                    mesh_instance_data.flags                = instance_transforms[i].flags;
                    mesh_instance_data.max_depth            = instance_transforms[i].max_depth;
                    mesh_instance_data.mask                 = instance_transforms[i].mask;
                    mesh_instance_data.average_depth        = instance_transforms[i].average_depth;
                    mesh_instance_data.min_triangle_sah     = instance_transforms[i].min_triangle_sah;
                    mesh_instance_data.average_triangle_sah = instance_transforms[i].average_triangle_sah;
                    mesh_instance_data.build_flags          = instance_transforms[i].build_flags;
                    mesh_instance_data.rebraided            = instance_transforms[i].rebraided;
                    mesh_instance_data.wireframe_metadata =
                        GetWireframeColor(render_state_.render_wireframe, instance_transforms[i].selected, current_scene_info_);

                    if (instance_transforms[i].selected)
                    {
                        mesh_instance_data.selection_count += 1;
                    }

                    temp_buffer.push_back(mesh_instance_data);
                }

                // Add the next instruction for this instance type if there is any.
                if (temp_buffer.size() > 0)
                {
                    render_instructions_.push_back({mesh.vertex_buffer,
                                                    mesh.vertex_index,
                                                    mesh.vertex_count,
                                                    static_cast<uint32_t>(mesh_info_buffer.size()),
                                                    static_cast<uint32_t>(temp_buffer.size())});
                }

                // Append the temp buffer into final info buffer.
                mesh_info_buffer.insert(mesh_info_buffer.end(), temp_buffer.begin(), temp_buffer.end());
            }

            if (custom_triangle_buffer.vertex_count > 0)
            {
                MeshInstanceData mesh_instance_data = {};

                mesh_instance_data.instance_transform = glm::mat4(1.0f);
                mesh_instance_data.instance_index     = 0;
                mesh_instance_data.instance_node      = 0;
                mesh_instance_data.instance_count     = 1;
                mesh_instance_data.blas_index         = 0;
                mesh_instance_data.triangle_count     = custom_triangle_buffer.vertex_count / 3;
                mesh_instance_data.max_depth          = 1;
                mesh_instance_data.average_depth      = 1;
                mesh_instance_data.wireframe_metadata = GetWireframeColor(render_state_.render_wireframe, false, current_scene_info_);

                render_instructions_.push_back(
                    {custom_triangle_buffer.buffer, 0, custom_triangle_buffer.vertex_count, static_cast<uint32_t>(mesh_info_buffer.size()), 1});
                mesh_info_buffer.push_back(mesh_instance_data);
            }

            per_blas_instance_buffer_.size = mesh_info_buffer.size() * sizeof(MeshInstanceData);

            struct
            {
                VmaAllocation allocation = VK_NULL_HANDLE;
                VkBuffer      buffer     = VK_NULL_HANDLE;
            } instance_staging_buffer;

            if (per_blas_instance_buffer_.size == 0)
            {
                render_instructions_ = {};
            }
            else
            {
                context_->device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                               VMA_MEMORY_USAGE_CPU_ONLY,
                                               instance_staging_buffer.buffer,
                                               instance_staging_buffer.allocation,
                                               mesh_info_buffer.data(),
                                               per_blas_instance_buffer_.size);

                per_blas_instance_buffer_.buffer     = VK_NULL_HANDLE;
                per_blas_instance_buffer_.allocation = VK_NULL_HANDLE;

                context_->device->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                               VMA_MEMORY_USAGE_GPU_ONLY,
                                               per_blas_instance_buffer_.buffer,
                                               per_blas_instance_buffer_.allocation,
                                               nullptr,
                                               per_blas_instance_buffer_.size);

                SetObjectName(
                    context_->device->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)instance_staging_buffer.buffer, "meshModuleInstanceStagingBuffer");
                SetObjectName(context_->device->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)per_blas_instance_buffer_.buffer, "meshModuleBuffer");

                VkBufferCopy copy_region = {};
                copy_region.size         = per_blas_instance_buffer_.size;
                vkCmdCopyBuffer(command_buffer, instance_staging_buffer.buffer, per_blas_instance_buffer_.buffer, 1, &copy_region);

                instance_guard.SetCurrentBuffer(per_blas_instance_buffer_.buffer, per_blas_instance_buffer_.allocation);
                instance_staging_guard.SetCurrentBuffer(instance_staging_buffer.buffer, instance_staging_buffer.allocation);
            }

            if (current_scene_info_->instance_map.size() == 0)
            {
                return 0.01f;
            }

            return glm::distance(camera_position, current_scene_info_->closest_point_to_camera);
        }

        void MeshRenderModule::InitializeDefaultRenderState()
        {
            // Initialize the render state settings to suitable default values.
            render_state_.render_geometry     = true;
            render_state_.render_wireframe    = true;
            render_state_.selection_only      = false;
            render_state_.coloring_mode_index = 0;
            // We don't initialize the culling mode here since it is set beforehand from settings.
        }
    }  // namespace renderer
}  // namespace rra

