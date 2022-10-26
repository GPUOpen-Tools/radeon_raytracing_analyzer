//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the transform gizmo render module.
//=============================================================================

#include <vector>

#include <QCoreApplication>

#include "public/rra_assert.h"
#include "public/orientation_gizmo.h"
#include "orientation_gizmo_module.h"
#include "vk/vk_graphics_context.h"

namespace rra
{
    namespace renderer
    {
        static const uint32_t kVertexBufferBindId   = 0;
        static const uint32_t kInstanceBufferBindId = 1;

        OrientationGizmoHitType OrientationGizmoRenderModule::selected = OrientationGizmoHitType::kNone;

        OrientationGizmoRenderModule::OrientationGizmoRenderModule()
            : RenderModule(RenderPassHint::kRenderPassHintClearDepthOnly)
        {
        }

        void OrientationGizmoRenderModule::Initialize(const RenderModuleContext* context)
        {
            context_ = context;

            orientation_gizmo_mesh_.Initialize(context->device, context->command_buffer_ring);

            VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
            pipeline_layout_create_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.setLayoutCount             = 0;

            // Create the pipeline layout.
            VkResult create_result = vkCreatePipelineLayout(context->device->GetDevice(), &pipeline_layout_create_info, nullptr, &pipeline_layout_);
            CheckResult(create_result, "Failed to create pipeline layout.");

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
            rasterization_state.lineWidth                              = 1.0f;

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
            multisample_state.rasterizationSamples                 = context->swapchain->GetMSAASamples();
            multisample_state.flags                                = 0;

            std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

            VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
            dynamic_state_info.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamic_state_info.pDynamicStates                   = dynamic_states.data();
            dynamic_state_info.dynamicStateCount                = static_cast<uint32_t>(dynamic_states.size());
            dynamic_state_info.flags                            = 0;

            // Binding point 0: Mesh vertex layout description at per-vertex rate
            VkVertexInputBindingDescription mesh_binding_description = {};
            mesh_binding_description.binding                         = kVertexBufferBindId;
            mesh_binding_description.stride                          = sizeof(OrientationGizmoMeshInfo::VertexType);
            mesh_binding_description.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

            // Binding point 1: Instanced data at per-instance rate
            VkVertexInputBindingDescription instance_binding_description = {};
            instance_binding_description.binding                         = kInstanceBufferBindId;
            instance_binding_description.stride                          = sizeof(OrientationGizmoInstance);
            instance_binding_description.inputRate                       = VK_VERTEX_INPUT_RATE_INSTANCE;

            std::vector<VkVertexInputBindingDescription> binding_descriptions = {mesh_binding_description, instance_binding_description};

            // Per-vertex attributes
            // These are advanced for each vertex fetched by the vertex shader.
            VkVertexInputAttributeDescription vertex_position_attrib = {
                0, kVertexBufferBindId, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexPosition, position)  // Location 0: Position
            };

            // Per-Instance attributes
            // These are fetched for each instance rendered.
            VkVertexInputAttributeDescription instance_color_attrib = {
                1, kInstanceBufferBindId, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(OrientationGizmoInstance, color)  // Location 1: Color
            };

            VkVertexInputAttributeDescription instance_fade_factor_attrib = {
                2, kInstanceBufferBindId, VK_FORMAT_R32_SFLOAT, offsetof(OrientationGizmoInstance, fade_factor)  // Location 2: Fade Factor
            };

            // Float4x4 transform matrix
            VkVertexInputAttributeDescription transform_attrib_0 = {
                3, kInstanceBufferBindId, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(OrientationGizmoInstance, transform) + sizeof(glm::vec4) * 0};

            VkVertexInputAttributeDescription transform_attrib_1 = {
                4, kInstanceBufferBindId, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(OrientationGizmoInstance, transform) + sizeof(glm::vec4) * 1};

            VkVertexInputAttributeDescription transform_attrib_2 = {
                5, kInstanceBufferBindId, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(OrientationGizmoInstance, transform) + sizeof(glm::vec4) * 2};

            VkVertexInputAttributeDescription transform_attrib_3 = {
                6, kInstanceBufferBindId, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(OrientationGizmoInstance, transform) + sizeof(glm::vec4) * 3};

            std::vector<VkVertexInputAttributeDescription> attribute_descriptions = {vertex_position_attrib,
                                                                                     instance_color_attrib,
                                                                                     instance_fade_factor_attrib,
                                                                                     transform_attrib_0,
                                                                                     transform_attrib_1,
                                                                                     transform_attrib_2,
                                                                                     transform_attrib_3};

            VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
            vertex_input_state_info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_state_info.pVertexBindingDescriptions           = binding_descriptions.data();
            vertex_input_state_info.vertexBindingDescriptionCount        = static_cast<uint32_t>(binding_descriptions.size());
            vertex_input_state_info.pVertexAttributeDescriptions         = attribute_descriptions.data();
            vertex_input_state_info.vertexAttributeDescriptionCount      = static_cast<uint32_t>(attribute_descriptions.size());

            // Load shaders.
            std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
            const char*                                    full_vertex_shader_path = "OrientationGizmo.vs.spv";
            const char*                                    full_pixel_shader_path  = "OrientationGizmo.ps.spv";
            LoadShader(full_vertex_shader_path, context->device, VK_SHADER_STAGE_VERTEX_BIT, "VSMain", shader_stages[0]);
            LoadShader(full_pixel_shader_path, context->device, VK_SHADER_STAGE_FRAGMENT_BIT, "PSMain", shader_stages[1]);

            // Use the constructed data to describe the pipeline.
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
            pipeline_create_info.pVertexInputState            = &vertex_input_state_info;

            // Create the pipeline.
            create_result =
                vkCreateGraphicsPipelines(context->device->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &orientation_gizmo_pipeline_);
            CheckResult(create_result, "Failed to create orientation gizmo pipeline.");

            // Destroy shader modules.
            vkDestroyShaderModule(context->device->GetDevice(), shader_stages[0].module, nullptr);
            vkDestroyShaderModule(context->device->GetDevice(), shader_stages[1].module, nullptr);

            instance_buffer_guard_.Initialize(context->swapchain->GetBackBufferCount());
            instance_buffer_staging_guard_.Initialize(context->swapchain->GetBackBufferCount());
        };

        void OrientationGizmoRenderModule::Draw(const RenderFrameContext* context)
        {
            auto instances = InitializeInstances(context->camera.GetRotationMatrix(), context->camera.GetAspectRatio());
            CreateAndUploadInstanceBuffer(instances, context->command_buffer);
            instance_buffer_guard_.ProcessFrame(context->current_frame, context->device);
            instance_buffer_staging_guard_.ProcessFrame(context->current_frame, context->device);

            if (instance_buffer_.size == 0)
            {
                return;
            }

            VkDeviceSize zero_offset = 0;

            /// Lines.
            context->begin_render_pass();

            vkCmdBindPipeline(context->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, orientation_gizmo_pipeline_);

            vkCmdBindVertexBuffers(context->command_buffer, kVertexBufferBindId, 1, &orientation_gizmo_mesh_.GetVertices().buffer, &zero_offset);
            vkCmdBindVertexBuffers(context->command_buffer, kInstanceBufferBindId, 1, &instance_buffer_.buffer, &zero_offset);
            vkCmdBindIndexBuffer(context->command_buffer, orientation_gizmo_mesh_.GetIndices().buffer, 0, VK_INDEX_TYPE_UINT16);

            // Draw instances.
            for (size_t i = 0; i < instances.size(); i++)
            {
                auto submesh_info = orientation_gizmo_mesh_.GetSubmeshInfo(instances[i].type);
                vkCmdDrawIndexed(
                    context->command_buffer, submesh_info.index_count, 1, submesh_info.first_index, submesh_info.vertex_offset, static_cast<uint32_t>(i));
            }

            context->end_render_pass();
        };

        void OrientationGizmoRenderModule::Cleanup(const RenderModuleContext* context)
        {
            orientation_gizmo_mesh_.Cleanup(context->device);
            vkDestroyPipeline(context->device->GetDevice(), orientation_gizmo_pipeline_, nullptr);
            vkDestroyPipelineLayout(context->device->GetDevice(), pipeline_layout_, nullptr);
            instance_buffer_guard_.Cleanup(context->device);
            instance_buffer_staging_guard_.Cleanup(context->device);
        };

        std::vector<OrientationGizmoInstance> OrientationGizmoRenderModule::InitializeInstances(glm::mat4 rotation, float window_ratio)
        {
            OrientationGizmoTransformInfo info = GetOrientationGizmoTransformInfo(rotation, window_ratio);

            const float     inner_circle_radius_factor = 0.85f;
            const float     inner_circle_darkening     = 0.75f;
            const float     minus_circle_alpha         = 0.2f;
            const glm::vec4 x_color                    = glm::vec4(0.9f, 0.21176f, 0.32549f, 1.0f);
            const glm::vec4 y_color                    = glm::vec4(0.54117f, 0.75882f, 0.0f, 1.0f);
            const glm::vec4 z_color                    = glm::vec4(0.17254f, 0.56078f, 1.0f, 0.9f);
            const glm::vec4 background_color           = glm::vec4(0.9f, 0.9f, 0.9f, 0.0f);
            const glm::vec4 background_color_highlight = glm::vec4(0.9f, 0.9f, 0.9f, 0.6f);

            OrientationGizmoInstance x_axis = {};
            x_axis.type                     = OrientationGizmoInstanceType::kCylinder;
            x_axis.color                    = x_color;
            x_axis.fade_factor              = 1.0f;
            x_axis.transform                = info.screen_transform * info.x_axis_transform;

            OrientationGizmoInstance y_axis = {};
            y_axis.type                     = OrientationGizmoInstanceType::kCylinder;
            y_axis.color                    = y_color;
            y_axis.fade_factor              = 1.0f;
            y_axis.transform                = info.screen_transform * info.y_axis_transform;

            OrientationGizmoInstance z_axis = {};
            y_axis.type                     = OrientationGizmoInstanceType::kCylinder;
            z_axis.color                    = z_color;
            z_axis.fade_factor              = 1.0f;
            z_axis.transform                = info.screen_transform * info.z_axis_transform;

            /// Circles.
            OrientationGizmoInstance x_circle = {};
            x_circle.type                     = OrientationGizmoInstanceType::kCircle;
            x_circle.color                    = x_color;
            x_circle.fade_factor              = 1.0f;
            x_circle.transform                = info.screen_transform * info.x_circle_transform;

            OrientationGizmoInstance y_circle = {};
            y_circle.type                     = OrientationGizmoInstanceType::kCircle;
            y_circle.color                    = y_color;
            y_circle.fade_factor              = 1.0f;
            y_circle.transform                = info.screen_transform * info.y_circle_transform;

            OrientationGizmoInstance z_circle = {};
            z_circle.type                     = OrientationGizmoInstanceType::kCircle;
            z_circle.color                    = z_color;
            z_circle.fade_factor              = 1.0f;
            z_circle.transform                = info.screen_transform * info.z_circle_transform;

            OrientationGizmoInstance inner_x_circle = {};
            inner_x_circle.type                     = OrientationGizmoInstanceType::kCircle;
            inner_x_circle.color                    = x_color;
            inner_x_circle.fade_factor              = 1.0f;
            inner_x_circle.transform                = info.screen_transform * info.x_circle_transform * glm::scale(glm::vec3(inner_circle_radius_factor));

            OrientationGizmoInstance inner_y_circle = {};
            inner_y_circle.type                     = OrientationGizmoInstanceType::kCircle;
            inner_y_circle.color                    = y_color;
            inner_y_circle.fade_factor              = 1.0f;
            inner_y_circle.transform                = info.screen_transform * info.y_circle_transform * glm::scale(glm::vec3(inner_circle_radius_factor));

            OrientationGizmoInstance inner_z_circle = {};
            inner_z_circle.type                     = OrientationGizmoInstanceType::kCircle;
            inner_z_circle.color                    = z_color;
            inner_z_circle.fade_factor              = 1.0f;
            inner_z_circle.transform                = info.screen_transform * info.z_circle_transform * glm::scale(glm::vec3(inner_circle_radius_factor));

            OrientationGizmoInstance minus_x_circle = {};
            minus_x_circle.type                     = OrientationGizmoInstanceType::kCircle;
            minus_x_circle.color                    = x_color;
            minus_x_circle.color.a                  = minus_circle_alpha;
            minus_x_circle.fade_factor              = 1.0f;
            minus_x_circle.transform                = info.screen_transform * info.minus_x_circle_transform;

            OrientationGizmoInstance minus_y_circle = {};
            minus_y_circle.type                     = OrientationGizmoInstanceType::kCircle;
            minus_y_circle.color                    = y_color;
            minus_y_circle.color.a                  = minus_circle_alpha;
            minus_y_circle.fade_factor              = 1.0f;
            minus_y_circle.transform                = info.screen_transform * info.minus_y_circle_transform;

            OrientationGizmoInstance minus_z_circle = {};
            minus_z_circle.type                     = OrientationGizmoInstanceType::kCircle;
            minus_z_circle.color                    = z_color;
            minus_z_circle.color.a                  = minus_circle_alpha;
            minus_z_circle.fade_factor              = 1.0f;
            minus_z_circle.transform                = info.screen_transform * info.minus_z_circle_transform;

            OrientationGizmoInstance inner_minus_x_circle = {};
            inner_minus_x_circle.type                     = OrientationGizmoInstanceType::kCircle;
            inner_minus_x_circle.color                    = x_color * inner_circle_darkening;
            inner_minus_x_circle.color.a                  = minus_circle_alpha;
            inner_minus_x_circle.fade_factor              = 1.0f;
            inner_minus_x_circle.transform = info.screen_transform * info.minus_x_circle_transform * glm::scale(glm::vec3(inner_circle_radius_factor));

            OrientationGizmoInstance inner_minus_y_circle = {};
            inner_minus_y_circle.type                     = OrientationGizmoInstanceType::kCircle;
            inner_minus_y_circle.color                    = y_color * inner_circle_darkening;
            inner_minus_y_circle.color.a                  = minus_circle_alpha;
            inner_minus_y_circle.fade_factor              = 1.0f;
            inner_minus_y_circle.transform = info.screen_transform * info.minus_y_circle_transform * glm::scale(glm::vec3(inner_circle_radius_factor));

            OrientationGizmoInstance inner_minus_z_circle = {};
            inner_minus_z_circle.type                     = OrientationGizmoInstanceType::kCircle;
            inner_minus_z_circle.color                    = z_color * inner_circle_darkening;
            inner_minus_z_circle.color.a                  = minus_circle_alpha;
            inner_minus_z_circle.fade_factor              = 1.0f;
            inner_minus_z_circle.transform = info.screen_transform * info.minus_z_circle_transform * glm::scale(glm::vec3(inner_circle_radius_factor));

            OrientationGizmoInstance background_circle = {};
            background_circle.type                     = OrientationGizmoInstanceType::kCircle;
            background_circle.color                    = background_color;
            background_circle.fade_factor              = 0.0f;
            background_circle.transform                = info.screen_transform * info.background_transform;

            glm::vec4 white = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            switch (selected)
            {
            case OrientationGizmoHitType::kX:
                x_circle.color          = white;
                x_circle.fade_factor    = 0.0f;
                background_circle.color = background_color_highlight;
                break;
            case OrientationGizmoHitType::kY:
                y_circle.color          = white;
                y_circle.fade_factor    = 0.0f;
                background_circle.color = background_color_highlight;
                break;
            case OrientationGizmoHitType::kZ:
                z_circle.color          = white;
                z_circle.fade_factor    = 0.0f;
                background_circle.color = background_color_highlight;
                break;
            case OrientationGizmoHitType::kMinusX:
                minus_x_circle.color       = white;
                minus_x_circle.fade_factor = 0.0f;
                background_circle.color    = background_color_highlight;
                break;
            case OrientationGizmoHitType::kMinusY:
                minus_y_circle.color       = white;
                minus_y_circle.fade_factor = 0.0f;
                background_circle.color    = background_color_highlight;
                break;
            case OrientationGizmoHitType::kMinusZ:
                minus_z_circle.color       = white;
                minus_z_circle.fade_factor = 0.0f;
                background_circle.color    = background_color_highlight;
                break;
            case OrientationGizmoHitType::kBackground:
                background_circle.color = background_color_highlight;
                break;
            }

            /// Letters.
            OrientationGizmoInstance x_letter = {};
            x_letter.type                     = OrientationGizmoInstanceType::kX;
            x_letter.color                    = glm::vec4(1.0f);
            x_letter.transform                = glm::translate(glm::vec3(0.0f, 0.0f, glm::epsilon<float>())) * x_circle.transform;

            OrientationGizmoInstance y_letter = {};
            y_letter.type                     = OrientationGizmoInstanceType::kY;
            y_letter.color                    = glm::vec4(1.0f);
            y_letter.transform                = glm::translate(glm::vec3(0.0f, 0.0f, glm::epsilon<float>())) * y_circle.transform;

            OrientationGizmoInstance z_letter = {};
            z_letter.type                     = OrientationGizmoInstanceType::kZ;
            z_letter.color                    = glm::vec4(1.0f);
            z_letter.transform                = glm::translate(glm::vec3(0.0f, 0.0f, glm::epsilon<float>())) * z_circle.transform;

            /// Collect instances.
            std::vector<OrientationGizmoInstance> gizmo_instances;

            // Sort order by z.
            gizmo_instances.push_back(background_circle);

            for (auto index : info.order)
            {
                if (index == 0)
                {
                    gizmo_instances.push_back(x_axis);
                    gizmo_instances.push_back(x_circle);
                    gizmo_instances.push_back(inner_x_circle);
                    gizmo_instances.push_back(x_letter);
                }
                else if (index == 1)
                {
                    gizmo_instances.push_back(y_axis);
                    gizmo_instances.push_back(y_circle);
                    gizmo_instances.push_back(inner_y_circle);
                    gizmo_instances.push_back(y_letter);
                }
                else if (index == 2)
                {
                    gizmo_instances.push_back(z_axis);
                    gizmo_instances.push_back(z_circle);
                    gizmo_instances.push_back(inner_z_circle);
                    gizmo_instances.push_back(z_letter);
                }
                else if (index == 3)
                {
                    gizmo_instances.push_back(minus_x_circle);
                    gizmo_instances.push_back(inner_minus_x_circle);
                }
                else if (index == 4)
                {
                    gizmo_instances.push_back(minus_y_circle);
                    gizmo_instances.push_back(inner_minus_y_circle);
                }
                else if (index == 5)
                {
                    gizmo_instances.push_back(minus_z_circle);
                    gizmo_instances.push_back(inner_minus_z_circle);
                }
            }

            return gizmo_instances;
        }

        void OrientationGizmoRenderModule::CreateAndUploadInstanceBuffer(const std::vector<OrientationGizmoInstance>& gizmo_instances, VkCommandBuffer copy_cmd)
        {
            auto device = context_->device;

            struct
            {
                VkBuffer      buffer     = VK_NULL_HANDLE;
                VmaAllocation allocation = VK_NULL_HANDLE;
            } staging_buffer = {};

            instance_buffer_.size           = gizmo_instances.size() * sizeof(OrientationGizmoInstance);
            instance_buffer_.instance_count = static_cast<uint32_t>(gizmo_instances.size());
            if (instance_buffer_.size == 0)
            {
                return;
            }

            device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VMA_MEMORY_USAGE_CPU_ONLY,
                                 staging_buffer.buffer,
                                 staging_buffer.allocation,
                                 gizmo_instances.data(),
                                 instance_buffer_.size);

            instance_buffer_.buffer     = VK_NULL_HANDLE;
            instance_buffer_.allocation = VK_NULL_HANDLE;

            device->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 VMA_MEMORY_USAGE_GPU_ONLY,
                                 instance_buffer_.buffer,
                                 instance_buffer_.allocation,
                                 nullptr,
                                 instance_buffer_.size);

            SetObjectName(context_->device->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)staging_buffer.buffer, "orientationGizmoStagingBuffer");
            SetObjectName(context_->device->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)instance_buffer_.buffer, "orientationGizmoBuffer");

            // Creates only an execution dependency to prevent WRITE_AFTER_READ sync error.
            vkCmdPipelineBarrier(
                copy_cmd, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 0, nullptr);

            VkBufferCopy copy_region = {};
            copy_region.size         = instance_buffer_.size;
            vkCmdCopyBuffer(copy_cmd, staging_buffer.buffer, instance_buffer_.buffer, 1, &copy_region);

            if (instance_buffer_.buffer)
            {
                VkBufferMemoryBarrier buffer_barrier{};
                buffer_barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                buffer_barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
                buffer_barrier.dstAccessMask       = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
                buffer_barrier.srcQueueFamilyIndex = context_->device->GetGraphicsQueueFamilyIndex();
                buffer_barrier.dstQueueFamilyIndex = context_->device->GetGraphicsQueueFamilyIndex();
                buffer_barrier.buffer              = instance_buffer_.buffer;
                buffer_barrier.offset              = 0;
                buffer_barrier.size                = VK_WHOLE_SIZE;

                // Creates execution and memory dependency to prevent READ_AFTER_WRITE sync error.
                vkCmdPipelineBarrier(copy_cmd,
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

            instance_buffer_guard_.SetCurrentBuffer(instance_buffer_.buffer, instance_buffer_.allocation);
            instance_buffer_staging_guard_.SetCurrentBuffer(staging_buffer.buffer, staging_buffer.allocation);
        }

    }  // namespace renderer
}  // namespace rra
