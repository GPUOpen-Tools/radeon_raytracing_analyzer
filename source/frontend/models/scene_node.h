//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the SceneNode class.
//=============================================================================

#ifndef RRA_RENDERER_SCENE_NODE_H_
#define RRA_RENDERER_SCENE_NODE_H_

#include <optional>
#include <unordered_set>

#include "public/renderer_types.h"

#include "util/stack_vector.h"

namespace rra
{
    class Scene;

    /// @brief A list of a scene raw vertex data.
    typedef std::vector<renderer::RraVertex> VertexList;

    /// @brief A structure that contains vertex information for a triangle.
    struct SceneTriangle
    {
        renderer::RraVertex a;
        renderer::RraVertex b;
        renderer::RraVertex c;
    };

    /// @brief Get the unique key for a primitive by geometry.
    ///
    /// @param geometry_index The geometry index.
    /// @param primitive_index The primitive index.
    ///
    /// @return The unique key.
    uint64_t GetGeometryPrimitiveIndexKey(uint32_t geometry_index, uint32_t primitive_index);

    /// @brief A tree structure to contain volume data and instances.
    class SceneNode
    {
    public:
        /// @brief Constructor
        SceneNode();

        /// @brief Destructor
        ~SceneNode();

        /// @brief Recursively adds instances to the given vector.
        ///
        /// @param [out] instances_map A reference to the map to add instances on.
        void AppendInstancesTo(renderer::InstanceMap& instances_map) const;

        /// @brief Recursively adds the render data of volumes that are in the given frustum.
        ///
        /// @param [out] instance_map A reference to instance map.
        /// @param [inout] rebraid_duplicates The ith index states whether API instance with index i has had a rebraided sibling inserted already.
        /// @param [in] scene A pointer to the scene that is requesting this from the node.
        /// @param [inout] frustum_info The information needed for the culling.
        ///
        /// Note: This function populates mutates the given frustum info struct.
        /// Specifically it populates the closest_distance_to_camera field for nearest plane calculation.
        void AppendFrustumCulledInstanceMap(renderer::InstanceMap& instance_map,
                                            std::vector<bool>&     rebraid_duplicates,
                                            const Scene*           scene,
                                            renderer::FrustumInfo& frustum_info) const;

        /// @brief Recursively adds the render data to the instance map.
        ///
        /// @param [out] instance_map A reference to instance map.
        /// @param [in] scene A pointer to the scene that is requesting this from the node.
        void AppendInstanceMap(renderer::InstanceMap& instance_map, const Scene* scene) const;

        /// @brief Recursively adds triangles (aligned vertices) to the given list.
        /// Note: Triangles in disabled branches are discarded.
        /// @param [out] vertex_list A reference to the map to add instances on.
        void AppendTrianglesTo(VertexList& vertex_list) const;

        /// @brief Construct the tree structure from BLAS.
        ///
        /// @param [in] blas_index The blas index.
        /// @param [out] vertex_buffer The vertex buffer that triangles nodes will sub-allocate from.
        /// @param [out] child_buffer The buffer to sub-allocate node children from.
        ///
        /// @returns A scene node.
        static SceneNode* ConstructFromBlas(uint32_t blas_index, renderer::RraVertex* vertex_buffer, std::byte* child_buffer);

        /// @brief Construct the tree structure from TLAS.
        ///
        /// @param [in] tlas_index The tlas index.
        ///
        /// @returns A scene node.
        static SceneNode* ConstructFromTlas(uint64_t tlas_index);

        /// @brief Get bounds for selection.
        ///
        /// @param [out] volume The volume of the selection.
        void GetBoundingVolumeForSelection(BoundingVolumeExtents& volume) const;

        /// @brief Reset selection and child nodes.
        ///
        /// @param [out] selected_node_ids The set of selected node IDs.
        void ResetSelection(std::unordered_set<uint32_t>& selected_node_ids);

        /// @brief Reset selection.
        void ResetSelectionNonRecursive();

        /// @brief Apply node selection.
        ///
        /// @param [out] selected_node_ids The set of selected node IDs.
        void ApplyNodeSelection(std::unordered_set<uint32_t>& selected_node_ids);

        /// @brief Get the bounding volume of this node.
        ///
        /// @returns The bounding volume of this node.
        BoundingVolumeExtents GetBoundingVolume() const;

        /// @brief Collect the nodes in a map.
        ///
        /// @param [out] nodes The node map to register on.
        void CollectNodes(std::map<uint32_t, SceneNode*>& nodes);

        /// @brief Enable the node.
        ///
        /// @param [in] scene The scene that this node belongs to.
        void Enable(Scene* scene);

        /// @brief Disable the node.
        ///
        /// @param [in] scene The scene that this node belongs to.
        void Disable(Scene* scene);

        /// @brief Set the visibility of the node.
        ///
        /// @param [in] visible The visibility to set.
        /// @param [in] scene The scene that this node belongs to.
        void SetVisible(bool visible, Scene* scene);

        /// @brief Set a node and all of its ancestors as visible.
        void ShowParentChain();

        /// @brief Set the all the children under this node as visible.
        ///
        /// @param [out] selected_node_ids The set of selected node IDs.
        void SetAllChildrenAsVisible(std::unordered_set<uint32_t>& selected_node_ids);

        /// @brief Check if the node is visible.
        ///
        /// @returns True if visible.
        bool IsVisible();

        /// @brief Check if the node is enabled.
        ///
        /// @returns True if the node is enabled.
        bool IsEnabled();

        /// @brief Check if the node is selected.
        ///
        /// @returns True if the node is selected.
        bool IsSelected();

        /// @brief Cast a ray and report intersections.
        ///
        /// @param [in] ray_origin The origin of the ray.
        /// @param [in] ray_direction The direction of the ray.
        /// @param [out] intersected_nodes The list to add onto in case of intersection.
        void CastRayCollectNodes(glm::vec3 ray_origin, glm::vec3 ray_direction, std::vector<SceneNode*>& intersected_nodes);

        /// @brief Get the instance if there is one.
        ///
        /// The returned pointer should be used then immediately discarded. It's a pointer to an element
        /// of a vector, which makes the pointer dangling when it's reallocated.
        ///
        /// @return Pointer to the instance if it exists, otherwise nullptr.
        renderer::Instance* GetInstance();

        /// @brief Get triangles of this node.
        ///
        /// @returns A list of triangles.
        StackVector<SceneTriangle, 8> GetTriangles() const;

        /// @brief Get the primitive index of this node.
        ///
        /// @returns The primitive index of the node.
        uint32_t GetPrimitiveIndex() const;

        /// @brief Get the geometry index of this node.
        ///
        /// @returns The geometry index of the node.
        uint32_t GetGeometryIndex() const;

        /// @brief Get node id.
        ///
        /// @returns The node id.
        uint32_t GetId() const;

        /// @brief Append bounding volumes to list.
        ///
        /// @param [out] volume_list The list to append onto.
        /// @param [in] lower_bound The lower depth bound.
        /// @param [in] upper_bound The upper depth bound.
        void AppendBoundingVolumesTo(renderer::BoundingVolumeList& volume_list, uint32_t lower_bound, uint32_t upper_bound) const;

        /// @brief Get the depth of this node.
        ///
        /// @returns The depth of this node.
        uint32_t GetDepth() const;

        /// @brief Get the path of nodes leading up to this node.
        ///
        /// @returns A list of nodes leading up to this node.
        std::vector<SceneNode*> GetPath() const;

        /// @brief Get the parent of this node.
        ///
        /// @returns The parent of the node.
        SceneNode* GetParent() const;

        /// @brief Adds nodes and all children to traversal tree.
        ///
        /// Caller must reserve a sufficient capacity in TraversalTree::volumes before calling this function.
        ///
        /// @param [in] populate_vertex_buffer Whether or not TraversalTree::vertices should be written to.
        /// @param [out] traversal_tree The traversal tree to add onto.
        void AddToTraversalTree(bool populate_vertex_buffer, renderer::TraversalTree& traversal_tree);

        /// @brief For each triangle vertex, write to a bit specifying if it's split or not.
        ///
        /// @param [out] root The root node of the BLAS.
        /// @param [in]  geometry_offsets Geometry of each offset into primitive_counts.
        /// @param [in]  primitive_count The number of primitives in the BLAS.
        static void PopulateSplitVertexAttribute(SceneNode* root, const std::vector<uint32_t>& geometry_offsets, uint32_t primitive_count);

        /// @brief For each triangle vertex, write to a bit specifying if it's split or not.
        ///
        /// @param [in]     geometry_offsets Geometry of each offset into primitive_counts.
        /// @param [in/out] primitive_counts The number of duplicates of each primitive id / geometry id pair.
        void PopulateSplitVertexAttribute(const std::vector<uint32_t>& geometry_offsets, std::vector<uint8_t>& primitive_counts);

        /// @brief Set whether or not this node is culled by the instance mask filter.
        ///
        /// @param filtered Culled if true.
        void SetFiltered(bool filtered);

    private:
        /// @brief Construct the tree structure from TLAS.
        ///
        /// @param [in] tlas_index The tlas index.
        /// @param [in] box_index The box index under this tlas.
        /// @param [in] depth The current depth for this node.
        ///
        /// @returns A scene node.
        static SceneNode* ConstructFromTlasBoxNode(uint64_t tlas_index, uint32_t box_index, uint32_t depth);

        /// @brief Construct the tree structure from BLAS.
        ///
        /// @param [in] blas_index The blas index.
        /// @param [in] root_id The id of the root BLAS node.
        /// @param [out] vertex_buffer The buffer to sub-allocate triangle node vertices from.
        /// @param [out] child_buffer The buffer to sub-allocate node children from.
        ///
        /// @returns A scene node.
        static SceneNode* ConstructFromBlasNode(uint64_t blas_index, uint32_t root_id, renderer::RraVertex* vertex_buffer, std::byte* child_buffer);

        /// @brief Appends the merged instance to the instance map.
        ///
        /// Caller must call this for only a single rebraid sibling per API instance.
        ///
        /// @param [in] instance The instance to append.
        /// @param [inout] instance_map The instance map to append to.
        /// @param [in] scene The scene to collect rebraid siblings from.
        void AppendMergedInstanceToInstanceMap(renderer::Instance instance, renderer::InstanceMap& instance_map, const Scene* scene) const;

        SceneNode*                        parent_ = nullptr;                   ///< The parent node.
        uint32_t                          node_id_;                            ///< The node id for this node.
        uint64_t                          bvh_index_;                          ///< The BVH index of the scene.
        uint32_t                          depth_           = 0;                ///< The depth of this node.
        bool                              enabled_         = true;             ///< A flag to represent enablement of this node.
        bool                              filtered_        = false;            ///< A flag to represent whether this node is disabled by being filtered.
        bool                              visible_         = true;             ///< A flag to represent the visibility of this node.
        bool                              selected_        = false;            ///< A flag to represent if this node is selected.
        BoundingVolumeExtents             bounding_volume_ = {};               ///< The bounding volume of this node.
        StackVector<SceneNode*, 8>        child_nodes_     = {};               ///< The child nodes of this node.
        std::optional<renderer::Instance> instance_;                           ///< The optional instance that this node contains.
        uint32_t                          vertex_count_    = 0;                ///< The number of vertices.
        renderer::RraVertex*              vertices_        = nullptr;          ///< The vertices that this node contains. Aligned by 3.
        uint32_t                          primitive_index_ = 0;                ///< The primitive index of this node.
        uint32_t                          geometry_index_  = 0;                ///< The geometry index of this node.
        uint32_t                          obb_index_       = {};               ///< The oriented bounding box matrix index.
        glm::mat3                         rotation_        = glm::mat3(1.0f);  ///< The rotation of a box node.
    };

}  // namespace rra

#endif  // RRA_RENDERER_SCENE_NODE_H_

