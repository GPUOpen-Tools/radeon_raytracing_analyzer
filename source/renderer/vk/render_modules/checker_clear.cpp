//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the checker clear module.
//=============================================================================

#include "checker_clear.h"

namespace rra
{
    namespace renderer
    {
        CheckerClearRenderModule::CheckerClearRenderModule()
            : RenderModule(RenderPassHint::kRenderPassHintClearColorAndDepth)
        {
        }

        void CheckerClearRenderModule::Initialize(const RenderModuleContext* context)
        {
            VkPushConstantRange push_constant_range = {};
            push_constant_range.offset              = 0;
            push_constant_range.size                = sizeof(ClearColorPushConstants);
            push_constant_range.stageFlags          = VK_SHADER_STAGE_FRAGMENT_BIT;

            // Setup the layout create info.
            VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
            pipeline_layout_create_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.setLayoutCount             = 0;
            pipeline_layout_create_info.pushConstantRangeCount     = 1;
            pipeline_layout_create_info.pPushConstantRanges        = &push_constant_range;

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
            multisample_state.rasterizationSamples                 = context->swapchain->GetMSAASamples();
            multisample_state.flags                                = 0;

            std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

            VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
            dynamic_state_info.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamic_state_info.pDynamicStates                   = dynamic_states.data();
            dynamic_state_info.dynamicStateCount                = static_cast<uint32_t>(dynamic_states.size());
            dynamic_state_info.flags                            = 0;

            // Describe vertex inputs.
            VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
            vertex_input_state_info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_state_info.vertexBindingDescriptionCount        = 0;
            vertex_input_state_info.vertexBindingDescriptionCount        = 0;
            vertex_input_state_info.vertexAttributeDescriptionCount      = 0;

            // Load shaders.
            std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
            const char*                                    full_vertex_shader_path = "Clear.vs.spv";
            const char*                                    full_pixel_shader_path  = "Clear.ps.spv";
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
                vkCreateGraphicsPipelines(context->device->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &checker_clear_pipeline_);
            CheckResult(create_result, "Failed to create checker clear pipeline.");

            // Destroy shader modules.
            vkDestroyShaderModule(context->device->GetDevice(), shader_stages[0].module, nullptr);
            vkDestroyShaderModule(context->device->GetDevice(), shader_stages[1].module, nullptr);
        };

        void CheckerClearRenderModule::Draw(const RenderFrameContext* context)
        {
            context->begin_render_pass();

            // Bind pipeline and draw a triangle. The shader generates a triangle big enough to saturate the fragment shader.
            vkCmdBindPipeline(context->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, checker_clear_pipeline_);

            ClearColorPushConstants push_constants = {};
            push_constants.checker_color_1         = context->scene_info->background1_color;
            push_constants.checker_color_2         = context->scene_info->background2_color;

            vkCmdPushConstants(context->command_buffer, pipeline_layout_, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push_constants), &push_constants);

            vkCmdDraw(context->command_buffer, 3, 1, 0, 0);

            context->end_render_pass();
        };

        void CheckerClearRenderModule::Cleanup(const RenderModuleContext* context)
        {
            vkDestroyPipeline(context->device->GetDevice(), checker_clear_pipeline_, nullptr);
            vkDestroyPipelineLayout(context->device->GetDevice(), pipeline_layout_, nullptr);
        };

    }  // namespace renderer
}  // namespace rra
