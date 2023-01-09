//=============================================================================
// Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the mesh render module.
//=============================================================================

#ifndef RRA_RENDERER_VK_RENDER_MODULES_BLAS_MESH_RENDER_MODULE_H_
#define RRA_RENDERER_VK_RENDER_MODULES_BLAS_MESH_RENDER_MODULE_H_

#include "public/renderer_types.h"
#include "../render_module.h"
#include "../util_vulkan.h"
#include "../buffer_guard.h"

#include <stdint.h>
#include <vector>
#include <unordered_map>

#include <volk/volk.h>

namespace rra
{
    namespace renderer
    {
        /// @brief A struct containing configurable render state settings.
        struct RenderState
        {
            bool            render_geometry;      ///< A flag to determine whether or not to draw the triangles of the geometry.
            bool            render_wireframe;     ///< A flag to determine whether or not to draw a wireframe overlay.
            bool            selection_only;       ///< A flag to determine whether or not to draw only the selected mesh.
            int             coloring_mode_index;  ///< The coloring mode index.
            VkCullModeFlags culling_mode;         ///< The culling mode configuration.
            bool            updated = true;       ///< A temporary flag to indicate if the render state has changed.
        };

        /// @brief The mesh renderer class is capable of rendering instances of arbitrary meshes.
        class MeshRenderModule : public RenderModule
        {
        public:
            /// @brief Constructor.
            MeshRenderModule();

            /// @brief Destructor.
            virtual ~MeshRenderModule() = default;

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

            /// @brief Retrieve a reference to the render state settings structure.
            ///
            /// @returns A reference to the render state settings structure.
            RenderState& GetRenderState();

            /// @brief Get the current geometry coloring mode.
            ///
            /// @returns The current coloring mode.
            GeometryColoringMode GetGeometryColoringMode() const;

            /// @brief Set the current geometry coloring mode.
            ///
            /// @param coloring_mode The coloring mode to set.
            void SetGeometryColoringMode(GeometryColoringMode coloring_mode);

        private:
            /// @brief Initialize the descriptor pool used for BVH rendering.
            void SetupDescriptorPool();

            /// @brief Initialize the BVH renderer descriptor set layout.
            void SetupDescriptorSetLayout();

            /// @brief Initialize the BVH renderer descriptor set configuration.
            void SetupDescriptorSet();

            /// @brief Initialize the renderer pipelines for each geometry coloring mode.
            void InitializePipelines();

            /// @brief Initialize a pipeline used to render BLAS geometry.
            ///
            /// @param [in] vertex_stage The vertex shader stage info.
            /// @param [in] fragment_stage The fragment shader stage info.
            /// @param [in] vertex_input_bindings The list of vertex input bindings.
            /// @param [in] vertex_attribute_descriptions The list of vertex input attribute descriptions.
            /// @param [in] cull_mode The triangle cull mode for this pipeline.
            /// @param [in] wireframe_only The pipeline to use for wireframe only rendering.
            ///
            /// @returns The new pipeline instance used to render geometry.
            VkPipeline InitializeMeshPipeline(const VkPipelineShaderStageCreateInfo&                vertex_stage,
                                              const VkPipelineShaderStageCreateInfo&                fragment_stage,
                                              const std::vector<VkVertexInputBindingDescription>&   vertex_input_bindings,
                                              const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions,
                                              VkCullModeFlags                                       cull_mode,
                                              bool                                                  wireframe_only) const;

            /// @brief Creates a pipeline for a single geometry coloring mode.
            /// @param vert_shader The path to a spirv vertex shader.
            /// @param frag_shader The path to a spirv fragment shader.
            /// @param attributes The vertex input attributes for the vertex shader.
            /// @param coloring_mode The geometry coloring mode that the shaders implement.
            void InitializeGeometryColorPipeline(const std::string&                                    vert_shader,
                                                 const std::string&                                    frag_shader,
                                                 const std::vector<VkVertexInputAttributeDescription>& attributes,
                                                 GeometryColoringMode                                  coloring_mode);

            /// @brief Creates a pipeline for wireframe display.
            /// @param vert_shader The path to a spirv vertex shader.
            /// @param frag_shader The path to a spirv fragment shader.
            /// @param attributes The vertex input attributes for the vertex shader.
            void InitializeWireframePipeline(const std::string&                                    vert_shader,
                                             const std::string&                                    frag_shader,
                                             const std::vector<VkVertexInputAttributeDescription>& attributes);

            /// @brief Upload the custom triangles from the scene.
            ///
            /// @param [in] command_buffer The command buffer to use while uploading data.
            void UploadCustomTriangles(VkCommandBuffer command_buffer);

            /// @brief Process the scene rendering resources.
            ///
            /// @param [in] command_buffer The command buffer to use while uploading data.
            /// @param [in] camera_position The camera position to use for fov-radius culling.
            ///
            /// @returns The near plane distance to feed back into the scene.
            float ProcessSceneData(VkCommandBuffer command_buffer, glm::vec3 camera_position);

            /// @brief Initialize the scene render state flags.
            void InitializeDefaultRenderState();

            /// @brief An instance buffer resource containing per-BLAS transform and metadata info.
            struct PerBlasInstanceBuffer
            {
                VkBuffer      buffer     = VK_NULL_HANDLE;  ///< A handle to the buffer object.
                VmaAllocation allocation = VK_NULL_HANDLE;  ///< A handle to the allocation.
                size_t        size       = 0;               ///< The total size of the buffer in bytes.
            } per_blas_instance_buffer_ = {};               ///< An instance buffer containing per-BLAS data.

            BufferGuard instance_guard;          ///< Buffer guard for instances.
            BufferGuard instance_staging_guard;  ///< Buffer guard for staging instances.

            /// @brief A buffer to contain custom triangle data.
            struct CustomTriangleBuffer
            {
                VkBuffer      buffer     = VK_NULL_HANDLE;  ///< A handle to the buffer object.
                VmaAllocation allocation = VK_NULL_HANDLE;  ///< A handle to the allocation.
                uint32_t      vertex_count;
            } custom_triangle_buffer = {};

            BufferGuard custom_triangles_guard;          ///< Buffer guard for custom triangles.
            BufferGuard custom_triangles_staging_guard;  ///< Buffer guard for custom traingles staging.

            struct RenderInstruction
            {
                VkBuffer vertex_buffer;
                uint32_t vertex_index;
                uint32_t vertex_count;
                uint32_t instance_index;
                uint32_t instance_count;
            };
            std::vector<RenderInstruction> render_instructions_;  ///< The instructions to render.

            struct TriangleCullPipelines
            {
                VkPipeline cull_none;
                VkPipeline cull_front;
                VkPipeline cull_back;
            };

            TriangleCullPipelines                                           geometry_wireframe_only_pipeline_;  ///< The wireframe only pipeline.
            std::unordered_map<GeometryColoringMode, TriangleCullPipelines> geometry_color_pipelines_;  ///< Pipelines used for each geometry coloring mode.
            GeometryColoringMode                                            coloring_mode_;             ///< The geometry color mode currently being drawn.

            std::vector<VkDescriptorSet> blas_mesh_descriptor_sets_;  ///< The descriptor sets used for rendering BVH geometry.

            VkDescriptorPool      descriptor_pool_       = VK_NULL_HANDLE;  ///< The descriptor pool used for rendering BVH nodes.
            VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;  ///< The descriptor set layout used when rendering BVH nodes.
            VkPipelineLayout      pipeline_layout_       = VK_NULL_HANDLE;  ///< The pipeline layout used for rendering BVH nodes.
            bool                  initialized_           = false;           ///< A flag indicating if BVH rendering resources have been initialized.

            RendererSceneInfo* current_scene_info_ = nullptr;  ///< A pointer to the scene to be rendered to keep track of changes.

            const RenderModuleContext* context_ = nullptr;  ///< The renderer context for the module.

            RenderState render_state_ = {};  ///< The render settings state.

            glm::mat4 last_view_projection_matrix_ = glm::mat4(1.0f);  ///< The last view projection combined matrix to check if culling should be re-triggered.
            uint64_t  last_scene_iteration_ =
                UINT64_MAX;  ///< Last rendered scene iteration to keep track of changes. Set to UINT64_MAX so that the first pass will be picked up.
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_RENDER_MODULES_BLAS_MESH_RENDER_MODULE_H_
