//=============================================================================
// Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definition of an instance node class.
//=============================================================================

#ifndef RRA_BACKEND_BVH_NODE_TYPES_INSTANCE_NODE_H_
#define RRA_BACKEND_BVH_NODE_TYPES_INSTANCE_NODE_H_

#include "bvh/node_pointer.h"
#include "float32_box_node.h"

namespace dxr
{
    constexpr std::uint32_t kInstanceDescSize = 64;  ///< Expected size of Instance descriptor struct.

    /// @brief Defines the type of the instance node for RAW / BVH4 dumps.
    enum class InstanceDescType : std::uint32_t
    {
        kDecoded = 0,
        kRaw     = 1
    };

    /// @brief Defines the type of geometry used in the bottom-level BVH.
    enum class GeometryType : std::uint32_t
    {
        kTriangle = 0,
        kAABB     = 1
    };

    /// @brief Instance descriptions should be defined for each top-level BVHs.
    class InstanceDesc final
    {
    public:
        /// @brief Constructor.
        InstanceDesc(const InstanceDescType type = InstanceDescType::kRaw);

        /// @brief Destructor.
        ~InstanceDesc() = default;

        /// @brief Set instance ID and active mask.
        ///
        /// @param id The instance id.
        /// @param mask The instance mask.
        void SetInstanceIdAndMask(const std::uint32_t id, const std::uint32_t mask = 0xFF);

        /// @brief Set contribution to the hit group index and index flags.
        ///
        /// @param hit_group The hit group.
        /// @parma flags The instance flags.
        void SetHitGroupAndInstanceFlags(const std::uint32_t hit_group, const InstanceFlags flags);

        /// @brief Set only the address, keep the current flags.
        void SetBottomLevelBvhGpuVa(const rta::GpuVirtualAddress blas_address, const InstanceDescType type);

        /// @brief Define the hardware instance flags for the raw bvh dumps.
        ///
        /// @param instance_flags The instance flags to set.
        void SetHardwareInstanceFlags(const HardwareInstanceFlags instance_flags);

        /// @brief Define the geometry type of the leaf nodes in each blas.
        ///
        /// @param geometry_type The geometry type to set.
        void SetGeometryType(const GeometryType geometry_type);

        /// @brief Obtain the type of geometry from the instance pointer.
        ///
        /// @return The geometry type.
        GeometryType GetGeometryType() const;

        /// @brief Obtain the index of the current instance.
        ///
        /// @param The instance ID.
        std::uint32_t GetInstanceID() const;

        /// @brief Obtain the mask (in example, != 0 means this instance is active)
        ///
        /// @return The mask.
        std::uint32_t GetMask() const;

        /// @brief Check if the instance is active.
        ///
        /// @return True if active, false if not.
        bool IsActive() const;

        /// @brief Obtain the hit group index.
        ///
        /// @return The hit group.
        std::uint32_t GetHitGroup() const;

        /// @brief Obtain all flags of this instance.
        ///
        /// @return The instance flags.
        InstanceFlags GetInstanceFlags() const;

        /// @brief Obtain the blas address from the instance base pointer.
        ///
        /// @param type The instance descripto type.
        ///
        /// @return The blas virtual address.
        uint64_t GetBottomLevelBvhGpuVa(const InstanceDescType type) const;

        /// @brief Obtain the instance flags from the instance pointer.
        ///
        /// @return The hardware instance flags.
        HardwareInstanceFlags GetHardwareInstanceFlags() const;

        /// @brief Obtain the instance transformation matrix.
        ///
        /// @return The transform.
        const Matrix3x4& GetTransform() const;

        /// @brief Set the instance transform.
        ///
        /// @param transform_matrix The new transform to apply.
        void SetTransform(const Matrix3x4& transform_matrix);

    private:
        /// @brief Set the virtual address and store it in two 32-bit integers.
        void SetBottomLevelBvhAddress(const rta::GpuVirtualAddress bottom_level_bvh_address);

        /// @brief Divide a uint64_t integer to two 32-bit integers.
        void SetInstancePtr(const std::uint64_t instance_base_pointer);

        /// @brief Pack two uint32_t integers to a single 64-bit address.
        ///
        /// Store the high and low bits in uint32_t member variables.
        uint64_t GetInstancePtr() const;

        Matrix3x4 transform_ = {};  ///< Instance transform of geometry in blas.
        union                       ///< 24-bit instanceID and 8-bit mask.
        {
            struct
            {
                std::uint32_t instance_id : 24;
                std::uint32_t mask : 8;
            };
            std::uint32_t instance_id_and_mask_ = 0;
        };

        union  ///< 24-bit contribution and 8-bit flags.
        {
            struct
            {
                std::uint32_t contribution_to_hit_group : 24;
                std::uint32_t instance_flags : 8;
            };
            std::uint32_t contribution_to_hit_group_index_and_flags_ = 0;
        };

        std::uint32_t bottom_level_bvh_address_low_ = 0;  ///< Address / index of blas + flags
        union
        {
            struct
            {
                std::uint32_t bottom_level_bvh_address_high : 22;
                std::uint32_t hw_instance_flags : 8;
                std::uint32_t bottom_level_bvh_leaf_node_type : 2;
            };
            std::uint32_t bottom_level_bvh_address_and_flags_high_ = 0;
        };
    };

    namespace amd
    {
        class InstanceExtraData final
        {
        public:
            /// @brief Constructor.
            InstanceExtraData() = default;

            /// @brief Obtain the instance index.
            ///
            /// @return The instance index.
            std::uint32_t GetInstanceIndex() const;

            /// @brief Obtain the size of the bottom level BVH meta data.
            ///
            /// @return The metadata size, in bytes.
            std::uint32_t GetBottomLevelBvhMetaDataSize() const;

            /// @brief Obtain the instance transform.
            ///
            /// @return The instance transform.
            const Matrix3x4& GetOriginalInstanceTransform() const;

        private:
            std::uint32_t index_                         = 0;                                 ///< Index of this instance.
            NodePointer   bottom_level_bvh_node_pointer_ = {NodeType::kAmdNodeTriangle0, 0};  ///< The bottom level node pointer.

            std::uint32_t bottom_level_bvh_meta_data_size_ = 0;  ///< Size of blas meta data (important for addr computation of blas).
            std::uint32_t padding_                         = 0;  ///< Padding.

            Matrix3x4 original_instance_transform_ = {};  ///< Contains the original (non-inverted) pre-build instance transform.
        };

        /// @brief General structure of an instance node for Tlas.
        class InstanceNode final
        {
        public:
            /// @brief Constructor.
            InstanceNode() = default;

            /// @brief Destructor.
            ~InstanceNode() = default;

            /// @brief Obtain the Descriptor data.
            ///
            /// @return The instance descriptor.
            const InstanceDesc& GetDesc() const;

            /// @brief Obtain the Descriptor data.
            ///
            /// @return The instance descriptor.
            InstanceDesc& GetDesc();

            /// @brief Obtain the extra data.
            ///
            /// @return The extra data.
            const InstanceExtraData& GetExtraData() const;

            /// @brief Checks if the pointer to the blas and the blas meta data size is zero,
            /// as defined in the DXR 1.0 spec.
            ///
            /// @return true if inactive, false otherwise.
            bool IsInactive() const;

        private:
            InstanceDesc      desc_;        ///< Description of instance (id, ...), and pointer to the header of the blas.
            InstanceExtraData extra_data_;  ///< Extra data: inverse transform, blas meta data size, blas pointer, ...
        };

        /// @brief General structure of a fused instance node for Tlas.
        struct FusedInstanceNode
        {
            InstanceNode   instance_node          = {};     ///< The instance node data.
            Float32BoxNode bottom_level_root_node = {};     ///< The bottom level root node data.
        };

    }  // namespace amd
}  // namespace dxr

#endif  // RRA_BACKEND_BVH_NODE_TYPES_INSTANCE_NODE_H_
