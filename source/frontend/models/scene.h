//=============================================================================
// Copyright (c) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the Scene class.
//=============================================================================

#ifndef RRA_RENDERER_SCENE_H_
#define RRA_RENDERER_SCENE_H_

#include <map>
#include <functional>

#include "public/renderer_types.h"
#include "scene_node.h"

namespace rra
{
    /// @brief The enumeration for the context menu.
    enum class SceneContextMenuLocation
    {
        kSceneContextMenuLocationTreeView,
        kSceneContextMenuLocationViewer
    };

    /// @brief The structure to ray test depending on the menu location.
    struct SceneContextMenuRequest
    {
        SceneContextMenuLocation location;
        glm::vec3                origin    = {};
        glm::vec3                direction = {};
    };

    /// @brief Colors to use in the renderer.
    struct SceneNodeColors
    {
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
    };

    /// @brief Set the global scene node colors.
    ///
    /// @param [in] new_colors The colors to set.
    void SetSceneNodeColors(SceneNodeColors new_colors);

    /// @brief Get the global scene node colors.
    ///
    /// @returns The global scene node colors.
    const SceneNodeColors& GetSceneNodeColors();

    /// @brief A structure containing computed scene metadata.
    struct SceneStatistics
    {
        uint32_t max_instance_count = 0;  ///< The maximum instance count across all scene instances.
        uint32_t max_triangle_count = 0;  ///< The highest triangle count out of each mesh in the scene.
        int32_t  max_tree_depth     = 0;  ///< The maximum BVH tree depth in the scene.
        uint32_t max_node_depth     = 0;  ///< The maximum node depth in the scene.
    };

    /// @brief Declaration for the Scene type.
    ///
    /// This type holds all meshes visible in the rendered scene.
    class Scene
    {
    public:
        /// @brief Constructor.
        Scene() = default;

        /// @brief Destructor.
        ~Scene();

        /// @brief Initialize the Scene with the input mesh and instance info.
        ///
        /// @param [in] root_node The root node for this scene.
        void Initialize(SceneNode* root_node);

        /// @brief Get the mesh instances map.
        ///
        /// @returns A map to the mesh instances by blas id.
        renderer::InstanceMap GetInstances() const;

        /// @brief Get the frustum culled render data.
        ///
        /// @param [in] frustum_info The information needed for the culling.
        ///
        /// Note: This function populates mutates the given frustum info struct.
        /// Specifically it populates the closest_distance_to_camera field for nearest plane calculation.
        ///
        /// @returns A map of instances.
        renderer::InstanceMap GetFrustumCulledInstanceMap(renderer::FrustumInfo& frustum_info) const;

        /// @brief Get the render data without frustum culling.
        ///
        /// @returns A map of instances.
        renderer::InstanceMap GetInstanceMap();

        /// @brief Get the bounding volume instances.
        ///
        /// @returns A list of bounding volume instances.
        const renderer::BoundingVolumeList* GetBoundingVolumeList() const;

        /// @brief Get the custom triangle list.
        ///
        /// @return A list of triangles in aligned vertex format.
        const VertexList* GetCustomTriangles() const;

        /// @brief Get statistics about the scene.
        ///
        /// @returns Statistics about the scene.
        const SceneStatistics& GetSceneStatistics() const;

        /// @brief Set the scene selection.
        ///
        /// @param [in] node_id The node id to select.
        void SetSceneSelection(uint32_t node_id);

        /// @brief Reset the scene selection.
        ///
        /// Deselect anything that was previously selected.
        void ResetSceneSelection();

        /// @brief Check if the scene has anything selected in it.
        ///
        /// @returns True if anything in the scene has been selected.
        bool HasSelection() const;

        /// @brief Get the most recent selected node id.
        ///
        /// @returns The most recent selected node id.
        uint32_t GetMostRecentSelectedNodeId() const;

        /// @brief Return an id of any selected node.
        ///
        /// Problems occur when we deselect the most recently selected node during
        /// multiselect, and most_recent_selected_node_id_ references an unselected node.
        /// So we assign it an arbitrary selected node.
        ///
        /// @return The id of a selected node.
        uint32_t GetArbitrarySelectedNodeId() const;

        /// @brief Get the scene volume.
        ///
        /// @param [out] volume The bounding volume.
        ///
        /// @returns True if the volume is retrieved properly.
        bool GetSceneBoundingVolume(BoundingVolumeExtents& volume) const;

        /// @brief Get the selection volume.
        ///
        /// @param [out] volume The bounding volume of the selection.
        ///
        /// @returns True if the volume is retrieved properly.
        bool GetBoundingVolumeForSelection(BoundingVolumeExtents& volume) const;

        /// @brief Returns the current scene iteration. This value can be used to check if the scene has changed.
        ///
        /// @returns The current scene iteration.
        uint64_t GetSceneIteration() const;

        /// @brief Iterates the scene, called when a change is made in the scene to notify downstream consumers.
        void IncrementSceneIteration();

        /// @brief Get total instance count for blas.
        ///
        /// @param [in] blas_index The blas to check for.
        ///
        /// @returns The total instance count for this blas (both visible and invisible).
        uint32_t GetTotalInstanceCountForBlas(uint64_t blas_index) const;

        /// @brief Enables the node with the given id.
        ///
        /// @param [in] node_id The node to enable.
        void EnableNode(uint32_t node_id);

        /// @brief Disables the node with the given id.
        ///
        /// @param [in] node_id The node to disable.
        void DisableNode(uint32_t node_id);

        /// @brief Check if the node is enabled.
        ///
        /// @param [in] node_id The node to check. Note: if the node does not exist, expect false.
        ///
        /// @returns True if the node is enabled.
        bool IsNodeEnabled(uint32_t node_id);

        /// @brief Cast a ray and report intersections.
        ///
        /// @param [in] ray_origin The origin of the ray.
        /// @param [in] ray_direction The direction of the ray.
        ///
        /// @returns The nodes that has their bounding volumes intersected by this ray.
        std::vector<SceneNode*> CastRay(glm::vec3 ray_origin, glm::vec3 ray_direction);

        /// @brief Get node by id.
        ///
        /// @param [in] node_id The node id to get.
        ///
        /// @returns A scene node.
        SceneNode* GetNodeById(uint32_t node_id);

        /// @brief Set the depth range for the scene.
        ///
        /// @param [in] lower_bound The lower bound.
        /// @param [in] upper_bound The uppper bound.
        void SetDepthRange(uint32_t lower_bound, uint32_t upper_bound);

        /// @brief Get depth range lower bound.
        ///
        /// @returns The lower bound.
        uint32_t GetDepthRangeLowerBound() const;

        /// @brief Get depth range upper bound.
        ///
        /// @returns The lower bound.
        uint32_t GetDepthRangeUpperBound() const;

        /// @brief Get the current options avaiable for the selection in the scene.
        ///
        /// @param [in] request The request that the options are requested with.
        ///
        /// @returns The selection context options with their corresponding functions.
        std::map<std::string, std::function<void()>> GetSceneContextOptions(SceneContextMenuRequest request);

        /// @brief Generates the traversal tree
        ///
        /// @returns The resulting traversal tree
        renderer::TraversalTree GenerateTraversalTree();

        /// @brief Get map of blas and instance counts.
        ///
        /// @returns Mapping of blas to instance counts.
        const std::map<uint64_t, uint32_t>* GetBlasInstanceCounts() const;

        /// @brief Get selected volume instances.
        ///
        /// @return The selected volume instances.
        const std::vector<renderer::SelectedVolumeInstance>& GetSelectedVolumeInstances() const;

        /// @brief Hides the currently selected node.
        void HideSelectedNodes();

        /// @brief Makes all the nodes in the scene visible.
        void ShowAllNodes();

        /// @brief Sets whether or not selecting multiple instance nodes is enabled.
        /// @param multi_select True if multiselect should be enabled.
        static void SetMultiSelect(bool multi_select);

    private:
        /// @brief Populate the scene info values.
        void PopulateSceneInfo();

        /// @brief Update custom triangle list.
        void UpdateCustomTriangles();

        /// @brief Update custom triangle list.
        void UpdateBoundingVolumes();

        /// @brief Compute the maximum triangle count across all BLAS meshes.
        ///
        /// @returns The maximum triangle count.
        uint32_t ComputeMaxTriangleCount() const;

        /// @brief Compute the maximum instance count across all scene instances.
        ///
        /// @returns The maximum instance count.
        uint32_t ComputeMaxInstanceCount() const;

        /// @brief Compute the maximum tree depth across all BLAS instances.
        ///
        /// @returns The maximum tree depth.
        uint32_t ComputeMaxTreeDepth() const;

        /// @brief Populate the selected volume instances.
        void PopulateSelectedVolumeInstances();

        SceneNode*                                    root_node_ = nullptr;               ///< The root node of the scene.
        renderer::BoundingVolumeList                  bounding_volume_list_;              ///< A list of all the bounding volumes to display.
        std::vector<renderer::SelectedVolumeInstance> selected_volume_instances_;         ///< A list of all the selected volume instances to be rendered.
        SceneStatistics                               scene_stats_ = {};                  ///< A structure containing computed scene info.
        std::map<uint64_t, uint32_t>                  blas_instance_counts_;              ///< A map to contain instance counts for a given blas.
        std::map<uint32_t, SceneNode*>                nodes_;                             ///< A map of all the nodes connected to root node (inclusive).
        VertexList                                    custom_triangles_;                  ///< A list of custom triangles in the scene.
        uint32_t                                      most_recent_selected_node_id_ = 0;  ///< The most recent selected node id.
        static bool                                   multi_select_;                      ///< Allows multiple nodes to be selected if true.
        renderer::InstanceMap                         cached_instance_map_{};  ///< Saved instance map of all instances, used when frustum culling is disabled.

        uint32_t depth_range_lower_bound_ = 0;  ///< The lower bound for the depth range.
        uint32_t depth_range_upper_bound_ = 0;  ///< The upper bound for the depth range.

        // Static so that it monotonically increases across all scenes.
        // This prevents problems when storing last scene iteration and switching scenes.
        static uint64_t scene_iteration_;  ///< A number to check on for changes in the scene.
    };
}  // namespace rra

#endif  // RRA_RENDERER_SCENE_H_
