//=============================================================================
// Copyright Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT
/// @author AMD Developer Tools Team
/// @file
/// @brief  Bottom level acceleration structure not specific to rt ip level.
//=============================================================================

#ifndef RRA_BACKEND_BVH_ENCODED_BOTTOM_LEVEL_BVH_H_
#define RRA_BACKEND_BVH_ENCODED_BOTTOM_LEVEL_BVH_H_

#include <unordered_map>

#include "public/rra_bvh.h"
#include "public/rra_error.h"

#include "bvh/geometry_info.h"
#include "bvh/ibvh.h"
#include "bvh/node_types/procedural_node.h"
#include "bvh/node_types/triangle_node.h"
#include "bvh/rtip31/primitive_node.h"

namespace rta
{
    class EncodedBottomLevelBvh : public IBvh
    {
    public:
        // Global identifier of tlas dump in chunk files.
        static constexpr const char* kChunkIdentifier = "GpuEncBlasDump";

        /// @brief Default constructor.
        EncodedBottomLevelBvh() = default;

        /// @brief Destructor.
        virtual ~EncodedBottomLevelBvh();

        /// @brief Get the total number of leaf nodes.
        ///
        /// @return The number of leaf nodes.
        virtual uint32_t GetLeafNodeCount() const = 0;

        /// @brief Get the geometry info data.
        ///
        /// @return The geometry info.
        const std::vector<dxr::amd::GeometryInfo>& GetGeometryInfos() const;

        /// @brief Get the list of primitive node pointers.
        ///
        /// @return The list of node pointers.
        const std::vector<dxr::amd::NodePointer>& GetPrimitiveNodePtrs() const;

        /// @brief Does this BVH have references.
        ///
        /// Call this function to recreate the original GPU virtual addresses stored in the
        /// instance nodes of this dump. Use this function before exporting the dumps
        /// to the original Dxc format.
        ///
        /// @return true if the TLAS has references, false if not.
        bool HasBvhReferences() const override;

        /// @brief Get the top-level surface area heuristic for this BLAS.
        ///
        /// @return The surface area heuristic.
        float GetSurfaceAreaHeuristic() const;

        /// @brief Set the top-level surface area heuristic for this BLAS.
        ///
        /// @param [in] surface_area_heuristic The surface area heuristic value to be set.
        void SetSurfaceAreaHeuristic(float surface_area_heuristic);

        /// @brief Does this BLAS contain procedural nodes?
        ///
        /// @return True if it contains procedural nodes.
        bool IsProcedural() const;

        /// @brief Get the node name for a node.
        ///
        /// @param [in]  node_id             The node id.
        /// @param [out] out_name            A pointer to receive the name string.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetNodeName(uint32_t node_id, const char** out_name) const = 0;

        /// @brief Get the node tooltip name for a node.
        ///
        /// @param [in]  node_id             The node id.
        /// @param [out] out_name            A pointer to receive the tooltip string.
        ///
        /// @return kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetNodeNameToolTip(uint32_t node_id, const char** out_tooltip) const = 0;

        /// @brief Retrieve the geometry index for the triangle node.
        ///
        /// @param [in]  node_id             The node whose geometry index is to be found.
        /// @param [in]  child_index		 The node's child index from its parent.
        /// @param [in]  global_child_index  The node's global child index.
        /// @param [out] out_geometry_index  The geometry index for the triangle node.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetGeometryIndex(uint32_t node_id, uint32_t child_index, uint32_t global_child_index, uint32_t* out_geometry_index) const = 0;

        /// @brief Retrieve the primitive index for the triangle node.
        ///
        /// @param [in]  node_id				The node whose primitive index is to be found.
        /// @param [in]  child_index			The node's child index from its parent.
        /// @param [in]  global_child_index     The node's global child index.
        /// @param [in]  local_primitive_index	The local primitive index within the given node.
        /// @param [out] out_primitive_index	The primitive index for the triangle node.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetPrimitiveIndex(uint32_t  node_id,
                                               uint32_t  child_index,
                                               uint32_t  global_child_index,
                                               uint32_t  local_primitive_index,
                                               uint32_t* out_primitive_index) const = 0;

        /// @brief Retrieve whether the node is inactive.
        ///
        /// @param [in]  node_id            The node whose geometry flags are to be retrieved.
        /// @param [in]  child_index		The node's child index from its parent.
        /// @param [in]  global_child_index The node's global child index.
        /// @param [out] out_is_inactive	Whether the node is inactive.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetIsInactive(uint32_t node_id, uint32_t child_index, uint32_t global_child_index, bool* out_is_inactive) const = 0;

        /// @brief Retrieve the number of triangles on a given node.
        ///
        /// @param [in]  node_id            The node ID to retrieve the triangles from.
        /// @param [in]  child_index		The node's child index from its parent.
        /// @param [in]  global_child_index The node's global child index.
        /// @param [out] out_triangle_count The number of triangles in the BLAS.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetNodeTriangleCount(uint32_t node_id, uint32_t child_index, uint32_t global_child_index, uint32_t* out_triangle_count) const = 0;

        /// @brief Retrieve the triangles stored in the given node id.
        ///
        /// @param [in]  node_id            The node ID to retrieve the triangles for.
        /// @param [in]  child_index	    The node's child index from its parent.
        /// @param [in]  global_child_index The node's global child index.
        /// @param [out] out_triangles      A preallocated pointer to dump the triangles into.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetNodeTriangles(uint32_t node_id, uint32_t child_index, uint32_t global_child_index, TriangleVertices* out_triangles) const = 0;

        /// @brief Retrieve the vertices stored in the given node id.
        ///
        /// @param [in]  node_id            The node ID to retrieve the vertices for.
        /// @param [in]  child_index        The node's child index from its parent.
        /// @param [in]  global_child_index The node's global child index.
        /// @param [out] out_count          The number of vertices in the triangle node.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetNodeVertexCount(uint32_t node_id, uint32_t child_index, uint32_t global_child_index, uint32_t* out_count) const = 0;

        /// @brief Retrieve the vertices stored in the given node id.
        ///
        /// @param [in]  node_id            The node ID to retrieve the vertices for.
        /// @param [in]  child_index        The node's child index from its parent.
        /// @param [in]  global_child_index The node's global child index.
        /// @param [out] out_triangles      A preallocated pointer to dump the vertices into.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        virtual RraErrorCode GetNodeVertices(uint32_t               node_id,
                                             uint32_t               child_index,
                                             uint32_t               global_child_index,
                                             struct VertexPosition* out_vertices) const = 0;

        /// @brief Get the surface area for a given triangle node.
        ///
        /// @param [in] triangle_node Reference to the triangle node whose surface area is to be calculated.
        /// @param [in] tri_count Number of triangles in triangle node.
        ///
        /// @return The triangle node surface area.
        float GetTriangleSurfaceArea(const dxr::amd::TriangleNode& triangle_node, uint32_t tri_count) const;

        /// @brief Calculate the surface area of a triangle.
        ///
        /// @param [in] v0 Vertex 0 of the triangle.
        /// @param [in] v1 Vertex 1 of the triangle.
        /// @param [in] v2 Vertex 2 of the triangle.
        ///
        /// @return The surface area of the triangle.
        float TriangleSurfaceArea(const dxr::amd::Float3& v0, const dxr::amd::Float3& v1, const dxr::amd::Float3& v2) const;

        /// @brief Retrieve the number of triangle nodes in a BLAS mesh.
        ///
        /// @param [out] out_triangle_count  The number of triangle nodes in the BLAS.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        RraErrorCode GetTriangleNodeCount(uint32_t* out_triangle_count) const;

        /// @brief Retrieve the total number of procedural nodes in a BLAS mesh.
        ///
        /// @param [out] out_procedural_node_count  The number of procedural nodes in the BLAS.
        ///
        /// @returns kRraOk if successful or an RraErrorCode if an error occurred.
        RraErrorCode GetProceduralNodeCount(uint32_t* out_procedural_node_count) const;

        /// @brief Get the total number of triangles in this BLAS.
        ///
        /// @return The total triangle count.
        virtual uint32_t GetTriangleCount() const;

        /// @brief Get the total number of triangles in a geometry.
        ///
        /// @param [in] geometry_index  The index of the geometry.
        ///
        /// @return The triangle count for the geometry.
        virtual uint32_t GetGeometryTriangleCount(uint32_t geometry_index) const;

        /// @brief Traverse the tree for compute leaf node surface area heuristics.
        virtual void ComputeSurfaceAreaHeuristic() = 0;

    protected:
        std::vector<dxr::amd::GeometryInfo> geom_infos_ = {};  ///< Array of geometry info.
        std::unordered_map<uint64_t, float> triangle_surface_area_heuristic_ =
            {};                                 ///< Surface area heuristic values for the triangles. Pairs of (node_child_id, SAH).
        float surface_area_heuristic_ = 0.0f;   ///< The precalculated Surface area heuristic for this BLAS.
        bool  is_procedural_          = false;  ///< Whether or not this BLAS contains procedural nodes in oppose to triangle nodes.

    private:
        /// @brief Obtain the byte size of the encoded buffer.
        ///
        /// @param [in] import_option Flag indicating which sections of the chunk to load/discard.
        ///
        /// @return The buffer size.
        std::uint64_t GetBufferByteSizeImpl(const ExportOption export_option) const override;

        /// @brief Private function to get the length of a vector, given 2 3D points.
        ///
        /// @param vert_1 The first vertex.
        /// @param vert_2 The second vertex.
        ///
        /// @return The vector length.
        double GetLength(const dxr::amd::Float3& vert_1, const dxr::amd::Float3& vert_2) const;

        /// @brief Validate the loaded BVH data for accuracy where possible.
        ///
        /// @return true if data is valid, false otherwise.
        bool Validate();
    };

}  // namespace rta

#endif  // RRA_BACKEND_BVH_ENCODED_BOTTOM_LEVEL_BVH_H_

