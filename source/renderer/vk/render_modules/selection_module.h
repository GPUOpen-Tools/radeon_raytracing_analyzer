//=============================================================================
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the selection rendering module.
//=============================================================================

#ifndef RRA_RENDERER_VK_RENDER_MODULES_SELECTION_MODULE_H_
#define RRA_RENDERER_VK_RENDER_MODULES_SELECTION_MODULE_H_

#include "../render_module.h"
#include "glm/glm/glm.hpp"

#include "../bounding_volume_mesh.h"
#include "../vk_graphics_context.h"
#include "../buffer_guard.h"

namespace rra
{
    namespace renderer
    {
        /// @brief Render module to render the checkerboard background
        class SelectionRenderModule : public RenderModule
        {
        public:
            /// @brief Constructor.
            SelectionRenderModule();

            /// @brief Destructor.
            virtual ~SelectionRenderModule() = default;

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

            /// @brief Enables outline rendering.
            void EnableOutlineRendering();

            /// @brief Disables outline rendering.
            void DisableOutlineRendering();

            /// @brief Enables substrate rendering.
            void EnableTransformRendering();

            /// @brief Disables substrate rendering.
            void DisableTransformRendering();

        private:
            WireframeBoxMeshInfo wireframe_box_mesh_;  ///< The wireframe bounding box mesh.

            // Contains the instanced data.
            struct InstanceBuffer
            {
                VkBuffer               buffer     = VK_NULL_HANDLE;
                VmaAllocation          allocation = VK_NULL_HANDLE;
                size_t                 size       = 0;
                VkDescriptorBufferInfo descriptor;
                uint32_t               instance_count = 0;
            } instance_buffer_ = {};  ///< The instance buffer info.

            BufferGuard instance_buffer_guard_;          ///< The instance buffer guard.
            BufferGuard instance_staging_buffer_guard_;  ///< The instance staging buffer guard.

            VkDescriptorPool             descriptor_pool_       = VK_NULL_HANDLE;  ///< The descriptor pool used for the scene buffer.
            VkDescriptorSetLayout        descriptor_set_layout_ = VK_NULL_HANDLE;  ///< The descriptor set layout used for the scene buffer.
            std::vector<VkDescriptorSet> descriptor_sets_;                         ///< The descriptor sets used for the scene buffer.

            VkPipeline       pipeline_        = VK_NULL_HANDLE;  ///< The pipeline used for rendering the checker shaders.
            VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;  ///< The pipeline layout used for rendering the checker shaders.

            const RenderModuleContext* context_ = nullptr;  ///< RenderModuleContext to keep track of. It is utilized by the draw call.

            uint64_t last_scene_iteration_ =
                UINT64_MAX;  ///< Last rendered scene iteration to keep track of changes. Set to UINT64_MAX so that the first pass will be picked up.
            RendererSceneInfo* last_scene_ = nullptr;  ///< The last scene pointer.

            int render_state_      = 0xF;  ///< The render state to toggle various rendering options.
            int last_render_state_ = 0;    ///< The last render state.

            /// @brief Create and upload the instance buffer.
            ///
            /// @param [in] selected_volume_instances The selected volume instances.
            /// @param [in] context A context struct used to upload selected instance volumes.
            void CreateAndUploadInstanceBuffer(const std::vector<SelectedVolumeInstance>& selected_volume_instances, const RenderFrameContext* context);

            /// @brief Initialize the descriptor pool used for BVH rendering.
            void SetupDescriptorPool();

            /// @brief Initialize the BVH renderer descriptor set layout.
            void SetupDescriptorSetLayout();

            /// @brief Initialize the BVH renderer descriptor set configuration.
            void SetupDescriptorSet();
        };
    }  // namespace renderer
}  // namespace rra

#endif
