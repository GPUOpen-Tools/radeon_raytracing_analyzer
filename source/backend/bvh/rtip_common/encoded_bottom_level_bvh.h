//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Bottom level acceleration structure not specific to rt ip level.
//=============================================================================

#ifndef RRA_BACKEND_BVH_ENCODED_BOTTOM_LEVEL_BVH_H_
#define RRA_BACKEND_BVH_ENCODED_BOTTOM_LEVEL_BVH_H_

#include "bvh/geometry_info.h"
#include "bvh/node_types/triangle_node.h"
#include "bvh/node_types/procedural_node.h"
#include "bvh/ibvh.h"
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

    protected:
        std::vector<dxr::amd::GeometryInfo> geom_infos_     = {};     ///< Array of geometry info.
        std::vector<float> triangle_surface_area_heuristic_ = {};     ///< Surface area heuristic values for the triangles. Pairs of (node_ptr, SAH).
        float              surface_area_heuristic_          = 0.0f;   ///< The precalculated Surface area heuristic for this BLAS.
        bool               is_procedural_                   = false;  ///< Whether or not this BLAS contains procedural nodes in oppose to triangle nodes.

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
    };

}  // namespace rta

#endif  // RRA_BACKEND_BVH_ENCODED_BOTTOM_LEVEL_BVH_H_
