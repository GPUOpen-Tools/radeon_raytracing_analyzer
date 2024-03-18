//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  RT IP 1.1 (Navi2x) specific base class definition.
//=============================================================================

#ifndef RRA_BACKEND_BVH_IENCODED_RT_IP_11_BVH_H_
#define RRA_BACKEND_BVH_IENCODED_RT_IP_11_BVH_H_

#include "bvh/ibvh.h"

#include "bvh/rtip11/irt_ip_11_acceleration_structure_header.h"
#include "bvh/node_types/float16_box_node.h"
#include "bvh/node_types/float32_box_node.h"
#include "bvh/node_types/instance_node.h"
#include "bvh/node_types/procedural_node.h"
#include "bvh/node_types/triangle_node.h"
#include "bvh/metadata_v1.h"
#include "bvh/parent_block.h"

#include "rdf/rdf/inc/amdrdf.h"

namespace rta
{
    // Miscellaneous flags describing the acceleration structure.
    union RdfAccelStructHeaderFlags
    {
        struct
        {
            std::uint32_t blas : 1;       // Indicates a bottom level acceleration structure
            std::uint32_t reserved : 31;  // Unused bits
        };

        std::uint32_t u32All;
    };

    struct RawAccelStructRdfChunkHeader
    {
        // GPUVA where the application built the acceleration structure
        std::uint32_t accel_struct_base_va_lo;
        std::uint32_t accel_struct_base_va_hi;

        // Metadata header (AccelStructMetadataHeader)
        std::uint32_t meta_header_offset;  // Offset of the driver metadata header from the start of the chunk payload
        std::uint32_t meta_header_size;    // Size of the driver metadata header

        // Primary Header (AccelStructHeader)
        std::uint32_t header_offset;  // Offset of the driver header from the start of the chunk payload
        std::uint32_t header_size;    // Size of the driver header

        RdfAccelStructHeaderFlags flags;  // Miscellaneous flags
    };

    // 180 Bytes
    // Base of all RtIp 1.1 chunk headers based on the first header version.
    struct RtIp11ChunkHeaderBase
    {
        RtIp11ChunkHeaderBase() = default;

        dxr::amd::MetaDataV1                                                 meta_data = {};
        std::array<std::uint8_t, dxr::amd::kAccelerationStructureHeaderSize> header    = {};

        // This index used to retrieve the virtual GPU addresses
        // of the tlas from the index map, or contains the actual memory address.
        std::uint32_t id_low  = UINT32_MAX;
        std::uint32_t id_high = UINT32_MAX;

        std::uint32_t parent_data_size          = 0;  ///< Size of parent data.
        std::uint32_t interior_node_buffer_size = 0;  ///< Size of the internal node buffer.
    };

    // New specific entries in the chunk header.
    struct RtIp11ChunkHeader : RtIp11ChunkHeaderBase
    {
        // Stores the exact BVH format and encoding (Amd RT IP 1.1)
        // Interior node branching factor is required for the
        // box node per interior node count.
        // Maybe also for later if we need to interpret this differently
        BvhFormat format = {RayTracingIpLevel::RtIp1_1, 0, 0 /*, 4, 1*/};
    };

    // Fixed byte sizes for the RT IP 1.1 chunk headers.
    constexpr std::uint32_t kTopLevelRtIp11BvhChunkHeaderSize    = 196;
    constexpr std::uint32_t kBottomLevelRtIp11BvhChunkHeaderSize = 208;

    enum class ExportOption : std::uint8_t
    {
        kAll        = 0,
        kNoMetaData = 1,
        kDefault    = kAll
    };

    /// @brief Base class for a ray-tracing IP 1.1-based BVH. This corresponds to Navi2x ray tracing.
    class IEncodedRtIp11Bvh : public IBvh
    {
    public:
        /// @brief Definition for minimum file size.
        static constexpr std::uint32_t kMinimumFileSize = dxr::amd::kMetaDataAlignment + dxr::amd::kAccelerationStructureHeaderSize;

        /// @brief Default constructor.
        IEncodedRtIp11Bvh();

        /// @brief Destructor.
        virtual ~IEncodedRtIp11Bvh();

        /// @brief Set index (in tlas array) or memory address for meta data.
        ///
        /// @param The index or address.
        void SetID(const std::uint64_t index_or_address);

        /// @brief Get index (in tlas array) or memory address for meta data.
        ///
        /// @return The index or address.
        std::uint64_t GetID() const;

        /// @brief Set the GPU virtual address for this BVH.
        ///
        /// @param [in] address The virtual address to set.
        virtual void SetVirtualAddress(const std::uint64_t address) override;

        /// @brief Get the GPU virtual address for this BVH.
        ///
        /// @return The virtual address.
        std::uint64_t GetVirtualAddress() const;

        /// @brief Get the meta data for this acceleration structure.
        ///
        /// @return The meta data.
        const dxr::amd::MetaDataV1& GetMetaData() const;

        /// @brief Get the header for this acceleration structure.
        ///
        /// @return The header.
        const IRtIp11AccelerationStructureHeader& GetHeader() const;

        /// @brief Get the parent data for this acceleration structure.
        ///
        /// @return The parent data.
        const dxr::amd::ParentBlock& GetParentData() const;

        /// @brief Get the list of interior nodes for this acceleration structure.
        ///
        /// @return The interior nodes.
        const std::vector<std::uint8_t>& GetInteriorNodesData() const;

        /// @brief Get the list of interior nodes for this acceleration structure.
        ///
        /// @return The interior nodes.
        std::vector<std::uint8_t>& GetInteriorNodesData();

        /// @brief Is this acceleration structure compacted.
        ///
        /// @return true if compacted, false if not.
        bool IsCompacted() const;

        /// @brief Get the size of the BVH data.
        ///
        /// @return The buffer size.
        std::uint64_t GetBufferByteSize() const override;

        /// @brief Get the BVH format.
        ///
        /// @return The format of this BVH.
        BvhFormat GetFormat() const override;

        /// @brief Get the number of nodes in the BVH.
        ///
        /// @param [in] flag A flag indicating which node count to return (leaf/interior).
        ///
        /// @return The node count.
        std::uint32_t GetNodeCount(const BvhNodeFlags flag) const override;

        /// @brief Load the BVH data from a file.
        ///
        /// @param [in] chunk_file        A Reference to a ChunkFile object which describes the file chunk being loaded.
        /// @param [in] chunk_index       The index of the chunk in the file.
        /// @param [in] header            The raw acceleration structure header.
        /// @param [in] chunk_identifier  The BVH chunk name.
        /// @param [in] import_option     Flag indicating which sections of the chunk to load/discard.
        ///
        /// @return true if the BVH data loaded successfully, false if not.
        virtual bool LoadRawAccelStrucFromFile(rdf::ChunkFile&                     chunk_file,
                                               const std::uint64_t                 chunk_index,
                                               const RawAccelStructRdfChunkHeader& header,
                                               const char* const                   chunk_identifier,
                                               const BvhBundleReadOption           import_option) = 0;

        /// @brief Replace all absolute references with relative references.
        ///
        /// This includes replacing absolute VA's with index values for quick lookup.
        ///
        /// @param [in] reference_map A map of virtual addresses to the acceleration structure index.
        /// @param [in] map_self If true, the map is the same type as the acceleration structure ie a BLAS using the BLAS mapping.
        /// Setting to false can be used when a TLAS needs to use a BLAS mapping to fix up the instance nodes.
        void SetRelativeReferences(const std::unordered_map<GpuVirtualAddress, std::uint64_t>& reference_map,
                                   bool                                                        map_self,
                                   std::unordered_set<GpuVirtualAddress>&                      missing_set) override;

        /// @brief Compute the bounding box for a root node.
        ///
        /// These don't have bounding boxes so the bounding box is calculated from the bounding boxes of the child nodes.
        ///
        /// @param [in] box_node The root node.
        ///
        /// @return The bounding box.
        dxr::amd::AxisAlignedBoundingBox ComputeRootNodeBoundingBox(const dxr::amd::Float32BoxNode* box_node) const;

        /// @brief Get the parent node of the node passed in.
        ///
        /// @param [in] node_ptr The node whose parent is to be found.
        ///
        /// @return The parent node. If the node passed in is the root node, the
        /// parent node will be an invalid node.
        dxr::amd::NodePointer GetParentNode(const dxr::amd::NodePointer* node_ptr) const;

        /// @brief Get the surface area heuristic for a given leaf node.
        ///
        /// To be implemented in derived classes.
        ///
        /// @param [in] node_ptr The leaf node whose SAH is to be found.
        ///
        /// @return The surface area heuristic.
        virtual float GetLeafNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr) const = 0;

        /// @brief Get the surface area heuristic for a given interior node.
        ///
        /// @param [in] node_ptr The interior node whose SAH is to be found.
        ///
        /// @return The surface area heuristic.
        float GetInteriorNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr) const;

        /// @brief Get the maximum tree depth of this BVH.
        ///
        /// @return The maximum tree depth.
        uint32_t GetMaxTreeDepth() const;

        /// @brief Get the average tree depth for a triangle node in this BVH.
        ///
        /// @return The average tree depth.
        uint32_t GetAvgTreeDepth() const;

        /// @brief Set the surface area heuristic for a given interior node.
        ///
        /// @param [in] node_ptr               The interior node whose SAH is to be set.
        /// @param [in] surface_area_heuristic The surface area heuristic value to be set.
        void SetInteriorNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr, float surface_area_heuristic);

        /// @brief Get the box32 node associated with the node pointer.
        ///
        /// Assumes that the node pointer passed in is a box32 node.
        ///
        /// @param node_pointer The node pointer.
        /// @param offset       Defines which node should be returned. Offset 0 => the one referenced by node_pointer, 1 => the node behind the node with offset 0.
        const dxr::amd::Float32BoxNode* GetFloat32Box(const dxr::amd::NodePointer node_pointer, const int offset = 0) const;

        /// @brief Get the box16 node associated with the node pointer.
        ///
        /// Assumes that the node pointer passed in is a box16 node.
        ///
        /// @param node_pointer The node pointer.
        /// @param offset       Defines which node should be returned. Offset 0 => the one referenced by node_pointer, 1 => the node behind the node with offset 0.
        const dxr::amd::Float16BoxNode* GetFloat16Box(const dxr::amd::NodePointer node_pointer, const int offset = 0) const;

        /// @brief Get the instance node associated with the node pointer.
        ///
        /// Assumes that the node pointer passed in is an instance node.
        ///
        /// @param node_pointer The node pointer.
        /// @param offset       Defines which node should be returned. Offset 0 => the one referenced by node_pointer, 1 => the node behind the node with offset 0.
        const dxr::amd::InstanceNode* GetInstanceNode(const dxr::amd::NodePointer node_pointer, const int offset = 0) const;

        /// @brief Get the triangle node associated with the node pointer.
        ///
        /// Assumes that the node pointer passed in is a triangle node.
        ///
        /// @param node_pointer The node pointer.
        /// @param offset       Defines which node should be returned. Offset 0 => the one referenced by node_pointer, 1 => the node behind the node with offset 0.
        const dxr::amd::TriangleNode* GetTriangleNode(const dxr::amd::NodePointer node_pointer, const int offset = 0) const;

        /// @brief Get the procedural node associated with the node pointer.
        ///
        /// Assumes that the node pointer passed in is a procedural node.
        ///
        /// @param node_pointer The node pointer.
        /// @param offset       Defines which node should be returned. Offset 0 => the one referenced by node_pointer, 1 => the node behind the node with offset 0.
        const dxr::amd::ProceduralNode* GetProceduralNode(const dxr::amd::NodePointer node_pointer, const int offset = 0) const;

        /// @brief Get a primitive node pointer at the specified index.
        ///
        /// Used to access the primitive (leaf) node data.
        ///
        /// @param [in] index  The index of the node pointer.
        ///
        /// @return Pointer to the primitive node pointer.
        const dxr::amd::NodePointer* GetPrimitiveNodePointer(int32_t index) const;

    protected:
        /// @brief Scan the tree to get the maximum and average tree depths.
        void ScanTreeDepth();

        /// @brief Load the common BVH data from the file.
        ///
        /// @param [in] metadata_stream The metadata file stream.
        /// @param [in] bvh_stream The bvh file stream.
        /// @param [in] interior_node_buffer_size The size of the internal node buffer.
        /// @param [in] import_option Flags to indicate how to read the file.
        void LoadBaseDataFromFile(rdf::Stream&              metadata_stream,
                                  rdf::Stream&              bvh_stream,
                                  const std::uint32_t       interior_node_buffer_size,
                                  const BvhBundleReadOption import_option);

        /// @brief Implementation for GetBufferByteSize() to be done by derived classes.
        ///
        /// @param [in] export_option Indicate which sections of the buffer should be counted.
        ///
        /// @return The buffer size.
        virtual std::uint64_t GetBufferByteSizeImpl(const ExportOption export_option) const = 0;

        std::uint64_t         id_          = UINT64_MAX;  ///< Index (in tlas or blas array) or memory address.
        dxr::amd::MetaDataV1  meta_data_   = {};          ///< Meta information, also defines byte offset to the real AccelerationStructureHeader.
        dxr::amd::ParentBlock parent_data_ = {};          ///< Parent data containing the pointer to the parents of each node.
        std::unique_ptr<IRtIp11AccelerationStructureHeader> header_                     = nullptr;  ///< Actual header of the acceleration structure.
        std::vector<std::uint8_t>                           interior_nodes_             = {};       ///< Interior nodes in bvh, bboxes are either FP32 or FP16.
        std::vector<dxr::amd::NodePointer>                  primitive_node_ptrs_        = {};       ///< Pointer to the leaf nodes.
        bool                                                is_compacted_               = false;    ///< States whether this BVH was compacted or not.
        std::vector<float>                                  box_surface_area_heuristic_ = {};  ///< Surface area heuristic values for the interior box nodes.
        uint32_t                                            max_tree_depth_             = 0;   ///< The maximum depth of the BVH tree.
        uint32_t                                            avg_tree_depth_             = 0;   ///< The average depth of a triangle node in the BVH tree.
        uint64_t                                            gpu_virtual_address_        = 0;   ///< The GPU virtual address.

    private:
        /// @brief Is this acceleration structure compacted.
        ///
        /// @return true if compacted, false if not.
        bool IsCompactedImpl() const override;

        /// @brief Test if the BVH is empty according to the format specifications.
        ///
        /// @return true if the BVH is empty, false if not.
        bool IsEmptyImpl() const override;

        /// @brief Derived class implementation of GetInactiveInstanceCount().
        ///
        /// @return The number of inactive instances.
        virtual uint64_t GetInactiveInstanceCountImpl() const override;
    };

}  // namespace rta

#endif  // RRA_BACKEND_BVH_IENCODED_RT_IP_11_BVH_H_
