#######################################################################################################################
### Copyright (c) 2020-2025 Advanced Micro Devices, Inc. All rights reserved.
### @author AMD Developer Tools Team
#######################################################################################################################

cmake_minimum_required(VERSION 3.11)
project(Renderer)

# switch on the autogenerators needed for Qt. Includes the MetaObject Compiler (MOC),
# the UI compiler (UIC) and the resource compiler (RCC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(AFTER ../backend)

IF (UNIX AND NOT APPLE)
    set(LINUX TRUE)
ENDIF ()

# Locate the Vulkan headers from the external dependencies directory.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../external/third_party/vulkan/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../external/qt_common/source)

if (NOT TARGET volk)
    # volk
    if (WIN32)
        set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
    elseif (LINUX)
        set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XCB_KHR)
    endif ()
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../external/third_party/volk volk)
endif()

IF (UNIX)
    find_package(Threads REQUIRED)

    # Include the qt version subdirectory path.
    include_directories(AFTER ${QT_NATIVE_PATH})
ENDIF (UNIX)

set(EXTERNAL_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../../external/vma/include/vk_mem_alloc.h"
)


set(SOURCES
        "camera.cpp"
        "graphics_context.cpp"
        "heatmap.cpp"
        "intersect.cpp"
        "orientation_gizmo.cpp"
        "renderer_interface.cpp"
        "renderer_widget.cpp"
        "shared.cpp"
        "vk/bounding_volume_mesh.cpp"
        "vk/bounding_volume_mesh.h"
        "vk/buffer_guard.cpp"
        "vk/buffer_guard.h"
        "vk/image_guard.cpp"
        "vk/image_guard.h"
        "vk/mesh.h"
        "vk/orientation_gizmo_mesh.cpp"
        "vk/orientation_gizmo_mesh.h"
        "vk/ray_history_offscreen_renderer.h"
        "vk/ray_history_offscreen_renderer.cpp"
        "vk/renderer_vulkan.cpp"
        "vk/renderer_vulkan.h"
        "vk/render_module.h"
        "vk/render_module.cpp"
        "vk/util_vulkan.cpp"
        "vk/util_vulkan.h"
        "vk/vk_graphics_context.h"
        "vk/vk_graphics_context.cpp"
        "vk/adapters/render_state_adapter.cpp"
        "vk/adapters/view_state_adapter.cpp"
        "vk/framework/command_buffer_ring.cpp"
        "vk/framework/command_buffer_ring.h"
        "vk/framework/device.cpp"
        "vk/framework/device.h"
        "vk/framework/device_properties.h"
        "vk/framework/device_properties.cpp"
        "vk/framework/ext_debug_utils.h"
        "vk/framework/ext_debug_utils.cpp"
        "vk/framework/ext_gpu_validation.h"
        "vk/framework/ext_gpu_validation.cpp"
        "vk/framework/ext_validation.h"
        "vk/framework/ext_validation.cpp"
        "vk/framework/instance.h"
        "vk/framework/instance.cpp"
        "vk/framework/instance_properties.h"
        "vk/framework/instance_properties.cpp"
        "vk/framework/swap_chain.cpp"
        "vk/framework/swap_chain.h"
        "vk/render_modules/checker_clear.h"
        "vk/render_modules/checker_clear.cpp"
        "vk/render_modules/bounding_volume.h"
        "vk/render_modules/bounding_volume.cpp"
        "vk/render_modules/mesh_render_module.h"
        "vk/render_modules/mesh_render_module.cpp"
        "vk/render_modules/orientation_gizmo_module.h"
        "vk/render_modules/orientation_gizmo_module.cpp"
        "vk/render_modules/ray_inspector_overlay.h"
        "vk/render_modules/ray_inspector_overlay.cpp"
        "vk/render_modules/selection_module.h"
        "vk/render_modules/selection_module.cpp"
        "vk/render_modules/traversal.h"
        "vk/render_modules/traversal.cpp"
        "public/camera.h"
        "public/include_vma.h"
        "public/shared.h"
        "public/heatmap.h"
        "public/renderer_widget.h"
        "public/renderer_interface.h"
        "public/renderer_adapter.h"
        "public/render_state_adapter.h"
        "public/view_state_adapter.h"
        "public/renderer_types.h"
        "public/graphics_context.h"
        "public/intersect.h"
        "public/orientation_gizmo.h"
        "shaders/shared_definitions.hlsl"
)

set(LINUX_SOURCES
)


set(SOURCES ${SOURCES} ${EXTERNAL_SOURCES})

# Specify output library name
IF (WIN32)
    add_library(${PROJECT_NAME} ${SOURCES})
    target_link_libraries(${PROJECT_NAME} Qt::Widgets RraBackend volk)
ELSEIF (UNIX)
    add_library(${PROJECT_NAME} ${SOURCES} ${LINUX_SOURCES})
    target_link_libraries(${PROJECT_NAME} Qt::Widgets Qt::GuiPrivate RraBackend volk ${CMAKE_DL_LIBS})
ENDIF (WIN32)

# Apply common developer tools target options and definitions
devtools_target_options(${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Set DXC path
IF (WIN32)
    set(DXC ${CMAKE_CURRENT_SOURCE_DIR}/../../external/third_party/dxc/bin/x64/dxc.exe)
ELSEIF (UNIX)
    set(DXC ${CMAKE_CURRENT_SOURCE_DIR}/../../external/third_party/dxc/bin/dxc)
ENDIF (WIN32)

# Set shader input and output directories
set(SHADERS_INPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../renderer/shaders/)
IF (WIN32)
    set(SHADERS_OUTPUT_DIR ${RRA_BINARY_DIR}/shaders/)
ELSEIF (UNIX)
    set(SHADERS_OUTPUT_DIR ${RRA_BINARY_DIR}/../shaders/)
ENDIF (WIN32)

# Set common hlsl files
set(COMMON_HLSL_FILES ${SHADERS_INPUT_DIR}/shared_definitions.hlsl ${SHADERS_INPUT_DIR}/shared_impl.hlsl ${SHADERS_INPUT_DIR}/Common.hlsl ${SHADERS_INPUT_DIR}/Heatmap.hlsl)

# HLSL shader compilation macro with DXC
macro(COMPILE_DXC_HLSL_SHADER shader_name entrypoint type output_name)
    set(OUTPUT_PATH ${SHADERS_OUTPUT_DIR}${output_name})

    IF (${entrypoint} STREQUAL "VSMain")
        set(EXTRA_COMPILER_FLAGS "-DVERTEX_SHADER")
    ELSEIF (${entrypoint} STREQUAL "PSMain")
        set(EXTRA_COMPILER_FLAGS "-DPIXEL_SHADER")
    ELSEIF (${entrypoint} STREQUAL "CSMain")
        set(EXTRA_COMPILER_FLAGS "-DCOMPUTE_SHADER")
    ENDIF ()

    set(HLSL_COMPILE_COMMAND ${DXC} -T ${type} ${EXTRA_COMPILER_FLAGS} ${SHADERS_INPUT_DIR}${shader_name} -spirv -E ${entrypoint} -Fo ${OUTPUT_PATH})

    add_custom_command(
            OUTPUT ${OUTPUT_PATH}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_OUTPUT_DIR}
            COMMAND ${HLSL_COMPILE_COMMAND}
            DEPENDS ${SHADERS_INPUT_DIR}/${shader_name} ${COMMON_HLSL_FILES}
    )

    set(SPV_FILES ${OUTPUT_PATH} ${SPV_FILES})
    set(HLSL_FILES ${SHADERS_INPUT_DIR}/${shader_name} ${HLSL_FILES})
endmacro()


# Compile HLSL shaders
COMPILE_DXC_HLSL_SHADER(TraversalShader.hlsl VSMain vs_6_4 TraversalShader.vs.spv)
COMPILE_DXC_HLSL_SHADER(TraversalShader.hlsl PSMain ps_6_4 TraversalShader.ps.spv)
COMPILE_DXC_HLSL_SHADER(TraversalShader.hlsl CSMain cs_6_4 TraversalShader.cs.spv)
COMPILE_DXC_HLSL_SHADER(rtip31/TraversalShaderOBB.hlsl VSMain vs_6_4 TraversalShaderOBB.vs.spv)
COMPILE_DXC_HLSL_SHADER(rtip31/TraversalShaderOBB.hlsl PSMain ps_6_4 TraversalShaderOBB.ps.spv)
COMPILE_DXC_HLSL_SHADER(rtip31/TraversalShaderOBB.hlsl CSMain cs_6_4 TraversalShaderOBB.cs.spv)
COMPILE_DXC_HLSL_SHADER(rtip31/TraversalShaderOBB.hlsl CSSubsample cs_6_4 TraversalShaderSubsample.cs.spv)
COMPILE_DXC_HLSL_SHADER(rtip31/GeometryColorPrimitiveStructure.hlsl VSMain vs_6_4 GeometryColorPrimitiveStructure.vs.spv)
COMPILE_DXC_HLSL_SHADER(rtip31/GeometryColorPrimitiveStructure.hlsl PSMain ps_6_4 GeometryColorPrimitiveStructure.ps.spv)
COMPILE_DXC_HLSL_SHADER(Clear.hlsl VSMain vs_6_4 Clear.vs.spv)
COMPILE_DXC_HLSL_SHADER(Clear.hlsl PSMain ps_6_4 Clear.ps.spv)
COMPILE_DXC_HLSL_SHADER(BoundingVolumeHierarchyWire.hlsl VSMain vs_6_4 BoundingVolumeHierarchyWire.vs.spv)
COMPILE_DXC_HLSL_SHADER(BoundingVolumeHierarchyWire.hlsl PSMain ps_6_4 BoundingVolumeHierarchyWire.ps.spv)
COMPILE_DXC_HLSL_SHADER(OrientationGizmo.hlsl VSMain vs_6_4 OrientationGizmo.vs.spv)
COMPILE_DXC_HLSL_SHADER(OrientationGizmo.hlsl PSMain ps_6_4 OrientationGizmo.ps.spv)
COMPILE_DXC_HLSL_SHADER(SelectionVolume.hlsl VSMain vs_6_4 SelectionVolume.vs.spv)
COMPILE_DXC_HLSL_SHADER(SelectionVolume.hlsl PSMain ps_6_4 SelectionVolume.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorTreeLevel.hlsl VSMain vs_6_4 GeometryColorTreeLevel.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorTreeLevel.hlsl PSMain ps_6_4 GeometryColorTreeLevel.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorBlasInstanceId.hlsl VSMain vs_6_4 GeometryColorBlasInstanceId.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorBlasInstanceId.hlsl PSMain ps_6_4 GeometryColorBlasInstanceId.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorGeometryIndex.hlsl VSMain vs_6_4 GeometryColorGeometryIndex.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorGeometryIndex.hlsl PSMain ps_6_4 GeometryColorGeometryIndex.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorOpacity.hlsl VSMain vs_6_4 GeometryColorOpacity.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorOpacity.hlsl PSMain ps_6_4 GeometryColorOpacity.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorFinalOpacity.hlsl VSMain vs_6_4 GeometryColorFinalOpacity.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorFinalOpacity.hlsl PSMain ps_6_4 GeometryColorFinalOpacity.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorLit.hlsl VSMain vs_6_4 GeometryColorLit.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorLit.hlsl PSMain ps_6_4 GeometryColorLit.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorTechnical.hlsl VSMain vs_6_4 GeometryColorTechnical.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorTechnical.hlsl PSMain ps_6_4 GeometryColorTechnical.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorBlasAverageSAH.hlsl VSMain vs_6_4 GeometryColorBlasAverageSAH.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorBlasAverageSAH.hlsl PSMain ps_6_4 GeometryColorBlasAverageSAH.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorBlasMinSAH.hlsl VSMain vs_6_4 GeometryColorBlasMinSAH.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorBlasMinSAH.hlsl PSMain ps_6_4 GeometryColorBlasMinSAH.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorBlasInstanceCount.hlsl VSMain vs_6_4 GeometryColorBlasInstanceCount.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorBlasInstanceCount.hlsl PSMain ps_6_4 GeometryColorBlasInstanceCount.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorBlasTriangleCount.hlsl VSMain vs_6_4 GeometryColorBlasTriangleCount.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorBlasTriangleCount.hlsl PSMain ps_6_4 GeometryColorBlasTriangleCount.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorBlasMaxDepth.hlsl VSMain vs_6_4 GeometryColorBlasMaxDepth.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorBlasMaxDepth.hlsl PSMain ps_6_4 GeometryColorBlasMaxDepth.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorBlasAverageDepth.hlsl VSMain vs_6_4 GeometryColorBlasAverageDepth.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorBlasAverageDepth.hlsl PSMain ps_6_4 GeometryColorBlasAverageDepth.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorInstanceIndex.hlsl VSMain vs_6_4 GeometryColorInstanceIndex.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorInstanceIndex.hlsl PSMain ps_6_4 GeometryColorInstanceIndex.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorInstanceMask.hlsl VSMain vs_6_4 GeometryColorInstanceMask.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorInstanceMask.hlsl PSMain ps_6_4 GeometryColorInstanceMask.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorTriangleIndex.hlsl VSMain vs_6_4 GeometryColorTriangleIndex.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorTriangleIndex.hlsl PSMain ps_6_4 GeometryColorTriangleIndex.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorWireframe.hlsl VSMain vs_6_4 GeometryColorWireframe.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorWireframe.hlsl PSMain ps_6_4 GeometryColorWireframe.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorTriangleSAH.hlsl VSMain vs_6_4 GeometryColorTriangleSAH.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorTriangleSAH.hlsl PSMain ps_6_4 GeometryColorTriangleSAH.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorPreferFastBuildOrTrace.hlsl VSMain vs_6_4 GeometryColorPreferFastBuildOrTrace.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorPreferFastBuildOrTrace.hlsl PSMain ps_6_4 GeometryColorPreferFastBuildOrTrace.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorAllowUpdateFlag.hlsl VSMain vs_6_4 GeometryColorAllowUpdateFlag.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorAllowUpdateFlag.hlsl PSMain ps_6_4 GeometryColorAllowUpdateFlag.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorAllowCompactionFlag.hlsl VSMain vs_6_4 GeometryColorAllowCompactionFlag.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorAllowCompactionFlag.hlsl PSMain ps_6_4 GeometryColorAllowCompactionFlag.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorLowMemoryFlag.hlsl VSMain vs_6_4 GeometryColorLowMemoryFlag.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorLowMemoryFlag.hlsl PSMain ps_6_4 GeometryColorLowMemoryFlag.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryInstanceFacingCullDisable.hlsl VSMain vs_6_4 GeometryInstanceFacingCullDisable.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryInstanceFacingCullDisable.hlsl PSMain ps_6_4 GeometryInstanceFacingCullDisable.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryInstanceFlipFacing.hlsl VSMain vs_6_4 GeometryInstanceFlipFacing.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryInstanceFlipFacing.hlsl PSMain ps_6_4 GeometryInstanceFlipFacing.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryInstanceForceOpaqueOrNoOpaque.hlsl VSMain vs_6_4 GeometryInstanceForceOpaqueOrNoOpaque.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryInstanceForceOpaqueOrNoOpaque.hlsl PSMain ps_6_4 GeometryInstanceForceOpaqueOrNoOpaque.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorRebraiding.hlsl VSMain vs_6_4 GeometryColorRebraiding.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorRebraiding.hlsl PSMain ps_6_4 GeometryColorRebraiding.ps.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorTriangleSplitting.hlsl VSMain vs_6_4 GeometryColorTriangleSplitting.vs.spv)
COMPILE_DXC_HLSL_SHADER(GeometryColorTriangleSplitting.hlsl PSMain ps_6_4 GeometryColorTriangleSplitting.ps.spv)
COMPILE_DXC_HLSL_SHADER(RayHistoryOffscreenRenderer.hlsl CSMain cs_6_4 RayHistoryOffscreenRenderer.cs.spv)
COMPILE_DXC_HLSL_SHADER(RayInspectorOverlay.hlsl VSMain vs_6_4 RayInspectorOverlay.vs.spv)
COMPILE_DXC_HLSL_SHADER(RayInspectorOverlay.hlsl PSMain ps_6_4 RayInspectorOverlay.ps.spv)
COMPILE_DXC_HLSL_SHADER(RayInspectorOverlayIcons.hlsl VSMain vs_6_4 RayInspectorOverlayIcons.vs.spv)
COMPILE_DXC_HLSL_SHADER(RayInspectorOverlayIcons.hlsl PSMain ps_6_4 RayInspectorOverlayIcons.ps.spv)

# Add common dependencies to HLSL files
set(HLSL_FILES ${COMMON_HLSL_FILES} ${HLSL_FILES})

# Prevent VS from assigning HLSL compiler to hlsl files
foreach (hlsl_file IN LISTS HLSL_FILES)
    set_source_files_properties(${hlsl_file} PROPERTIES VS_TOOL_OVERRIDE "None")
endforeach ()

# Add the shader target
add_custom_target(
        Shaders
        DEPENDS ${SPV_FILES} ${HLSL_FILES}
        SOURCES ${HLSL_FILES}
)

# Add Shaders as a dependency to the project
add_dependencies(${PROJECT_NAME} Shaders)

if (WIN32)
    install(DIRECTORY ${RRA_BINARY_DIR}/shaders DESTINATION . COMPONENT apps)
elseif(LINUX)
    install(DIRECTORY ${RRA_BINARY_DIR}/../shaders DESTINATION . COMPONENT apps)
endif()

# Copy the compiled shader binaries into the target output directories.
IF (WIN32)
    add_custom_command(TARGET Shaders POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Copying shader binaries"
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG}/shaders/
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}/shaders/
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${RRA_BINARY_DIR}/shaders/ ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG}/shaders/
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${RRA_BINARY_DIR}/shaders/ ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}/shaders/
    )
ELSEIF (UNIX)
    add_custom_command(TARGET Shaders POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Copying shader binaries"
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG}/shaders/
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}/shaders/
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${RRA_BINARY_DIR}/../shaders/ ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG}/shaders/
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${RRA_BINARY_DIR}/../shaders/ ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}/shaders/
    )
ENDIF (WIN32)

IF (WIN32)
    # Create Visual Studio filters so that the source files in the project match the directory structure
    foreach (source IN LISTS SOURCES)
        get_filename_component(source_path "${source}" PATH)
        string(REPLACE "/" "\\" source_path_msvc "${source_path}")
        source_group("${source_path_msvc}" FILES "${source}")
    endforeach ()
ENDIF (WIN32)

