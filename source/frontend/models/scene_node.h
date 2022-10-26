//=============================================================================
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration for the SceneNode class.
//=============================================================================

#ifndef RRA_RENDERER_SCENE_NODE_H_
#define RRA_RENDERER_SCENE_NODE_H_

#include "public/renderer_types.h"

namespace rra
{
    /// @brief A list of a scene raw vertex data.
    typedef std::vector<renderer::RraVertex> VertexList;

    /// @brief A structure that contains vertex information for a triangle.
    struct SceneTriangle
    {
        renderer::RraVertex a;
        renderer::RraVertex b;
        renderer::RraVertex c;
    };

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
        /// @param [inout] frustum_info The information needed for the culling.
        ///
        /// Note: This function populates mutates the given frustum info struct.
        /// Specifically it populates the closest_distance_to_camera field for nearest plane calculation.
        void AppendFrustumCulledInstanceMap(renderer::InstanceMap& instance_map, renderer::FrustumInfo& frustum_info) const;

        /// @brief Recursively adds the render data to the instance map.
        ///
        /// @param [out] instance_map A reference to instance map.
        void AppendInstanceMap(renderer::InstanceMap& instance_map) const;

        /// @brief Recursively adds triangles (aligned vertices) to the given list.
        /// Note: Triangles in disabled branches are discarded.
        /// @param [out] vertex_list A reference to the map to add instances on.
        void AppendTrianglesTo(VertexList& vertex_list) const;

        /// @brief Construct the tree structure from BLAS.
        ///
        /// @param [in] blas_index The blas index.
        ///
        /// @returns A scene node.
        static SceneNode* ConstructFromBlas(uint32_t blas_index);

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

        /// @brief Reset selection.
        void ResetSelection();

        /// @brief Apply node selection.
        void ApplyNodeSelection();

        /// @brief Get the bounding volume of this node.
        ///
        /// @returns The bounding volume of this node.
        BoundingVolumeExtents GetBoundingVolume() const;

        /// @brief Collect the nodes in a map.
        ///
        /// @param [out] nodes The node map to register on.
        void CollectNodes(std::map<uint32_t, SceneNode*>& nodes);

        /// @brief Enable the node.
        void Enable();

        /// @brief Disable the node.
        void Disable();

        /// @brief Set the visibility of the node.
        ///
        /// @param [in] visible The visibility to set.
        void SetVisible(bool visible);

        /// @brief Set the all the children under this node as visible.
        void SetAllChildrenAsVisible();

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
        void CastRay(glm::vec3 ray_origin, glm::vec3 ray_direction, std::vector<SceneNode*>& intersected_nodes);

        /// @brief Get instances of this node.
        ///
        /// @returns A list of instances.
        std::vector<renderer::Instance> GetInstances() const;

        /// @brief Get triangles of this node.
        ///
        /// @returns A list of triangles.
        std::vector<SceneTriangle> GetTriangles() const;

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

        /// @brief Adds nodes recursively to the traversal tree.
        ///
        /// @param [out] node_to_address_map The mapping from node_id to the address that will end up in the tree.
        /// @param [out] traversal_tree The traversal tree to add onto.
        ///
        /// @returns The index address registered at the address buffer.
        uint32_t AddToTraversalTree(renderer::TraversalTree& traversal_tree);

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
        /// @param [in] node_id The box index under this blas.
        /// @param [in] depth The current depth for this node.
        ///
        /// @returns A scene node.
        static SceneNode* ConstructFromBlasNode(uint64_t blas_index, uint32_t node_id, uint32_t depth);

        SceneNode*                       parent_ = nullptr;         ///< The parent node.
        uint32_t                         node_id_;                  ///< The node id for this node.
        uint32_t                         depth_           = 0;      ///< The depth of this node.
        bool                             enabled_         = true;   ///< A flag to represent enablement of this node.
        bool                             visible_         = true;   ///< A flag to represent the visibility of this node.
        bool                             selected_        = false;  ///< A flag to represent if this node is selected.
        BoundingVolumeExtents            bounding_volume_ = {};     ///< The bounding volume of this node.
        std::vector<SceneNode*>          child_nodes_;              ///< The child nodes of this node.
        std::vector<renderer::Instance>  instances_;                ///< The instances that this node contains.
        std::vector<renderer::RraVertex> vertices_;                 ///< The vertices that this node contains. Aligned by 3.
        uint32_t                         primitive_index_ = 0;      ///< The primitive index of this node.
        uint32_t                         geometry_index_  = 0;      ///< The geometry index of this node.
    };

}  // namespace rra

#endif  // RRA_RENDERER_SCENE_NODE_H_
