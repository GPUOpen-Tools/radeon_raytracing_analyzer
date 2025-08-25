#######################################################################################################################
### Copyright (c) 2020-2025 Advanced Micro Devices, Inc. All rights reserved.
### @author AMD Developer Tools Team
#######################################################################################################################

cmake_minimum_required(VERSION 3.11)
project(RraBackendTest)

find_package (Threads)

add_definitions(-DSYSTEM_INFO_ENABLE_RDF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(AFTER
    ../../../source/backend
    ../../../source/renderer/
    ../../../external/third_party/catch2/single_include/catch2
    ../../../source/frontend)

if(WIN32)
    set(ADDITIONAL_COMPILE_OPTIONS ${ADDITIONAL_COMPILE_OPTIONS}
                                     -wd4389)
    # the test fails to run when built with incremental linking -- disable it for debug builds (where by default it is enabled)
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO")
endif()

# specify output executable name
add_executable(${PROJECT_NAME})

# List of all source files. It may be possible to have the build process call cmake to update the makefiles
# only when this file has changed (ie source files have been added or removed)
target_sources(${PROJECT_NAME} PRIVATE
    "main.cpp"
    "log.cpp"
    "log.h"
    "rra_test_asic_chunk.cpp"
    "rra_test_asic_chunk.h"
    "rra_test_base.cpp"
    "rra_test_base.h"
    "rra_test_blas.cpp"
    "rra_test_blas.h"
    "rra_test_cases.cpp"
    "rra_test_cases.h"
    "rra_test_config.h"
    "rra_test_file_chunks.cpp"
    "rra_test_file_chunks.h"
    "rra_test_hlsl.cpp"
    "rra_test_hlsl.h"
    "rra_test_ray_history.cpp"
    "rra_test_ray_history.h"
    "rra_test_tlas.cpp"
    "rra_test_tlas.h"
    "rra_test_tlas_blas_mapping.cpp"
    "rra_test_tlas_blas_mapping.h"
    "rra_test_system_info_chunk.cpp"
    "rra_test_system_info_chunk.h"
    "tests.cpp"

    # Get shared code from the renderer.
    "../../../source/renderer/shared.cpp"
)

if (WIN32)
    target_sources (${PROJECT_NAME} PRIVATE
        "windows/backend_test.rc"
        "windows/resource.h"
    )
endif()

if (WIN32)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION . OPTIONAL COMPONENT debug)
endif()

# Apply common developer tools target options and definitions
devtools_target_options(${PROJECT_NAME})

# CMAKE_<CONFIG>_POSTFIX isn't applied automatically to executable targets so apply manually
IF(CMAKE_DEBUG_POSTFIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
ENDIF(CMAKE_DEBUG_POSTFIX)
IF(CMAKE_RELEASE_POSTFIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES RELEASE_POSTFIX ${CMAKE_RELEASE_POSTFIX})
ENDIF(CMAKE_RELEASE_POSTFIX)

IF (UNIX)
    target_link_libraries(${PROJECT_NAME} RraBackend system_info rdf Threads::Threads dl)
ELSE()
    target_link_libraries(${PROJECT_NAME} RraBackend system_info rdf Threads::Threads)
ENDIF()

IF (WIN32 OR APPLE)
SOURCE_GROUP_BY_FOLDER(${PROJECT_NAME})
ENDIF()

