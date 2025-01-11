//=============================================================================
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 3.1 (Navi4x) specific bottom level acceleration structure
/// definition.
//=============================================================================

#ifndef RRA_BACKEND_BVH_ENCODED_RT_IP_31_BOTTOM_LEVEL_BVH_H_
#define RRA_BACKEND_BVH_ENCODED_RT_IP_31_BOTTOM_LEVEL_BVH_H_

#include <array>
#include <utility>
#include <unordered_map>
#include "bvh/geometry_info.h"
#include "bvh/node_types/triangle_node.h"
#include "bvh/node_types/procedural_node.h"
#include "bvh/rtip31/primitive_node.h"
#include "bvh/rtip_common/encoded_bottom_level_bvh.h"

namespace rta
{
    class EncodedRtIp31BottomLevelBvh final : public EncodedBottomLevelBvh
    {
    public:
        // Global identifier of tlas dump in chunk files.
        static constexpr const char* kChunkIdentifier = "GpuEncBlasDump";

        /// @brief Default constructor.
        EncodedRtIp31BottomLevelBvh();

        /// @brief Destructor.
        virtual ~EncodedRtIp31BottomLevelBvh();

        virtual uint32_t GetLeafNodeCount() const override;

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

        /// @brief Load the BVH data from a file.
        ///
        /// @param [in] chunk_file        A Reference to a ChunkFile object which describes the file chunk being loaded.
        /// @param [in] chunk_index       The index of the chunk in the file.
        /// @param [in] header            The raw acceleration structure header.
        /// @param [in] chunk_identifier  The BVH chunk name.
        /// @param [in] import_option     Flag indicating which sections of the chunk to load/discard.
        ///
        /// @return true if the BVH data loaded successfully, false if not.
        bool LoadRawAccelStrucFromFile(rdf::ChunkFile&                     chunk_file,
                                       const std::uint64_t                 chunk_index,
                                       const RawAccelStructRdfChunkHeader& header,
                                       const char* const                   chunk_identifier,
                                       const BvhBundleReadOption           import_option) override;

        /// @brief Get the top-level surface area heuristic for this BLAS.
        ///
        /// @return The surface area heuristic.
        float GetSurfaceAreaHeuristic() const;

        /// @brief Set the top-level surface area heuristic for this BLAS.
        ///
        /// @param [in] surface_area_heuristic The surface area heuristic value to be set.
        void SetSurfaceAreaHeuristic(float surface_area_heuristic);

        /// @brief Get triangle pair indices of a triangle node.
        ///
        /// @param [in]  node_ptr  The node pointer.
        /// @param [out] out_count The number of triangle pair indices.
        ///
        /// @return List of pairs of (primitive_structure, triangle_pair_index).
        std::array<std::pair<PrimitiveStructure*, uint32_t>, 8> GetTrianglePairIndices(dxr::amd::NodePointer node_ptr, uint32_t* out_count);

        /// @brief Get triangle primitive structure offsets of a triangle node.
        ///
        /// @param [in]  node_ptr  The node pointer.
        /// @param [out] out_count The number of primitive structure offsets.
        ///
        /// @return List of PrimitiveStructure offsets corresponding to each triangle pair of node_ptr.
        std::array<uint32_t, 8> GetPrimitiveStructureOffsets(dxr::amd::NodePointer node_ptr, uint32_t* out_count);

        /// @brief Get the number of interior/leaf nodes.
        ///
        /// @param [in] flag A BvhNodeFlags indicating which node count to return.
        ///
        /// @return The number of nodes.
        std::uint32_t GetNodeCount(const BvhNodeFlags flag) override;

        /// @brief Do the post-load step.
        ///
        /// This will be called once all the acceleration structures are loaded and fixed up. Tasks here include
        /// the surface area heuristic calculations.
        ///
        /// @return true if successful, false if error.
        bool PostLoad() override;

        /// @brief Count the number of interior and leaf nodes. Needs to only be called once before calling GetNodeCount.
        void CountNodes();

        /// @brief Traverse the tree for compute leaf node surface area heuristics.
        void ComputeSurfaceAreaHeuristic();

        /// @brief Get the parent node of the node passed in.
        ///
        /// @param [in] node_ptr The node whose parent is to be found.
        ///
        /// @return The parent node. If the node passed in is the root node, the
        /// parent node will be an invalid node.
        virtual dxr::amd::NodePointer GetParentNode(const dxr::amd::NodePointer* node_ptr) const;

        /// @brief Traverse nodes to associate children with parents where necessary.
        virtual void PreprocessParents() override;

        /// @brief Get the surface area heuristic for a given leaf node.
        ///
        /// @param [in] node_ptr The leaf node whose SAH is to be found.
        ///
        /// @return The surface area heuristic.
        float GetLeafNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr) const override;

        /// @brief Set the surface area heuristic for a given leaf node.
        ///
        /// @param [in] node_ptr               The node pointer whose SAH is to be set.
        /// @param [in] surface_area_heuristic The surface area heuristic value to be set.
        void SetLeafNodeSurfaceAreaHeuristic(uint32_t node_ptr, float surface_area_heuristic);

    private:
        /// @brief Obtain the byte size of the encoded buffer.
        ///
        /// @param [in] import_option Flag indicating which sections of the chunk to load/discard.
        ///
        /// @return The buffer size.
        std::uint64_t GetBufferByteSizeImpl(const ExportOption export_option) const override;

        /// @brief Validate the loaded BVH data for accuracy where possible.
        ///
        /// @return true if data is valid, false otherwise.
        bool Validate();

        uint32_t                               interior_node_count_{};
        uint32_t                               leaf_node_count_{};
        std::unordered_map<uint32_t, uint32_t> triangle_node_parents_{};  // Pairs of (triangle_node_pointer, parent_pointer).
    };

}  // namespace rta

#endif  // RRA_BACKEND_BVH_ENCODED_RT_IP_31_BOTTOM_LEVEL_BVH_H_
