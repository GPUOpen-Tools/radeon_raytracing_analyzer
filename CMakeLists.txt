#######################################################################################################################
### Copyright (c) 2020-2025 Advanced Micro Devices, Inc. All rights reserved.
### @author AMD Developer Tools Team
#######################################################################################################################

cmake_minimum_required(VERSION 3.11)

set(SYSTEM_INFO_BUILD_RDF_INTERFACES ON)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(dev_tools)

## Specify the top level name of the project - this will define the solution name for Visual Studio
project(RRA)

# Define version information
set(RRA_MAJOR_VERSION 1)
set(RRA_MINOR_VERSION 9)
if (NOT RRA_BUGFIX_NUMBER)
    set(RRA_BUGFIX_NUMBER 0)
endif ()
if (NOT RRA_BUILD_NUMBER)
    set(RRA_BUILD_NUMBER 0)
endif ()
string(TIMESTAMP DATE "\"%m/%d/%Y\"")
string(TIMESTAMP YEAR "%Y")
string(TIMESTAMP YEAR_STRING "\"%Y\"")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Buildinfo.properties.in" "${CMAKE_CURRENT_SOURCE_DIR}/Buildinfo.properties")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/source/frontend/version.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/source/frontend/version.h")

# Add cmake utilities
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(devtools_qt_helper)
include(dev_tools)

option(RDF_ENABLE_CXX_BINDINGS "Allow usage of C++ interface for RDF library" ON)
option(RDF_STATIC              "Build RDF as a static library" ON)

## For RRA we only care about the Debug and Release configuration types
set(CMAKE_CONFIGURATION_TYPES Debug Release)

## Determine build suffixes based on configuration and bitness status
## These values will be inherited by all child projects
set(ADT_PLATFORM_POSTFIX "-x86")
IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ADT_PLATFORM_POSTFIX "-x64")
ENDIF()

# As default for RRA, include the debug status in filename - but not the platform bitness
set (CMAKE_DEBUG_POSTFIX -d)
set (CMAKE_RELEASE_POSTFIX )


# Add definitions for supported hardware
add_definitions(-DGPURT_BUILD_RTIP3)
add_definitions(-DGPURT_BUILD_RTIP3_1)

# Add definitions needed by GLM
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

# Add for CentOS compiler warning
add_definitions(-DJSON_SKIP_UNSUPPORTED_COMPILER_CHECK)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/qt_common/")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/third_party/")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/third_party/vulkan/include/")

# Global compiler options
IF(WIN32)
    # bump the stack size
    add_link_options(/STACK:16777216)
ELSEIF(UNIX)
    add_compile_options(-mno-avx2)
    # Allow executable to be double clicked.
    add_link_options(-no-pie)
    # Use _DEBUG on Unix for Debug Builds (defined automatically on Windows)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
ENDIF(WIN32)

IF(WIN32)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
ENDIF(WIN32)

# Macro to build source groups to match directory structure
MACRO(SOURCE_GROUP_BY_FOLDER target)
    SET(SOURCE_GROUP_DELIMITER "/")
    SET(last_dir "")
    SET(files "")
    FOREACH(file ${SOURCES})
        GET_FILENAME_COMPONENT(dir "${file}" PATH)
        IF (NOT "${dir}" STREQUAL "${last_dir}")
            IF (files)
            SOURCE_GROUP("${last_dir}" FILES ${files})
            ENDIF (files)
            SET(files "")
        ENDIF (NOT "${dir}" STREQUAL "${last_dir}")
        SET(files ${files} ${file})
        SET(last_dir "${dir}")
    ENDFOREACH(file)
    IF (files)
        SOURCE_GROUP("${last_dir}" FILES ${files})
    ENDIF (files)
ENDMACRO(SOURCE_GROUP_BY_FOLDER)

if (NOT TARGET QtUtils)
    add_subdirectory(external/qt_common qt_common)
endif()

if (NOT TARGET amdrdf)
    add_subdirectory(external/rdf/imported/zstd)
    add_subdirectory(external/rdf/rdf)
endif()

if (NOT TARGET system_info)
    add_subdirectory(external/system_info_utils)
endif()

add_subdirectory(source/backend backend)
add_subdirectory(source/frontend frontend)
add_subdirectory(source/renderer renderer)

# Group external dependency targets into folder
IF(WIN32)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties(QtCustomWidgets
                      QtUtils
    PROPERTIES
    FOLDER Dependencies
)
ENDIF()

IF(WIN32)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT RadeonRaytracingAnalyzer)
ENDIF(WIN32)

## Copy Documentation and Samples to output directory. Note - this target is intentionally not included in
## the default project build. It needs to be explicitly built as a separate project
set(DOCS_OUTPUT_DIR ${CMAKE_BINARY_DIR})

# group sphinx source files into a sphinx folder
file(GLOB SPHINX_DOC_FILES ${SPHINX_DOC_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/documentation/source/*.rst)
set (SPHINX_DOC_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/documentation/source/conf.py)
set (ALL_SPHINX_FILES ${SPHINX_DOC_FILES} ${SPHINX_DOC_MAIN})
source_group("sphinx" FILES ${ALL_SPHINX_FILES})

# group release documents into a release_docs folder
set (RELEASE_DOCS_IN_ROOT
     ${CMAKE_CURRENT_SOURCE_DIR}/README.md
     ${CMAKE_CURRENT_SOURCE_DIR}/RELEASE_NOTES.txt
     ${CMAKE_CURRENT_SOURCE_DIR}/NOTICES.txt
     ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt
)
set (RELEASE_DOCS ${RELEASE_DOCS_IN_ROOT})
source_group("release_docs" FILES ${RELEASE_DOCS})

install(FILES ${RELEASE_DOCS_IN_ROOT} DESTINATION . COMPONENT apps)

find_program(SPHINX_EXECUTABLE sphinx-build)
if(SPHINX_EXECUTABLE)
    # Define the option to pass to the sphinx documentation job
    set(SPHINX_OPTION public)

    # hang the sphinx build on the conf.py file and specify a dummy output ("sphinx_output")
    # this ensures the sphinx docs are built everytime you ask to build the Documentation target
    # Sphinx has proper dependency checking, so this works as expected.
    # Once built, clean up any unneeded files.
    add_custom_target(RRADocumentation ALL SOURCES ${ALL_SPHINX_FILES} ${RELEASE_DOCS} DEPENDS sphinx_output)
    add_custom_command(MAIN_DEPENDENCY ${SPHINX_DOC_MAIN} OUTPUT sphinx_output
        COMMAND ${CMAKE_COMMAND} -E echo "building Sphinx documentation"
        COMMAND ${SPHINX_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/documentation/source ${DOCS_OUTPUT_DIR}/help/rra/. -t ${SPHINX_OPTION}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${DOCS_OUTPUT_DIR}/help/rra/.doctrees
    )
    install(DIRECTORY ${DOCS_OUTPUT_DIR}/help DESTINATION . COMPONENT apps)
else()
    message(WARNING "SPHINX_EXECUTABLE (sphinx-build) is not found! Documentation will not be built!")
    # If the sphinx binary isn't found, then just create the Documentation project with only the release docs in it.
    add_custom_target(RRADocumentation ALL SOURCES ${ALL_SPHINX_FILES} ${RELEASE_DOCS})
endif()

add_custom_command(TARGET RRADocumentation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "copying Documentation to output directory"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RELEASE_DOCS_IN_ROOT}           ${DOCS_OUTPUT_DIR}/.
    COMMAND ${CMAKE_COMMAND} -E echo "copying Samples to output directory"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DOCS_OUTPUT_DIR}/samples
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/samples/landscape.rra  ${DOCS_OUTPUT_DIR}/samples/sample_trace.rra
)

install(FILES
       ${DOCS_OUTPUT_DIR}/samples/sample_trace.rra
        DESTINATION
        samples
        COMPONENT apps
)

if (WIN32)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    include(InstallRequiredSystemLibraries)

    install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION . COMPONENT apps)
endif (WIN32)

# CPack packaging

set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_GROUPING IGNORE)

set(CPACK_ARCHIVE_APPS_FILE_NAME "RadeonRaytracingAnalyzer_${RRA_MAJOR_VERSION}.${RRA_MINOR_VERSION}.${RRA_BUGFIX_NUMBER}.${RRA_BUILD_NUMBER}")
set(CPACK_ARCHIVE_DEBUG_FILE_NAME "RadeonRaytracingAnalyzer-PDB_${RRA_MAJOR_VERSION}.${RRA_MINOR_VERSION}.${RRA_BUGFIX_NUMBER}.${RRA_BUILD_NUMBER}")

include(CPack)

cpack_add_component(apps
        DISPLAY_NAME "Applications"
        DESCRIPTION "RRA application")
cpack_add_component(debug
        DISPLAY_NAME "Debug Symbols"
        DESCRIPTION "Debug Symbols")

