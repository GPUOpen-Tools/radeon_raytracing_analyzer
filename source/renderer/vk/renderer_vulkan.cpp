//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for the Vulkan renderer.
//=============================================================================

#include "renderer_vulkan.h"
#include "util_vulkan.h"
#include "string.h"
#include "float.h"
#include "vk_graphics_context.h"

#include <cstring>

#include "qt_common/utils/qt_util.h"

#define FRUSTUM_CULLING_ENABLE 1  ///< A flag to determine if frustum culling is used or not.
#define PROFILE_READY 0           ///< A flag to force the flags below to the optimal settings for profiling.

#define FORCE_UPDATES 0                  ///< A flag to force renderer updates for profiling or other tests.
#define FORCE_FRUSTUM_CULLING_UPDATES 0  ///< A flag to force frustum culling even if camera hasn't moved.
#define VSYNC_ENABLE 1                   ///< A flag to determine if vsync is used or not.

#if PROFILE_READY == 1
#undef FORCE_UPDATES
#undef FORCE_FRUSTUM_CULLING_UPDATES
#undef VSYNC_ENABLE
#define FORCE_UPDATES 1
#define FORCE_FRUSTUM_CULLING_UPDATES 1
#define VSYNC_ENABLE 0
#endif

static int const      kBackBufferCount           = 3;  ///< The number of image buffers in the swapchain.
static const uint32_t kCommandListsPerBackBuffer = 8;  ///< The number of command buffers for each frame.

namespace rra
{
    namespace renderer
    {
        RendererVulkan::RendererVulkan(std::vector<RenderModule*> render_modules)
            : device_(GetVkGraphicsContext()->GetDevice())
            , current_frame_index_(0)
            , initialized_(false)
        {
            render_modules_                                            = render_modules;
            scene_uniform_buffer_.traversal_counter_use_custom_min_max = 1;
        }

        RendererVulkan ::~RendererVulkan()
        {
            for (auto render_module : render_modules_)
            {
                delete render_module;
            }
        }

        bool RendererVulkan::InitializeDevice()
        {
            if (GetVkGraphicsContext()->IsInitialized())
            {
                swapchain_.OnCreate(&device_, kBackBufferCount, window_info_);

                command_buffer_ring_.OnCreate(&device_, kBackBufferCount, kCommandListsPerBackBuffer);

                InitializeSceneUniformBuffer();

                render_module_context_.device              = &device_;
                render_module_context_.swapchain           = &swapchain_;
                render_module_context_.command_buffer_ring = &command_buffer_ring_;
                render_module_context_.framebuffer_height  = height_;
                render_module_context_.framebuffer_width   = width_;

                command_buffers_per_frame_.resize(kBackBufferCount);

                for (auto render_module : render_modules_)
                {
                    render_module->SetRendererInterface(this);
                    render_module->Initialize(&render_module_context_);
                }

                states_.resize(swapchain_.GetBackBufferCount());

                // Graphics context needs access to scene to build traversal tree.
                GetVkGraphicsContext()->SetSceneInfo(&scene_info_);

                // Mark initialization as complete.
                initialized_ = true;
            }

            return initialized_;
        }

        void RendererVulkan::MoveToNextFrame()
        {
            swapchain_.WaitForSwapChain();
        }

        void RendererVulkan::WaitForGpu()
        {
            device_.GPUFlush();
        }

        void RendererVulkan::Shutdown()
        {
            // Only attempt to shut the renderer down if it was initialized.
            if (initialized_)
            {
                WaitForGpu();

                for (auto render_module : render_modules_)
                {
                    render_module->Cleanup(&render_module_context_);
                }

                for (auto& ubo : scene_ubos_)
                {
                    device_.DestroyBuffer(ubo.buffer, ubo.allocation);
                }

                CleanupHeatmap();

                command_buffer_ring_.OnDestroy();
                swapchain_.OnDestroy();
            }
        }

        void RendererVulkan::BeginScene(VkCommandBuffer command_buffer)
        {
            VkCommandBufferBeginInfo cmd_buf_info = {};
            cmd_buf_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmd_buf_info.pNext                    = nullptr;
            cmd_buf_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            cmd_buf_info.pInheritanceInfo         = nullptr;

            VkResult begin_result = vkBeginCommandBuffer(command_buffer, &cmd_buf_info);
            CheckResult(begin_result, "Failed to begin command buffer.");
        }

        void RendererVulkan::BeginRenderPass(VkCommandBuffer command_buffer, RenderPassHint render_pass_hint)
        {
            // Clear the scene to a known color and depth value.
            VkClearValue clear_values[3] = {};
            clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
            clear_values[1].depthStencil = {1.0f, 0};
            clear_values[2].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};

            // Start the render pass.
            VkRenderPassBeginInfo render_pass_begin_info    = {};
            render_pass_begin_info.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_begin_info.pNext                    = nullptr;
            render_pass_begin_info.renderPass               = swapchain_.GetRenderPass(render_pass_hint);
            uint32_t current_image_index                    = swapchain_.GetImageIndex();
            render_pass_begin_info.framebuffer              = swapchain_.GetFramebuffer(render_pass_hint, current_image_index);
            render_pass_begin_info.renderArea.offset.x      = 0;
            render_pass_begin_info.renderArea.offset.y      = 0;
            render_pass_begin_info.renderArea.extent.width  = width_;
            render_pass_begin_info.renderArea.extent.height = height_;
            render_pass_begin_info.clearValueCount          = 3;
            render_pass_begin_info.pClearValues             = clear_values;
            vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

            // Set the scissor and viewport configuration.
            vkCmdSetScissor(command_buffer, 0, 1, &scissor_);
            vkCmdSetViewport(command_buffer, 0, 1, &viewport_);
        }

        void RendererVulkan::BuildScene()
        {
            // Start with a new set of command buffers for the frame.
            command_buffer_ring_.OnBeginFrame();

            uint32_t current_frame_index = swapchain_.GetImageIndex();
            command_buffers_per_frame_[current_frame_index].clear();

            UpdateSceneUniformBuffer(current_frame_index);
            UpdateHeatmap();

            RendererVulkanStateTracker current_state;
            current_state.renderer_iteration_  = renderer_iteration_;
            current_state.scene_iteration      = scene_info_.scene_iteration;
            current_state.scene_uniform_buffer = scene_uniform_buffer_;

            // Start with a new set of command buffers for the frame.
            auto cmd = command_buffer_ring_.GetNewCommandBuffer();

            BeginScene(cmd);

            VkDescriptorBufferInfo scene_buffer_info = {};
            scene_buffer_info.buffer                 = scene_ubos_[current_frame_index].buffer;
            scene_buffer_info.offset                 = 0;
            scene_buffer_info.range                  = sizeof(SceneUBO);

            // Only record new commands if there are changes.
            if (!states_[current_frame_index].Equal(current_state) || FORCE_UPDATES)
            {
                RenderFrameContext context = {&device_,
                                              current_frame_index,
                                              cmd,
                                              scene_buffer_info,
                                              vulkan_heatmap_.image_info,
                                              &scene_info_,
                                              scene_uniform_buffer_.view_projection,
                                              camera_.GetPosition(),
                                              camera_.GetFieldOfView(),
                                              camera_,
                                              static_cast<uint32_t>(width_),
                                              static_cast<uint32_t>(height_),
                                              nullptr,
                                              nullptr};

                for (auto render_module : render_modules_)
                {
                    if (render_module->IsEnabled())
                    {
                        context.begin_render_pass = [=, &render_module]() { BeginRenderPass(cmd, render_module->GetRenderPassHint()); };
                        context.end_render_pass   = [=]() { EndRenderPass(cmd); };
                        render_module->Draw(&context);

                        if (render_module->ShouldCopyDepthBuffer())
                        {
                            CopyDepthBuffer(cmd);
                        }
                    }
                };

                states_[current_frame_index] = current_state;
            }

            // Update every frame even if force updates is false.
            for (auto render_module : render_modules_)
            {
                if (render_module->IsEnabled())
                {
                    render_module->EveryFrameUpdate(&device_, current_frame_index);
                }
            };

            BeginRenderPass(cmd, RenderPassHint::kRenderPassHintResolve);
            EndRenderPass(cmd);

            EndScene(cmd);

            // Add to the command buffers for this frame.
            command_buffers_per_frame_[current_frame_index].push_back(cmd);
        }

        void RendererVulkan::DrawFrame()
        {
            camera_.ProcessInputs();

            // This is where the data is passed from the scene to the renderer each frame.
            // Must be called after camera processes inputs to have up to date frustum culling.
            if (update_scene_info_ != nullptr)
            {
                // Frustum culling checks only for instance nodes, so it's wasted computation in the BLAS pane. So disable it for the BLAS pane.
                bool frustum_culling{!scene_info_.custom_triangles || scene_info_.custom_triangles->empty() ? (bool)FRUSTUM_CULLING_ENABLE : false};
                update_scene_info_(scene_info_, &camera_, frustum_culling, (bool)FORCE_FRUSTUM_CULLING_UPDATES);
            }

            HandleSceneChanged();

            BuildScene();

            PresentScene();
        }

        void RendererVulkan::MarkAsDirty()
        {
            renderer_iteration_++;
        }

        Device& RendererVulkan::GetDevice()
        {
            return device_;
        }

        CommandBufferRing& RendererVulkan::GetCommandBufferRing()
        {
            return command_buffer_ring_;
        }

        SceneUniformBuffer& RendererVulkan::GetSceneUbo()
        {
            return scene_uniform_buffer_;
        }

        void RendererVulkan::HandleSceneChanged()
        {
            // Fill the scene uniform buffer with the new scene's info.
            scene_uniform_buffer_.max_instance_count = scene_info_.max_instance_count;
            scene_uniform_buffer_.max_triangle_count = scene_info_.max_triangle_count;
            scene_uniform_buffer_.max_tree_depth     = scene_info_.max_tree_depth;
            scene_uniform_buffer_.max_node_depth     = scene_info_.max_node_depth;
        }

        void RendererVulkan::EndRenderPass(VkCommandBuffer command_buffer)
        {
            vkCmdEndRenderPass(command_buffer);
        }

        void RendererVulkan::EndScene(VkCommandBuffer command_buffer)
        {
            VkResult res = vkEndCommandBuffer(command_buffer);
            CheckResult(res, "Failed to end command buffer.");
        }

        void RendererVulkan::PresentScene()
        {
            uint32_t current_frame_index = swapchain_.GetImageIndex();

            VkSemaphore image_available_semaphore;
            VkSemaphore render_finished_semaphores;
            VkFence     cmd_buf_executed_fences;
            swapchain_.GetSemaphores(&image_available_semaphore, &render_finished_semaphores, &cmd_buf_executed_fences);

            VkPipelineStageFlags submit_wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo         submit_info       = {};
            submit_info.sType                      = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.pNext                      = nullptr;
            submit_info.waitSemaphoreCount         = 1;
            submit_info.pWaitSemaphores            = &image_available_semaphore;
            submit_info.pWaitDstStageMask          = &submit_wait_stage;
            submit_info.commandBufferCount         = static_cast<uint32_t>(command_buffers_per_frame_[current_frame_index].size());
            submit_info.pCommandBuffers            = command_buffers_per_frame_[current_frame_index].data();
            submit_info.signalSemaphoreCount       = 1;
            submit_info.pSignalSemaphores          = &render_finished_semaphores;

            VkResult res = vkQueueSubmit(device_.GetGraphicsQueue(), 1, &submit_info, cmd_buf_executed_fences);
            CheckResult(res, "Command buffer submission was not VK_SUCCESS.");

            swapchain_.Present();
        }

        void RendererVulkan::InitializeSceneUniformBuffer()
        {
            scene_ubos_.resize(swapchain_.GetBackBufferCount());

            for (auto& ubo : scene_ubos_)
            {
                device_.CreateBuffer(
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, ubo.buffer, ubo.allocation, nullptr, sizeof(scene_uniform_buffer_));
            }
        }

        void RendererVulkan::UpdateSceneUniformBuffer(uint32_t frame_to_update)
        {
            if (camera_.updated_)
            {
                camera_.updated_ = false;

                scene_uniform_buffer_.view_projection = camera_.GetViewProjection();

                // Used by the traversal shader.
                scene_uniform_buffer_.camera_rotation = glm::inverse(glm::mat3(camera_.GetRotationMatrix()));
                scene_uniform_buffer_.inverse_camera_projection =
                    glm::transpose(glm::inverse(glm::perspective(glm::radians(camera_.GetFieldOfView()), camera_.GetAspectRatio(), 0.1f, 1.0f)));
                scene_uniform_buffer_.camera_position = glm::vec4(camera_.GetPosition(), 1.0f);
                scene_uniform_buffer_.ortho_scale     = camera_.Orthographic() ? camera_.GetOrthoScale() : -1.0f;

                // Snap the light to the camera position.
                scene_uniform_buffer_.light_position = glm::vec4(camera_.GetPosition(), 1.0f);
            }

            scene_uniform_buffer_.depth_range_lower_bound = scene_info_.depth_range_lower_bound;
            scene_uniform_buffer_.depth_range_upper_bound = scene_info_.depth_range_upper_bound;
            scene_uniform_buffer_.max_node_depth          = scene_info_.max_node_depth;
            scene_uniform_buffer_.box16_node_color        = scene_info_.box16_node_color;
            scene_uniform_buffer_.box32_node_color        = scene_info_.box32_node_color;
            scene_uniform_buffer_.instance_node_color     = scene_info_.instance_node_color;
            scene_uniform_buffer_.procedural_node_color   = scene_info_.procedural_node_color;
            scene_uniform_buffer_.triangle_node_color     = scene_info_.triangle_node_color;
            scene_uniform_buffer_.selected_node_color     = scene_info_.selected_node_color;
            scene_uniform_buffer_.selected_geometry_color = scene_info_.selected_geometry_color;
            scene_uniform_buffer_.selected_ray_color      = scene_info_.selected_ray_color;
            scene_uniform_buffer_.ray_color               = scene_info_.ray_color;
            scene_uniform_buffer_.shadow_ray_color        = scene_info_.shadow_ray_color;

            scene_uniform_buffer_.transparent_color = scene_info_.transparent_color;
            scene_uniform_buffer_.opaque_color      = scene_info_.opaque_color;

            scene_uniform_buffer_.positive_color = scene_info_.positive_color;
            scene_uniform_buffer_.negative_color = scene_info_.negative_color;

            scene_uniform_buffer_.build_algorithm_none_color       = scene_info_.build_algorithm_none_color;
            scene_uniform_buffer_.build_algorithm_fast_build_color = scene_info_.build_algorithm_fast_build_color;
            scene_uniform_buffer_.build_algorithm_fast_trace_color = scene_info_.build_algorithm_fast_trace_color;
            scene_uniform_buffer_.build_algorithm_both_color       = scene_info_.build_algorithm_both_color;

            scene_uniform_buffer_.instance_opaque_none_color            = scene_info_.instance_opaque_none_color;
            scene_uniform_buffer_.instance_opaque_force_opaque_color    = scene_info_.instance_opaque_force_opaque_color;
            scene_uniform_buffer_.instance_opaque_force_no_opaque_color = scene_info_.instance_opaque_force_no_opaque_color;
            scene_uniform_buffer_.instance_opaque_force_both_color      = scene_info_.instance_opaque_force_both_color;

            scene_uniform_buffer_.wireframe_normal_color   = scene_info_.wireframe_normal_color;
            scene_uniform_buffer_.wireframe_selected_color = scene_info_.wireframe_selected_color;

            scene_uniform_buffer_.screen_width  = width_;
            scene_uniform_buffer_.screen_height = height_;

            scene_uniform_buffer_.count_as_fused_instances = scene_info_.fused_instances_enabled ? 1 : 0;

            // Used by traversal shader for heatmap color brightness.
            scene_uniform_buffer_.color_theme = QtCommon::QtUtils::ColorTheme::Get().GetColorTheme();

            device_.WriteToBuffer(scene_ubos_[frame_to_update].allocation, &scene_uniform_buffer_, sizeof(scene_uniform_buffer_));
        }

        void RendererVulkan::UpdateHeatmap()
        {
            if (!should_update_heatmap_ || heatmap_ == nullptr)
            {
                return;
            }

            WaitForGpu();
            CleanupHeatmap();

            vulkan_heatmap_        = GetVkGraphicsContext()->CreateVulkanHeatmapResources(command_buffer_ring_.GetUploadCommandBuffer(), heatmap_, false);
            should_update_heatmap_ = false;
        }

        void RendererVulkan::CleanupHeatmap()
        {
            vkDestroySampler(device_.GetDevice(), vulkan_heatmap_.sampler, nullptr);

            vkDestroyImageView(device_.GetDevice(), vulkan_heatmap_.image_view, nullptr);

            device_.DestroyImage(vulkan_heatmap_.image, vulkan_heatmap_.allocation);

            vulkan_heatmap_ = {};
        }

        void RendererVulkan::CopyDepthBuffer(VkCommandBuffer cmd)
        {
            // Wait for depth buffer to finish being written to and transition to transfer src.
            VkImageSubresourceRange subresource_range{};
            subresource_range.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            subresource_range.baseMipLevel   = 0;
            subresource_range.levelCount     = 1;
            subresource_range.baseArrayLayer = 0;
            subresource_range.layerCount     = 1;

            VkImageMemoryBarrier image_barrier{};
            image_barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            image_barrier.srcAccessMask       = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            image_barrier.dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT;
            image_barrier.oldLayout           = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            image_barrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_barrier.image               = swapchain_.GetDepthStencilImage();
            image_barrier.subresourceRange    = subresource_range;

            // Transition destination image for transfer.
            VkImageMemoryBarrier image_barrier_dst{};
            image_barrier_dst.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            image_barrier_dst.srcAccessMask       = VK_ACCESS_NONE;
            image_barrier_dst.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
            image_barrier_dst.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
            image_barrier_dst.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            image_barrier_dst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_barrier_dst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_barrier_dst.image               = swapchain_.GetDepthInputImage();
            image_barrier_dst.subresourceRange    = subresource_range;

            std::array<VkImageMemoryBarrier, 2> image_barriers = {image_barrier, image_barrier_dst};

            vkCmdPipelineBarrier(cmd,
                                 VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_DEPENDENCY_BY_REGION_BIT,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 (uint32_t)image_barriers.size(),
                                 image_barriers.data());

            // Copy image.
            VkImageSubresourceLayers subresource{};
            subresource.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            subresource.mipLevel       = 0;
            subresource.baseArrayLayer = 0;
            subresource.layerCount     = 1;

            VkExtent2D swap_extent{swapchain_.GetSwapchainExtent()};

            VkImageCopy image_copy{};
            image_copy.srcSubresource = subresource;
            image_copy.srcOffset      = {0, 0, 0};
            image_copy.dstSubresource = subresource;
            image_copy.dstOffset      = {0, 0, 0};
            image_copy.extent         = {swap_extent.width, swap_extent.height, 1};

            vkCmdCopyImage(cmd,
                           swapchain_.GetDepthStencilImage(),
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           swapchain_.GetDepthInputImage(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &image_copy);

            // Transition images back to original layouts.
            image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            image_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            image_barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            image_barrier.newLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            image_barrier_dst.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            image_barrier_dst.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            image_barrier_dst.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            image_barrier_dst.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            image_barriers = {image_barrier, image_barrier_dst};

            vkCmdPipelineBarrier(cmd,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 VK_DEPENDENCY_BY_REGION_BIT,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 (uint32_t)image_barriers.size(),
                                 image_barriers.data());
        }

        void RendererVulkan::WaitForSwapchain()
        {
            current_frame_index_ = swapchain_.WaitForSwapChain();
        }

        void RendererVulkan::HandleDimensionsUpdated()
        {
            // Set the viewport dimensions.
            viewport_.x        = 0;
            viewport_.y        = static_cast<float>(height_);
            viewport_.width    = static_cast<float>(width_);
            viewport_.height   = -static_cast<float>(height_);
            viewport_.minDepth = 0.0f;
            viewport_.maxDepth = 1.0f;

            // Initialize the scissor rectangle to the full extent of the frame buffer.
            scissor_.extent.width  = width_;
            scissor_.extent.height = height_;
            scissor_.offset.x      = 0;
            scissor_.offset.y      = 0;

            swapchain_.OnDestroyWindowSizeDependentResources();

            swapchain_.OnCreateWindowSizeDependentResources(width_, height_, (bool)VSYNC_ENABLE);

            float aspect_ratio = static_cast<float>(width_) / static_cast<float>(height_);
            camera_.UpdateAspectRatio(aspect_ratio);
        }

        bool RendererVulkanStateTracker::Equal(const RendererVulkanStateTracker& other) const
        {
            return renderer_iteration_ == other.renderer_iteration_ && scene_iteration == other.scene_iteration &&
                   EqualsSceneUniformBuffer(scene_uniform_buffer, other.scene_uniform_buffer);
        }

        bool EqualsSceneUniformBuffer(const SceneUniformBuffer& a, const SceneUniformBuffer& b)
        {
            return std::memcmp(&a, &b, sizeof(SceneUniformBuffer)) == 0;
        }
    }  // namespace renderer
}  // namespace rra
