//=============================================================================
// Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  BVH base class definitions.
//=============================================================================

#ifndef RRA_BACKEND_BVH_IBVH_H_
#define RRA_BACKEND_BVH_IBVH_H_

#include <unordered_map>
#include <unordered_set>

#include "rdf/rdf/inc/amdrdf.h"

#include "bvh/gpu_def.h"
#include "bvh/metadata_v1.h"
#include "bvh/node_types/float16_box_node.h"
#include "bvh/node_types/float32_box_node.h"
#include "bvh/node_types/instance_node.h"
#include "bvh/node_types/procedural_node.h"
#include "bvh/node_types/triangle_node.h"
#include "bvh/parent_block.h"
#include "bvh/rtip31/child_info.h"
#include "bvh/rtip31/internal_node.h"
#include "bvh/rtip_common/i_acceleration_structure_header.h"

// RawAccelStruct currently supported version numbers.
#define GPURT_ACCEL_STRUCT_MAJOR_VERSION 16
#define GPURT_ACCEL_STRUCT_MINOR_VERSION 3
#define GPURT_ACCEL_STRUCT_VERSION ((GPURT_ACCEL_STRUCT_MAJOR_VERSION << 16) | GPURT_ACCEL_STRUCT_MINOR_VERSION)

namespace rta
{
    /// @brief Definition for minimum file size.
    constexpr std::uint32_t kMinimumFileSize = dxr::amd::kMetaDataAlignment + dxr::amd::kAccelerationStructureHeaderSize;

    enum class BvhNodeFlags
    {
        kNone,
        kIsInteriorNode = 1 << 0,
        kIsLeafNode     = 1 << 1
    };

    // Combines the encoding type and branching factor ranges (max and min child node reference count)
    // for interior and leaf nodes.
    struct BvhFormat
    {
        RayTracingIpLevel encoding;
        std::int32_t      interior_node_branching_factor;
        std::int32_t      leaf_node_branching_factor;
    };

    // Version of chunk files
    constexpr std::uint32_t kBvhChunkVersion = 4;

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

    enum class BvhBundleReadOption : std::uint8_t
    {
        // Load meta data, header, and the data containing the information of all
        // actually stored nodes. Skip all padding data.
        kIgnoreUnknown = 0,

        // Assume that there is no meta data stored.
        kNoMetaData = 0x1,

        // Skip the padding data by default.
        kDefault = kIgnoreUnknown
    };

    enum class ExportOption : std::uint8_t
    {
        kAll        = 0,
        kNoMetaData = 1,
        kDefault    = kAll
    };

    /// @brief We introduce an interface here as we might want to link across BVHs with
    /// different lead data types, and there's no easy way to express that if the
    /// types are completely unrelated.
    class IBvh
    {
    public:
        // Global identifier of acceleration structure data in chunk files.
        static constexpr const char* kAccelChunkIdentifier1 = "RawAccelStruc";
        static constexpr const char* kAccelChunkIdentifier2 = "RawAccelStruct";

        /// @brief Constructor.
        IBvh();

        /// @brief Destructor.
        virtual ~IBvh();

        /// @brief Set index (in tlas array) or memory address for meta data.
        ///
        /// @param The index or address.
        virtual void SetID(const std::uint64_t index_or_address);

        /// @brief Get index (in tlas array) or memory address for meta data.
        ///
        /// @return The index or address.
        virtual std::uint64_t GetID() const;

        /// @brief Get the format for this BVH.
        ///
        /// @return The BVH format.
        virtual BvhFormat GetFormat() const;

        /// @brief Get the number of interior/leaf nodes.
        ///
        /// @param [in] flag A BvhNodeFlags indicating which node count to return.
        ///
        /// @return The number of nodes.
        virtual std::uint32_t GetNodeCount(const BvhNodeFlags flag);

        /// @brief Does this BVH have BVH references?
        ///
        /// Generally, TLAS's have references, BLAS's do not, so this is a quick way of testing if
        /// the derived class is a TLAS or BLAS.
        ///
        /// @return true if the BVH has BVH references, false if not.
        virtual bool HasBvhReferences() const = 0;

        /// @brief Get the size of a buffer, in bytes.
        virtual std::uint64_t GetBufferByteSize() const;

        /// @brief Set the GPU virtual address for this BVH.
        ///
        /// @param [in] address The virtual address to set.
        virtual void SetVirtualAddress(const std::uint64_t address);

        /// @brief Get the GPU virtual address for this BVH.
        ///
        /// @return The virtual address.
        virtual std::uint64_t GetVirtualAddress() const;

        /// @brief Replace all absolute references with relative references.
        ///
        /// This includes replacing absolute VA's with index values for quick lookup.
        ///
        /// @param [in] reference_map A map of virtual addresses to the acceleration structure index.
        /// @param [in] map_self If true, the map is the same type as the acceleration structure ie a BLAS using the BLAS mapping.
        /// Setting to false can be used when a TLAS needs to use a BLAS mapping to fix up the instance nodes.
        virtual void SetRelativeReferences(const std::unordered_map<GpuVirtualAddress, std::uint64_t>& reference_map,
                                           bool                                                        map_self,
                                           std::unordered_set<GpuVirtualAddress>&                      missing_set);

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

        /// @brief Do the post-load step.
        ///
        /// This will be called once all the acceleration structures are loaded and fixed up. Tasks here include
        /// the surface area heuristic calculations.
        ///
        /// @return true if successful, false if error.
        virtual bool PostLoad() = 0;

        /// @brief Get the header for this acceleration structure.
        ///
        /// @return The header.
        virtual const IRtIpCommonAccelerationStructureHeader& GetHeader() const;

        /// @brief Get the surface area heuristic for a given leaf node.
        ///
        /// @param [in] node_ptr The leaf node whose SAH is to be found.
        ///
        /// @return The surface area heuristic.
        virtual float GetLeafNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr) const = 0;

        /// @brief Load the common BVH data from the file.
        ///
        /// @param [in] metadata_stream The metadata file stream.
        /// @param [in] bvh_stream The bvh file stream.
        /// @param [in] interior_node_buffer_size The size of the internal node buffer.
        /// @param [in] import_option Flags to indicate how to read the file.
        virtual void LoadBaseDataFromFile(rdf::Stream&              metadata_stream,
                                          rdf::Stream&              bvh_stream,
                                          const std::uint32_t       interior_node_buffer_size,
                                          const BvhBundleReadOption import_option);

        /// @brief Scan the tree to get the maximum and average tree depths.
        ///
        /// @return The number of leaf nodes.
        virtual uint32_t ScanTreeDepth();

        /// @brief Get the list of interior nodes for this acceleration structure.
        ///
        /// @return The interior nodes.
        virtual const std::vector<std::uint8_t>& GetInteriorNodesData() const;

        /// @brief Get the list of interior nodes for this acceleration structure.
        ///
        /// @return The interior nodes.
        virtual std::vector<std::uint8_t>& GetInteriorNodesData();

        /// @brief Get the surface area heuristic for a given interior node.
        ///
        /// @param [in] node_ptr The interior node whose SAH is to be found.
        ///
        /// @return The surface area heuristic.
        float GetInteriorNodeSurfaceAreaHeuristic(const dxr::amd::NodePointer node_ptr) const;

        /// @brief Compute the bounding box for a root node.
        ///
        /// These don't have bounding boxes so the bounding box is calculated from the bounding boxes of the child nodes.
        ///
        /// @param [in] box_node The root node.
        ///
        /// @return The bounding box.
        dxr::amd::AxisAlignedBoundingBox ComputeRootNodeBoundingBox(const dxr::amd::Float32BoxNode* box_node) const;

        /// @brief Compute the bounding box for a root node.
        ///
        /// These don't have bounding boxes so the bounding box is calculated from the bounding boxes of the child nodes.
        ///
        /// @param [in] box_node The root node.
        ///
        /// @return The bounding box.
        dxr::amd::AxisAlignedBoundingBox ComputeRootNodeBoundingBox(const QuantizedBVH8BoxNode* box_node) const;

        /// @brief Get the node's oriented bounding box index.
        ///
        /// @param [in] node_ptr The node to get the orientation of.
        ///
        /// @return The bounding box.
        uint32_t GetNodeObbIndex(const dxr::amd::NodePointer node_ptr) const;

        /// @brief Get the orientation of node's OBB.
        ///
        /// @param [in] node_ptr The node to get the orientation of.
        ///
        /// @return The bounding box.
        glm::mat3 GetNodeBoundingVolumeOrientation(const dxr::amd::NodePointer node_ptr) const;

        /// @brief Checks whether the BVH has been compacted.
        ///
        /// Here, compaction means that the memory consumed by the BVH fits the actually required memory,
        /// so that the BVH does not waste unnecessary memory space.
        ///
        /// @return true if compacted, false if not.
        bool IsCompacted() const;

        /// @brief Test if the BVH is empty according to the format specifications.
        ///
        /// @return true if the BVH is empty, false if not.
        bool IsEmpty() const;

        /// @brief Get the number of inactive instances in this BVH.
        ///
        /// @return The number of inactive instances.
        uint64_t GetInactiveInstanceCount() const;

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

        /// @brief Get the meta data for this acceleration structure.
        ///
        /// @return The meta data.
        const dxr::amd::MetaDataV1& GetMetaData() const;

        /// @brief Get a primitive node pointer at the specified index.
        ///
        /// Used to access the primitive (leaf) node data.
        ///
        /// @param [in] index  The index of the node pointer.
        ///
        /// @return Pointer to the primitive node pointer.
        const dxr::amd::NodePointer* GetPrimitiveNodePointer(int32_t index) const;

        /// @brief Get the header offset.
        /// Or equivalently the rest of the chunk after all the metadata.
        uint64_t GetHeaderOffset() const;

        /// @brief Get the parent node of the node passed in.
        ///
        /// @param [in] node_ptr The node whose parent is to be found.
        ///
        /// @return The parent node. If the node passed in is the root node, the
        /// parent node will be an invalid node.
        virtual dxr::amd::NodePointer GetParentNode(const dxr::amd::NodePointer* node_ptr) const = 0;

        /// @brief Traverse nodes to associate children with parents where necessary.
        virtual void PreprocessParents();

    protected:
        /// @brief Implementation for GetBufferByteSize() to be done by derived classes.
        ///
        /// @param [in] export_option Indicate which sections of the buffer should be counted.
        ///
        /// @return The buffer size.
        virtual std::uint64_t GetBufferByteSizeImpl(const ExportOption export_option) const = 0;

        std::uint64_t         id_          = UINT64_MAX;  ///< Index (in tlas or blas array) or memory address.
        dxr::amd::MetaDataV1  meta_data_   = {};          ///< Meta information, also defines byte offset to the real RtIpCommonAccelerationStructureHeader.
        dxr::amd::ParentBlock parent_data_ = {};          ///< Parent data containing the pointer to the parents of each node.
        std::unique_ptr<IRtIpCommonAccelerationStructureHeader> header_              = nullptr;  ///< Actual header of the acceleration structure.
        std::vector<std::uint8_t>                               interior_nodes_      = {};       ///< Interior nodes in bvh, bboxes are either FP32 or FP16.
        std::vector<std::uint8_t>                               sideband_data_       = {};       ///< Sideband data for rtip3.
        std::vector<dxr::amd::NodePointer>                      primitive_node_ptrs_ = {};       ///< Pointer to the leaf nodes.
        bool                                                    is_compacted_        = false;    ///< States whether this BVH was compacted or not.
        std::vector<float> box_surface_area_heuristic_                               = {};       ///< Surface area heuristic values for the interior box nodes.
        uint32_t           max_tree_depth_                                           = 0;        ///< The maximum depth of the BVH tree.
        uint32_t           avg_tree_depth_                                           = 0;        ///< The average depth of a triangle node in the BVH tree.
        uint64_t           gpu_virtual_address_                                      = 0;        ///< The GPU virtual address.
        uint64_t           header_offset_                                            = 0;        ///< The offset to the header, chunk base + metadata.

    private:
        /// @brief Derived class implementation of IsCompacted().
        ///
        /// @return true if compacted, false if not.
        virtual bool IsCompactedImpl() const;

        /// @brief Derived class implementation of IsEmpty().
        ///
        /// @return true if the BVH is empty, false if not.
        virtual bool IsEmptyImpl() const;

        /// @brief Derived class implementation of GetInactiveInstanceCount().
        ///
        /// @return The number of inactive instances.
        virtual uint64_t GetInactiveInstanceCountImpl() const;
    };
}  // namespace rta

#endif  // RRA_BACKEND_BVH_IBVH_H_

