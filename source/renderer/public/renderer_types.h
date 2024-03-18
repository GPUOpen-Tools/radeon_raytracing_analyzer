//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  Declaration of types used in the renderer.
//=============================================================================

#ifndef RRA_RENDERER_TYPES_H_
#define RRA_RENDERER_TYPES_H_

#include <array>
#include <unordered_map>
#include <vector>
#include <map>
#include <string>

#include "public/shared.h"
#include "public/rra_bvh.h"

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#define NOMINMAX
#include <windows.h>
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#include <xcb/xcb.h>
#include <qpa/qplatformnativeinterface.h>
#include <QGuiApplication>
#endif

namespace rra
{
    namespace renderer
    {
        class RendererInterface;

        /// @brief Info related to the application window we're rendering to.
        struct WindowInfo
        {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
            HWND window_handle;  ///< The window handle used under Win32.
#elif defined(VK_USE_PLATFORM_XCB_KHR)
            bool                     quit = false;
            xcb_connection_t*        connection;             ///< The XCB window connection.
            xcb_screen_t*            screen;                 ///< The XCB screen.
            xcb_window_t             window;                 ///< The XCB window.
            xcb_intern_atom_reply_t* atom_wm_delete_window;  ///< The XCB window destruction callback.
#endif
        };

        /// @brief A single bounding volume instance.
        struct BoundingVolumeInstance
        {
            glm::vec4 min;       ///< The bounding volume minimum bounds in the XYZ components. The W component is the tree-level.
            glm::vec3 max;       ///< The bounding volume maximum bounds.
            glm::vec4 metadata;  ///< Packed metadata for the volume instance.
        };

        /// @brief A structure to represent the volume selection.
        struct SelectedVolumeInstance
        {
            glm::vec3 min          = {};
            glm::vec3 max          = {};
            uint32_t  is_transform = false;  /// uint type since this will be interpreted by a graphics device.
            glm::mat4 transform    = glm::mat4(1.0f);
        };

        /// @brief Data structure containing frustum info.
        struct FrustumInfo
        {
            glm::vec3 camera_position;         ///< The camera position.
            float     camera_fov;              ///< The camera fov.
            glm::mat4 camera_view_projection;  ///< The camera view projection matrix for frustum planes.
            float fov_threshold_ratio;  ///< The fov threshold ratio. Used to determine if volume is too small (ideally as small as a pixel after projection).
            glm::vec3 closest_point_to_camera;  ///< The closest point to camera.
        };

        /// @brief The transform and info for a single instance.
        class Instance
        {
        public:
            /// @brief Constructor.
            Instance() = default;

            /// @brief Destructor.
            ~Instance() = default;

            uint64_t              tlas_root_node{};               ///< The parent TLAS node id.
            uint64_t              blas_index{};                   ///< The blas index for this instance.
            uint32_t              build_flags{};                  ///< The build flags of the referenced blas.
            glm::mat4x4           transform{};                    ///< The instance transform.
            BoundingVolumeExtents bounding_volume{};              ///< The axis aligned bounding volume.
            uint32_t              bvh_start_instance_location{};  ///< The BVH instance start location.
            uint32_t              per_mesh_ubo_index{};           ///< The instance UBO index.
            uint32_t              instance_node{};                ///< The instance node.
            uint32_t              instance_unique_index{};        ///< The instance index which is unique even among rebraided instances.
            uint32_t              instance_index{};               ///< The instance index which is shared by rebraided instances.
            uint32_t              flags{};                        ///< The instance flags.
            uint32_t              depth{};                        ///< The depth for this specific instance.
            uint32_t              max_depth{};                    ///< The max depth for this instance.
            uint32_t              average_depth{};                ///< The average depth for this instance.
            uint32_t              mask{};                         ///< The instance mask. A mask of 0 means it's totally inactive.
            float                 min_triangle_sah{};             ///< The minimum triangle SAH in this instance.
            float                 average_triangle_sah{};         ///< The average triangle SAH in this instance.
            bool                  selected             = false;   ///< The flag to indicate if this instance is selected.
            bool                  use_custom_triangles = false;   ///< The flag to indicate that this instance should use custom triangles.
            bool                  rebraided            = false;   ///< Whether or not this instance was rebraided by the driver.
        };

        /// @brief Map of a RenderMesh instance to the instancing data used to draw it.
        typedef std::unordered_map<uint64_t, std::vector<Instance>> InstanceMap;

        enum class OrientationGizmoInstanceType
        {
            kCylinder = 0,
            kCircle   = 1,
            kX        = 2,
            kY        = 3,
            kZ        = 4,
        };

        /// @brief A single orientation gizmo instance.
        struct OrientationGizmoInstance
        {
            OrientationGizmoInstanceType type;         ///< Instance type.
            glm::vec4                    color;        ///< Color of instance.
            float                        fade_factor;  ///< Strength of depth fade effect (0.0 is none, 1.0 is full).
            glm::mat4                    transform;    ///< Transform of instance.
        };

        /// @brief The BoundingVolumeList is a resizeable array of bounding volumes.
        typedef std::vector<BoundingVolumeInstance> BoundingVolumeList;

        enum BvhTypeFlags : uint8_t
        {
            TopLevel    = 1 << 0,
            BottomLevel = 1 << 1,
            All         = TopLevel | BottomLevel
        };

        enum class GeometryColoringMode
        {
            kTreeLevel,
            kBlasInstanceId,
            kGeometryIndex,
            kOpacity,
            kFinalOpacity,
            kLit,
            kTechnical,
            kBlasAverageSAH,
            kBlasMinSAH,
            kTriangleSAH,
            kBlasInstanceCount,
            kBlasTriangleCount,
            kBlasMaxDepth,
            kBlasAverageDepth,
            kInstanceIndex,
            kInstanceMask,
            kCount,
            kFastBuildOrTraceFlag,
            kAllowUpdateFlag,
            kAllowCompactionFlag,
            kLowMemoryFlag,
            kInstanceFacingCullDisableBit,
            kInstanceFlipFacingBit,
            kInstanceForceOpaqueOrNoOpaqueBits,
            kInstanceRebraiding,
            kTriangleSplitting,
        };

        /// @brief Determines the plane of a 3D dispatch to be rendered in the ray history pane.
        enum SlicePlane
        {
            kSlicePlaneXY,
            kSlicePlaneXZ,
            kSlicePlaneYZ,
        };

        /// @brief Geometry color mode info structure.
        struct GeometryColoringModeInfo
        {
            GeometryColoringMode value;                ///< The coloring mode value.
            BvhTypeFlags         viewer_availability;  ///< Which viewer type is the coloring mode available in?
            std::string          name;                 ///< The coloring mode name.
            std::string          description;          ///< The coloring mode description.
        };

        /// @brief BVH color modes.
        enum class BVHColoringMode : uint32_t
        {
            VolumeType,
            TreeDepth
        };

        /// @brief BVH color mode info structure.
        struct BVHColoringModeInfo
        {
            BVHColoringMode value;        ///< The coloring mode value.
            std::string     name;         ///< The coloring mode name.
            std::string     description;  ///< The coloring mode description.
        };

        /// @brief Traversal counter modes.
        enum class TraversalCounterMode : uint32_t
        {
            TraversalLoopCount,
            InstanceHit,
            BoxVolumeHit,
            BoxVolumeMiss,
            BoxVolumeTest,
            TriangleHit,
            TriangleMiss,
            TriangleTest,
        };

        /// @brief Traversal counter mode info.
        struct TraversalCounterModeInfo
        {
            TraversalCounterMode value;                ///< The counter mode value.
            BvhTypeFlags         viewer_availability;  ///< Which viewer type is the coloring mode available in?
            std::string          name;                 ///< The counter mode name.
            std::string          description;          ///< The counter mode description.
        };

        typedef SceneUBO SceneUniformBuffer;

        /// @brief Structure for instance data.
        struct MeshInstanceData
        {
            glm::mat4x4 instance_transform;    ///< The world space transform.
            int32_t     instance_index;        ///< The instance index.
            uint32_t    instance_node;         ///< The instance node.
            uint32_t    flags;                 ///< The mesh instance flags.
            uint32_t    instance_count;        ///< The number of instances of the mesh.
            uint32_t    triangle_count;        ///< The triangle count for the mesh geometry.
            uint32_t    blas_index;            ///< The BLAS index.
            uint32_t    max_depth;             ///< The maximum depth.
            float       average_depth;         ///< The average depth.
            float       min_triangle_sah;      ///< The minimum triangle SAH in this instance.
            float       average_triangle_sah;  ///< The average triangle SAH in this instance.
            glm::vec4   wireframe_metadata;    ///< The wireframe metadata for the instance.
            uint32_t    build_flags;           ///< The build flags of the referenced blas.
            uint32_t    mask;                  ///< The instance mask flags.
            uint32_t    rebraided;             ///< Whether or not this instance is rebraided.
            uint32_t    selection_count;       ///< The number of selections on this instance index.
        };

        /// Traversal Rendering
        ///
        /// The traversal rendering is a small software ray tracing system.
        /// It requires a buffer set of three components:
        /// * Volumes
        /// * Addresses
        /// * Instances
        /// Since we don't know the number of childs in the node beforehand
        /// we can refer to a range in the Address buffer.
        ///
        /// Instance in this context is used to transform the ray such that it
        /// ends up in the local space of the next set of volume.

        /// @brief An enum to represent volume type in a traversal volume.
        enum class TraversalVolumeType : uint32_t
        {
            kDisabled = 0,
            kBox      = 1,
            kInstance = 2,
            kTriangle = 3,
        };

        /// @brief Structure to represent a volume in the traversal tree.
        struct TraversalVolume
        {
            glm::vec3 min             = {};  ///< The min bound for the volume.
            uint32_t  parent          = 0;   ///< The parend index.
            glm::vec3 max             = {};  ///< The maximum bound for the volume.
            int32_t   index_at_parent = -1;  ///< The index of this node inside the parent.

            TraversalVolumeType volume_type = TraversalVolumeType::kDisabled;  ///< The type of the volume.
            uint32_t            leaf_start  = 0;                               ///< The leaf start index. Leaf may be instance or triangle.
            uint32_t            leaf_end    = 0;                               ///< The leaf end index.

            int32_t  child_mask     = 0;   ///< The mask to represent which children are enabled.
            uint32_t child_nodes[4] = {};  ///< The child node indexes.

            glm::vec4 child_nodes_min[4] = {};  ///< The min bounds for the child nodes.
            glm::vec4 child_nodes_max[4] = {};  ///< The max bounds for the child nodes.
        };

        /// @brief Structure to represent a transform in the traversal tree.
        struct TraversalInstance
        {
            glm::mat4 transform         = glm::mat4(1.0f);  ///< The transform of the instance.
            glm::mat4 inverse_transform = glm::mat4(1.0f);  ///< The inverse transform of the instance.
            uint32_t  blas_index        = 0;                ///< The blas of this instance.
            uint32_t  geometry_index    = 0;                ///< The geometry inside the instance.
            uint32_t  selected          = 0;
            uint32_t  flags             = {};  ///< The instance flags to use in the traversal.
        };

        /// @brief Structure to represent the traversal tree as a whole.
        struct TraversalTree
        {
            std::vector<TraversalVolume>   volumes;    ///< The volumes of the structure.
            std::vector<RraVertex>         vertices;   ///< The aligned vertices of the volumes.
            std::vector<TraversalInstance> instances;  ///< The aligned instances under volumes.
        };

        /// @brief Structure to use as the result of ray traversal.
        struct TraversalResult
        {
            uint32_t counter;
            uint32_t hit_flags;
            uint32_t instance_index;
            uint32_t triangle_index;
            uint32_t blas_index;
            uint32_t exit_properly;
            uint32_t placeholder_2;
            uint32_t placeholder_3;
        };

        enum RayHistoryColorMode
        {
            kRayHistoryColorModeRayCount,
            kRayHistoryColorModeTraversalCount,
            kRayHistoryColorModeInstanceIntersectionCount,
            kRayHistoryColorModeAnyHitInvocationCount,
            kRayHistoryColorModeRayDirection,
        };

        // Ray history dispatch ID data.
        struct DispatchIdData
        {
            uint32_t ray_count;
            uint32_t traversal_count;
            uint32_t instance_intersection_count;
            uint32_t any_hit_invocation_count;
            uint32_t first_ray_index;
        };

        struct DispatchRayData
        {
            glm::vec3 direction;
            float     placeholder;
        };

        struct RayInspectorRay
        {
            glm::vec3 origin = {};
            float     tmin   = 0.0;

            glm::vec3 direction = {};
            float     tmax      = 0.0;

            glm::vec4 color = {};

            int   is_outline   = 0;
            float hit_distance = -1.0;  // -1.0 means no hit.

            int ray_flags = {};

            uint64_t tlas_address = 0;
            uint32_t cull_mask    = 0;
            uint32_t padding      = {};
        };

    }  // namespace renderer
}  // namespace rra

#endif  // RRA_RENDERER_TYPES_H_
