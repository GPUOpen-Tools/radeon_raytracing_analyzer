cmake_minimum_required(VERSION 3.11)
project(Renderer)

# switch on the autogenerators needed for Qt. Includes the MetaObject Compiler (MOC),
# the UI compiler (UIC) and the resource compiler (RCC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(AFTER ../backend)

IF(UNIX AND NOT APPLE)
    set(LINUX TRUE)
ENDIF()

# Locate the Vulkan headers from the external dependencies directory.
include_directories(${CMAKE_SOURCE_DIR}/external/vulkan/include/)

find_package(Qt5 COMPONENTS Core Gui Widgets Svg REQUIRED)

# volk
if(WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif(LINUX)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XCB_KHR)
endif()
add_subdirectory(${CMAKE_SOURCE_DIR}/external/volk volk)

IF(WIN32)
    # Warnings as errors for Windows
    add_compile_options(/W4 /WX)
ELSEIF(LINUX)
    # Remove warnings for Linux
    # TODO: Re-enable the Linux based warnings once we have a solution for VMA files. Windows side relies on pragma warning so it does not need to disable warnings.
    # add_compile_options(-D_LINUX -Wall -Wextra -Werror -Wno-missing-field-initializers -Wno-sign-compare -Wno-uninitialized)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    find_package(Threads REQUIRED)
ENDIF(WIN32)

set( EXTERNAL_SOURCES
    "${CMAKE_SOURCE_DIR}/external/vma/include/vk_mem_alloc.h"
)

set( SOURCES
    "public/camera.h"
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
    "shared.cpp"
    "camera.cpp"
    "heatmap.cpp"
    "renderer_widget.cpp"
    "renderer_interface.cpp"
    "graphics_context.cpp"
    "intersect.cpp"
    "orientation_gizmo.cpp"
    "vk/adapters/render_state_adapter.cpp"
    "vk/adapters/view_state_adapter.cpp"
    "vk/mesh.h"
    "vk/buffer_guard.cpp"
    "vk/buffer_guard.h"
    "vk/image_guard.cpp"
    "vk/image_guard.h"
    "vk/bounding_volume_mesh.cpp"
    "vk/bounding_volume_mesh.h"
    "vk/orientation_gizmo_mesh.cpp"
    "vk/orientation_gizmo_mesh.h"
    "vk/renderer_vulkan.cpp"
    "vk/renderer_vulkan.h"
    "vk/util_vulkan.cpp"
    "vk/util_vulkan.h"
    "vk/vk_graphics_context.h"
    "vk/vk_graphics_context.cpp"
    "vk/framework/device.cpp"
    "vk/framework/device.h"
    "vk/framework/command_buffer_ring.cpp"
    "vk/framework/command_buffer_ring.h"
    "vk/framework/swap_chain.cpp"
    "vk/framework/swap_chain.h"
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
    "vk/render_module.h"
    "vk/render_module.cpp"
    "vk/render_modules/mesh_render_module.h"
    "vk/render_modules/mesh_render_module.cpp"
    "vk/render_modules/checker_clear.h"
    "vk/render_modules/checker_clear.cpp"
    "vk/render_modules/bounding_volume.h"
    "vk/render_modules/bounding_volume.cpp"
    "vk/render_modules/traversal.h"
    "vk/render_modules/traversal.cpp"
    "vk/render_modules/orientation_gizmo_module.h"
    "vk/render_modules/orientation_gizmo_module.cpp"
    "vk/render_modules/selection_module.h"
    "vk/render_modules/selection_module.cpp"
)

set( LINUX_SOURCES
)

set(SOURCES ${SOURCES} ${EXTERNAL_SOURCES})

# Specify output library name
IF(WIN32)
    add_library(${PROJECT_NAME} ${SOURCES})
    target_link_libraries(${PROJECT_NAME} Qt5::Widgets Backend volk)
ELSEIF(UNIX)
    add_library(${PROJECT_NAME} ${SOURCES} ${LINUX_SOURCES})
    target_link_libraries(${PROJECT_NAME} Qt5::Widgets Backend volk ${CMAKE_DL_LIBS})
ENDIF(WIN32)

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Set shader input and output directories
set(SHADERS_INPUT_DIR ${CMAKE_SOURCE_DIR}/source/renderer/shaders/)

# Set common hlsl files
set(HLSL_FILES ${SHADERS_INPUT_DIR}/shared_definitions.hlsl ${SHADERS_INPUT_DIR}/shared_impl.hlsl)

# Prevent VS from assigning HLSL compiler to hlsl files
foreach(hlsl_file IN LISTS HLSL_FILES)
    set_source_files_properties(${hlsl_file} PROPERTIES VS_TOOL_OVERRIDE "None")
endforeach()

# Copy the compiled shader binaries into the target output directories.
IF(WIN32)
add_custom_command(TARGET Renderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying shader binaries"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG}/shaders/
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}/shaders/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders/ ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG}/shaders/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders/ ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}/shaders/
)
ELSEIF(UNIX)
add_custom_command(TARGET Renderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying shader binaries"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG}/shaders/
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}/shaders/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders/ ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG}/shaders/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders/ ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}/shaders/
)
ENDIF(WIN32)

IF(WIN32)
# Create Visual Studio filters so that the source files in the project match the directory structure
foreach(source IN LISTS SOURCES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()
ENDIF(WIN32)
