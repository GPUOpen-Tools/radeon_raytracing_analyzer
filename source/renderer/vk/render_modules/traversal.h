//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for traversal module.
//=============================================================================

#ifndef RRA_RENDERER_VK_RENDER_MODULES_TRAVERSAL_H_
#define RRA_RENDERER_VK_RENDER_MODULES_TRAVERSAL_H_

#include "vk/buffer_guard.h"
#include "vk/image_guard.h"
#include "vk/render_module.h"

namespace rra
{
    namespace renderer
    {
        /// @brief Render module to render the traversal
        class TraversalRenderModule : public RenderModule
        {
        public:
            /// @brief Constructor.
            TraversalRenderModule();

            /// @brief Destructor.
            virtual ~TraversalRenderModule() = default;

            /// @brief Initialize render module.
            ///
            /// @param [in] context The context to initialize.
            virtual void Initialize(const RenderModuleContext* context) override;

            /// @brief Draw onto given command buffer.
            ///
            /// @param [in] context The frame context used to draw a new frame.
            virtual void Draw(const RenderFrameContext* context) override;

            /// @brief Cleanup render module.
            ///
            /// @param [in] context The context to cleanup.
            virtual void Cleanup(const RenderModuleContext* context) override;

            /// @brief Render module functionality invoked every frame, even when there is no rendering update when the camera is not moved.
            ///
            /// @param [in] device        The graphics device.
            /// @param [in] current_frame The current frame index.
            virtual void EveryFrameUpdate(Device* device, uint32_t current_frame) override;

            /// @brief Uploads the required storage buffers to the device.
            ///
            /// @param [in] context The draw context.
            void UploadTraversalData(const RenderFrameContext* context);

            /// @brief Initialize the descriptor set layout and pipeline layout.
            void SetupDescriptorSetLayoutAndPipelineLayout();

            /// @brief Initialize the descriptor pool.
            void SetupDescriptorPool();

            /// @brief Initialize the descriptor set.
            ///
            /// @param [in] frame_id The frame id to update for.
            void UpdateDescriptorSet(uint32_t frame_id);

            /// @brief Queue traversal counter min max updates.
            ///
            /// @param [in] update_function The update function to call on the next update.
            void QueueTraversalCounterRangeUpdate(std::function<void(uint32_t min, uint32_t max)> update_function);

            /// @brief Queue traversal counter min max updates.
            ///
            /// @param [in] update_function The update function to call on the next update.
            void SetTraversalCounterContinuousUpdateFunction(std::function<void(uint32_t min, uint32_t max)> update_function);

            /// @brief Queue histogram data updates.
            ///
            /// @param [in] update_function The update function to call on the next update.
            /// @param [in] max_traversal_setting The maximum traversal count set in the settings.
            void SetHistogramUpdateFunction(std::function<void(const std::vector<uint32_t>&, uint32_t, uint32_t)> update_function,
                                            uint32_t                                                              max_traversal_setting);

            /// @brief Checks if the traversal counter continuous update function is set.
            ///
            /// @returns True if the traversal counter continuous update function is set.
            bool IsTraversalCounterContinuousUpdateFunctionSet() const;

        private:
            const RenderModuleContext* context_ = nullptr;  ///< The renderer context for the module.

            std::vector<bool>            traversal_descriptor_set_update_flags_;             ///< Flags to check if the descriptor set should be updated.
            std::vector<VkDescriptorSet> traversal_descriptor_sets_;                         ///< The descriptor sets used for rendering traversal.
            VkDescriptorSetLayout        traversal_descriptor_set_layout_ = VK_NULL_HANDLE;  ///< The descriptor set layout used for rendering traversal.

            VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;  ///< The descriptor pool used for rendering traversal.

            VkPipeline       compute_pipeline_                = VK_NULL_HANDLE;  ///< The compute pipeline.
            VkPipeline       subsample_pipeline_              = VK_NULL_HANDLE;  ///< The subsample pipeline.
            VkPipeline       trace_traversal_pipeline_        = VK_NULL_HANDLE;  ///< The pipeline used for tracing traversal.
            VkPipelineLayout trace_traversal_pipeline_layout_ = VK_NULL_HANDLE;  ///< The pipeline layout used for tracing traversal.

            bool empty_scene_ = true;  ///< Flag to indicate that we have an empty scene.

            BufferGuard top_level_volumes_guard_;          ///< Buffer guard for volumes.
            BufferGuard top_level_volumes_staging_guard_;  ///< Buffer guard for staging volumes.

            BufferGuard top_level_vertices_guard_;          ///< Buffer guard for vertexes.
            BufferGuard top_level_vertices_staging_guard_;  ///< Buffer guard for staging vertexes.

            BufferGuard top_level_instances_guard_;          ///< Buffer guard for instances.
            BufferGuard top_level_instances_staging_guard_;  ///< Buffer guard for staging instances.

            uint64_t           last_scene_iteration_ = UINT64_MAX;  ///< Last rendered scene iteration to keep track of changes.
            RendererSceneInfo* last_scene_           = nullptr;     ///< The last scene pointer.

            struct Counter
            {
                VkBuffer      buffer     = VK_NULL_HANDLE;
                VmaAllocation allocation = VK_NULL_HANDLE;
            };

            std::vector<Counter> counter_gpu_buffers_;            ///< Buffer guard for the counters in gpu.
            std::vector<Counter> counter_cpu_buffers_;            ///< Buffer guard for the counters in cpu.
            uint32_t             counter_gpu_buffer_size_   = 0;  ///< Counter buffer size.
            uint32_t             counter_cpu_buffer_size_   = 0;  ///< Counter cpu side buffer size.
            uint32_t             histogram_gpu_buffer_size_ = 0;  ///< Histogram data buffer size.

            std::vector<Counter> histogram_gpu_buffers_;  ///< The frequency of each traversal count, for use with histogram.

            uint32_t last_offscreen_image_width_  = 0;  ///< The last offscreen image width.
            uint32_t last_offscreen_image_height_ = 0;  ///< The last offscreen image height.

            std::vector<std::function<void(uint32_t min, uint32_t max)>>
                traversal_counter_range_update_functions_;  ///< The set of temporary functions to call when a new update is available.

            std::function<void(uint32_t min, uint32_t max)> traversal_counter_range_continuous_update_function_ =
                nullptr;  ///< The update function to call for continuous updates.

            uint32_t max_traversal_count_setting_{};  ///< Maximum traversal count value from the settings.
            bool     traversal_count_setting_changed_{};

            bool rendered_this_frame_{};  ///< True when the traversal module draw function was called this frame.

            std::function<void(const std::vector<uint32_t>& data, uint32_t buffer_width, uint32_t buffer_height)> histogram_update_function_ =
                nullptr;  ///< The update function to call for histogram data.

            /// @brief Creates the counter buffers.
            ///
            /// @param [in] context the context to use to create the counter buffers.
            void CreateCounterBuffers(const RenderFrameContext* context);
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_VK_RENDER_MODULES_CHECKER_CLEAR_H_

