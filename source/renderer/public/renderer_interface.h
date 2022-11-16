//=============================================================================
// Copyright (c) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the Renderer Interface. This component is
///         responsible for using the underlying rendering API to draw a scene
///         which will ultimately be presented to the user in a RendererWidget.
//=============================================================================

#ifndef RRA_RENDERER_RENDERER_INTERFACE_H_
#define RRA_RENDERER_RENDERER_INTERFACE_H_

#include <functional>
#include "public/heatmap.h"
#include "public/renderer_types.h"
#include "camera.h"
#include "renderer_adapter.h"

namespace rra
{
    namespace renderer
    {
        struct WindowInfo;

        /// @brief Info about the scene that is updated every frame.
        ///
        /// This is the link between the scene and the renderer. Any scene data that the renderer
        /// needs access to will be passed through this structure and GraphicsContextSceneInfo.
        /// Some data is passed through render_state_adapter.h too... Might should make that part of this struct.
        struct RendererSceneInfo
        {
            uint64_t scene_iteration         = 0;  ///< The current scene iteration. Can be check to see if scene has changed.
            uint64_t last_iteration          = 0;  ///< The scene iteration from the last frame.
            uint32_t depth_range_lower_bound = 0;  ///< The lower bound of the depth range.
            uint32_t depth_range_upper_bound = 0;  ///< The upper bound of the depth range.

            // SceneStatistics.
            uint32_t max_instance_count;  ///< The maximum instance count across all scene instances.
            uint32_t max_triangle_count;  ///< The highest triangle count out of each mesh in the scene.
            int32_t  max_tree_depth;      ///< The maximum BVH tree depth in the scene.
            uint32_t max_node_depth;      ///< The maximum node depth in the scene.

            // SceneNodeColors.
            glm::vec4 box16_node_color;                       ///< The box16  node color.
            glm::vec4 box32_node_color;                       ///< The box32 node color.
            glm::vec4 instance_node_color;                    ///< The instance node color.
            glm::vec4 procedural_node_color;                  ///< The procedural nodecolor.
            glm::vec4 triangle_node_color;                    ///< The triangle node color.
            glm::vec4 selected_node_color;                    ///< The selected node color.
            glm::vec4 wireframe_normal_color;                 ///< The color of the wireframe if not selected.
            glm::vec4 wireframe_selected_color;               ///< The color of the wireframe if not selected.
            glm::vec4 selected_geometry_color;                ///< The color of the geometry if selected.
            glm::vec4 background1_color;                      ///< The color of half of the checkered background.
            glm::vec4 background2_color;                      ///< The color of half of the checkered background.
            glm::vec4 transparent_color;                      ///< The color indicating transparent.
            glm::vec4 opaque_color;                           ///< The color indicating opaque.
            glm::vec4 positive_color;                         ///< The color indicating positive.
            glm::vec4 negative_color;                         ///< The color indicating negative.
            glm::vec4 build_algorithm_none_color;             ///< The color indicating the user hasn't specified a build algorithm.
            glm::vec4 build_algorithm_fast_build_color;       ///< The color indicating the user specified PREFER_FAST_BUILD.
            glm::vec4 build_algorithm_fast_trace_color;       ///< The color indicating the user specified PREFER_FAST_TRACE.
            glm::vec4 build_algorithm_both_color;             ///< The color indicating the user specified both PREFER_FAST_TRACE and PREFER_FAST_BUILD.
            glm::vec4 instance_opaque_none_color;             ///< The color indicating the user hasn't specified an opaque flag.
            glm::vec4 instance_opaque_force_opaque_color;     ///< The color indicating the user specified force opaque flag.
            glm::vec4 instance_opaque_force_no_opaque_color;  ///< The color indicating the user specified force no opaque flag.
            glm::vec4 instance_opaque_force_both_color;       ///< The color indicating the user specified both force opaque and force no opaque.

            // Bounding volume render module.
            const std::vector<RraVertex>*              custom_triangles;           ///< The custom triangle list.
            const std::vector<BoundingVolumeInstance>* bounding_volume_list;       ///< A list of bounding volume instances.
            std::vector<SelectedVolumeInstance>        selected_volume_instances;  ///< The list of selected volumes to render.

            // Traversal Render module.
            TraversalTree traversal_tree;  ///< Traversal tree for traversal compute shader.

            // For frustum culling.
            InstanceMap                         instance_map;             ///< Instance map after frustum culling has been applied.
            glm::vec3                           closest_point_to_camera;  ///< The location of closest point on geometry to the camera.
            const std::map<uint64_t, uint32_t>* instance_counts;          ///< Contains the pairs (blas_index, count).
            Camera*                             camera;                   ///< The camera.
            glm::mat4                           last_view_proj;           ///< The view projection matrix the camera used on the last frame.

            bool fused_instances_enabled;  ///< The indicator for fused instances in traversal.
        };

        /// @brief Info about the scene that is needed at startup.
        ///
        /// The graphics context is created before the rest of the renderer, so we need this separate
        /// struct that is populated early with the info the graphics context needs at creation.
        struct GraphicsContextSceneInfo
        {
            std::vector<TraversalTree> acceleration_structures;
        };

        /// @brief The RendererInterface class declaration.
        ///
        /// The RendererInterface is an abstract interface that can be extended
        /// to utilize a graphics API to interact with GPU hardware.
        class RendererInterface
        {
        public:
            /// @brief Constructor.
            RendererInterface();

            /// @brief Destructor.
            virtual ~RendererInterface();

            /// @brief Initialize the underlying graphics device.
            ///
            /// @returns True if the device was initialized successfully, and false in case of failure.
            virtual bool InitializeDevice() = 0;

            /// @brief Handle any synchronization required to advance to rendering the next frame.
            virtual void MoveToNextFrame() = 0;

            /// @brief Wait for all in flight operations in the GPU queue to complete.
            virtual void WaitForGpu() = 0;

            /// @brief Shut down the renderer interface.
            virtual void Shutdown() = 0;

            /// @brief Wait for the next swapchain image to become available.
            virtual void WaitForSwapchain() = 0;

            /// @brief Handle the renderer resizing.
            virtual void HandleDimensionsUpdated() = 0;

            /// @brief Render the scene.
            virtual void DrawFrame() = 0;

            /// @brief Mark the scene as dirty.
            virtual void MarkAsDirty() = 0;

            /// @brief Set heatmap.
            ///
            /// @param [in] heatmap The new heatmap to set.
            void SetHeatmapData(HeatmapData heatmap_data);

            /// @brief Set the window info structure.
            ///
            /// @param [in] window_info The window info.
            void SetWindowInfo(const WindowInfo* window_info);

            /// @brief Set the renderer dimensions.
            ///
            /// @param [in] width The window width.
            /// @param [in] height The window height.
            void SetDimensions(int width, int height);

            /// @brief Retrieve a reference to the renderer's camera.
            ///
            /// @returns A reference to the renderer's camera.
            Camera& GetCamera();

            /// @brief Set a callback to update info about the scene every frame.
            ///
            /// @param [in] callback The callback function.
            void SetSceneInfoCallback(std::function<void(RendererSceneInfo&, Camera* camera, bool frustum_culling, bool force_camera_update)> callback);

        protected:
            /// @brief Handle updating the renderer after the scene has changed.
            virtual void HandleSceneChanged() = 0;

            Camera            camera_ = {};            ///< The camera viewing the scene.
            int               width_;                  ///< The viewport width.
            int               height_;                 ///< The viewport height.
            const WindowInfo* window_info_ = nullptr;  ///< The window info.
            RendererSceneInfo scene_info_  = {};       ///< The scene being rendered.

            bool     should_update_heatmap_ = false;    ///< The flag to track of heatmap updates.
            Heatmap* heatmap_               = nullptr;  ///< The current heatmap.

            std::function<void(RendererSceneInfo&, Camera* camera, bool frustum_culling, bool force_camera_update)> update_scene_info_ =
                nullptr;  ///< Callback to update scene_info_ every frame.
        };

        /// @brief A factory used to create a renderer instance.
        class RendererFactory
        {
        public:
            /// @brief Create a renderer instance.
            ///
            /// @param [in] renderer_adapter_map A map of RendererAdapter instances used to alter renderer state.
            ///
            /// @returns A new RendererInterface instance.
            static RendererInterface* CreateRenderer(RendererAdapterMap& renderer_adapter_map);

        private:
            /// @brief Delete the constructor and destructor to prevent instantiation.
            RendererFactory()  = delete;
            ~RendererFactory() = delete;
        };
    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_RENDERER_INTERFACE_H_
