//=============================================================================
// Copyright (c) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation of the graphics context for the Vulkan API.
//=============================================================================

#include "vk_graphics_context.h"
#include "public/rra_error.h"
#include "public/rra_blas.h"
#include "framework/ext_debug_utils.h"

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

        VkTraversalTree VkGraphicsContext::GetBlasTraversalTree() const
        {
            return traversal_tree_;
        }

        uint32_t VkGraphicsContext::GetBlasTraversalTreeRootVolumeIndex(uint32_t blas_index) const
        {
            return traversal_tree_blas_offsets_[blas_index];
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

        bool VkGraphicsContext::Initialize(const GraphicsContextSceneInfo& info)
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
            }

            error_window_primed_ = initialized_;
            return initialized_;
        }

        void VkGraphicsContext::Cleanup()
        {
            if (initialized_)
            {
                device_.GPUFlush();
                device_.DestroyBuffer(geometry_buffer_, geometry_buffer_allocation_);

                device_.DestroyBuffer(traversal_tree_.vertex_buffer, traversal_tree_.vertex_allocation);
                device_.DestroyBuffer(traversal_tree_.volume_buffer, traversal_tree_.volume_allocation);

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

        bool VkGraphicsContext::CollectAndUploadTraversalTrees(const GraphicsContextSceneInfo& info)
        {
            PRE_RENDER_CHECK_HEALTH();

            traversal_tree_blas_offsets_ = info.traversal_tree_blas_structure_offsets;
            VkTraversalTree vk_tree;

            VkBuffer      volume_staging_buffer     = VK_NULL_HANDLE;
            VmaAllocation volume_staging_allocation = VK_NULL_HANDLE;

            VkBuffer      triangle_staging_buffer     = VK_NULL_HANDLE;
            VmaAllocation triangle_staging_allocation = VK_NULL_HANDLE;

            VkResult result;

            VkCommandPool   command_pool;
            VkCommandBuffer command_buffer;

            VkCommandPoolCreateInfo pool_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
            pool_info.queueFamilyIndex        = device_.GetGraphicsQueueFamilyIndex();

            result = vkCreateCommandPool(device_.GetDevice(), &pool_info, nullptr, &command_pool);
            PRE_RENDER_CHECK_RESULT(result, "Failed to create global upload command buffer pool.");

            VkCommandBufferAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
            alloc_info.commandPool                 = command_pool;
            alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            alloc_info.commandBufferCount          = 1;

            result = vkAllocateCommandBuffers(device_.GetDevice(), &alloc_info, &command_buffer);
            PRE_RENDER_CHECK_RESULT(result, "Failed to allocate global upload command buffer.");

            VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            result                              = vkBeginCommandBuffer(command_buffer, &begin_info);
            PRE_RENDER_CHECK_RESULT(result, "Failed to begin global upload command buffer recording.");

            {
                auto buffer_size = info.blas_tree.volumes.size() * sizeof(TraversalVolume);

                PRE_RENDER_CHECK_HEALTH();
                device_.CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                     VMA_MEMORY_USAGE_CPU_ONLY,
                                     volume_staging_buffer,
                                     volume_staging_allocation,
                                     info.blas_tree.volumes.data(),
                                     buffer_size);

                PRE_RENDER_CHECK_HEALTH();
                device_.CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                     VMA_MEMORY_USAGE_GPU_ONLY,
                                     vk_tree.volume_buffer,
                                     vk_tree.volume_allocation,
                                     nullptr,
                                     buffer_size);

                SetObjectName(device_.GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)volume_staging_buffer, "volumeStagingBufferContext");
                SetObjectName(device_.GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)vk_tree.volume_buffer, "volumeBufferContext");

                VkBufferCopy copy_region = {};
                copy_region.size         = buffer_size;
                vkCmdCopyBuffer(command_buffer, volume_staging_buffer, vk_tree.volume_buffer, 1, &copy_region);
            }

            {
                auto buffer_size = info.blas_tree.vertices.size() * sizeof(RraVertex);

                PRE_RENDER_CHECK_HEALTH();
                device_.CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                     VMA_MEMORY_USAGE_CPU_ONLY,
                                     triangle_staging_buffer,
                                     triangle_staging_allocation,
                                     info.blas_tree.vertices.data(),
                                     buffer_size);

                PRE_RENDER_CHECK_HEALTH();
                device_.CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                     VMA_MEMORY_USAGE_GPU_ONLY,
                                     vk_tree.vertex_buffer,
                                     vk_tree.vertex_allocation,
                                     nullptr,
                                     buffer_size);

                SetObjectName(device_.GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)triangle_staging_buffer, "triangleStagingBufferContext");
                SetObjectName(device_.GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)vk_tree.vertex_buffer, "triangleBufferContext");

                VkBufferCopy copy_region = {};
                copy_region.size         = buffer_size;
                vkCmdCopyBuffer(command_buffer, triangle_staging_buffer, vk_tree.vertex_buffer, 1, &copy_region);
            }

            result = vkEndCommandBuffer(command_buffer);
            PRE_RENDER_CHECK_RESULT(result, "Failed to end global upload command buffer recording.");

            VkSubmitInfo submit_info       = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers    = &command_buffer;

            result = vkQueueSubmit(device_.GetGraphicsQueue(), 1, &submit_info, VK_NULL_HANDLE);
            PRE_RENDER_CHECK_RESULT(result, "Failed to submit global buffer uploads to the graphics queue.");

            PRE_RENDER_CHECK_HEALTH();
            device_.GPUFlush();

            vkDestroyCommandPool(device_.GetDevice(), command_pool, nullptr);

            PRE_RENDER_CHECK_HEALTH();
            device_.DestroyBuffer(volume_staging_buffer, volume_staging_allocation);

            PRE_RENDER_CHECK_HEALTH();
            device_.DestroyBuffer(triangle_staging_buffer, triangle_staging_allocation);

            traversal_tree_ = vk_tree;

            for (size_t i = 0; i < info.traversal_tree_blas_triangle_offsets.size(); i++)
            {
                BlasDrawInstruction geometry_instruction = {};
                geometry_instruction.vertex_index        = info.traversal_tree_blas_triangle_offsets[i];
                geometry_instruction.vertex_count        = info.traversal_tree_blas_triangle_count[i];
                geometry_instruction.vertex_buffer       = vk_tree.vertex_buffer;
                geometry_instructions_.push_back(geometry_instruction);
            }

            return true;
        }

    }  // namespace renderer

}  // namespace rra
