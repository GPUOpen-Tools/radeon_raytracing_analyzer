cmake_minimum_required(VERSION 3.11)

project(Backend)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(AFTER ../backend)

IF(WIN32)
    # Warnings as errors for Windows
    add_compile_options(/W4 /WX)
ELSEIF(UNIX)
    # Remove warnings for Linux in backend. Unused function used by flags_util.h
    add_compile_options(-D_LINUX -Wall -Wextra -Werror -Wno-unused-function -Wno-class-memaccess)
ENDIF(WIN32)

# List of all source files. It may be possible to have the build process call cmake to update the makefiles
# only when this file has changed (ie source files have been added or removed)
set( SOURCES
    # common interface include files
    "public/rra_api_info.h"
    "public/rra_asic_info.h"
    "public/rra_assert.h"
    "public/rra_blas.h"
    "public/rra_bvh.h"
    "public/rra_error.h"
    "public/rra_macro.h"
    "public/rra_print.h"
    "public/rra_tlas.h"
    "public/rra_trace_loader.h"
    "public/rra_ray_history.h"
    "public/rra_async_ray_history_loader.h"

    # private backend files
    "api_info.cpp"
    "api_info.h"
    "asic_info.cpp"
    "asic_info.h"
    "math_util.cpp"
    "math_util.h"
    "rra_api_info.cpp"
    "rra_asic_info.cpp"
    "rra_assert.cpp"
    "rra_blas.cpp"
    "rra_blas_impl.h"
    "rra_bvh.cpp"
    "rra_bvh_impl.h"
    "rra_configuration.h"
    "rra_data_set.cpp"
    "rra_data_set.h"
    "rra_print.cpp"
    "rra_ray_history.cpp"
    "rra_tlas.cpp"
    "rra_tlas_impl.h"
    "rra_trace_loader.cpp"
    "rra_ray_history.cpp"
    "surface_area_heuristic.cpp"
    "surface_area_heuristic.h"
    "rra_async_ray_history_loader.cpp"

    # other dependencies
    "bvh/bvh_bundle.cpp"
    "bvh/bvh_bundle.h"
    "bvh/bvh_index_reference_map.cpp"
    "bvh/bvh_index_reference_map.h"
    "bvh/dxr_definitions.h"
    "bvh/dxr_type_conversion.cpp"
    "bvh/dxr_type_conversion.h"
    "bvh/encoded_rt_ip_11_bottom_level_bvh.cpp"
    "bvh/encoded_rt_ip_11_bottom_level_bvh.h"
    "bvh/encoded_rt_ip_11_top_level_bvh.cpp"
    "bvh/encoded_rt_ip_11_top_level_bvh.h"
    "bvh/flags_util.h"
    "bvh/geometry_info.cpp"
    "bvh/geometry_info.h"
    "bvh/gpu_def.h"
    "bvh/ibvh.cpp"
    "bvh/ibvh.h"
    "bvh/iencoded_rt_ip_11_bvh.cpp"
    "bvh/iencoded_rt_ip_11_bvh.h"
    "bvh/irt_ip_11_acceleration_structure_header.cpp"
    "bvh/irt_ip_11_acceleration_structure_header.h"
    "bvh/irt_ip_11_acceleration_structure_post_build_info.cpp"
    "bvh/irt_ip_11_acceleration_structure_post_build_info.h"
    "bvh/metadata_v1.cpp"
    "bvh/metadata_v1.h"
    "bvh/node_pointer.cpp"
    "bvh/node_pointer.h"
    "bvh/parent_block.cpp"
    "bvh/parent_block.h"
    "bvh/rt_binary_file_defs.cpp"
    "bvh/rt_binary_file_defs.h"
    "bvh/utils.cpp"
    "bvh/utils.h"
    "bvh/node_types/box_node.cpp"
    "bvh/node_types/box_node.h"
    "bvh/node_types/float16_box_node.cpp"
    "bvh/node_types/float16_box_node.h"
    "bvh/node_types/float32_box_node.cpp"
    "bvh/node_types/float32_box_node.h"
    "bvh/node_types/instance_node.cpp"
    "bvh/node_types/instance_node.h"
    "bvh/node_types/procedural_node.cpp"
    "bvh/node_types/procedural_node.h"
    "bvh/node_types/triangle_node.cpp"
    "bvh/node_types/triangle_node.h"

    # Ray history
    "ray_history/ray_history.h"
    "ray_history/ray_history.cpp"
    "ray_history/counter.h"
    "ray_history/raytracing_counter.h"
    "ray_history/loader.h"
    "ray_history/loader.cpp"
)

set( LINUX_SOURCES
    "public/linux/safe_crt.h"
    "linux/safe_crt.cpp"
)

add_definitions(-DRDF_CXX_BINDINGS)
IF (WIN32)
    add_definitions(-DRDF_PLATFORM_WINDOWS)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

ELSEIF(UNIX)
    add_definitions(-DRDF_PLATFORM_UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -mavx2 -mf16c")
ENDIF(WIN32)

# specify output library name
IF(WIN32)
    add_library(${PROJECT_NAME} ${SOURCES})
ELSEIF(UNIX)
    add_library(${PROJECT_NAME} ${SOURCES} ${LINUX_SOURCES})
ENDIF(WIN32)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

IF(WIN32)
# Create Visual Studio filters so that the source files in the project match the directory structure
foreach(source IN LISTS SOURCES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()
ENDIF(WIN32)

