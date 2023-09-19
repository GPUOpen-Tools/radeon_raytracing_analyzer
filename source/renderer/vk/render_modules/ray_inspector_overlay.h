//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the ray inspector overlay render module.
//=============================================================================

#ifndef RRA_RENDERER_VK_RENDER_MODULES_RAY_INSPECTOR_OVERLAY_H_
#define RRA_RENDERER_VK_RENDER_MODULES_RAY_INSPECTOR_OVERLAY_H_

#include "../render_module.h"
#include "glm/glm/glm.hpp"
#include "../buffer_guard.h"

namespace rra::renderer
{
    struct IconDescription
    {
        glm::vec2 position     = {};
        uint32_t  type         = 0;  // 0 Circle, 1 Diamond, 2 Plus
        float     aspect_ratio = 1.0f;
        glm::vec4 color        = {};
        glm::vec2 size         = {0.0f, 0.0f};
        float     angle        = 0.0f;
        float     placeholder  = {};
    };

    class RayInspectorOverlayRenderModule : public RenderModule
    {
    public:
        RayInspectorOverlayRenderModule();

        /// @brief Initialize render module.
        ///
        /// @param [in] context The context to initialize.
        virtual void Initialize(const RenderModuleContext* context) override;

        /// @brief Draw onto given command buffer.
        ///
        /// @param [in] context A context struct used to render a new frame.
        virtual void Draw(const RenderFrameContext* context) override;

        /// @brief Cleanup render module.
        ///
        /// @param [in] context The context to cleanup.
        virtual void Cleanup(const RenderModuleContext* context) override;

        /// @brief Set rays.
        ///
        /// @param [in] rays The list of rays to set.
        void SetRays(std::vector<RayInspectorRay> rays);

    private:
        /// RAY FUNCTIONS

        /// @brief Updates the ray buffer.
        ///
        /// @param [in] scene_info The scene info to use to modify rays.
        void UpdateRayBuffer(RendererSceneInfo* scene_info);

        /// @brief Creates the pipeline and pipeline layout for the ray lines.
        void CreateRayLinesPipelineAndLayout();

        /// @brief Update the line pipeline descriptor set.
        void UpdateRayLinesDescriptorSet();

        /// @brief Render the line pipeline.
        void RenderRayLinesPipeline();

        /// @brief Initialize the descriptor set layout.
        void SetupRayLinesDescriptorSetLayout();

        /// @brief Initialize the descriptor pool.
        void SetupRayLinesDescriptorPool();

        /// ICON FUNCTIONS

        /// @brief Updates the icon buffer.
        void UpdateIconBuffer();

        /// @brief Creates the pipeline and pipeline layout for the ray lines.
        void CreateIconPipelineAndLayout();

        /// @brief Update the line pipeline descriptor set.
        void UpdateIconDescriptorSet();

        /// @brief Render the icon pipeline.
        void RenderIconPipeline();

        /// @brief Initialize the descriptor set layout.
        void SetupIconDescriptorSetLayout();

        /// @brief Initialize the descriptor pool.
        void SetupIconDescriptorPool();

        struct RayInspectorOverlayMemory
        {
            VkBuffer      buffer     = VK_NULL_HANDLE;
            VmaAllocation allocation = VK_NULL_HANDLE;
        };

        const RenderModuleContext* module_context_ = nullptr;  ///< Cached module context.
        const RenderFrameContext*  frame_context_  = nullptr;  ///< Cached frame context.

        bool                         update_ray_buffer_ = false;  ///< A flag to check if the ray buffer should be updated.
        std::vector<RayInspectorRay> rays_;                       ///< Rays to render.
        std::vector<IconDescription> icons_;                      ///< Icons to render.
        uint32_t                     number_of_rays_     = 0;     ///< The number of rays.
        uint32_t                     number_of_icons_    = 0;     ///< The number of icons.
        uint32_t                     rays_in_other_tlas_ = 0;     ///< The number of rays not in the selected ray's TLAS.

        RayInspectorOverlayMemory current_ray_memory_         = {};  ///< The current ray memory.
        RayInspectorOverlayMemory current_ray_staging_memory_ = {};  ///< The current ray staging memory.
        BufferGuard               ray_buffer_guard_;                 ///< The ray memory buffer guard.
        BufferGuard               ray_staging_buffer_guard_;         ///< The ray memory staging buffer guard.

        RayInspectorOverlayMemory current_icon_memory_         = {};  ///< The current ray memory.
        RayInspectorOverlayMemory current_icon_staging_memory_ = {};  ///< The current ray staging memory.
        BufferGuard               icon_buffer_guard_;                 ///< The ray memory buffer guard.
        BufferGuard               icon_staging_buffer_guard_;         ///< The ray memory staging buffer guard.

        VkPipeline       ray_lines_pipeline_        = VK_NULL_HANDLE;  ///< The pipeline used for rendering the rays.
        VkPipelineLayout ray_lines_pipeline_layout_ = VK_NULL_HANDLE;  ///< The pipeline layout used for rendering rays.

        VkPipeline         icon_pipeline_           = VK_NULL_HANDLE;  ///< The pipeline used for rendering the origin of rays.
        VkPipelineLayout   icon_pipeline_layout_    = VK_NULL_HANDLE;  ///< The pipeline layout used for rendering the origin of rays.
        const VkImageView* depth_input_buffer_view_ = VK_NULL_HANDLE;  ///< The image view of the depth input attachment.

        VkDescriptorPool             ray_lines_descriptor_pool_       = VK_NULL_HANDLE;  ///< The descriptor pool used for buffers.
        VkDescriptorSetLayout        ray_lines_descriptor_set_layout_ = VK_NULL_HANDLE;  ///< The descriptor set layout used for buffers.
        std::vector<VkDescriptorSet> ray_lines_descriptor_sets_;                         ///< The descriptor sets used for the buffers.

        VkDescriptorPool             icon_descriptor_pool_       = VK_NULL_HANDLE;  ///< The descriptor pool used for buffers.
        VkDescriptorSetLayout        icon_descriptor_set_layout_ = VK_NULL_HANDLE;  ///< The descriptor set layout used for buffers.
        std::vector<VkDescriptorSet> icon_descriptor_sets_;                         ///< The descriptor sets used for the buffers.
    };
}  // namespace rra::renderer
#endif
