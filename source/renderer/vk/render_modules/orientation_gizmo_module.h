//=============================================================================
// Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the orientation gizmo render module.
//=============================================================================

#ifndef RRA_RENDERER_VK_RENDER_MODULES_ORIENTATION_GIZMO_MODULE_H_
#define RRA_RENDERER_VK_RENDER_MODULES_ORIENTATION_GIZMO_MODULE_H_

#include "public/renderer_types.h"
#include "public/orientation_gizmo.h"
#include "../render_module.h"
#include "../util_vulkan.h"
#include "../buffer_guard.h"
#include "../orientation_gizmo_mesh.h"

#include <stdint.h>
#include <vector>

#include <volk/volk.h>

namespace rra
{
    namespace renderer
    {
        /// @brief The mesh renderer class is capable of rendering instances of arbitrary meshes.
        class OrientationGizmoRenderModule : public RenderModule
        {
        public:
            /// @brief Constructor.
            OrientationGizmoRenderModule();

            /// @brief Destructor.
            virtual ~OrientationGizmoRenderModule() = default;

            /// @brief Initialize render module.
            ///
            /// @param [in] context The context to initialize.
            virtual void Initialize(const RenderModuleContext* context) override;

            /// @brief Draw onto given command buffer
            ///
            /// @param [in] context The context to render on.
            virtual void Draw(const RenderFrameContext* context) override;

            /// @brief Cleanup render module.
            ///
            /// @param [in] context The context to cleanup.
            virtual void Cleanup(const RenderModuleContext* context) override;

            static OrientationGizmoHitType selected;  ///< The axis the mouse is currently hovering over.

        private:
            /// @brief Initializes instance data for each instance used to draw the orientation gizmo.
            ///
            /// @returns Vector of gizmo instances.
            std::vector<OrientationGizmoInstance> InitializeInstances(glm::mat4 rotation, float window_ratio);

            /// @brief Creates and populates the Vulkan buffer containing instance data.
            ///
            /// @param gizmo_instances List of all the instances we want to draw.
            /// @param copy_cmd Command buffer to be used for copying.
            void CreateAndUploadInstanceBuffer(const std::vector<OrientationGizmoInstance>& gizmo_instances, VkCommandBuffer copy_cmd);

            OrientationGizmoMeshInfo   orientation_gizmo_mesh_;                       ///< The orientation gizmo mesh.
            VkPipeline                 orientation_gizmo_pipeline_ = VK_NULL_HANDLE;  ///< The pipeline used for rendering the orientation gizmo.
            VkPipelineLayout           pipeline_layout_            = VK_NULL_HANDLE;  ///< The pipeline layout used for rendering the orientation gizmo.
            const RenderModuleContext* context_                    = nullptr;  ///< RenderModuleContext to keep track of. It is utilized by the draw call.

            // Contains the instanced data.
            struct InstanceBuffer
            {
                VkBuffer               buffer     = VK_NULL_HANDLE;
                VmaAllocation          allocation = VK_NULL_HANDLE;
                size_t                 size       = 0;
                VkDescriptorBufferInfo descriptor;
                uint32_t               instance_count = 0;
            } instance_buffer_ = {};                     ///< The instance buffer info.
            BufferGuard instance_buffer_guard_;          ///< The instance buffer guard.
            BufferGuard instance_buffer_staging_guard_;  ///< The instance buffer staging guard.
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_RENDER_MODULES_TRANSFORM_GIZMO_MODULE_H_
