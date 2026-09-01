//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the SceneNode class.
//=============================================================================

#ifndef RRA_RENDERER_SCENE_NODE_H_
#define RRA_RENDERER_SCENE_NODE_H_

#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "public/renderer_types.h"

#include "util/stack_vector.h"

namespace rra
{
    class Scene;

    /// @brief BLAS-intrinsic values reused across every TLAS instance that references the same BLAS.
    ///
    /// These are properties of the BLAS alone (independent of the instance/transform), so computing them once per
    /// unique BLAS instead of once per instance avoids an O(instances * BLAS-nodes) blow-up when building the TLAS scene.
    struct BlasIntrinsicInfo
    {
        uint32_t max_depth;
        uint32_t average_depth;
        uint32_t triangle_count;
        float    average_triangle_sah;
        float    min_triangle_sah;
    };

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

        /// @brief Construct the tree structure from BLAS.
        ///
        /// @param [in]  blas_index      The blas index.
        /// @param [out] vertex_buffer   The vertex buffer that triangles nodes will sub-allocate from.
        /// @param [out] child_buffer    The buffer to sub-allocate node children from.
        ///
        /// @returns A scene node.
        static SceneNode* ConstructFromBlas(uint32_t blas_index, renderer::RraVertex* vertex_buffer, std::byte* child_buffer);

        /// @brief Count the triangles the BLAS construction traversal will emit into the vertex buffer.
        ///
        /// This mirrors the exact traversal in ConstructFromBlasNode (same node visitation order and the same
        /// child_index / global_child_index bookkeeping) so the returned count matches the number of triangles the
        /// fill pass writes. The header's unique-triangle count (geometry-info primitive sum) can under-count when
        /// triangle nodes share prim-ranges, so it must not be used to size the vertex buffer.
        ///
        /// @param [in] blas_index The blas index.
        ///
        /// @returns The number of triangles emitted by the traversal (multiply by 3 for the vertex count).
        static uint32_t CountBlasEmittedTriangles(uint64_t blas_index);

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
        void ResetSelection(std::unordered_set<uint64_t>& selected_node_ids);

        /// @brief Reset selection.
        void ResetSelectionNonRecursive();

        /// @brief Apply node selection.
        ///
        /// @param [out] selected_node_ids The set of selected node IDs.
        void ApplyNodeSelection(std::unordered_set<uint64_t>& selected_node_ids);

        /// @brief Get the bounding volume of this node.
        ///
        /// @returns The bounding volume of this node.
        BoundingVolumeExtents GetBoundingVolume() const;

        /// @brief Collect the nodes in a map.
        ///
        /// @param [out] nodes The node map to register on.
        void CollectNodes(std::map<uint64_t, SceneNode*>& nodes);

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
        void SetAllChildrenAsVisible(std::unordered_set<uint64_t>& selected_node_ids);

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

        /// @brief Get the flattened CLAS sub-instances of a CBLAS TLAS instance node.
        ///
        /// Empty for ordinary instances; populated for a Cluster BLAS (CBLAS) instance, one entry per
        /// referenced CLAS with a world->CLAS-object transform (see BuildClusterSubInstances).
        ///
        /// @return The cluster sub-instances.
        const std::vector<renderer::Instance>& GetClusterSubInstances() const;

        /// @brief Get triangles of this node.
        ///
        /// @returns A list of triangles.
        StackVector<SceneTriangle, MAX_CHILD_NODES> GetTriangles() const;

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
        uint64_t GetId() const;

        /// @brief Append bounding volumes to list.
        ///
        /// @param [out] volume_list The list to append onto.
        /// @param [in] lower_bound The lower depth bound.
        /// @param [in] upper_bound The upper depth bound.
        void AppendBoundingVolumesTo(renderer::BoundingVolumeList& volume_list,
                                     uint32_t                      lower_bound,
                                     uint32_t                      upper_bound,
                                     bool                          show_internal_bounds,
                                     bool                          show_leaf_bounds) const;

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
        /// @param [in] is_tlas                True if this bvh is a TLAS.
        /// @param [out] traversal_tree The traversal tree to add onto.
        void AddToTraversalTree(bool populate_vertex_buffer, bool is_tlas, renderer::TraversalTree& traversal_tree);

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

        /// @brief Get the child index of this node.
        ///
        /// @return The child index.
        uint32_t GetChildIndex() const;

        /// @brief Get the global child index of this node.
        ///
        /// @return The global child index.
        uint32_t GetGlobalChildIndex() const;

        /// @brief Get the number of children this node has.
        ///
        /// @return The child count.
        size_t GetChildCount() const;

        /// @brief Get a specific child of this node.
        ///
        /// @param child_index The child index to get.
        ///
        /// @return The child node.
        SceneNode* GetChild(uint32_t child_index);

        /// @brief Get the rotation of the node.
        ///
        /// @return The rotation.
        glm::mat3 GetRotation();

        /// @brief Get the BLAS index.
        ///
        /// @return The BLAS index.
        uint64_t GetBvhIndex();

        /// @brief Get the primary sibling this node duplicates under node packing (RTIP3.1), or null.
        ///
        /// A packed-ref node carries its own per-slot bounding box but no geometry/instance of its own; picking
        /// or descending it must redirect to the primary sibling that owns the shared subtree.
        ///
        /// @return The primary SceneNode, or nullptr when this node is not a packed-ref.
        SceneNode* GetPackedPrimary() const;

    private:
        /// @brief Construct the tree structure from TLAS.
        ///
        /// @param [in] tlas_index  The tlas index.
        /// @param [in] node_id     The ID of this node.
        /// @param [in] child_index   The child index of this node.
        /// @param [in] depth         The current depth for this node.
        /// @param [in] blas_cache    Optional per-BLAS intrinsic cache.
        /// @param [in] is_packed_ref When true (RTIP3.1 node packing), build a shallow node that carries only its own
        ///                           per-slot bounding box; the primary sibling owns the shared subtree/instance so this
        ///                           node neither builds an instance nor recurses into children.
        ///
        /// @returns A scene node.
        static SceneNode* ConstructFromTlasBoxNode(uint64_t                                             tlas_index,
                                                   uint32_t                                             node_id,
                                                   uint32_t                                             child_index,
                                                   uint32_t                                             depth,
                                                   std::unordered_map<uint64_t, BlasIntrinsicInfo>*     blas_cache = nullptr,
                                                   bool                                                 is_packed_ref = false);

        /// @brief Construct the tree structure from BLAS.
        ///
        /// @param [in] blas_index       The blas index.
        /// @param [in] root_id          The id of the root BLAS node.
        /// @param [out] vertex_buffer   The buffer to sub-allocate triangle node vertices from.
        /// @param [out] child_buffer    The buffer to sub-allocate node children from.
        ///
        /// @returns A scene node.
        static SceneNode* ConstructFromBlasNode(uint64_t blas_index, uint32_t root_id, renderer::RraVertex* vertex_buffer, std::byte* child_buffer);

        /// @brief Populate a scene node's instance from a Cluster BLAS (CBLAS) cluster-reference leaf.
        ///
        /// A CBLAS leaf is a hardware instance node that references a CLAS (nested instancing). This builds a
        /// renderer::Instance pointing at the referenced CLAS so the BLAS pane can render it like a TLAS instance.
        ///
        /// @param [in] blas_index The Cluster BLAS index.
        /// @param [in,out] node   The scene node for the cluster-reference leaf; its instance is populated.
        static void ConstructClusterRefInstance(uint64_t blas_index, SceneNode* node);

        /// @brief Precompute flattened CLAS instances for a TLAS instance that references a Cluster BLAS (CBLAS).
        ///
        /// A CBLAS has no triangles of its own; its geometry is the nested CLAS instances it references. So the
        /// TLAS view can draw real geometry (not just a box), enumerate the CBLAS cluster-reference leaves and
        /// build one renderer::Instance per referenced CLAS, composing the CBLAS instance transform with each
        /// leaf's transform. These are emitted to the instance map in place of the (empty) CBLAS instance. No-op
        /// for an ordinary BLAS instance.
        ///
        /// @param [in,out] node           The TLAS instance scene node; its cluster_sub_instances_ is populated.
        /// @param [in]     cblas_instance The instance referencing the CBLAS (supplies transform + shared metadata).
        static void BuildClusterSubInstances(SceneNode* node, const renderer::Instance& cblas_instance);

        /// @brief Appends the merged instance to the instance map.
        ///
        /// Caller must call this for only a single rebraid sibling per API instance.
        ///
        /// @param [in] instance The instance to append.
        /// @param [inout] instance_map The instance map to append to.
        /// @param [in] scene The scene to collect rebraid siblings from.
        void AppendMergedInstanceToInstanceMap(renderer::Instance instance, renderer::InstanceMap& instance_map, const Scene* scene) const;

        /// @brief Get a uint64_t hash of the child index and node ID.
        ///
        /// @return The hash of the child index and node ID.
        uint64_t GetChildIdHash() const;

        SceneNode*                               parent_ = nullptr;              ///< The parent node.
        uint32_t                                 node_id_;                       ///< The node id for this node.
        uint64_t                                 bvh_index_;                     ///< The BVH index of the scene.
        uint32_t                                 depth_           = 0;           ///< The depth of this node.
        bool                                     enabled_         = true;        ///< A flag to represent enablement of this node.
        bool                                     filtered_        = false;       ///< A flag to represent whether this node is disabled by being filtered.
        bool                                     visible_         = true;        ///< A flag to represent the visibility of this node.
        bool                                     selected_        = false;       ///< A flag to represent if this node is selected.
        bool                                     is_tlas_         = false;       ///< A flag to represent if this node is in a TLAS scene.
        BoundingVolumeExtents                    bounding_volume_ = {};          ///< The bounding volume of this node.
        StackVector<SceneNode*, MAX_CHILD_NODES> child_nodes_     = {};          ///< The child nodes of this node.
        std::optional<renderer::Instance>        instance_;                      ///< The optional instance that this node contains.
        std::vector<renderer::Instance>          cluster_sub_instances_;         ///< Flattened CLAS instances for a CBLAS TLAS instance (see BuildClusterSubInstances).
        uint32_t                                 vertex_count_       = 0;        ///< The number of vertices.
        renderer::RraVertex*                     vertices_           = nullptr;  ///< The vertices that this node contains. Aligned by 3.
        uint32_t                                 primitive_index_    = 0;        ///< The primitive index of this node.
        uint32_t                                 geometry_index_     = 0;        ///< The geometry index of this node.
        uint32_t                                 obb_index_          = {};       ///< The oriented bounding box matrix index.
        glm::mat3                                rotation_           = glm::mat3(1.0f);  ///< The rotation of a box node.
        uint32_t                                 child_index_        = 0;                ///< The parent node's child index of this node.
        uint32_t                                 global_child_index_ = 0;                ///< The node index of all nodes in the BVH.

        /// @brief Node packing (RTIP3.1): when several sibling slots point to the same child node, one SceneNode is
        /// created per slot so each slot's distinct bounding box is drawn, but only the primary slot expands the shared
        /// subtree. A packed-ref node points at its primary sibling here; the primary (and all non-packed nodes) leave
        /// this null. Packed-ref nodes carry no children/geometry of their own.
        SceneNode* packed_primary_ = nullptr;  ///< The primary sibling SceneNode this packed-ref duplicates, or null.

        /// @brief True when this node is a packed-ref duplicate of a sibling that shares its node id (see packed_primary_).
        bool IsPackedRef() const
        {
            return packed_primary_ != nullptr;
        }
    };

}  // namespace rra

#endif  // RRA_RENDERER_SCENE_NODE_H_

